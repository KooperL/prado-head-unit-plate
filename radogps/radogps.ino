#include <TFT_eSPI.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprGrid = TFT_eSprite(&tft);
TFT_eSprite sprSpeed = TFT_eSprite(&tft);

TinyGPSPlus gps;
HardwareSerial GPSSerial(2);

#define RX_PIN 33
#define TX_PIN 32
const long GPS_BAUD = 9600;

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define HALF_WIDTH 160

const int GRID_CENTER_X = HALF_WIDTH / 2;
const int HORIZON_Y = 60;
const float gridSpacing = 30.0;
float gridOffset = 0;

float currentSpeed = 0.0;
float smoothedSpeed = 0.0;
unsigned long lastUpdateTime = 0;

int satelliteCount = 0;
bool gpsValid = false;
unsigned long lastGPSUpdate = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("GPS Speed Display Starting...");
  
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  sprSpeed.createSprite(HALF_WIDTH, SCREEN_HEIGHT);
  sprSpeed.setTextDatum(MC_DATUM);
  
  sprGrid.createSprite(HALF_WIDTH, SCREEN_HEIGHT);
  
  lastUpdateTime = millis();
  
  Serial.println("Setup complete!");
}

void drawPerspectiveLine(int x1, int y1) {
  sprGrid.drawWideLine(x1, y1, GRID_CENTER_X, HORIZON_Y, 2, TFT_WHITE);
}

void drawPerspectiveGrid(float offset) {
  sprGrid.fillSprite(TFT_BLUE);
  
  for (float z = offset; z < 250; z += gridSpacing) {
    if (z <= 0) continue;
    
    float depth = z / 80.0f;
    float scale = 1.0f / depth;
    
    int y = HORIZON_Y + (SCREEN_HEIGHT - HORIZON_Y) * scale;
    
    if (y < HORIZON_Y || y >= SCREEN_HEIGHT) continue;
    
    int width = HALF_WIDTH * scale;
    int x1 = GRID_CENTER_X - width / 2;
    int x2 = GRID_CENTER_X + width / 2;
    
    x1 = constrain(x1, 0, HALF_WIDTH - 1);
    x2 = constrain(x2, 0, HALF_WIDTH - 1);
    
    sprGrid.drawWideLine(x1, y, x2, y, 2, TFT_WHITE);
  }
  
  int numVerticalLines = 4;
  for (int i = 0; i <= numVerticalLines; i++) {
    float ratio = (float)i / numVerticalLines;
    int bottomX = (int)(HALF_WIDTH * ratio);
    drawPerspectiveLine(bottomX, SCREEN_HEIGHT - 1);
  }
  
  sprGrid.drawWideLine(0, HORIZON_Y, HALF_WIDTH, HORIZON_Y, 2, TFT_CYAN);
  
  sprGrid.pushSprite(HALF_WIDTH, 0);
}

void drawSpeedDisplay() {
  sprSpeed.fillSprite(TFT_BLACK);
  
  sprSpeed.setTextColor(TFT_WHITE);
  sprSpeed.setTextSize(1);
  sprSpeed.drawString("GPS SPEED", HALF_WIDTH / 2, 15);
  
  sprSpeed.setTextSize(3);
  sprSpeed.setTextColor(TFT_GREEN);
  
  if (gpsValid) {
    String speedStr = String(smoothedSpeed, 1);
    sprSpeed.drawString(speedStr, HALF_WIDTH / 2, 70);
    
    sprSpeed.setTextSize(2);
    sprSpeed.setTextColor(TFT_WHITE);
    sprSpeed.drawString("km/h", HALF_WIDTH / 2, 105);
    
    sprSpeed.setTextSize(1);
    sprSpeed.setTextColor(TFT_CYAN);
    
    float mps = smoothedSpeed / 3.6;
    String mpsStr = String(mps, 1) + " m/s";
    sprSpeed.drawString(mpsStr, HALF_WIDTH / 2, 135);
    
    float knots = smoothedSpeed * 0.539957;
    String knotsStr = String(knots, 1) + " kts";
    sprSpeed.drawString(knotsStr, HALF_WIDTH / 2, 155);
    
  } else {
    sprSpeed.setTextSize(2);
    sprSpeed.setTextColor(TFT_RED);
    sprSpeed.drawString("NO GPS", HALF_WIDTH / 2, 70);
  }
  
  sprSpeed.setTextSize(1);
  sprSpeed.setTextColor(TFT_YELLOW);
  String satStr = "Sats: " + String(satelliteCount);
  sprSpeed.drawString(satStr, HALF_WIDTH / 2, 190);
  
  if (gpsValid) {
    sprSpeed.fillCircle(HALF_WIDTH / 2, 215, 8, TFT_GREEN);
  } else {
    sprSpeed.fillCircle(HALF_WIDTH / 2, 215, 8, TFT_RED);
  }
  
  sprSpeed.pushSprite(0, 0);
}

void updateGPS() {
  bool dataReceived = false;
  
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    if (gps.encode(c)) {
      dataReceived = true;
    }
  }
  
  if (dataReceived) {
    lastGPSUpdate = millis();
  }
  
  if (gps.location.isUpdated() || gps.speed.isUpdated()) {
    if (gps.location.isValid()) {
      gpsValid = true;
      currentSpeed = gps.speed.kmph();
      
      float alpha = 0.3;
      smoothedSpeed = alpha * currentSpeed + (1 - alpha) * smoothedSpeed;
      
      Serial.print("Speed: ");
      Serial.print(smoothedSpeed, 1);
      Serial.println(" km/h");
    }
  }
  
  if (gps.satellites.isUpdated()) {
    satelliteCount = gps.satellites.value();
  }
  
  if (millis() - lastGPSUpdate > 5000) {
    gpsValid = false;
    satelliteCount = 0;
  }
}

void loop() {
  updateGPS();
  
  unsigned long now = millis();
  float deltaTime = (now - lastUpdateTime) / 1000.0f;
  lastUpdateTime = now;
  
  float speedMultiplier = smoothedSpeed / 30.0;
  speedMultiplier = constrain(speedMultiplier, 0, 4.0);
  
  float gridSpeed = 15.0;
  gridOffset += gridSpeed * deltaTime * speedMultiplier;
  
  if (gridOffset >= gridSpacing) {
    gridOffset -= gridSpacing;
  }
  
  drawSpeedDisplay();
  drawPerspectiveGrid(gridOffset);
  
  delay(50);
}