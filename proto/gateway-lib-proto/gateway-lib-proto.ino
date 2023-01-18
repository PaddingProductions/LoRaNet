#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include "LoRaNet.h";
 
#include <SPI.h>
#include <LoRa.h>
 

#define WIFI_SSID "Puffin2"
#define WIFI_PASS "0919915740"

#define QUEUE_SIZE 10
#define ADJ_PING_INTERVAL 30000
#define TX_INTERVAL 3000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000

#define SS 15
#define RST 16
#define DIO0 2

Map packetIdRecord;
uint16_t gatewayId;

uint8_t adjacencies_cnt = 0;
uint16_t adjacencies[ADJ_LIST_SIZE];
uint16_t adj_ping_tick = 0;

uint16_t packets_lost = 0;



void setup() {
  // Randomize seed
  randomSeed(analogRead(0));
  gatewayId = random(1, 10000);
  // Start Serial
  Serial.begin(9600);
  while (!Serial);
  Serial.println("=== LoRa ESP-8266 Gateway -- Proto ===");

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

  // POST
  Serial.print("New Msg. Queuing for forward. \n");

  char buf* = Lib::encodePacketForHTTP();
  POST(buf);
  free(buf);
}

// Takes a null-terminated char array as a pointer.
void POST (char* buf) {
  Serial.print("Posting.. ");
  WiFiClient client;
  HTTPClient http;    //Declare object of class HTTPClient

  http.begin(client, "http://192.168.4.120:8282/test");      //Specify request destination
  http.addHeader("Content-Type", "text/plain");  //Specify content-type header

  int httpCode = http.POST(buf);   //Send the request

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

  // TX if adjacency list ping tick is up. 
  if (adj_ping_tick-- == 0) {
    uint16_t len = adjacencies_cnt;
    char* msg = Lib::constructGatewayAdjPkt(gateway, packetId++, adjacencies, &len);
    POST(msg);
    free(msg);
    
    Serial.print("Sent ADJ list ping: [");
    for (int i=0; i<adjacencies_cnt; i++) {
      Serial.print(adjacencies[i]);
      Serial.print(", ");
    }
    Serial.println("]");

    adjacencies_cnt = 0;
    adj_ping_tick = (ADJ_PING_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }
  delay(TICK_INTERVAL);
}