#define USE_HARDWARE_SPI
#include <SPI.h>
#include <TFT_eSPI.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// Display setup
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprRadar = TFT_eSprite(&tft);

// CC1101 SPI pins
#define PIN_SCK  19
#define PIN_MISO 34
#define PIN_MOSI 26
#define PIN_CS   33

// Display dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Radar parameters (left side)
#define RADAR_CENTER_X 100
#define RADAR_CENTER_Y 120
#define RADAR_RADIUS 100
#define NUM_CHANNELS 80

// UHF CB frequencies (MHz)
float channels[NUM_CHANNELS];

// RSSI storage
int rssiValues[NUM_CHANNELS];
int maxRssiValues[NUM_CHANNELS];
int rssiHistory[NUM_CHANNELS][10];
int historyIndex = 0;

// Radar sweep
float sweepAngle = 0;
int currentChannel = 0;
unsigned long lastSweepTime = 0;
const int SWEEP_DELAY = 20; // ms between scans

// RSSI thresholds
#define RSSI_THRESHOLD_HIGH -60
#define RSSI_THRESHOLD_MED  -80
#define RSSI_THRESHOLD_LOW  -100

// ---- Setup ----
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("UHF CB Radar Starting...");

  // Initialize channel frequencies
  for (int i = 0; i < 16; i++) channels[i] = 476.425 + i * 0.025; // Ch1–16
  for (int i = 16; i < NUM_CHANNELS; i++) channels[i] = 477.000 + (i - 16) * 0.0125; // Ch17–80

  // Initialize RSSI arrays
  for (int i = 0; i < NUM_CHANNELS; i++) {
    rssiValues[i] = -120;
    maxRssiValues[i] = -120;
    for (int j = 0; j < 10; j++) rssiHistory[i][j] = -120;
  }

  // Initialize CC1101
  ELECHOUSE_cc1101.setSpiPin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setPA(7);
  Serial.println("CC1101 initialized!");

  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  sprRadar.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  sprRadar.setTextDatum(MC_DATUM);

  lastSweepTime = millis();
  Serial.println("Setup complete!");
}

// ---- Helper Functions ----
uint16_t getRSSIColor(int rssi) {
  if (rssi > RSSI_THRESHOLD_HIGH) return TFT_RED;
  else if (rssi > RSSI_THRESHOLD_MED) return TFT_ORANGE;
  else if (rssi > RSSI_THRESHOLD_LOW) return TFT_YELLOW;
  else return TFT_DARKGREY;
}

int getSmoothedRSSI(int channel) {
  long sum = 0;
  for (int i = 0; i < 10; i++) sum += rssiHistory[channel][i];
  return sum / 10;
}

void scanChannel(int channelIndex) {
  // Set frequency
  ELECHOUSE_cc1101.setMHZ(channels[channelIndex]);

  // Put CC1101 into receive mode
  ELECHOUSE_cc1101.SetRx();

  // Short delay for PLL lock
  delay(10);

  // Read RSSI
  int rssi = ELECHOUSE_cc1101.getRssi();

  // Store in history for smoothing
  rssiHistory[channelIndex][historyIndex] = rssi;
  rssiValues[channelIndex] = getSmoothedRSSI(channelIndex);

  // Update max RSSI for leaderboard
  if (rssiValues[channelIndex] > maxRssiValues[channelIndex])
    maxRssiValues[channelIndex] = rssiValues[channelIndex];

  Serial.print("Ch");
  Serial.print(channelIndex + 1);
  Serial.print(" (");
  Serial.print(channels[channelIndex], 3);
  Serial.print(" MHz) RSSI: ");
  Serial.println(rssiValues[channelIndex]);
}


// ---- Drawing functions ----
void drawRadarGrid() {
  for (int r = RADAR_RADIUS; r > 0; r -= 25)
    sprRadar.drawCircle(RADAR_CENTER_X, RADAR_CENTER_Y, r, TFT_BLUE);

  for (int i = 0; i < NUM_CHANNELS; i++) {
    float angle = (360.0 / NUM_CHANNELS) * i;
    float rad = radians(angle - 90);
    int x2 = RADAR_CENTER_X + cos(rad) * RADAR_RADIUS;
    int y2 = RADAR_CENTER_Y + sin(rad) * RADAR_RADIUS;
    sprRadar.drawLine(RADAR_CENTER_X, RADAR_CENTER_Y, x2, y2, TFT_BLUE);
  }

  sprRadar.fillCircle(RADAR_CENTER_X, RADAR_CENTER_Y, 3, TFT_BLUE);
}

void drawChannelMarkers() {
  int top5[5] = {-1, -1, -1, -1, -1};
  int topRSSI[5] = {-120, -120, -120, -120, -120};

  for (int i = 0; i < NUM_CHANNELS; i++) {
    int rssi = rssiValues[i];
    for (int j = 0; j < 5; j++) {
      if (rssi > topRSSI[j]) {
        for (int k = 4; k > j; k--) {
          topRSSI[k] = topRSSI[k - 1];
          top5[k] = top5[k - 1];
        }
        topRSSI[j] = rssi;
        top5[j] = i;
        break;
      }
    }
  }

  for (int i = 0; i < NUM_CHANNELS; i++) {
    float angle = (360.0 / NUM_CHANNELS) * i;
    float rad = radians(angle - 90);
    int rssi = rssiValues[i];
    float rssiNorm = constrain(map(rssi, -120, -40, 0, 100), 0, 100) / 100.0;
    int markerRadius = RADAR_RADIUS * rssiNorm;

    if (markerRadius > 5) {
      int x = RADAR_CENTER_X + cos(rad) * markerRadius;
      int y = RADAR_CENTER_Y + sin(rad) * markerRadius;
      uint16_t color = getRSSIColor(rssi);
      int blobSize = 3;

      for (int t = 0; t < 5; t++)
        if (i == top5[t]) blobSize = 7 - t;

      sprRadar.fillCircle(x, y, blobSize, color);
      sprRadar.drawCircle(x, y, blobSize + 1, TFT_WHITE);
    }
  }

  // Draw leaderboard text on right side
  sprRadar.setTextDatum(TL_DATUM);
  sprRadar.setTextColor(TFT_WHITE);
  int textX = 220;
  int textY = 20;
  sprRadar.drawString("Top 5 Channels:", textX, textY);
  for (int i = 0; i < 5; i++) {
    if (top5[i] >= 0) {
      String s = "Ch" + String(top5[i] + 1) + ": " + String(rssiValues[top5[i]]) + " dBm";
      sprRadar.drawString(s, textX, textY + 15 * (i + 1));
    }
  }
}

void drawSweepLine() {
  float rad = radians(sweepAngle - 90);
  int x2 = RADAR_CENTER_X + cos(rad) * RADAR_RADIUS;
  int y2 = RADAR_CENTER_Y + sin(rad) * RADAR_RADIUS;
  sprRadar.drawWideLine(RADAR_CENTER_X, RADAR_CENTER_Y, x2, y2, 3, TFT_WHITE);

  for (int i = 1; i <= 5; i++) {
    float trailAngle = sweepAngle - (i * 10);
    float trailRad = radians(trailAngle - 90);
    int tx2 = RADAR_CENTER_X + cos(trailRad) * RADAR_RADIUS;
    int ty2 = RADAR_CENTER_Y + sin(trailRad) * RADAR_RADIUS;
    uint16_t fadeColor = tft.color565(255 - i * 40, 255 - i * 40, 255);
    sprRadar.drawLine(RADAR_CENTER_X, RADAR_CENTER_Y, tx2, ty2, fadeColor);
  }
}

void drawInfoPanel() {
  sprRadar.setTextDatum(TL_DATUM);
  sprRadar.setTextColor(TFT_WHITE);
  int textX = 220;
  int textY = 120;

  String chInfo = "Current Ch: " + String(currentChannel + 1);
  String freqInfo = "Freq: " + String(channels[currentChannel], 3) + " MHz";
  String rssiInfo = "RSSI: " + String(rssiValues[currentChannel]) + " dBm";

  sprRadar.drawString(chInfo, textX, textY);
  sprRadar.drawString(freqInfo, textX, textY + 15);
  sprRadar.drawString(rssiInfo, textX, textY + 30);
}

// ---- Loop ----
void loop() {
  unsigned long now = millis();
  if (now - lastSweepTime >= SWEEP_DELAY) {
    lastSweepTime = now;

    scanChannel(currentChannel);
    sweepAngle = (360.0 / NUM_CHANNELS) * currentChannel;

    currentChannel++;
    if (currentChannel >= NUM_CHANNELS) {
      currentChannel = 0;
      historyIndex = (historyIndex + 1) % 10;
    }
  }

  sprRadar.fillSprite(TFT_BLACK);
  drawRadarGrid();
  drawChannelMarkers();
  drawSweepLine();
  drawInfoPanel();
  sprRadar.pushSprite(0, 0);

  delay(5);
}
