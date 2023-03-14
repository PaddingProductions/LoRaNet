#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <EEPROM.h>

#include "LoRaNet.h"
#include "MQ2.h"

#define TX_INTERVAL 5000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 100
#define BUTTON_PIN 4

#define TEST

uint16_t nodeId;

void setup() {
  // Randomize seed
  randomSeed(analogRead(0));

#ifdef TEST 
  // Read configs from EEPROM
  EEPROM.get(EEPROM_ADDR_ID, nodeId);
#else 
  // Generated configs on deploy
  nodeId = random(1, 10000);
#endif 

  // Start Serial
  Serial.begin(9600);
  while (!Serial);
  Serial.println("-- LoRaNet Arduino Nano Node <PROTO> --");
  
  // Start LoRa 
  Serial.println("Starting LoRa...");
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }

  // DEMO Button 
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Lib::init();

  Serial.println("Starting.");
}

uint32_t packetId = 1;
uint16_t tick = 0;

void loop() {
  Serial.flush();
  // TX if tick counter up.
  
  if (tick-- == 0) {

    uint8_t humidity = 11;
    uint16_t mq2_val = 22;
    uint8_t temp = 100 * (digitalRead(BUTTON_PIN) == LOW);
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button!");
    }

    Packet packet;
    packet.srcId = nodeId;
    packet.prevId = nodeId;
    packet.id = packetId++;
    packet.humidity = humidity;
    packet.temp = temp;
    packet.mq2 = mq2_val;

    uint16_t len;
    char* msg = Lib::encodePacket(&packet, &len);

    delay(100);
    LoRa.beginPacket();
    LoRa.write(msg, len);
    LoRa.endPacket();

    free(msg);

    delay(100);
    LoRa.receive();
    tick = (TX_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }

  delay(TICK_INTERVAL);
}
