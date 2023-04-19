#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

int brightness = 80;
int dir = 1;
int wait = 40;
int minBrightness, maxBrightness;
int highMin = 40;
int lowMin = 1;
int highMax = 250, lowMax = 190;
int minBrightnesses[40];
int minFrequencies[40];
int minRunningSums[40];
int maxBrightnesses[61];
int maxFrequencies[61];
int maxRunningSums[61];
int minFreqSum, maxFreqSum;
bool decreaseWait = false;
bool ledShouldFlicker[16];
bool flicker = false;
bool flickerStarted = false;
int flickerFrame = 0;
int currentFlicker = 1;
int currentFlickerLength = 0;
int numOfFlickers = 0;
bool flickerPaused = false;
int pauseLength = 0;
int pauseFrame = 0;
int delayBeforeFlicker = 0;
int delayFrame = 0;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  // minBrightness = random(5, 30);
  // maxBrightness = random(180, 200);

  int minSum = 0;
  int maxSum = 0;

  for (int i = 0; i < 40; i++) {
    minBrightnesses[i] = i + 1;
    minFrequencies[i] = 40 - i;
    minRunningSums[i] = minSum + 40 - i;
    minSum += (40 - i);
  }

  for (int i = 0; i < 61; i++) {
    maxBrightnesses[i] = i + 190;
    maxFrequencies[i] = i + 1;
    maxRunningSums[i] = maxSum + i + 1;
    maxSum += (i + 1);    
  }

  minBrightness = getMinBrightness();
  maxBrightness = getMaxBrightness();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  brightness += dir;
  if (decreaseWait) wait -= 10;
  if (decreaseWait && wait <= 10) decreaseWait = false;
  if (brightness == maxBrightness || brightness == minBrightness) {
    dir *= -1;
    // randomly decide whether flicker should happen
    double rand = random(0, 99) / 100.0;
    if (rand >= 0.54) {
      delayBeforeFlicker = random(5, 30);
      flicker = true;
    }
  }
  if (delayFrame < delayBeforeFlicker) {
    delayFrame++;
  }
  if (brightness == maxBrightness) {
    minBrightness = getMinBrightness();
    wait = (int) random(5, 30);
  }
  if (brightness == minBrightness) {
    maxBrightness = getMaxBrightness();
    wait = 200;
    decreaseWait = true;
  }

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 180, 80));
  }

  strip.setPixelColor(0, strip.Color(222, 49, 99));
  strip.setPixelColor(10, strip.Color(222, 49, 99));
  strip.setPixelColor(2, strip.Color(255, 129, 0));
  strip.setPixelColor(8, strip.Color(255, 129, 0));
  strip.setPixelColor(13, strip.Color(255, 129, 0));
  strip.setPixelColor(4, strip.Color(204, 85, 0));
  strip.setPixelColor(7, strip.Color(204, 85, 0));
  strip.setPixelColor(14, strip.Color(204, 85, 0));
  strip.setPixelColor(12, strip.Color(204, 85, 0));
  strip.setPixelColor(9, strip.Color(204, 85, 0));
  strip.setPixelColor(1, strip.Color(204, 85, 0));
  strip.setBrightness(brightness);

  if (delayFrame == delayBeforeFlicker) doFlicker();

  strip.show();
  delay(wait);

  strip.show();
}

int getMaxBrightness() {
  // get sum of all elements (last element of running sum arr)
  // generate a random number between 1 and that sum
  // find first index in running sum arr with value higher than random val
  // return val at same index in maxbrightnesses arr
  int maxBrightnessSum = maxRunningSums[60];
  int r = random(1, maxBrightnessSum);
  int indexc = findCeil(maxRunningSums, r, 0, 60);
  return maxBrightnesses[indexc];
}

int getMinBrightness() {
  int minBrightnessSum = minRunningSums[39];
  int r = random(1, minBrightnessSum);
  int indexc = findCeil(minRunningSums, r, 0, 39);
  return minBrightnesses[indexc];
}

int findCeil(int arr[], int r, int l, int h)
{
    int mid;
    while (l < h)
    {
        mid = l + ((h - l) >> 1); // Same as mid = (l+h)/2
        if(r > arr[mid])
            l = mid + 1;
        else
            h = mid;
    }
    return (arr[l] >= r) ? l : -1;
}



void doFlicker() {
  // initial settings for this chunk of flicker
  if (flicker && !flickerStarted) { 
    // number of times it'll turn on and off  
    numOfFlickers = (int) random(2, 5);
    currentFlicker = 1;
    // set which lights should turn off when it flickers
    for (int i = 0; i < 16; i++) {
      double rand = (random(0, 99)) / 100.0;
      if (rand > 0.2) ledShouldFlicker[i] = true;
    }    
    flickerStarted = true;
    // how long the first time it turns off should last
    currentFlickerLength = random(3, 14);
  }

  // lights are currently off, increment number of frames they've been off for
  if (!flickerPaused && flickerStarted) {
    flickerFrame++;
  }

  // if lights have been off for currentFlickerLength:
    // pause the flicker so light comes through until it turns off again
    // set length for next time lights turn off
  if (flickerFrame == currentFlickerLength) {
    flickerPaused = true;
    pauseLength = random(2, 10);
    pauseFrame = 0;
    currentFlickerLength = random(3, 14); 
  }

  // keep track of how long lights have been on for in between flickers
  if (flickerPaused) {
    pauseFrame++;
  }

  // time for a flicker to start again
  if (flickerPaused && pauseFrame == pauseLength) {
    flickerPaused = false;
    flickerFrame = 0;
    currentFlicker++;
  }

  // turn off the individual LEDs according to boolean value in ledShouldFlicker array
  if (flicker && currentFlicker <= numOfFlickers && !flickerPaused) {
    for (int i = 0; i < 16; i++) {
      if (ledShouldFlicker[i]) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }    
  }

  // done with entire round of flickering
  if (currentFlicker > numOfFlickers) {
    flicker = false;
    flickerStarted = false;
    currentFlicker = 0;
    flickerPaused = false;
    delayFrame = 0;
    delayBeforeFlicker = 0;
  }
}

