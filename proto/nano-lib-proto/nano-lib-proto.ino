#include <SPI.h>
#include <LoRa.h>
#include "LoRaNet.h";
#include <DHT.h>;
#include <AESLib.h>

#define QUEUE_SIZE 10
#define TX_INTERVAL 3000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000

// Sensor Consts
#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

Map packetIdRecord;
uint8_t queueCnt = 0;
Packet* forwardQueue[QUEUE_SIZE];

uint16_t nodeId;



void setup() {
  // Randomize seed
  randomSeed(analogRead(0));
  nodeId = random(1, 10000);

  // Start Serial
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Arduino Nano Node -- Proto w/ Compression --");

  
  // Start LoRa 
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }
  // Set LoRa receive callback
  LoRa.onReceive(onReceive);
  LoRa.receive();

  // Sensors Init
  dht.begin();

  Serial.println("Starting.");
}

// Callback func on LoRa receive
void onReceive (int packetSize) {
  // Parse Packet
  Packet* packet = new Packet();

  if (Lib::parsePacket(packetSize, packet)) {
    Serial.print("ERR: invalid MSG format, dropping.\n");
    goto ERR;
  }
  Serial.print("Parsed: ");
  Lib::printPacket(packet);
  Serial.println();

  // If MSG srcID is self, meaning echo, drop.
  if (packet->srcId == nodeId) {
    Serial.print("Echoed packet, dropping.\n");
    goto ERR;
  }
  // If MSG ID is repeat or outdated, drop.
  int lastPktId = packetIdRecord.get(packet->srcId);
  Serial.print("LastPktId:");
  Serial.print(lastPktId); 
  Serial.print(". ");
  
  if (lastPktId != -1 && lastPktId >= packet->id) {
    Serial.print("Repeated or outdated packet received, dropping.\n");
    goto ERR;
  }
  Serial.print("New Msg. Queuing for forward. ");

  // Set new latest Pkt Id
  packetIdRecord.set(packet->srcId, packet->id);

  // Store in queue
  if (queueCnt >= QUEUE_SIZE) {
    Serial.print("Queue full, dropping.\n");
    goto ERR;
  }
  forwardQueue[queueCnt++] = packet;
  Serial.println();
  return;
ERR:
  free(packet);
  return;
}


uint32_t packetId = 1;
uint16_t tick = 0;

void loop() {
  Serial.flush();
  // TX if tick counter up.
  if (tick-- == 0) {
    float humidity = dht.readHumidity();
    float temp = dht.readTemperature();
    
    Packet packet;
    packet.srcId = nodeId;
    packet.id = packetId++;
    packet.humidity = humidity;
    packet.temp = temp;

    uint16_t len;
    char* msg = Lib::encodePacket(&packet, &len);
    //uint16_t len = sizeof(msg);

    delay(100);
    LoRa.beginPacket();
    LoRa.write(msg, len);
    LoRa.endPacket();
   
    Serial.print("Sent MSG: ");
    Lib::printPacket(&packet);
    Serial.println();

    free(msg);

    delay(100);
    LoRa.receive();
    tick = (TX_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }
  
  // If queue
  if (queueCnt) 
    delay(100);
  for (int i=0; i<queueCnt; i++) {
    
    uint16_t len;
    char* buf = Lib::encodePacket(forwardQueue[i], &len);

    Serial.println(len);
    for (int i=0; i<len; i++) 
      Serial.print(buf[i]);
    Serial.println();
  
    LoRa.beginPacket();
    LoRa.write(buf, len);
    LoRa.endPacket();

    // Dealloc Packet Object & String
    free(buf);

    free(forwardQueue[i]);
  }
  if (queueCnt) {
    queueCnt = 0;
    delay(100);
    LoRa.receive();

    Serial.print("Post-Forward RAM delta: ");
  }
  delay(TICK_INTERVAL);
}
