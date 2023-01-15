/**
  Note on this code:
    This is the code for a Ping-Pong node, meaning each node will ping the peer, expecting a pong/ACK packet.
  The purpose of this code is to assess the consistency of a Duplex Transmission protocol, identifying the 
  'right' way to program such a system.

  Implementation:
    An onReceive callback function will push an incomming 'ping' MSG into a queue. The queue will be processd in the main loop,
  sending a 'pong' for every 'ping'. The program will also record the packet loss rate.
*/

#include <SPI.h>
#include <LoRa.h>

#define TICK_INTERVAL 100
#define TX_INTERVAL 3000
#define QUEUE_SIZE 10
#define RANDOM_RANGE 3000

String queue[QUEUE_SIZE];
int queueCnt = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Arduino Receiver");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.onReceive(onReceive);  
  LoRa.receive();
  Serial.println("LoRa init done.");

  Serial.println("Starting.");
}

// Queue - based callback implementation. TX not implemented, but does TX Pong
int counter = 0;
int sucesses = 0;
int tick = 0;

void loop() 
{
  if (!tick --) {
    // Log sucess %
    if (counter % 10 == 0 ) {
      Serial.println("=== Packet Report, Acknowledged / Sent: " + String(sucesses, DEC) +"/"+ String(counter,DEC)
        + " (" + (double) sucesses / counter + ") ===");
    }

    String msg = "ping no." + String(counter++, DEC);
    delay(100);
  
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();

    Serial.print("Sent ping, '" + msg + "'\n");
    delay(100);
    LoRa.receive();

    tick = (TX_INTERVAL + random(0, RANDOM_RANGE)) / TICK_INTERVAL;
  }
  
  // Check & process msg queue.
  if (queueCnt) 
    delay(100);
  for (int i=0; i<queueCnt; i++) {
    String ping = queue[i];

    String numStr = ""; 
    for (int i = ping.lastIndexOf("no.") + 3; i<ping.length(); i++)
      numStr += ping[i];
    int number = numStr.toInt();

    
    // Respond pong
    String msg = "pong no." + String(number, DEC);
    
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();

    //Serial.print("Sent pong, '" + msg + "'\n");
  }
  if (queueCnt) {
    queueCnt = 0;
    delay(100);
    LoRa.receive();
  }

  delay(TICK_INTERVAL);
}

void onReceive(int packetSize) 
{
  // Store MSG in queue.
  String msg = "";
  while (LoRa.available()) 
    msg += (char)LoRa.read();
  
  // If queue full, drop.
  if (queueCnt >= QUEUE_SIZE) {
    Serial.println("Queue full, dropping.");
    return; 
  }

  // If is ping
  if (msg.lastIndexOf("ping") != -1) {
    //Serial.println("Got ping: " + msg + "'. Storing in queue.");
    queue[queueCnt++] = msg;
  } else { // If pong
    Serial.println("Got pong: " + msg + "'.");
    sucesses ++;
  }
}