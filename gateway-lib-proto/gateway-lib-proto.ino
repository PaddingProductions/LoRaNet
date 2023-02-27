#include <Arduino.h>
#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <LoRa.h>

#include "LoRaNet.h";
#include "base64.hpp"

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

#define TEST

Map packetIdRecord;
uint16_t gatewayId;
uint16_t packetId = 1;

uint8_t adjacencies_cnt = 0;
uint16_t adjacencies[ADJ_LIST_SIZE];
uint16_t adj_ping_tick = 0;

uint16_t packets_lost = 0;
uint16_t blacklist[10] = {0};

void setup() {
  // Randomize seed
  randomSeed(analogRead(0));
#ifdef TEST 
  // Read configs from EEPROM
  EEPROM.begin(64);
  EEPROM.get(EEPROM_ADDR_ID, gatewayId);
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
  Serial.println("=== LoRa ESP-8266 Gateway <PROTO> ===");

  // Start WiFi
  {
  	
    Serial.println("Starting WiFi connection...");
    WiFi.begin(wifi_ssid, wifi_pass);
    while (WiFi.status() != WL_CONNECTED) 
      delay(500);
  }
  
  // Start LoRa 
  Serial.println("Starting LoRa...");
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (true);
  }
  // Set LoRa receive callback
  LoRa.onReceive(onReceive);
  LoRa.receive();

  Lib::init();

  // POST node spawn message
  {
    WiFiClient client;
    HTTPClient http;    //Declare object of class HTTPClient
    char URL[100];
    sprintf(URL, "http://192.168.4.120:3000/api/node?id=%d&longitude=%.3f&latitude=%.3f&gateway=true", gatewayId, 12.3, 45.6);

    Serial.print("Sending gateway PUT request... URL: ");
    Serial.println(URL);
    http.begin(client, URL);
    http.addHeader("Content-Type", "text/plain");  //Specify content-type header

    int httpCode = http.PUT("");
    if (httpCode != 201) {
      Serial.println(httpCode);
      Serial.println(http.getString());
      Serial.println("Error on spawn request, halting.");
      while(1) delay (1000);
    }
    http.end();  //Close connection
  }

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
  Serial.print(", src: ");
  Serial.print(srcId);
  Serial.print(" ===== "); Serial.print(prevId);
  Serial.print(", pId: "); Serial.print(pId);
  Serial.print(", ");
  
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
    Serial.print("Outdated.\n"); 
    return;
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
  Serial.print("POST-ing. msg: ");
  uint16_t enc_len = BASE64::encodeLength(len);
  char buf[enc_len];
  BASE64::encode((uint8_t*) msgbuf, len, buf);
  Serial.println(buf);

  POST(buf);
}

// Takes a null-terminated char array as a pointer.
void POST (char* buf) {
  WiFiClient client;
  HTTPClient http;    //Declare object of class HTTPClient

  http.begin(client, "http://192.168.4.120:3000/api/packet");      //Specify request destination
  http.addHeader("Content-Type", "text/plain");  //Specify content-type header

  int httpCode = http.POST(buf);   //Send the request

  String payload = http.getString();  //Get the response payload

  
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
    char* msg = Lib::constructAdjPkt(gatewayId, packetId++, adjacencies, &len, true); 

    uint16_t enc_len = BASE64::encodeLength(len); // Encode into b64
    char buf[enc_len];
    BASE64::encode((uint8_t*) msg, len, buf);
    free(msg);
    Serial.println(buf);

    POST(buf);

    adjacencies_cnt = 0;
    adj_ping_tick = (ADJ_PING_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }
  delay(TICK_INTERVAL);
}
