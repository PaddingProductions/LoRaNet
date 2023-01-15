#include <SPI.h>
#include <LoRa.h>

String queue[10];
int queueCnt = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Arduino Ping-Pong node");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa init done.");

  Serial.println("Starting.");
}

int counter = 0;
int tick = 0;
void loop () 
{
  // Ping every 3 sec
  if (++tick > 3000) {
    String msg = "ping no." + String(counter++, DEC);

    Serial.println("Sending msg: '" + msg + "'");

    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();
        
    tick = 0;
  } 
  
  // If Ping
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    String msg = "";
    for (int i=0; i < packetSize; i++) 
      if (LoRa.available() > 0) msg += (char)LoRa.read();
      else break;

    if (msg.length() == 0) return;
    Serial.print("Got msg, size: " + String(packetSize, DEC) + ": '" + msg + "' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
  delay(1);
}