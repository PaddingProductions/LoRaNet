#include <SPI.h>
#include <LoRa.h>

void setup () 
{
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Arduino Transmitter");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while(1);
  }
  Serial.println("LoRa init done.");

  Serial.println("Starting.");
}

int counter = 0;
void loop () 
{
  String msg = "ping no." + String(counter++, DEC);
    
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  
  Serial.println("Sent msg: '" + msg + "'");

  delay(3000);
}
