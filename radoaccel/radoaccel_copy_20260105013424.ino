/**
 * @file      TFT_eSPI_Sprite.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-06-14
 *
 */

#include "rm67162.h"
#include <TFT_eSPI.h>   //https://github.com/Bodmer/TFT_eSPI
#include "true_color.h"

#include <Adafruit_LSM303_Accel.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>


#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB CDC On boot to ENABLE, otherwise you will not see any data in the serial monitor"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to OPI PSRAM in ArduinoIDE"
#endif

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);




float gridOffset = 0;
const float gridSpeed = 5.0;
const float gridSpacing = 40.0;
unsigned long lastTime;
#define WIDTH  536
#define HEIGHT 240
const int SCREEN_WIDTH  = WIDTH;   // 536
const int SCREEN_HEIGHT = HEIGHT;  // 240
const int CENTER_X = SCREEN_WIDTH / 2;
const int HORIZON_Y = SCREEN_HEIGHT / 3;
unsigned long targetTime = 0;
byte red = 31;
byte green = 0;
byte blue = 0;
byte state = 0;
unsigned int colour = red << 11;


/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);

void displaySensorDetails(void) {
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print("Sensor:       ");
  Serial.println(sensor.name);
  Serial.print("Driver Ver:   ");
  Serial.println(sensor.version);
  Serial.print("Unique ID:    ");
  Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    ");
  Serial.print(sensor.max_value);
  Serial.println(" m/s^2");
  Serial.print("Min Value:    ");
  Serial.print(sensor.min_value);
  Serial.println(" m/s^2");
  Serial.print("Resolution:   ");
  Serial.print(sensor.resolution);
  Serial.println(" m/s^2");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}


void drawPerspectiveLine(int x1, int y1, int x2, int y2) {
 int vx = CENTER_X;
 int vy = HORIZON_Y;
 spr.drawWideLine(x1, y1, vx, vy, 3, TFT_WHITE);
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void drawGrid() {
 // Draw to sprite instead of screen
 spr.fillSprite(color565(9, 89, 148));

 // Draw horizontal lines (going into distance)
for (float z = gridOffset; z < 300; z += gridSpacing) {
  if (z <= 0) continue;

  float depth = z / 100.0f;
  float scale = 1.0f / depth;

  int y = HORIZON_Y + (SCREEN_HEIGHT - HORIZON_Y) * scale;

  if (y < HORIZON_Y || y > SCREEN_HEIGHT) continue;

  int width = SCREEN_WIDTH * scale;
  int x1 = CENTER_X - width / 2;
  int x2 = CENTER_X + width / 2;

  spr.drawWideLine(x1, y, x2, y, 3, TFT_WHITE);
}


 // Draw vertical lines
 int numVerticalLines = 5;
 for (int i = 0; i <= numVerticalLines; i++) {
  float ratio = (float)i / numVerticalLines;
  int bottomX = (int)(SCREEN_WIDTH * ratio);
  drawPerspectiveLine(bottomX, SCREEN_HEIGHT, CENTER_X, HORIZON_Y);

 }

 // Draw horizon line
 spr.drawWideLine(0, HORIZON_Y, SCREEN_WIDTH, HORIZON_Y, 3, TFT_MAGENTA);

lcd_PushColors(
  0,
  0,
  WIDTH,
  HEIGHT,
  (uint16_t *)spr.getPointer()
);
}

void setup()
{
lastTime = millis();
    Wire.begin(43, 44); // SDA = 43, SCL = 44

    // Use TFT_eSPI Sprite made by framebuffer , unnecessary calling during use tft.xxxx function
    Serial.begin(115200);
    /*
    * Compatible with touch version
    * Touch version, IO38 is the screen power enable
    * Non-touch version, IO38 is an onboard LED light
    * * */
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    rm67162_init();
    lcd_setRotation(1);
    spr.createSprite(WIDTH, HEIGHT);
    spr.setSwapBytes(1);


      /* Initialise the sensor */
  if (!accel.begin()) {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while (1)
      ;
  }


  /* Display some basic information on this sensor */
  displaySensorDetails();

  accel.setRange(LSM303_RANGE_4G);
  Serial.print("Range set to: ");
  lsm303_accel_range_t new_range = accel.getRange();
  switch (new_range) {
  case LSM303_RANGE_2G:
    Serial.println("+- 2G");
    break;
  case LSM303_RANGE_4G:
    Serial.println("+- 4G");
    break;
  case LSM303_RANGE_8G:
    Serial.println("+- 8G");
    break;
  case LSM303_RANGE_16G:
    Serial.println("+- 16G");
    break;
  }

  accel.setMode(LSM303_MODE_NORMAL);
  Serial.print("Mode set to: ");
  lsm303_accel_mode_t new_mode = accel.getMode();
  switch (new_mode) {
  case LSM303_MODE_NORMAL:
    Serial.println("Normal");
    break;
  case LSM303_MODE_LOW_POWER:
    Serial.println("Low Power");
    break;
  case LSM303_MODE_HIGH_RESOLUTION:
    Serial.println("High Resolution");
    break;
  }
  
}

void loop() {
    sensors_event_t event;
    accel.getEvent(&event);

    // Compute movement magnitude
    float movement = sqrt(
        sq(event.acceleration.x) +
        sq(event.acceleration.y) +
        sq(event.acceleration.z)
    );
    movement = abs(movement - 9.8); // remove gravity

    // Map movement to time scale
    const float TIME_SCALE_MIN = 0.5;
    const float TIME_SCALE_MAX = 500.0;
static float smoothedMovement = 0;
float alpha = 0.5; // smoothing factor, 0 < alpha < 1
smoothedMovement = alpha * movement + (1 - alpha) * smoothedMovement;
float timeScale = constrain(smoothedMovement * 2.0, TIME_SCALE_MIN, TIME_SCALE_MAX);    Serial.println(timeScale);
    // Update grid with delta time
    unsigned long now = millis();
    float deltaTime = (now - lastTime) / 1000.0f; // seconds
    lastTime = now;

    gridOffset += gridSpeed * deltaTime * timeScale * 5;

    // Wrap grid offset
    if (gridOffset >= gridSpacing) {
        gridOffset -= gridSpacing;
    }

    drawGrid();

    delay(30);
}

