#include <SPI.h>
#include <LoRa.h>

#define QUEUE_SIZE 10
#define TX_INTERVAL 3000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000
#define MAP_SIZE 20
#define ARG_COUNT 3
#define MSG_SIZE 20

struct Packet {
  uint16_t srcId = 0;
  uint16_t id = 0;
  int16_t val = 0;
};

struct Map {
  int16_t val[MAP_SIZE];
  uint16_t key[MAP_SIZE];
  uint16_t size = 0;

  int get(uint16_t k) {
    int i = 0;
    while (i < size && key[i] != k) i++;
    return i == size ? -1 : val[i];
  };

  void set (uint16_t k, int16_t v) {
    int i = 0;
    while (i < size && key[i] != k) i++;

    if (i == size) {
      if (size == sizeof(key)) 
        return;
      key[size++] = k;
    }
    val[i] = v;
  }
};

Map packetIdRecord;
uint8_t queueCnt = 0;
Packet* forwardQueue[QUEUE_SIZE];

uint16_t nodeId;

void display_freeram() {
  Serial.print(F("- SRAM left:"));
  Serial.println(freeRam());
}

int freeRam() {
  extern int __heap_start,*__brkval;
  int v;
  return (int)&v - (__brkval == 0  ? (int)&__heap_start : (int) __brkval);  
}

void setup() {
  // Randomize seed
  randomSeed(analogRead(0));
  nodeId = random(1, 10000);
  // Start Serial
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Arduino Nano Node -- Proto --");

  // Start LoRa 
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }
  // Set LoRa receive callback
  LoRa.onReceive(onReceive);
  LoRa.receive();

  Serial.println("Starting.");
}

// Parses packet from raw string. Returns null if invalid format.
Packet* parsePacket(int packetSize) { 
  // Parse message into args array, without using 'String' objects.
  // Format: "{srcId}:{id}:{val}";
  int argCnt = 0;
  int args[ARG_COUNT];
  
  int bufCnt = 0;
  char buf[10];
  Serial.print("Recv MSG: '");
  for (int i=0; LoRa.available() && i<packetSize; i++) {
    char c = (char) LoRa.read();
    
    if (c == '\0') break;
    if (c == ':') {
      buf[bufCnt++] = '\0'; 
      int val = atoi(buf);
      
      Serial.print(buf);
      Serial.print("->");
      Serial.print(val);
      Serial.print(',');
      
      args[argCnt++] = val;
      if (argCnt == ARG_COUNT) break;

      bufCnt = 0;
    } else {
      buf[bufCnt++] = c;
    }
  }
  //free(buf);
  
  
  Serial.print("', args: ");
  for (int i=0; i<argCnt; i++) {
    Serial.print(args[i]);
    Serial.print(", ");
  }

  if (argCnt < ARG_COUNT) {
    Serial.print("Insufficient args on parse. ");
    return nullptr;
  }
  Packet* packet = new Packet();
  packet->srcId = args[0];
  packet->id = args[1];
  packet->val = args[2];
  
  //free(args);
  return packet;
}

char* encodePacket (Packet* packet) {
  if (packet == nullptr) return nullptr;
  char* buf = (char*) malloc(MSG_SIZE);
  int cnt = 0;
  cnt += sprintf(buf+cnt, "%d:%d:%d:\0", packet->srcId, packet->id, packet->val);

  return buf;
}
// Callback func on LoRa receive
void onReceive (int packetSize) {
  // Parse Packet
  int delta = freeRam();

  Packet* packet = parsePacket(packetSize);
  if (packet == nullptr) {
    Serial.print("ERR: invalid MSG format, dropping.\n");
    return;
  }
  Serial.print("Parsed: ");
  char* buf = encodePacket(packet);
  Serial.print(buf);
  Serial.print(". ");
  free(buf);

  // If MSG srcID is self, meaning echo, drop.
  if (packet->srcId == nodeId) {
    Serial.print("Echoed packet, dropping.\n");
    goto err;
  }
  // If MSG ID is repeat or outdated, drop.
  int lastPktId = packetIdRecord.get(packet->srcId);
  Serial.print("LastPktId:");
  Serial.print(lastPktId); 
  Serial.print(". ");
  
  if (lastPktId != -1 && lastPktId >= packet->id) {
    Serial.print("Repeated or outdated packet received, dropping.\n");
    goto err;
  }
  Serial.print("New Msg. Queuing for forward. ");

  // Set new latest Pkt Id
  packetIdRecord.set(packet->srcId, packet->id);

  // Store in queue
  if (queueCnt >= QUEUE_SIZE) {
    Serial.print("Queue full, dropping.\n");
    goto err;
  }
  forwardQueue[queueCnt++] = packet;
  Serial.println();

  Serial.print("Post-Receive Inturrupt Delta: ");
  Serial.println(freeRam() - delta);
  return;
err: 
  free(packet);
  return;
}

uint32_t packetId = 1;
uint16_t tick = 0;

void loop() {
  // TX if tick counter up.

  if (tick-- == 0) {
    char msg[MSG_SIZE];
    int cnt = 0;
    int randint = random(1, 1000);
    cnt += sprintf(msg + cnt, "%d:%d:%d:\0", nodeId, packetId++, randint);

    delay(100);
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();

    Serial.print("Sent MSG: ");
    Serial.println(msg);

    delay(100);
    LoRa.receive();
    tick = (TX_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }

  // If queue
  int delta = freeRam();
  if (queueCnt) 
    delay(100);

  for (int i=0; i<queueCnt; i++) {
    char* buf = encodePacket(forwardQueue[i]);

    LoRa.beginPacket();
    LoRa.print(buf);
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
    Serial.println(freeRam() - delta);
  }
  delay(TICK_INTERVAL);
}
