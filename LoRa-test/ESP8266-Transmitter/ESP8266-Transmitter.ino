#include <SPI.h>
#include <LoRa.h>
 
#define SS 15
#define RST 16
#define DIO0 2
 
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver Callback");
 
  LoRa.setPins(SS, RST, DIO0);
 
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.onReceive(onReceive);
  LoRa.receive();

  Serial.println("Starting.");
}

int counter = 0;
String queueMsg = "";
void loop () 
{
  Serial.print(queueMsg);
  queueMsg = "";

  String msg = "ping no." + String(counter++, DEC);
  
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.print("Sent ping, '" + msg + "'\n");
  delay(10);
  LoRa.receive();

  delay(3000);
}
// Print out Pong
void onReceive (int packetSize) 
{
  queueMsg += "Received ping, '";
  while (LoRa.available()) 
    queueMsg += (char)LoRa.read();

  queueMsg += "'\n";  
}