#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
 
#include <SPI.h>
#include <LoRa.h>
 

#define WIFI_SSID "Puffin2"
#define WIFI_PASS "0919915740"

#define QUEUE_SIZE 10
#define TX_INTERVAL 3000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000
#define MAP_SIZE 20
#define ARG_COUNT 4
#define MSG_SIZE 20

#define SS 15
#define RST 16
#define DIO0 2

struct Packet {
  uint16_t srcId = 0;
  uint16_t id = 0;
  int16_t temp = 0;
  int16_t humidity = 0;
};

struct Map {
  int16_t val[MAP_SIZE];
  uint16_t key[MAP_SIZE];
  uint16_t size = 0;

  int16_t get(uint16_t k) {
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

uint16_t nodeId;

void setup() {
  // Randomize seed
  randomSeed(analogRead(0));
  nodeId = random(1, 10000);
  // Start Serial
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa ESP-8266 Node -- Proto --");

  // Start WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Waiting for connection");
  }

  // Start LoRa 
  LoRa.setPins(SS, RST, DIO0);
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
  packet->temp = args[2];
  packet->humidity = args[3];
  
  return packet;
}

char* encodePacket (Packet* packet) {
  if (packet == nullptr) return nullptr;
  char* buf = (char*) malloc(MSG_SIZE);
  int cnt = 0;
  cnt += sprintf(buf+cnt, "%d:%d:", packet->srcId, packet->id);
  cnt += sprintf(buf+cnt, "%d:%d:\0", packet->temp, packet->humidity);

  return buf;
}

// Callback func on LoRa receive
void onReceive (int packetSize) {
  // Parse Packet
  int16_t lastPktId;
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
    goto ret;
  }
  // If MSG ID is repeat or outdated, drop.
  lastPktId = packetIdRecord.get(packet->srcId);
  Serial.print("LastPktId:");
  Serial.print(lastPktId); 
  Serial.print(". ");
  
  if (lastPktId != -1 && lastPktId >= packet->id) {
    Serial.print("Repeated or outdated packet received, dropping.\n");
    goto ret;
  }
  Serial.print("New Msg. Forwarding.\n");

  // Set new latest Pkt Id
  packetIdRecord.set(packet->srcId, packet->id);
  postPacket(packet);
ret: 
  free(packet);
  return;
}

void postPacket (Packet* packet) {
  Serial.print("Posting... ");
  WiFiClient client;
  HTTPClient http;    //Declare object of class HTTPClient

  http.begin(client, "http://192.168.4.120:8282/test");      //Specify request destination
  http.addHeader("Content-Type", "text/plain");  //Specify content-type header

  char* buf = encodePacket(packet);
  int httpCode = http.POST(buf);   //Send the request
  free(buf);

  String payload = http.getString();                  //Get the response payload

  Serial.print("Code: ");
  Serial.print(httpCode);   //Print HTTP return code
  Serial.print(", Res: ");
  Serial.println(payload);    //Print request response payload

  http.end();  //Close connection
}

void loop() {
  // If Rx
  int packetSize = LoRa.parsePacket();
  while (packetSize) {
    onReceive(packetSize);
    packetSize = LoRa.parsePacket();
  }
  delay(TICK_INTERVAL);
}