#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <EEPROM.h>

#include "LoRaNet.h"
#include "MQ2.h"

#define QUEUE_SIZE 10
#define ADJ_PING_INTERVAL 40000
#define TX_INTERVAL 10000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000

#define USING_SENSORS
#define TEST

#define MQ2_PIN A7
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
MQ2 mq2(MQ2_PIN);

Map packetIdRecord;
uint8_t queueCnt = 0;
uint16_t forwardLen[QUEUE_SIZE];
char* forward[QUEUE_SIZE];

uint8_t adjacencies_cnt = 0;
uint16_t adjacencies[ADJ_LIST_SIZE];
uint16_t adj_ping_tick = 0;

uint16_t packets_lost = 0;

uint16_t nodeId;
uint16_t blacklist[10] = {0};

void setup() {
  // Randomize seed
  randomSeed(analogRead(0));

#ifdef TEST 
  // Read configs from EEPROM
  EEPROM.get(EEPROM_ADDR_ID, nodeId);
  for (int i=0; i<10; i++) {
    EEPROM.get(EEPROM_ADDR_BLACKLIST + i*2, blacklist[i]);
    if (blacklist[i] == 0) break;
  }
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
  // Set LoRa receive callback
  LoRa.onReceive(onReceive);
  LoRa.receive();
  
  // Sensors Init
#ifdef USING_SENSORS
  dht.begin();
  mq2.calibrate_r0();
#endif

  Lib::init();

  Serial.println("Starting.");
}

// Callback func on LoRa receive
void onReceive (int packetSize) {
  // Parse msg
  uint8_t header;
  uint16_t srcId;
  uint16_t prevId;
  uint16_t pId;
  uint16_t len;

  if (!Lib::parsePacket(packetSize, &header, &srcId, &prevId, &pId, &len)) {
    return;
  }

  Serial.print("RECV. header: ");
  Serial.print(header);
  Serial.print(", srcId: "); Serial.print(srcId);
  Serial.print(", prevID: "); Serial.print(srcId);
  Serial.print(", pId: "); Serial.print(pId);
  Serial.print(", ");

  // If MSG srcID is self, meaning echo, drop.
  if (srcId == nodeId) {
    Serial.print("Echoed.\n"); return;
  }

#ifdef TEST
  // If MSG srcID is blacklisted (topology testing)
  for (int i=0; i<10; i++) 
    if (prevId == blacklist[i]) {
      Serial.print("Blocked prevID.\n"); return;
    }
#endif

  // If MSG ID is repeat or outdated, drop.
  int lastPktId = packetIdRecord.get(srcId);
  Serial.print("LastId:"); Serial.print(lastPktId); Serial.print(". ");
  if (lastPktId != -1 && lastPktId >= pId) {
    Serial.print("Outdated.\n"); return;
  }
  // keep track of packets lost
  if (lastPktId != -1) 
    packets_lost += pId - lastPktId -1;
  // Set new latest Pkt Id
  packetIdRecord.set(srcId, pId);

  // Add to adjacencies list
  {
    bool b = true;
    for (int i=0; i<adjacencies_cnt; i++) 
      if (adjacencies[i] == srcId) 
        b = false;
    if (b && adjacencies_cnt < ADJ_LIST_SIZE) 
      adjacencies[adjacencies_cnt++] = srcId;
  }

  // Store in queue
  Serial.print("New, queuing. ");
  if (queueCnt >= QUEUE_SIZE) {
    Serial.print("queue full.\n"); return;
  }
  
  // Copy from msg buf.
  uint16_t t;
  forward[queueCnt] = Lib::getForwardBuf(nodeId, len);
  forwardLen[queueCnt++] = len;

  Serial.println();
}

uint32_t packetId = 1;
uint16_t tick = 0;

void loop() {
  Serial.flush();
  // TX if tick counter up.
  
  if (tick-- == 0) {

#ifdef USING_SENSORS
    float humidity = dht.readHumidity();
    float temp = dht.readTemperature();
    uint16_t mq2_val = mq2.get_ppm();
#else
    float humidity = 123;
    float temp = 456;
    uint16_t mq2_val = 789;
#endif

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

    Serial.print("Packets Lost: ");
    Serial.println(packets_lost);

    free(msg);

    delay(100);
    LoRa.receive();
    tick = (TX_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }

  // TX if adjacency list ping tick is up. 
  if (adj_ping_tick-- == 0) {
    uint16_t len = adjacencies_cnt;
    char* msg = Lib::constructAdjPkt (nodeId, packetId++, adjacencies, &len);
    Serial.println(); // The function above does not print '\n'.

    delay(100);
    LoRa.beginPacket();
    LoRa.write(msg, len);
    LoRa.endPacket();

    free(msg);
    delay(100);
    LoRa.receive();
    adjacencies_cnt = 0;
    adj_ping_tick = (ADJ_PING_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }
  

  // If queue
  if (queueCnt) 
    delay(100);
  for (int i=0; i<queueCnt; i++) {
    char* msg = forward[i];
    uint16_t len = forwardLen[i];

    Serial.print("Fowarding For: ");
    Serial.print((uint8_t) msg[0]);
    Serial.print(", [");  
    Serial.print((uint8_t) msg[1]);
    Serial.print(", ");
    Serial.print((uint8_t) msg[2]);
    Serial.print("] === [");
    Serial.print((uint8_t) msg[3]);
    Serial.print(", ");
    Serial.print((uint8_t) msg[4]);
    Serial.println("]");

    delay(100);
    LoRa.beginPacket();
    LoRa.write(msg, len);
    LoRa.endPacket();

    // Dealloc Packet Object & String 
    free(forward[i]);
  }
  if (queueCnt) {
    queueCnt = 0;
    delay(100);
    LoRa.receive();
  }
  delay(TICK_INTERVAL);
}
