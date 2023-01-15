#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
 
#define WIFI_SSID "Puffin2"
#define WIFI_PASS "0919915740"

void setup() { 
  Serial.begin(115200);                 //Serial connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);   //WiFi connection

  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
    delay(500);
    Serial.println("Waiting for connection");
  }
}
 
void loop() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    WiFiClient client;
    HTTPClient http;    //Declare object of class HTTPClient
 
    http.begin(client, "http://192.168.4.120:8282/test");      //Specify request destination
    http.addHeader("Content-Type", "text/plain");  //Specify content-type header
 
    int httpCode = http.POST("Hello Node.js from ESP8266!");   //Send the request
    String payload = http.getString();                  //Get the response payload
 
    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload
 
    http.end();  //Close connection
  } else {
    Serial.println("Error in WiFi connection");
  }
  delay(3000);  //Send a request every 30 seconds
}