#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string>
#include <cstddef>
#include <Wire.h>
#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
using namespace std;

/* ===== compile settings ===== */
#define MAX_CH 14       // 1 - 14 channels (1-11 for US, 1-13 for EU and 1-14 for Japan)
#define SNAP_LEN 2324   // max len of each recieved packet

// TFT Display settings for T4 v1.3
#define USE_DISPLAY
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define GRAPH_HEIGHT 160
#define GRAPH_Y_START 60

#if CONFIG_FREERTOS_UNICORE
#define RUNNING_CORE 0
#else
#define RUNNING_CORE 1
#endif

#ifdef USE_DISPLAY
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
#endif

esp_err_t event_handler(void* ctx, system_event_t* event) {
  return ESP_OK;
}

/* ===== run-time variables ===== */
Preferences preferences;

uint32_t lastDrawTime;
uint32_t lastChannelTime;
uint32_t tmpPacketCounter;
uint32_t channelPackets[MAX_CH];  // packets per channel
uint32_t channelDeauths[MAX_CH];  // deauths per channel
int channelRssi[MAX_CH];          // average RSSI per channel
uint32_t deauths = 0;             // current deauth frames
unsigned int ch = 1;              // current 802.11 channel
int rssiSum;
int rssiCount;
uint32_t scanCycles = 0;          // total scan cycles completed

const int CHANNEL_DWELL_TIME = 500; // ms per channel (faster updates)

/* ===== functions ===== */
void setChannel(int newChannel) {
  ch = newChannel;
  if (ch > MAX_CH || ch < 1) ch = 1;

  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}

void wifi_promiscuous(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

  if (type == WIFI_PKT_MGMT && (pkt->payload[0] == 0xA0 || pkt->payload[0] == 0xC0 )) deauths++;

  if (type == WIFI_PKT_MISC) return;             // wrong packet type
  if (ctrl.sig_len > SNAP_LEN) return;           // packet too long

  tmpPacketCounter++;
  rssiSum += ctrl.rssi;
  rssiCount++;
}

void draw() {
#ifdef USE_DISPLAY
  sprite.fillSprite(TFT_BLACK);

  // Title
  sprite.setTextDatum(TC_DATUM);
  sprite.setTextColor(TFT_CYAN, TFT_BLACK);
  sprite.drawString("WiFi Packet Monitor - Channel Scanner", SCREEN_WIDTH/2, 5, 2);
  
  // Current channel indicator and stats
  sprite.setTextDatum(TL_DATUM);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawString("Scanning CH:" + String(ch), 10, 25, 2);
  sprite.setTextDatum(TR_DATUM);
  sprite.drawString("Cycle: " + String(scanCycles), SCREEN_WIDTH - 10, 25, 2);

  // Draw horizontal reference line
  sprite.drawLine(0, GRAPH_Y_START + GRAPH_HEIGHT, SCREEN_WIDTH, GRAPH_Y_START + GRAPH_HEIGHT, TFT_DARKGREY);

  // Find max value for scaling
  uint32_t maxPackets = 1;
  for (int i = 0; i < MAX_CH; i++) {
    if (channelPackets[i] > maxPackets) maxPackets = channelPackets[i];
  }

  // Draw channel bars
  int barWidth = SCREEN_WIDTH / MAX_CH;
  for (int i = 0; i < MAX_CH; i++) {
    int x = i * barWidth;
    int barHeight = (channelPackets[i] * GRAPH_HEIGHT) / maxPackets;
    
    // Color based on activity level
    uint16_t color;
    if (channelPackets[i] > maxPackets * 0.7) color = TFT_RED;
    else if (channelPackets[i] > maxPackets * 0.4) color = TFT_ORANGE;
    else if (channelPackets[i] > maxPackets * 0.2) color = TFT_YELLOW;
    else color = TFT_BLUE;
    
    // Draw bar
    if (barHeight > 0) {
      sprite.fillRect(x + 1, GRAPH_Y_START + GRAPH_HEIGHT - barHeight, barWidth - 4, barHeight, color);
    }
    
    // Draw channel number
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.drawString(String(i + 1), x + barWidth/2, GRAPH_Y_START + GRAPH_HEIGHT + 5, 1);
    
    // Draw packet count for significant values
    if (channelPackets[i] > 0 && barHeight > 15) {
      sprite.setTextColor(TFT_WHITE, TFT_BLACK);
      sprite.drawString(String(channelPackets[i]), x + barWidth/2, GRAPH_Y_START + GRAPH_HEIGHT - barHeight - 10, 1);
    }
  }

  // Draw grid lines
  for (int i = 1; i < 5; i++) {
    int y = GRAPH_Y_START + (GRAPH_HEIGHT * i / 4);
    sprite.drawLine(0, y, SCREEN_WIDTH, y, TFT_DARKGREY);
  }

  // Draw animated scan line on current channel
  int scanX = (ch - 1) * barWidth + barWidth / 2;
  
  // Pulsing scan line effect
  static int pulsePhase = 0;
  pulsePhase = (pulsePhase + 1) % 20;
  int pulseAlpha = abs(pulsePhase - 10) * 25; // 0-250 pulse
  
  // Draw scan line with glow effect
  for (int offset = -2; offset <= 2; offset++) {
    uint16_t lineColor;
    if (offset == 0) {
      lineColor = TFT_WHITE; // Bright center
    } else {
      lineColor = tft.color565(0, pulseAlpha, pulseAlpha); // Cyan glow
    }
    sprite.drawLine(scanX + offset, GRAPH_Y_START, scanX + offset, GRAPH_Y_START + GRAPH_HEIGHT, lineColor);
  }
  
  // Draw scan line top indicator (arrow pointing down)
  sprite.fillTriangle(scanX - 5, GRAPH_Y_START - 10, 
                      scanX + 5, GRAPH_Y_START - 10,
                      scanX, GRAPH_Y_START - 3, TFT_CYAN);
  
  // Draw scan line bottom indicator (arrow pointing up)
  sprite.fillTriangle(scanX - 5, GRAPH_Y_START + GRAPH_HEIGHT + 10, 
                      scanX + 5, GRAPH_Y_START + GRAPH_HEIGHT + 10,
                      scanX, GRAPH_Y_START + GRAPH_HEIGHT + 3, TFT_CYAN);

  // Status bar at bottom
  sprite.setTextDatum(TL_DATUM);
  sprite.setTextColor(TFT_GREEN, TFT_BLACK);
  sprite.drawString("CH" + String(ch) + ": " + String(tmpPacketCounter) + " pkts | " + 
                   String(rssiCount > 0 ? rssiSum / rssiCount : 0) + " dBm", 
                   5, 225, 1);

  sprite.pushSprite(0, 0);
#endif
}

/* ===== main program ===== */
void setup() {

  // Serial
  Serial.begin(115200);

  // System & WiFi
  nvs_flash_init();
  tcpip_adapter_init();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());

  // Display - TFT initialization
#ifdef USE_DISPLAY
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);
  sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  sprite.setTextDatum(MC_DATUM);

  /* show start screen */
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_CYAN, TFT_BLACK);
  sprite.setTextDatum(MC_DATUM);
  sprite.drawString("PacketMonitor32", SCREEN_WIDTH/2, 80, 4);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.drawString("LilyGO T4 v1.3 Edition", SCREEN_WIDTH/2, 120, 2);
  sprite.drawString("Channel Scanner Mode", SCREEN_WIDTH/2, 150, 2);
  sprite.pushSprite(0, 0);

  delay(2000);
#endif

  // Initialize channel arrays
  for (int i = 0; i < MAX_CH; i++) {
    channelPackets[i] = 0;
    channelDeauths[i] = 0;
    channelRssi[i] = 0;
  }

  // Start on channel 1
  setChannel(1);
  lastChannelTime = millis();
  lastDrawTime = 0;

  // second core
  xTaskCreatePinnedToCore(
    coreTask,               /* Function to implement the task */
    "coreTask",             /* Name of the task */
    2500,                   /* Stack size in words */
    NULL,                   /* Task input parameter */
    0,                      /* Priority of the task */
    NULL,                   /* Task handle. */
    RUNNING_CORE);          /* Core where the task should run */

  // start Wifi sniffer
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous);
  esp_wifi_set_promiscuous(true);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}

void coreTask( void * p ) {

  uint32_t currentTime;

  while (true) {

    currentTime = millis();

    // Auto-scan channels
    if (currentTime - lastChannelTime >= CHANNEL_DWELL_TIME) {
      lastChannelTime = currentTime;
      
      // Store data for current channel
      channelPackets[ch - 1] = tmpPacketCounter;
      channelDeauths[ch - 1] = deauths;
      if (rssiCount > 0) {
        channelRssi[ch - 1] = rssiSum / rssiCount;
      }
      
      // Print to serial
      Serial.printf("CH%d: %d pkts, %d deauths, %d dBm\n", 
                    ch, tmpPacketCounter, deauths, 
                    rssiCount > 0 ? rssiSum / rssiCount : 0);
      
      // Move to next channel
      setChannel(ch + 1);
      
      // Increment scan cycle counter when completing a full cycle
      if (ch == 1) {
        scanCycles++;
      }
      
      // Reset counters for next channel
      tmpPacketCounter = 0;
      deauths = 0;
      rssiSum = 0;
      rssiCount = 0;
      
      // Draw after each channel update for real-time display
      draw();
    }

    delay(10);
  }
}