#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define PIN_SCK  19
#define PIN_MISO 34
#define PIN_MOSI 26
#define PIN_CS   33

void setup() {
  Serial.begin(115200);
  ELECHOUSE_cc1101.setSpiPin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(476.725); // Channel 13
  ELECHOUSE_cc1101.SetRx();          // Start receive mode
}

void loop() {
  int rssi = ELECHOUSE_cc1101.getRssi();
  Serial.println(rssi);
  delay(200);
}
