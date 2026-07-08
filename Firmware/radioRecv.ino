#include <Arduino.h>
#include <RH_ASK.h>
#include <SPI.h>
#include "bell.h"

// RH_ASK — explicitly request Timer2 so it stays off Timer1
// Constructor: speed, rxPin, txPin, pttPin, pttInverted
RH_ASK driver(1000, 8, 4, 0);

const int speakerPin = 9;  // OC1A

volatile uint16_t sampleIndex = 0;
volatile bool playing = false;

// Target sample period in microseconds (8000 Hz → 125 µs)
const unsigned int SAMPLE_PERIOD_US = 125;
unsigned long lastSampleTime = 0;

unsigned long lastChimeTime = 0;
const unsigned long chimeCooldown = 1200;

// ── PWM setup (Timer1, 10-bit Fast PWM, no prescaler) ──────────────────────
void setupPWM() {
  pinMode(speakerPin, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(WGM10) | _BV(WGM11);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 512;
}

// ── Called every loop iteration; updates OCR1A at 8 kHz via micros() ───────
void updateSample() {
  if (!playing) return;

  unsigned long now = micros();
  if (now - lastSampleTime < SAMPLE_PERIOD_US) return;
  lastSampleTime = now;

  if (sampleIndex < BELL_LEN) {
    uint8_t sample = pgm_read_byte(&bell[sampleIndex]);
    OCR1A = (uint16_t)sample << 2;  // 8-bit → 10-bit
    sampleIndex++;
  } else {
    playing = false;
    OCR1A = 512;
  }
}

void playBell() {
  sampleIndex = 0;
  lastSampleTime = micros();
  playing = true;
}

void setup() {
  Serial.begin(9600);
  setupPWM();  // Timer1 PWM — must come before driver.init()

  if (!driver.init()) {
    Serial.println("Radio init failed!");
  } else {
    Serial.println("Radio initialized successfully.");
  }
}

void loop() {
  // ── Service the sample engine first (tight timing) ─────────────────────
  updateSample();

  // ── RF receive (non-blocking) ───────────────────────────────────────────
  uint8_t buf[32];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    buf[buflen] = '\0';
    if (strcmp((char*)buf, "Deer Detected") == 0) {
      if (millis() - lastChimeTime > chimeCooldown) {
        Serial.println("Playing bell...");
        playBell();
        lastChimeTime = millis();
      }
    }
  }
}