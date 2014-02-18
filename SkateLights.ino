#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <CapacitiveSensor.h>


/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);

CapacitiveSensor  cs_12_9 = CapacitiveSensor(12,9);        // 10M resistor between pins 4 & 2, pin 2 is sensor pin, add a wire and or foil if desired
//CapacitiveSensor  cs_12_10 = CapacitiveSensor(12,10); // This one has a 100 ohm resistor between the pad and the recv pin
long previousMillis = 0;

#define PIN 6
#define CSTHRESH 200

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

int mode = 0; // 0 = rainbowSweep, 1 = compassColor, 2 = stepSplash, 3 = speedSweep, 4 = Serial debug
boolean changeMode;

void setup() {

  // init the lights
  strip.begin();
  strip.setBrightness(255); // 0-255
  strip.show(); // Initialize all pixels to 'off'
  
  // Get serial ready for debugging
  // This can probably go when it's time for Gemma
  Serial.begin(9600);

  // Check for the accelerometer
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  
  
}

void loop() {

  // Set some defaults
  
  
  /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);
  mag.getEvent(&event);
    
  long cap1 = cs_12_9.capacitiveSensor(30);
  Serial.print(cap1);
  Serial.print("\t");
  
  while (cap1 > CSTHRESH) {
    cap1 = cs_12_9.capacitiveSensor(30);
    changeMode = true;
    Serial.println("change mode triggered");
    delay(10);
  }
  
  if (changeMode == true  && mode < 4) {
      mode++;
      Serial.print("mode changed to: ");
      Serial.println(mode);
      changeMode = false;
      delay(10);
    }
    if (changeMode == true && mode == 4) {
      mode = 0;
      Serial.print("mode changed to: ");
      Serial.println(mode);
      changeMode = false;
      delay(10);
    }
  


  
  float Pi = 3.14159;
  float heading = (atan2(event.magnetic.y,event.magnetic.x) * 180) / Pi;
  
  // Handle the various modes
  switch (mode) {
  case 0:    // Rainbow cycle
    Serial.println("Mode 0, Rainbow Chase: ");
    rainbowCycle(10, 2);
    break;
  case 1:    // Compass color
    Serial.println("Mode 1, Compass");
    compassColor(heading);
    break;
  case 2:    // Splash on step
    Serial.println("Mode 2, Splash Step");
    splashStep(event.acceleration.z, strip.Color(127, 127, 127));
    break;
  case 3:    // Sweep speed changes on skate speed
    Serial.println("Mode 3, Knight Rider");
    knightRider(3, 32, 4, 0xFF1000); //Cycles, Speed, WIdth, RGB
    //blockChase(200,1);
    break;
  case 4:
    Serial.println("Mode 4, Rainbow");
    rainbow(20);
  } 

}

void compassColor (float heading) {
  // Normalize to 0-360
  if (heading < 0)
  {
    heading = 360 + heading;
  }
//  Serial.print("Compass Heading: ");
//  Serial.println(heading);
//  Serial.print("Byte Value: ");
//  Serial.println(ByteHeading(heading));
//  Serial.print("Wheel Value: ");
//  Serial.println(Wheel(ByteHeading(heading)));

    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(ByteHeading(heading)));
    }
    strip.show();
}

void splashStep(double zAccel, uint32_t color) {
  Serial.print("zAccel: ");
  Serial.println(zAccel);
  sensors_event_t event; 
  accel.getEvent(&event);
  Serial.print("new event: ");
  Serial.println(abs(event.acceleration.z));

  strip.setBrightness(64);
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, color);
    }
    
  if (abs(event.acceleration.z) > 10) {
    fadeDown(255, 64, 20, color);
    Serial.println(" Fading!");
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
// Split specifies how many cycles are required to display the full rainbow
// i.e. 2 means 1/2 of the rainbow will display at a time
void rainbowCycle(uint8_t wait, int split) {
  uint16_t i, j;
  
  for(j=0; j<256; j++) { 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / split / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
    
    if(cs_12_9.capacitiveSensor(30) > CSTHRESH) {
      break;
    }
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int q=0; q < strip.numPixels(); q++) {
    for (int i=0; i < 5; i++) {
      strip.setPixelColor(i+q-5, c);    //turn every third pixel on
    }
    strip.show();
   
    delay(wait);
   
    for (int i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i+q-5, 0);        //turn every third pixel off
    }
  }
}

void blockChase(uint8_t wait, int split) {
  uint16_t i, j;
  
  for(j=0; j<5; j++) { 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i+j, 0xFFFFFF);
      strip.setPixelColor(i+j+1, 0);
      strip.setPixelColor(j,0);
      
      strip.show();
      delay(wait);
    }

    
//    if(cs_12_9.capacitiveSensor(30) > CSTHRESH) {
//      break;
//    }
  }
}

void knightRider(uint16_t cycles, uint16_t speed, uint8_t width, uint32_t color) {
  uint32_t old_val[strip.numPixels()]; // up to 256 lights!
  // Larson time baby!
  for(int i = 0; i < cycles; i++){
    for (int count = 1; count<strip.numPixels(); count++) {
      strip.setPixelColor(count, color); strip.show();
      delay(speed);
      old_val[count] = color;
      for(int x = count; x>0; x--) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        strip.setPixelColor(x-1, old_val[x-1]); strip.show();
      }
    }
    for (int count = strip.numPixels()-1; count>=0; count--) {
      strip.setPixelColor(count, color); strip.show();
      delay(speed);
      old_val[count] = color;
      for(int x = count; x<=strip.numPixels() ;x++) {
        old_val[x-1] = dimColor(old_val[x-1], width);
        strip.setPixelColor(x+1, old_val[x+1]); strip.show();
      }
    }
    if(cs_12_9.capacitiveSensor(30) > CSTHRESH) {
      break;
    }
  }
}

void clearStrip() {
  for( int i = 0; i<strip.numPixels(); i++){
    strip.setPixelColor(i, 0x000000); strip.show();
  }
}

uint32_t dimColor(uint32_t color, uint8_t width) {
   return (((color&0xFF0000)/width)&0xFF0000) + (((color&0x00FF00)/width)&0x00FF00) + (((color&0x0000FF)/width)&0x0000FF);
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+2) {
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

// Take a compass heading and return a 0-255 value. 
// Handy for sending to the Wheel() func to get back a color
uint32_t ByteHeading(uint32_t Heading) {
  float adj = 255.0/360.0;
  return (unsigned int) (Heading * adj);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void fadeDown(uint32_t highVal, uint32_t lowVal, uint8_t rate, uint32_t color) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.setBrightness(highVal);
  strip.show();
  for(int j = highVal; j > lowVal; j--) {
    strip.setBrightness(j);
    strip.show();
    delay(2);
  }
}

void sensorMonitor () {
  sensors_event_t event; 
  accel.getEvent(&event);
  mag.getEvent(&event);
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");
  Serial.print("Sum: "); Serial.print(event.acceleration.x + event.acceleration.y + event.acceleration.z); Serial.print("  ");
  Serial.println("m/s^2 ");
  delay(10);
}
