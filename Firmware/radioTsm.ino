#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h>
#endif

RH_ASK driver(1000, 4, 8, 0);  // RX=4 (unused), TX=8

const int triggerPin = 9;  // Pin connected to Jetson. PB1, physical pin 13

void setup()
{
  Serial.begin(9600);
  pinMode(triggerPin, INPUT);

  Serial.println("Transmitter starting...");

  if (!driver.init()) {
    Serial.println("Radio init failed!");
  } else {
    Serial.println("Radio initialized successfully.");
  }
}

void loop()
{
  if (digitalRead(triggerPin) == HIGH)
  {
    const char *msg = "Deer Detected";

    Serial.println("Transmitting message...");
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();

    // small delay to avoid overwhelming the channel
    delay(100);
  }
  else
  {
    // idle when LOW
    delay(50);
  }
}