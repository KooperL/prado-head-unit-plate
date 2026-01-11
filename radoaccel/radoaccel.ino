#include "rm67162.h"
#include <TFT_eSPI.h>
#include <Wire.h>
#include <Adafruit_LSM303_Accel.h>
#include <Adafruit_Sensor.h>

#define WIDTH       536
#define HEIGHT      240
#define CELL_SIZE   8
#define GRID_W      (WIDTH / CELL_SIZE)
#define GRID_H      (HEIGHT / CELL_SIZE)
#define NUM_PARTS   180

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

Adafruit_LSM303_Accel_Unified accel(54321);

struct Particle {
    int gx;
    int gy;
};

Particle particles[NUM_PARTS];

float offsetX = 0.0f;
float offsetY = 0.0f;

uint16_t bgColor;

uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

void initParticles()
{
    for (int i = 0; i < NUM_PARTS; i++) {
        particles[i].gx = random(GRID_W);
        particles[i].gy = random(GRID_H);
    }
}

void setup()
{
    Serial.begin(115200);
    Wire.begin(43, 44);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    rm67162_init();
    lcd_setRotation(1);

    spr.createSprite(WIDTH, HEIGHT);
    spr.setSwapBytes(1);

    if (!accel.begin()) {
        while (1) {}
    }

    accel.setRange(LSM303_RANGE_4G);
    accel.setMode(LSM303_MODE_NORMAL);

    bgColor = color565(64, 115, 241);

    initParticles();
}

void loop()
{
sensors_event_t event;
accel.getEvent(&event);

static float gravityX = 0;
static float gravityY = 0;
float alpha = 0.9;

gravityX = alpha * gravityX + (1 - alpha) * event.acceleration.x;
gravityY = alpha * gravityY + (1 - alpha) * event.acceleration.y;

float linearX = event.acceleration.x - gravityX;
float linearY = event.acceleration.y - gravityY;

// scale and feed into particle offset
offsetX += linearX * 1.5f;
offsetY -= linearY * 1.5f;

// optional damping
offsetX *= 0.85f;
offsetY *= 0.85f;

    spr.fillSprite(bgColor);

    int gridOffsetX = (int)offsetX;
    int gridOffsetY = (int)offsetY;

    for (int i = 0; i < NUM_PARTS; i++) {
        int gx = (particles[i].gx + gridOffsetX) % GRID_W;
        int gy = (particles[i].gy + gridOffsetY) % GRID_H;

        if (gx < 0) gx += GRID_W;
        if (gy < 0) gy += GRID_H;

        int px = gx * CELL_SIZE;
        int py = gy * CELL_SIZE;

        spr.fillRect(
            px,
            py,
            CELL_SIZE,
            CELL_SIZE,
            TFT_WHITE
        );
    }

    lcd_PushColors(
        0,
        0,
        WIDTH,
        HEIGHT,
        (uint16_t *)spr.getPointer()
    );

    delay(16);
}
