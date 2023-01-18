#include <SPI.h>
#include <LoRa.h>
#include "LoRaNet.h";
#include <DHT.h>;
#include <AESLib.h>

#define QUEUE_SIZE 10
#define ADJ_PING_INTERVAL 30000
#define ADJ_LIST_SIZE 10
#define TX_INTERVAL 10000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000

// Sensor Consts
#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

Map packetIdRecord;
uint8_t queueCnt = 0;
uint16_t forwardLen[QUEUE_SIZE];
char* forward[QUEUE_SIZE];

uint8_t adjacencies_cnt = 0;
uint16_t adjacencies[ADJ_LIST_SIZE];
uint16_t adj_ping_tick = 0;

uint16_t packets_lost = 0;

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
  // Parse msg
  uint8_t header;
  uint16_t srcId;
  uint16_t pId;
  if (!Lib::parsePacket(packetSize, &header, &srcId, &pId)) {
    return;
  }

  Serial.print("header: ");
  Serial.print(header);
  Serial.print(", srcId: "); Serial.print(srcId);
  Serial.print(", pId: "); Serial.print(pId);
  Serial.print(", ");
  // If MSG srcID is self, meaning echo, drop.
  if (srcId == nodeId) {
    Serial.print("Echoed packet, dropping.\n"); return;
  }

  // If MSG ID is repeat or outdated, drop.
  int lastPktId = packetIdRecord.get(srcId);
  Serial.print("LastPktId:"); Serial.print(lastPktId); Serial.print(". ");
  if (lastPktId != -1 && lastPktId >= pId) {
    Serial.print("Outdated packet, dropping.\n"); return;
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
  Serial.print("New Msg. Queuing for forward. ");
  if (queueCnt >= QUEUE_SIZE) {
    Serial.print("Queue full, dropping.\n"); return;
  }
  

  // Copy from msg buf.
  char* buf = (char*) malloc(packetSize); 
  memcpy(buf, msgbuf, packetSize);
  Serial.print("Queued: [");
  for (int i=0; i<packetSize; i++) {
    Serial.print((uint8_t) buf[i]);
    Serial.print(", ");
  }
  Serial.println("]");  forwardLen[queueCnt++] = packetSize;
  forward[queueCnt++] = buf;

  Serial.println();
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

    
    Serial.print("MSG: [");
    for (int i=0; i<len; i++) {
      Serial.print((uint8_t) msg[i]);
      Serial.print(", ");
    }
    Serial.println("]");

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

    delay(100);
    LoRa.beginPacket();
    LoRa.write(msg, len);
    LoRa.endPacket();
   
    Serial.print("Sent ADJ list ping: [");
    for (int i=0; i<adjacencies_cnt; i++) {
      Serial.print(adjacencies[i]);
      Serial.print(", ");
    }
    Serial.println("]");

    
    Serial.print("MSG: [");
    for (int i=0; i<len; i++) {
      Serial.print((uint8_t) msg[i]);
      Serial.print(", ");
    }
    Serial.println("]");
    

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

  
    Serial.print("Forwarded: len: ");
    Serial.print(len);
    Serial.print(", [");
    for (int i=0; i<len; i++) {
      Serial.print((uint8_t) msg[i]);
      Serial.print(", ");
    }
    Serial.println("]");
    
    
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
