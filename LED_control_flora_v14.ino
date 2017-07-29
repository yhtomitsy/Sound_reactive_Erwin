//#include <MemoryFree.h>

#include <Adafruit_NeoPixel.h>

#define PIN 6 // LED data pin

Adafruit_NeoPixel strip = Adafruit_NeoPixel(55, PIN, NEO_GRB + NEO_KHZ800); // declare neopixel library

#define LOG_OUT 1 // use the log output function
#define FHT_N 128 // set to 256 point fht
//#define OCT_NORM 0
#include <FHT.h> // include the library


// for general sound activated modes
int analogVal = 0;
boolean test = true;

// sound analysis
#define volDeficit 0 
int range[] = {20, 20, 50, 50, 41, 41, 40}; // 15 20 40 59 {20, 20, 50, 65, 41, 41, 40};
int minMag[] = {176, 160, 140, 121, 99, 79, 51};
int maxMag[] = {210, 210, 180, 180, 140, 120, 90};
int offset[] = {0, 0, 0, 0, 0, 0, 0}; // {10, 15, 40, 24, 23, 20, 30};
int frequencies[7] = {0};

// for frequncy magnitude analysis procedure
long tot = 0;
long track = 0;
long sampleMags[7] = {0};
int samples = 0;

// get r procedure variables
#define timeInterval 10000
volatile long timeTrack = 0;
volatile int r =  0; // used to set colors in line throb pattern and select blot in random pattern

//random colors
uint8_t blot0[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
uint8_t blot1[] = {10, 11, 12, 13, 14, 15, 22, 21, 20};
uint8_t blot2[] = {9, 10, 15, 16, 17, 18, 19, 20, 28, 29, 30};
uint8_t blot3[] = {22, 23, 24, 25, 26, 35, 36, 36, 27};
uint8_t blot4[] = {33, 34, 35, 37, 38, 39, 43, 44, 45};
uint8_t blot5[] = {31, 32, 33, 39, 40, 41, 42, 43, 43};
uint8_t blot6[] = {46, 47, 48, 49, 50, 51, 52, 53, 54};
float blotBrightness[7] = {0, 0, 0, 0, 0, 0, 0};

// brightness control
volatile float brightness = 0.6;  // controls brightness of the LEDs
volatile long ISRTimer = 0;
volatile uint8_t brt = 1;

//pattern change
volatile uint8_t pattern = 0;

//mode selection
volatile boolean sound = false;

//ring arrays for circle pattern
uint8_t ring1[] = {26, 27, 28, 34, 21};
uint8_t ring2[] = {25, 22, 14, 20, 29, 33, 38, 35};
uint8_t ring3[] = {24, 23, 13, 11, 15, 19, 30, 32, 39, 44, 37, 36};
uint8_t ring4[] = {12, 7, 10, 16, 18, 31, 40, 43, 47, 45};
uint8_t ring5[] = {6, 4, 8, 9, 17, 41, 42, 48, 50, 46};
uint8_t ring6[] = {5, 0, 1, 2, 3, 49, 54, 53, 52, 51};

uint8_t firstPixel = 0;      // holds the first pixel of the selected column
uint8_t lastPixel = 0;       // holds the last pixel of the selected column

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  randomSeed(analogRead(12));
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), adjustBrightness, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), patternChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(0), modeToggle, CHANGE);
}
// ISR for changing brightness
void adjustBrightness(){
  if(millis() - ISRTimer > 200){
    if(brightness < 1)brightness+= 0.2;
    else brightness = 0.2;
    ISRTimer = millis();
  }
}

//ISR for pattern change
void patternChange(){
  if(millis() - ISRTimer > 200){
    if(sound){
      if(pattern < 3)pattern++;
      else pattern = 0;
    }
    else{
      if(pattern < 5)pattern++;
      else pattern = 0;
    }
    brt = 1;
    timeTrack = 0;
    r = 0;
    clearLEDs();
    ISRTimer = millis();
  }
}
//ISR for pattern change
void modeToggle(){
  if(millis() - ISRTimer > 200){
    if(sound)sound = false;
    else sound = true;
    pattern = 0;
    brt = 1;
    timeTrack = 0;
    r = 0;
    clearLEDs();
    ISRTimer = millis();
  }
}
void soundActivation(){
  Serial.print("Brightness: ");Serial.println(brightness);
  Serial.print("Pattern: ");Serial.println(pattern);
  Serial.print("Sound Patterns: ");Serial.println(sound);
  //Serial.println(freeMemory());
}
void loop() {
  if(sound){
    soundActivated();
  }
  else{  
    if(pattern == 0)mainGlitter(); 
    else if(pattern == 1)wipePattern(); 
    else if(pattern == 2)linePattern();
    else if(pattern == 3)strobe(70);
    else if(pattern == 4)chase(255,0,0,50);
    else if(pattern == 5)circlesPattern(70);
  }
}

/*******************************Sound modes**********************************************/
void soundActivated(){
  if(sound == false) return;
  firstPixel = 0;
  lastPixel = 54;
  //brightness/=10;
  clearLEDs(); 
  strip.setPixelColor(27, strip.Color(64,64,64));
  //brightness*=10;
  strip.show();
  setupADC();
  while(1) { // reduces jitter
    soundActivation();
    if(sound == false) return;
    //Serial.println(millis());
    TIMSK0 = 0; // turn off timer0 for lower jitter
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < FHT_N ; i++) { // save 128 samples
      while(!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      analogVal = (j << 8) | m; // form into an int
      analogVal -= 0x0200; // form into a signed int
      analogVal <<= 6; // form into a 16b signed int
      fht_input[i] = analogVal; // put real data into bins
    }
    TIMSK0 = 1; // turn on timer0
    
    fht_window(); // window the data for better frequency response
    fht_reorder(); // reorder the data before doing the fht
    fht_run(); // process the data in the fht
    fht_mag_log(); // take the output of the fht
    sei();
    
    if(test){
      frequencyMagAnlyzer(1, 0, 0, 1, 1);
      frequencyMagAnlyzer(1, 1, 1, 1, 1);
      frequencyMagAnlyzer(3, 2, 2, 1, 1);
      frequencyMagAnlyzer(4, 5, 3, 1, 1);
      frequencyMagAnlyzer(8, 9, 4, 1, 1);
      frequencyMagAnlyzer(16, 17, 5, 1, 1);
      frequencyMagAnlyzer(31, 33, 6, 1, 1);
      samples++;
      if(samples >= 1000){
        for(uint8_t i = 0; i < 7; i++){
          minMag[i] = sampleMags[i]/samples;
          offset[i] -= minMag[i];
          maxMag[i] = minMag[i] + offset[i] + range[i];
        }
        test = false;
      }
    }
    else{
      int sums = 0;
      for (byte i = 0 ; i < FHT_N/2 ; i++) {
        if(i == 0){
          if(fht_log_out[i] < (minMag[i] - offset[i])){
            fht_log_out[i] = (minMag[i] + offset[i])+((minMag[i] - offset[i]) - fht_log_out[i]);
          }
          sums+= fht_log_out[i];
          if(i == 0){
            frequencies[0] = sums;
            sums=0;
          }
        }
        else if(i == 1){
          if(fht_log_out[i] < (minMag[i] - offset[i])){
            fht_log_out[i] = (minMag[i] + offset[i])+((minMag[i] - offset[i]) - fht_log_out[i]);
          }
          sums+= fht_log_out[i];
          if(i == 1){
            frequencies[1] = (sums);
            sums=0;
          }
        }
        else if(i >= 2 && i<=4){
          sums+= fht_log_out[i];
          if(i == 4){
            frequencies[2] = (sums/3);
            sums=0;
          }
        }
        else if(i >= 5 && i<=8){
          sums+= fht_log_out[i];
          if(i == 8){
            frequencies[3] = (sums/4);
            sums=0;
          }
        }
        else if(i >= 9 && i<=16){
          sums+= fht_log_out[i];
          if(i == 16){
            frequencies[4] = (sums/8);
            sums=0;
          }
        }
        else if(i >= 17 && i<=32){
          sums+= fht_log_out[i];
          if(i == 32){
            frequencies[5] = (sums/16);
            sums=0;
          }
        }
        else if(i >= 33 && i<=64){
          sums+= fht_log_out[i];
          if(i == 63){
            frequencies[6] = (sums/31);
            sums=0;
          }
        }
      }
      
      if(pattern == 0){
        vuDisplay();
      }
      else if(pattern == 1){
        if((millis() -  timeTrack) > timeInterval){
          if(r < 5)r++;
          else r = 0;
          timeTrack = millis(); // for time tracking, controls color changes
        } 
        randomColors(getHighestPitch(), 60);
        strip.show();
      }
      else if(pattern == 2){
        lineThrob();
      }
      else if(pattern == 3){
        lineDispers();
      }
    }
  }
}

// used for analysis of magnitudes for Vu meter and sound activated modes.
// set b to 1 to use the procedure to measure the lower limit magnitude of
// the frequency that is being measured. Set b to 0 to get the maximum magnitude
void frequencyMagAnlyzer(uint8_t num, uint8_t initBin, uint8_t band, boolean multiple, boolean b){
  float x = 0; 
  uint8_t j = 0;
  for (j = 0; j < num; j++) x+= fht_log_out[initBin+j];
    
  x/=j;
  if(!multiple){
    if(b){
      tot += x;
      track++;
      Serial.print(tot/track);Serial.print(" ");
    }
    else{
      Serial.print(x);Serial.print(" ");
    }
  }
  else{
    sampleMags[band] += x;
    if(offset[band] < x) offset[band] = x;
  }
}

void lineDispers(){
  getR(); // gets value of r
  int bar = map(getHighestPitch(), 40, 100, 0, 6);
  //Serial.println(getHighestPitch());
  if(bar < 1) bar = 0;
  if(bar == 0){
    getBoundaries(7);
    wipeLine(r);
  }
  else if(bar == 1){
    getBoundaries(6);
    wipeLine(r);
    getBoundaries(8);
    wipeLine(r);
  }
  else if(bar == 2){
    getBoundaries(5);
    wipeLine(r);
    getBoundaries(9);
    wipeLine(r);
  }
  else if(bar == 3){
    getBoundaries(4);
    wipeLine(r);
    getBoundaries(10);
    wipeLine(r);
  }
  else if(bar == 4){
    getBoundaries(3);
    wipeLine(r);
    getBoundaries(11);
    wipeLine(r);
  }
  else if(bar == 5){
    getBoundaries(2);
    wipeLine(r);
    getBoundaries(12);
    wipeLine(r);
  }
  else if(bar >= 6){
    getBoundaries(1);
    wipeLine(r);
    getBoundaries(13);
    wipeLine(r);
  }
  strip.show();
  clearLEDs();
}

//fill line with color
void wipeLine(uint8_t c){
  for(uint8_t i = 0; i < (lastPixel - firstPixel) + 1; i++)strip.setPixelColor(firstPixel + i, Wheel(c));
}

// get the frequency that has highest magnitude
uint8_t getHighestPitch(){ 
  uint8_t highest = 0;
  for(uint8_t i = 0; i < 7; i++){
    float x = frequencies[i] - (minMag[i] + (offset[i]));
    x /= (maxMag[i] - (minMag[i] + (offset[i])));
    x*= 100;
    if(x > highest) highest = x;
  }
  return highest;
}

//random colors
void randomColors(int x, int z){
  int blot = 0;
  if(x > z + 15){
    //Serial.print(x); Serial.print(" ");Serial.println(z);
    blot = random(7);
    blotBrightness[blot] = brightness;
  }
  if(r == 0){
    for(uint8_t i = 0; i < 9; i++){
      strip.setPixelColor(blot0[i], strip.Color(255*blotBrightness[0], 0, 0));
    }
    for(uint8_t i = 0; i < 9; i++){
      strip.setPixelColor(blot1[i], strip.Color(255*blotBrightness[1], 255*blotBrightness[1], 0));
    }
    for(uint8_t i = 0; i < 11; i++){
      strip.setPixelColor(blot2[i], strip.Color(255*blotBrightness[2], 128*blotBrightness[2], 0));
    }
    for(uint8_t i = 0; i < 9; i++){
      strip.setPixelColor(blot3[i], strip.Color(0, 255*blotBrightness[3], 0));
    }
    for(uint8_t i = 0; i < 9; i++){
      strip.setPixelColor(blot4[i], strip.Color(0, 0, 255*blotBrightness[4]));
    }
    for(uint8_t i = 0; i < 9; i++){
      strip.setPixelColor(blot5[i], strip.Color(255*blotBrightness[5], 0, 255*blotBrightness[5]));
    }
    for(uint8_t i = 0; i < 9; i++){
      strip.setPixelColor(blot6[i], strip.Color(128*blotBrightness[6], 0, 255*blotBrightness[6]));
    }
    
  }
  else if(r == 1){
    getBlot(255, 0, 0);
  }
  else if(r == 2){
    getBlot(255, 128, 0);
  }
  else if(r == 3){
    getBlot(0, 255, 0);
  }
  else if(r == 4){
    getBlot(0, 0, 255);
  }
  else if(r == 5){
    getBlot(255, 0, 255);
  }
  
  for(uint8_t i = 0; i < 7; i++){
    if(blotBrightness[i] > 0.09){
      blotBrightness[i] -= 0.1;
    }    
  }
}

//blot selection
void getBlot(uint8_t r, uint32_t g, uint32_t b){
  for(uint8_t i = 0; i < 9; i++){
    strip.setPixelColor(blot0[i], strip.Color(r*blotBrightness[0], g*blotBrightness[0], b*blotBrightness[0]));
  }
  for(uint8_t i = 0; i < 9; i++){
    strip.setPixelColor(blot1[i], strip.Color(r*blotBrightness[1], g*blotBrightness[1], b*blotBrightness[1]));
  }
  for(uint8_t i = 0; i < 11; i++){
    strip.setPixelColor(blot2[i], strip.Color(r*blotBrightness[2], g*blotBrightness[2], b*blotBrightness[2]));
  }
  for(uint8_t i = 0; i < 9; i++){
    strip.setPixelColor(blot3[i], strip.Color(r*blotBrightness[3], g*blotBrightness[3], b*blotBrightness[3]));
  }
  for(uint8_t i = 0; i < 9; i++){
    strip.setPixelColor(blot4[i], strip.Color(r*blotBrightness[4], g*blotBrightness[4], b*blotBrightness[4]));
  }
  for(uint8_t i = 0; i < 9; i++){
    strip.setPixelColor(blot5[i], strip.Color(r*blotBrightness[5], g*blotBrightness[5], b*blotBrightness[5]));
  }
  for(uint8_t i = 0; i < 9; i++){
    strip.setPixelColor(blot6[i], strip.Color(r*blotBrightness[6], g*blotBrightness[6], b*blotBrightness[6]));
  }
}

//display line throb pattern
void lineThrob(){
  getR(); // gets value of r
  getPeaks(frequencies[0], 30, 24, 27,minMag[0] + offset[0], maxMag[0] - volDeficit, r);
  getPeaks(frequencies[1], 23, 18, 21,minMag[1] + offset[1], maxMag[1] - volDeficit, r);
  getPeaks(frequencies[1], 36, 31, 34,minMag[1] + offset[1], maxMag[1] - volDeficit, r);
  getPeaks(frequencies[2], 17, 13, 14,minMag[2] + offset[2], maxMag[2] - volDeficit, r);
  getPeaks(frequencies[2], 41, 37, 38,minMag[2] + offset[2], maxMag[2] - volDeficit, r);
  getPeaks(frequencies[3], 45, 42, 44,minMag[3] + offset[3], maxMag[3] - volDeficit, r);
  getPeaks(frequencies[3], 12, 9, 11,minMag[3] + offset[3], maxMag[3] - volDeficit, r);
  getPeaks(frequencies[4], 48, 46, 47,minMag[4] + offset[4], maxMag[4] - volDeficit, r);
  getPeaks(frequencies[4], 8, 6, 7,minMag[4] + offset[4], maxMag[4] - volDeficit, r);
  getPeaks(frequencies[5], 51, 49, 50,minMag[5] + offset[5]+10, maxMag[5] - volDeficit+10, r);
  getPeaks(frequencies[5], 5, 3, 4,minMag[5] + offset[5]+10, maxMag[5] - volDeficit+10, r);
  getPeaks(frequencies[6], 54, 52, 53,minMag[6] + offset[6]+10, maxMag[6] - volDeficit+10, r);
  getPeaks(frequencies[6], 2, 0, 1,minMag[6] + offset[6]+10, maxMag[6] - volDeficit+10, r);

  strip.show();
  clearLEDs();
}

// line throb
void getPeaks(int p2p, uint8_t upperPix, uint8_t lowerPix, uint8_t midPix, int lowerLim, int upperLim, uint8_t c){
  float pixNum = map(p2p, lowerLim, upperLim, 0, 3);
  //Serial.print(p2p);Serial.print(" ");Serial.println(pixNum);
  if(pixNum < 1.0) pixNum = 0;
  
  if(upperPix - midPix >= pixNum){
    for(uint8_t i = 0; i < pixNum + 1; i++){
      strip.setPixelColor(midPix + i, Wheel(c));
    }
  }
  else{
    for(uint8_t i = 0; i < upperPix - midPix + 1; i++){
      strip.setPixelColor(midPix + i, Wheel(c));
    }
  }
  if(midPix - lowerPix >= pixNum){
    for(uint8_t i = 0; i < pixNum + 1; i++){
      strip.setPixelColor(midPix - i, Wheel(c));
    }
  }
  else{
    for(uint8_t i = 0; i < midPix - lowerPix + 1; i++){
      strip.setPixelColor(midPix - i, Wheel(c));
    }
  } 
}

//display vu meter
void vuDisplay(){
  vuMeter(frequencies[0], 7, minMag[0] + offset[0], maxMag[0] - volDeficit, 30, -1); // 130
  vuMeter(frequencies[1], 6, minMag[1] + offset[1], maxMag[1] - volDeficit, 31, 1);
  vuMeter(frequencies[1], 6, minMag[1] + offset[1], maxMag[1] - volDeficit, 18, 1);
  vuMeter(frequencies[2], 5, minMag[2] + offset[2], maxMag[2] - volDeficit, 41, -1);
  vuMeter(frequencies[2], 5, minMag[2] + offset[2], maxMag[2] - volDeficit, 17, -1);
  vuMeter(frequencies[3], 4, minMag[3] + offset[3], maxMag[3] - volDeficit, 42, 1);
  vuMeter(frequencies[3], 4, minMag[3] + offset[3], maxMag[3] - volDeficit, 9, 1);
  vuMeter(frequencies[4], 3, minMag[4] + offset[4], maxMag[4] - volDeficit, 48, -1);
  vuMeter(frequencies[4], 3, minMag[4] + offset[4], maxMag[4] - volDeficit, 8, -1);
  vuMeter(frequencies[5], 3, minMag[5] + offset[5], maxMag[5] - volDeficit, 49, 1);
  vuMeter(frequencies[5], 3, minMag[5] + offset[5], maxMag[5] - volDeficit, 3, 1);
  vuMeter(frequencies[6], 3, minMag[6] + offset[6], maxMag[6] - volDeficit, 54, -1);
  vuMeter(frequencies[6], 3, minMag[6] + offset[6], maxMag[6] - volDeficit, 2, -1);
  strip.show();
  //clear pixels
  clearLEDs();
}
//Vu meter
void vuMeter(uint8_t bin, uint8_t num, uint8_t lowerLim, uint8_t upperLim, uint8_t startPixel, int order){
  uint32_t c = 0;
  float topPixel = map(bin, lowerLim, upperLim, 0, num);
  // eliminate false 1 readings
  if(topPixel < 1) topPixel = 0;
  float x = float(num)/float(3);
  
  if(x >= 2) x = 3;
  else if (x > 1) x = 2;
   
  //Serial.println(x);
  
  for(int i = 0; i < num; i++){
    if(i <= topPixel-1){
      if(x == 1){
        if(i == 0) c = strip.Color(0, 255*brightness, 0);
        else if(i == 1) c = strip.Color(255*brightness, 128*brightness, 0);
        else c = strip.Color(255*brightness, 0, 0);
      }
      else if(x == 2){
        if(i < 2) c = strip.Color(0, 255*brightness, 0);
        else if(i == 2) c = strip.Color(255*brightness, 128*brightness, 0);
        else c = strip.Color(255*brightness, 0, 0);
      }
      else if(x == 3){
        //Serial.println("kulemanturi3");
        if(i < 2)c = strip.Color(0, 255*brightness, 0);
        else if(i == 2 || i == 3) c = strip.Color(255*brightness, 128*brightness, 0);
        else c = strip.Color(255*brightness, 0, 0);
      }
      strip.setPixelColor(startPixel+(i*order), c);
    }
  }  
}
/*******************************************************************************************/

/*******************************circles*****************************************************/
void circlesPattern(uint8_t wait){
  for (uint8_t i = 0; i < 256; i+=15) {
    soundActivation();
    if(!exitPattern(5))return;
    if(sound) return;
    circles(i,wait);
  }
}
void circles(uint8_t c,uint8_t wait){
  for (int i=0; i < sizeof(ring1); i++) {
    strip.setPixelColor(ring1[i], Wheel(c+75));    
  }
  for (int i=0; i < sizeof(ring2); i++) {
    strip.setPixelColor(ring2[i], Wheel(c+60));    
  }
  for (int i=0; i < sizeof(ring3); i++) {
    strip.setPixelColor(ring3[i], Wheel(c+45));    
  }
  for (int i=0; i < sizeof(ring4); i++) {
    strip.setPixelColor(ring4[i], Wheel(c+30));    
  }
  for (int i=0; i < sizeof(ring5); i++) {
    strip.setPixelColor(ring5[i], Wheel(c+15));    
  }
  for (int i=0; i < sizeof(ring6); i++) {
    strip.setPixelColor(ring6[i], Wheel(c));    
  }
  strip.show();
  delay(wait);
}
/******************************************************************************************/

/*******************************Chase*****************************************************/

void chase(uint8_t r, uint8_t g, uint8_t b,uint8_t wait){
  for(uint8_t i = 0; i < 8; i++){
    uint8_t color = random(255);
    theaterChase(Wheel(color), wait);
  }
  theaterChaseRainbow(wait);
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    soundActivation();
    if(!exitPattern(4))return;
    if(sound) return;
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    soundActivation();
    if(!exitPattern(4))return;
    if(sound) return;
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}

/******************************************************************************************/

/*******************************Strobe*****************************************************/
void strobe(int wait){
  soundActivation();
  if(!exitPattern(3))return;
  if(sound) return;
  uint8_t color = random(255);
  for(uint8_t i = 0; i < strip.numPixels(); i++){
    strip.setPixelColor(i, Wheel(color));
  }
  strip.show();
  delay(wait);
  for(uint8_t i = 0; i < strip.numPixels(); i++){
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  delay(wait);
}
/******************************************************************************************/

/*******************************Droping lines**********************************************/
void linePattern(){
  lineGenerator(0, 0, 0,100);
  if(!exitPattern(2))return;
  if(sound) return;
  lineGenerator(255, 0, 0,100);
  if(!exitPattern(2))return;
  if(sound) return;
  lineGenerator(255, 128, 0,100);
  if(!exitPattern(2))return;
  if(sound) return;
  lineGenerator(0, 255, 0,100);
  if(!exitPattern(2))return;
  if(sound) return;
  lineGenerator(0, 0, 255,100);
  if(!exitPattern(2))return;
  if(sound) return;
  lineGenerator(0, 128, 255,100);
  if(!exitPattern(2))return;
  if(sound) return;
}
void lineGenerator(uint8_t r, uint8_t g, uint8_t b,int wait){
  uint8_t col = random(5,9);
  for(uint8_t i = 0; i < 5; i++){
     if(r == 0 && g ==0 && b == 0)following(r, g, b, col, 1, 50);
     else following(r, g, b, col, 0, 50);
     if(!exitPattern(2))return;
     if(sound) return;
  }
  delay(wait);
}
void following(uint8_t red1, uint8_t green1, uint8_t blue1,uint8_t col, boolean rainbow, int wait){  
  soundActivation();
  if(!exitPattern(2))return;
  if(sound) return;
  //declare variables
  float x = 1; // controls fading along the line
  int y = 1; // controls fading along the line
  uint8_t pixelPrev = 0; // first pixels and ranges for other 2 lines apart from main line
  uint8_t rangePrev = 0;
  uint8_t pixelNext = 0;
  uint8_t rangeNext = 0;
  
  //adjust brightness
  red1*=brightness;
  green1*=brightness;
  blue1*=brightness;
  
  
  if(col%2 > 0){
    //Serial.println("odd");
    uint8_t otherCol1 = random(1, 13);
    getBoundaries(otherCol1);
    if(otherCol1%2 > 0){
      pixelPrev = lastPixel;
      rangePrev = lastPixel - firstPixel;
    }
    else{
      pixelPrev = firstPixel;
      rangePrev = lastPixel - firstPixel;
    }
    uint8_t otherCol2 = random(1, 13);
    getBoundaries(otherCol2);
    if(otherCol2%2 > 0){
      pixelNext = lastPixel;
      rangeNext = lastPixel - firstPixel;
    }
    else{
      pixelNext = firstPixel;
      rangeNext = lastPixel - firstPixel;
    }
    
    getBoundaries(col);
    for(int k = 1; k < 3; k++){
      for(uint8_t i = firstPixel; i <= lastPixel; i++){ // 24 30
        for(uint8_t j = 1; j <= (i - firstPixel + 1); j++){ // 
          if(rainbow)rainbowFormation(&red1, &green1, &blue1, firstPixel, firstPixel+j, 1);
          y = (y/2)+(j*k*2);
          if(k == 2)x = 0;
          else x = float(1) /float(y);
          strip.setPixelColor(i-(j -1), strip.Color(red1*(x), green1*(x), blue1*(x)));
          if(rangePrev > 0){
            uint8_t pixelDif = (lastPixel - firstPixel) - rangePrev; // 6-2 = 4
            if(i >= firstPixel + pixelDif && j >= pixelDif+1){ //i >= 28 j >= 5
              //                          (2 - (30 - 28))  -(5 - 4 + 1)
              if(otherCol1%2 > 0)strip.setPixelColor((pixelPrev-(lastPixel-i)- (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
              else strip.setPixelColor((pixelPrev+(lastPixel-i) + (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
            }
          }
          if(rangeNext > 0){
            uint8_t pixelDif = (lastPixel - firstPixel) - rangeNext;
            if(i >= firstPixel + pixelDif && j >= pixelDif+1){
              if(otherCol2%2 > 0)strip.setPixelColor((pixelNext-(lastPixel-i)-(j-(pixelDif + 1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
              else strip.setPixelColor((pixelNext+(lastPixel-i) + (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
            }
          }
        }
        strip.show();
        delay(wait);
      }
    }
  }
  else{
    //Serial.println("even");
    uint8_t otherCol1 = random(1, 13);
    getBoundaries(otherCol1);
    if(otherCol1%2 > 0){
      pixelPrev = lastPixel;
      rangePrev = lastPixel - firstPixel;
    }
    else{
      pixelPrev = firstPixel;
      rangePrev = lastPixel - firstPixel;
    }
    uint8_t otherCol2 = random(1, 13);
    getBoundaries(otherCol2);
    if(otherCol2%2 > 0){
      pixelNext = lastPixel;
      rangeNext = lastPixel - firstPixel;
    }
    else{
      pixelNext = firstPixel;
      rangeNext = lastPixel - firstPixel;
    }

    getBoundaries(col);
    for(int k = 1; k < 3; k++){
      for(uint8_t i = lastPixel; i >= firstPixel; i--){ //23 - 18
        for(uint8_t j = 1; j <= (lastPixel - i + 1) ; j++){
          if(rainbow)rainbowFormation(&red1, &green1, &blue1, lastPixel, lastPixel - j, -1);
          y = (y/2)+(j*k*2);
          if(k == 2)x = 0;
          else x = float(1) /float(y);
          strip.setPixelColor(i+(j -1), strip.Color(red1*(x), green1*(x), blue1*(x)));

          if(rangePrev > 0){
            uint8_t pixelDif = (lastPixel - firstPixel) - rangePrev; // 5-3 = 2
            if(i <= lastPixel - pixelDif && j >= pixelDif+1){ //i >= 20 j >= 4
              
              if(otherCol1%2 > 0)strip.setPixelColor((pixelPrev-(i-firstPixel)- (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
              //                          (3 + (20 - 18))  - (4 - 3 + 1)
              else strip.setPixelColor((pixelPrev+(i-firstPixel) + (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
            }
          }
          if(rangeNext > 0){
            uint8_t pixelDif = (lastPixel - firstPixel) - rangeNext;
            if(i >= firstPixel + pixelDif && j >= pixelDif+1){
              if(otherCol2%2 > 0)strip.setPixelColor((pixelNext-(i-firstPixel)- (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
              else strip.setPixelColor((pixelNext+(i-firstPixel) + (j-(pixelDif+1))), strip.Color(red1*(x), green1*(x), blue1*(x)));
            }
          }          
        }
        strip.show();
        delay(wait);
      }
    }
  }
}
void rainbowFormation(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t fp, uint8_t i, int order){
  if(i == (fp + 0)){
    *r = 255;
    *g = 0;
    *b = 0;
  }
  else if(i == (fp + (1*order))){
    *r = 255;
    *g = 128;
    *b = 0;
  }
  else if(i == (fp + (2*order))){
    *r = 255;
    *g = 255;
    *b = 0;
  }
  else if(i == (fp + (3*order))){
    *r = 0;
    *g = 255;
    *b = 0;
  }
  else if(i == (fp + (4*order))){
    *r = 0;
    *g = 0;
    *b = 255;
  }
  else if(i == (fp + (5*order))){
    *r = 75;
    *g = 0;
    *b = 130;
  }
  else if(i == (fp + (6*order))){
    *r = 238;
    *g = 100;
    *b = 230;
  }
}
/******************************************************************************************/

/*******************************Wipe patterns**********************************************/
void wipePattern(){
  wipe(255, 0, 0, 20);
  wipe(255, 255, 0, 20);
  wipe(0, 255, 0, 20);
  wipe(0, 0, 255, 20);
  wipe(0, 255, 255, 20);
  if(!exitPattern(1))return;
  if(sound) return;
}
void wipe(uint8_t r, uint8_t g, uint8_t b,int wait){
  // control movement of the columns accross the led display
  for (uint8_t i = 1; i <= 22; i++){
    soundActivation();
    if(!exitPattern(1))return;
    if(sound) return;
    setColumn(r*brightness, g*brightness, b*brightness, i, 1); // sets columns on
    delay(wait);
  }
  for (int i = 13; i >= -8; i--){
    soundActivation();
    if(!exitPattern(1))return;
    if(sound) return;
    setColumn(r*brightness, g*brightness, b*brightness, i, 0); // sets columns on
    delay(wait);
  }
}
void setColumn(uint8_t r, uint8_t g, uint8_t b, int col, byte dir){
  //set color/ brightness for all columns
  if (dir == 1){
    for (int i = 1; i <= 13; i++){
      if(i == col)columnWipe(r, g, b, 16, i);
      else if(i == col-1)columnWipe(r, g, b, 16, i);
      else if(i == col-2)columnWipe(r, g, b, 8, i);
      else if(i == col-3)columnWipe(r, g, b, 2, i);
      else if(i == col-4)columnWipe(r, g, b, 2, i);
      else if(i == col-5)columnWipe(r, g, b, 2, i);
      else if(i == col-6)columnWipe(r, g, b, 8, i);
      else if(i == col-7)columnWipe(r, g, b, 16, i);
      else if(i == col-8)columnWipe(r, g, b, 16, i);
      else if(i == col-9)columnWipe(0, 0, 0, 1, i);
      if (col == 22) columnWipe(0, 0, 0, 1, 13);
    }
  }
  else{
    for (int i = 13; i >= 0; i--){
      if(i == col)columnWipe(r, g, b, 16, i);
      else if(i == col+1)columnWipe(r, g, b, 16, i);
      else if(i == col+2)columnWipe(r, g, b, 8, i);
      else if(i == col+3)columnWipe(r, g, b, 2, i);
      else if(i == col+4)columnWipe(r, g, b, 2, i);
      else if(i == col+5)columnWipe(r, g, b, 2, i);
      else if(i == col+6)columnWipe(r, g, b, 8, i);
      else if(i == col+7)columnWipe(r, g, b, 16, i);
      else if(i == col+8)columnWipe(r, g, b, 16, i);
      else if(i == col+9)columnWipe(0, 0, 0, 1, i);
      if (col == -8) columnWipe(0, 0, 0, 1, 1);
    }
  }
  strip.show();
}
void columnWipe(uint8_t red, uint8_t green, uint8_t blu, uint8_t brt, int column){
  getBoundaries(column); // gets start and stop pixels for selected column
  for (uint8_t i = firstPixel; i <= lastPixel; i++){ // sets the color for the column
    strip.setPixelColor(i, strip.Color(red/brt, green/brt, blu/brt));
  }
}
/******************************************************************************************/

/*************************************glitter patterns************************************/
// main glitter pattern
void mainGlitter(){
  rainbowCycle(50); // glitter rainbow
  if(!exitPattern(0))return;
  if(sound) return;
  glitter(255, 0, 0, 50); // glitter red
  if(!exitPattern(0))return;
  if(sound) return;
  glitter(255, 128, 0, 50); // glitter orange
  if(!exitPattern(0))return;
  if(sound) return;
  glitter(0, 255, 0, 50); // glitter green
  if(!exitPattern(0))return;
  if(sound) return;
  glitter(0, 0, 255, 50); // glitter blue
  if(!exitPattern(0))return;
  if(sound) return;
  glitter(255, 0, 255, 50); // glitter purple
  if(!exitPattern(0))return;
  if(sound) return;
}

// Fill the dots one after the other with a color
void glitter(uint8_t r, uint8_t g, uint8_t b,uint8_t wait) {
  for(uint8_t j=0; j<50; j++) {
    soundActivation();
    if(!exitPattern(0))return;
    if(sound) return;
    for(uint8_t i=0; i<strip.numPixels(); i++) {
        brt = random(1, 10);
        strip.setPixelColor(i, strip.Color(r*(brightness/brt), g*(brightness/brt), b*(brightness/brt)));
        strip.show();
    }
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for(j=0; j<50; j++) { // 1 cycles of all colors on wheel
    soundActivation();
    if(!exitPattern(0))return;
    if(sound) return;
    for(i=0; i< strip.numPixels(); i++) {
      brt = random(1, 10);
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      strip.show();
    }
    delay(wait);
  }
}

/******************************************************************************************/

/********************************general*************************************************/
/*
 * get boundaries of the different strips
 */
void getBoundaries(int x){
  switch (x){
    case 1:
      firstPixel = 0;
      lastPixel = 2;
      break;
    case 2:
      firstPixel = 3;
      lastPixel = 5;
      break;
    case 3:
      firstPixel = 6;
      lastPixel = 8;
      break;
    case 4:
      firstPixel = 9;
      lastPixel = 12;
      break;
    case 5:
      firstPixel = 13;
      lastPixel = 17;
      break;
    case 6:
      firstPixel = 18;
      lastPixel = 23;
      break;
    case 7:
      firstPixel = 24;
      lastPixel = 30;
      break;
    case 8:
      firstPixel = 31;
      lastPixel = 36;
      break;
    case 9:
      firstPixel = 37;
      lastPixel = 41;
      break;
    case 10:
      firstPixel = 42;
      lastPixel = 45;
      break;
    case 11:
      firstPixel = 46;
      lastPixel = 48;
      break;
    case 12:
      firstPixel = 49;
      lastPixel = 51;
      break;
    case 13:
      firstPixel = 52;
      lastPixel = 54;
      break;
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return strip.Color((255 - WheelPos * 3)*(brightness/brt), 0, (WheelPos * 3)*(brightness/brt));
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return strip.Color(0, (WheelPos * 3)*(brightness/brt), (255 - WheelPos * 3)*(brightness/brt));
  } else {
   WheelPos -= 170;
   return strip.Color((WheelPos * 3)*(brightness/brt), (255 - WheelPos * 3)*(brightness/brt), 0);
  }
}

// set ADC to free running mode ready for sound reactive patterns
void setupADC(){
  ADCSRA = 0xe5; // set the adc to free running mode  
  ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((13 >> 3) & 0x01) << MUX5);  
  ADMUX = 0x40 | (13 & 0x7); // use adc13  
  DIDR2 = 0x10; // turn off the digital input for adc13
}

void clearLEDs(){
  //clear pixels
  for(uint8_t i = 0; i < strip.numPixels(); i++){
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
}

void getR(){
  if(millis() -  timeTrack > timeInterval){
    if(pattern == 2){
      if(r < 5)r++;
      else r = 0;
    }
    else{
      r = random(255);
    }
    timeTrack = millis(); // for time tracking, controls color changes
  }
}

boolean exitPattern(uint8_t x){
  if(pattern != x)return false;
  else return true;
}

