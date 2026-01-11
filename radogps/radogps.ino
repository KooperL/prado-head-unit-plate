#include <TinyGPS++.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial GPSSerial(2);
#define RX_PIN 33
#define TX_PIN 32

const long GPS_BAUD = 9600;

void setup() {
  Serial.begin(115200);
  Serial.println("Grove GPS starting at 9600 baud...");

  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    gps.encode(c);
  }

  if (gps.location.isUpdated() && gps.location.isValid()) {
    Serial.print("Latitude: "); Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: "); Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: "); Serial.println(gps.altitude.meters());
    Serial.print("Satellites: "); Serial.println(gps.satellites.value());
    Serial.print("Date: "); Serial.print(gps.date.day()); Serial.print("/");
    Serial.print(gps.date.month()); Serial.print("/");
    Serial.println(gps.date.year());
    Serial.print("Time: "); Serial.print(gps.time.hour()); Serial.print(":");
    Serial.print(gps.time.minute()); Serial.print(":"); Serial.println(gps.time.second());

    // Speed directly from GPS
    Serial.print("Speed (km/h): "); Serial.println(gps.speed.kmph());
    Serial.print("Speed (m/s): "); Serial.println(gps.speed.mps());
    Serial.print("Speed (knots): "); Serial.println(gps.speed.knots());
    
    Serial.println("-------------------");
  }
}
