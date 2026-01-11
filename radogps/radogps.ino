#include <TFT_eSPI.h>
#include <SPI.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprGrid = TFT_eSprite(&tft);  // Sprite for grid half
TFT_eSprite sprSpeed = TFT_eSprite(&tft); // Sprite for speed half

TinyGPSPlus gps;
HardwareSerial GPSSerial(2);

#define RX_PIN 33
#define TX_PIN 32
const long GPS_BAUD = 9600;

// Display dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define HALF_WIDTH 160

// Grid parameters for right half
const int GRID_CENTER_X = HALF_WIDTH / 2;
const int HORIZON_Y = 60;
const float gridSpacing = 30.0;
float gridOffset = 0;

// Speed tracking
float currentSpeed = 0.0; // km/h
float smoothedSpeed = 0.0;
unsigned long lastUpdateTime = 0;

// GPS status
int satelliteCount = 0;
bool gpsValid = false;
unsigned long lastGPSUpdate = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("GPS Speed Display Starting...");
  
  // Initialize GPS
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape
  tft.fillScreen(TFT_BLACK);
  
  // Create sprites for double buffering
  // Left half: Speed display
  sprSpeed.createSprite(HALF_WIDTH, SCREEN_HEIGHT);
  sprSpeed.setTextDatum(MC_DATUM); // Middle center
  
  // Right half: Grid display
  sprGrid.createSprite(HALF_WIDTH, SCREEN_HEIGHT);
  
  lastUpdateTime = millis();
  
  Serial.println("Setup complete!");
}

void drawPerspectiveLine(int x1, int y1) {
  // Draw line from bottom point to vanishing point
  sprGrid.drawWideLine(x1, y1, GRID_CENTER_X, HORIZON_Y, 2, TFT_WHITE);
}

void drawPerspectiveGrid(float offset) {
  // Blue background
  sprGrid.fillSprite(TFT_BLUE);
  
  // Draw horizontal lines (depth perception)
  for (float z = offset; z < 250; z += gridSpacing) {
    if (z <= 0) continue;
    
    float depth = z / 80.0f;
    float scale = 1.0f / depth;
    
    int y = HORIZON_Y + (SCREEN_HEIGHT - HORIZON_Y) * scale;
    
    if (y < HORIZON_Y || y >= SCREEN_HEIGHT) continue;
    
    int width = HALF_WIDTH * scale;
    int x1 = GRID_CENTER_X - width / 2;
    int x2 = GRID_CENTER_X + width / 2;
    
    // Constrain to sprite bounds
    x1 = constrain(x1, 0, HALF_WIDTH - 1);
    x2 = constrain(x2, 0, HALF_WIDTH - 1);
    
    sprGrid.drawWideLine(x1, y, x2, y, 2, TFT_WHITE);
  }
  
  // Draw vertical lines
  int numVerticalLines = 4;
  for (int i = 0; i <= numVerticalLines; i++) {
    float ratio = (float)i / numVerticalLines;
    int bottomX = (int)(HALF_WIDTH * ratio);
    drawPerspectiveLine(bottomX, SCREEN_HEIGHT - 1);
  }
  
  // Draw horizon line
  sprGrid.drawWideLine(0, HORIZON_Y, HALF_WIDTH, HORIZON_Y, 2, TFT_CYAN);
  
  // Push sprite to right half of screen
  sprGrid.pushSprite(HALF_WIDTH, 0);
}

void drawSpeedDisplay() {
  // Black background
  sprSpeed.fillSprite(TFT_BLACK);
  
  // Title
  sprSpeed.setTextColor(TFT_WHITE);
  sprSpeed.setTextSize(1);
  sprSpeed.drawString("GPS SPEED", HALF_WIDTH / 2, 15);
  
  // Main speed display
  sprSpeed.setTextSize(3);
  sprSpeed.setTextColor(TFT_GREEN);
  
  if (gpsValid) {
    // Display speed in km/h
    String speedStr = String(smoothedSpeed, 1);
    sprSpeed.drawString(speedStr, HALF_WIDTH / 2, 70);
    
    sprSpeed.setTextSize(2);
    sprSpeed.setTextColor(TFT_WHITE);
    sprSpeed.drawString("km/h", HALF_WIDTH / 2, 105);
    
    // Additional speed units
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
  
  // Satellite count
  sprSpeed.setTextSize(1);
  sprSpeed.setTextColor(TFT_YELLOW);
  String satStr = "Sats: " + String(satelliteCount);
  sprSpeed.drawString(satStr, HALF_WIDTH / 2, 190);
  
  // GPS status indicator
  if (gpsValid) {
    sprSpeed.fillCircle(HALF_WIDTH / 2, 215, 8, TFT_GREEN);
  } else {
    sprSpeed.fillCircle(HALF_WIDTH / 2, 215, 8, TFT_RED);
  }
  
  // Push sprite to left half of screen
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
  
  // Update timestamp if we received any GPS data
  if (dataReceived) {
    lastGPSUpdate = millis();
  }
  
  // Update GPS data
  if (gps.location.isUpdated() || gps.speed.isUpdated()) {
    if (gps.location.isValid()) {
      gpsValid = true;
      currentSpeed = gps.speed.kmph();
      
      // Smooth the speed for display
      float alpha = 0.3; // Smoothing factor
      smoothedSpeed = alpha * currentSpeed + (1 - alpha) * smoothedSpeed;
      
      Serial.print("Speed: ");
      Serial.print(smoothedSpeed, 1);
      Serial.println(" km/h");
    }
  }
  
  if (gps.satellites.isUpdated()) {
    satelliteCount = gps.satellites.value();
  }
  
  // Check if GPS data is stale (no valid sentences in 5 seconds)
  if (millis() - lastGPSUpdate > 5000) {
    gpsValid = false;
    satelliteCount = 0;
  }
}

void loop() {
  // Update GPS data
  updateGPS();
  
  // Calculate delta time
  unsigned long now = millis();
  float deltaTime = (now - lastUpdateTime) / 1000.0f;
  lastUpdateTime = now;
  
  // Update grid animation based on speed
  // Speed range: 0-120 km/h maps to grid speed multiplier
  float speedMultiplier = smoothedSpeed / 30.0; // Scale factor
  speedMultiplier = constrain(speedMultiplier, 0, 4.0);
  
  float gridSpeed = 15.0; // Base speed
  gridOffset += gridSpeed * deltaTime * speedMultiplier;
  
  // Wrap grid offset
  if (gridOffset >= gridSpacing) {
    gridOffset -= gridSpacing;
  }
  
  // Draw both halves
  drawSpeedDisplay();
  drawPerspectiveGrid(gridOffset);
  
  delay(50); // ~20 FPS
}