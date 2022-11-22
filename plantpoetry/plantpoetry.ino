#include <Adafruit_NeoPixel.h>

// See https://github.com/justtheman/Ws2812_Arduino/blob/master/WS2812_Definitions.h (pretty colors! Unused atm)
//#include "WS2812_Definitions.h" 

#define SAMPLE_COUNT 32 //sample 32 mic values before using those samples as an average to detemine LED color.
#define PIN 5 //serial pin the leds are attached
#define LED_COUNT 16 //number of leds on the strip... Can this be dynamic?

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRB + NEO_KHZ800);

// Hex colors for each LED
uint32_t colours[] = 
{
  0xFEA121,
  0xFAD10B,
  0xFEEF21,
  0xD4FE21,
  0x42FE21,
  0x7AFA8A,
  0x7AFAB8,
  0x21FE99,
  0x7AFAD7,
  0x21FEF4,
  0x21B8FE,
  0x215BFE,
  0x3221FE,
  0x361895,
  0x310E62, 
  0x310E62, 
//  0xff0000, 
};

long micAccum = 0;
int micCount = 0;

float height = 0;
float volVal = 0;

void setup() 
{
  leds.begin();
  pinMode(A3, INPUT); //Set analog pin 3 to input mode 

  //Sets the data rate in bits per second (baud) for serial data transmission. For USB baudrate is fixed on 115200 bps
  //So I guess we are connecting to the PC via USB here?
  Serial.begin(115200);
}

uint8_t t(uint8_t v) 
{
  //return v;

  // Practically this seems to reduce the intensity of the light somewhat. To be more colorful/less washed out.

  // See https://www.wolframalpha.com/input?key=&i=%28%28x%2F255%29%5E2%29+*+255
  // dividing by 255 to normalize 8bit color to float between 1 and 0.
  // for x = 1 y = 0.004 therefore power in a normalized range creates very small outputs. 
  // (good for input into set pixel colors? need to read more about expectations of input arguments)

  return pow((float)v / 255, 2.0) * 255;
}

void updateLights() 
{
  //height = 1;
  for (int i = 0; i < LED_COUNT; i++) 
  {
    uint32_t c = colours[i];

    uint16_t pixId  =LED_COUNT - i - 1;

    //determine if this LED should be lit
    int cutoff = pixId < (height * LED_COUNT) ? 1 : 0;
    
    //bitshift color into 8bit int representations per channel
    uint8_t r = cutoff * ((c & 0xff0000) >> 16);
    uint8_t g = cutoff * ((c & 0x00ff00) >> 8);
    uint8_t b = cutoff * ((c & 0x0000ff) >> 0);

    leds.setPixelColor(pixId, t(r) ,t(g), t(b));      
  }

  leds.show();  
}

void loop() 
{
  //Reading analog voltage from pin 3... - some magic number?
  // See https://www.arduino.cc/reference/en/language/functions/analog-io/analogread/

  //Output between 0 and 1023... subtracted by 405 to reduce sensitivity?
  int analogSignal = analogRead(A3) - 405;

  //but then we abs it anyway so maybe we are just offsetting around 0? Not sure
  int delta = abs(analogSignal);

  // Accumulate signal delta until reaching a point where we have enough samples to make an educated guess at the current volume of the mic.
  micAccum += delta; 
  micCount ++;

  if (micCount == SAMPLE_COUNT)
  {
    //average volume and add it to the previous volume value.
    volVal += (delta / SAMPLE_COUNT);
    volVal *= .6; //reduce overall volume by magic number to ensure total loudness does not grow (this probably gonna cause issues? frame dependant? why 0.6?)
    
    //remap volume to a smaller value for height? I guess re earlier docs 1 unit analog is 1/1024 (0.0009765625) so maybe this approximates that to normalize it somewhat
    height += abs(max(0, volVal - 3) * .01); 
    //height += abs(max(0, volVal - 3) * .0225); //Looks like a trial and error situation here?
    height = min(height, 4); //Clamp to no greater than 4? but isnt this normalized to 1?
    height *= .992;  //frame dependant reduction over time.  
    
    // Reset for next sample phase
    micAccum = 0;
    micCount = 0;

    updateLights();
  }
}
