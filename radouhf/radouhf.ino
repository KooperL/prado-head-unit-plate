#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <TFT_eSPI.h>
#include <SPI.h>

#define PIN_SCK  19
#define PIN_MISO 34
#define PIN_MOSI 26
#define PIN_CS   33

TFT_eSPI tft = TFT_eSPI();

// UHF CB channel 13 frequency (MHz)
#define CHANNEL_FREQ 476.725

void setup() {
  Serial.begin(115200);

  // Initialize CC1101
  ELECHOUSE_cc1101.setSpiPin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CHANNEL_FREQ); // Channel 13
  ELECHOUSE_cc1101.SetRx();              // Put in receive mode

  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
}

uint16_t getRSSIColor(int rssi) {
  if (rssi > -60) return TFT_RED;
  else if (rssi > -80) return TFT_ORANGE;
  else if (rssi > -100) return TFT_YELLOW;
  else return TFT_DARKGREY;
}

void loop() {
  // Read RSSI
  int rssi = ELECHOUSE_cc1101.getRssi();
  Serial.println(rssi);

  // Clear screen
  tft.fillScreen(TFT_BLACK);

  // Draw color-coded RSSI
  tft.setTextSize(3);
  tft.setTextColor(getRSSIColor(rssi));
  tft.drawString("Ch 13 RSSI", 160, 80);
  tft.drawString(String(rssi) + " dBm", 160, 140);

  delay(200);
}
