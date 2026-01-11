#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI(); // TFT instance

void setup() {
  Serial.begin(115200);
  tft.init();              // init display
  tft.setRotation(1);      // set orientation
  tft.fillScreen(TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 10);
  tft.print("Hello, T4 v1.3!");
}

void loop() {
  // animate a simple rectangle
  tft.fillRect(10, 50, 100, 20, TFT_BLUE);
  delay(1000);
  tft.fillRect(10, 50, 100, 20, TFT_BLACK);
  delay(1000);
}
