#include <SPI.h>
#include <LoRa.h>

void setup() 
{
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Arduino Receiver");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa init done.");

  Serial.println("Starting.");
}

void loop () 
{
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    Serial.print("Got msg, size: " + String(packetSize, DEC) + ", available: " + String(LoRa.available(), DEC) + ", : '");
    for (int i=0; i < packetSize; i++) 
      Serial.print((char)LoRa.read());
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}