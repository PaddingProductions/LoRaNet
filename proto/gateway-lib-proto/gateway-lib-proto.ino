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
#define TX_INTERVAL 3000
#define TICK_INTERVAL 100
#define RANDOM_RANGE 2000

#define SS 15
#define RST 16
#define DIO0 2

Map packetIdRecord;
uint16_t gatewayId;

void setup() {
  // Randomize seed
  randomSeed(analogRead(0));
  gatewayId = random(1, 10000);
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

// Callback func on LoRa receive
void onReceive (int packetSize) {
  // Parse Packet
  Packet* packet = new Packet();

  if (Lib::parsePacket(packetSize, packet)) {
    Serial.print("ERR: invalid MSG format, dropping.\n");
    free(packet);
    return;
  }
  Serial.print("Parsed: ");
  Lib::printPacket(packet);
  Serial.println();

  // If MSG srcID is self, meaning echo, drop.
  if (packet->srcId == gatewayId) {
    Serial.print("Echoed packet, dropping.\n");
    free(packet);
    return;
  }
  // If MSG ID is repeat or outdated, drop.
  int lastPktId = packetIdRecord.get(packet->srcId);
  Serial.print("LastPktId:");
  Serial.print(lastPktId); 
  Serial.print(". ");
  
  if (lastPktId != -1 && lastPktId >= packet->id) {
    Serial.print("Repeated or outdated packet received, dropping.\n");
    free(packet);
    return;
  }
  Serial.print("New Msg. POST-ing.\n");

  // Set new latest Pkt Id
  packetIdRecord.set(packet->srcId, packet->id);
  postPacket(packet);
}

void postPacket (Packet* packet) {
  Serial.print("Posting.. ");
  WiFiClient client;
  HTTPClient http;    //Declare object of class HTTPClient

  http.begin(client, "http://192.168.4.120:8282/test");      //Specify request destination
  http.addHeader("Content-Type", "text/plain");  //Specify content-type header

  uint16_t len;
  char* buf = Lib::encodePacketForHTTP(packet, &len);
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