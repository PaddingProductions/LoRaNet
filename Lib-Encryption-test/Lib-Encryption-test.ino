#include <Arduino.h>
#include <LoRaNet.h>

void setup() {
  Serial.begin(9600);
  Serial.println("--Starting--");

  Lib::init();

  {
  Packet packet = Packet();
  packet.srcId = 7128;
  packet.id = 99;
  packet.humidity = 50;
  packet.temp = 10;

  uint16_t len = 0;
  char* buf = Lib::encodePacket(&packet, &len); 
  
  // Expected Ciphertext (b64): [5, 28, 229, 235, 172, 82, 252, 187, 127, 247, 209, 80, 203, 165, 188, 254]
  // Expected b64 decode : [255, 255, 191, 253, 31, 191, 255, 255, 207, 255, 255, 191, ]

  Serial.print("Ciphertext, len: ");
  Serial.print(len);
  Serial.print(" [");
  for (int i=0; i<len; i++) {
    Serial.print((uint8_t) buf[i]);
    Serial.print(", ");
  }
  Serial.println("] ");

  Lib::test(buf, len);
  Serial.println();
  }

  {
  Serial.println("==============");
  char in[] = {27, 216, 0, 99, 10, 50, 0, 0};
  char ciphertext[sizeof(in) * 2] = {0};
  char out[sizeof(in)] = {0};

  byte key[] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
  {
  byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
  aesLib.encrypt64((byte *) in, sizeof(in), ciphertext, key, sizeof(key), iv);
  }

  Serial.print("Ciphertext, len: ");
  Serial.print(strlen(ciphertext));
  Serial.print(" [");
  for (int i=0; i<strlen(ciphertext); i++) {
    Serial.print((uint8_t) ciphertext[i]);
    Serial.print(", ");
  }
  Serial.println("] ");

  byte b64decoded[50] = {0};
  base64_decode((char*) b64decoded, ciphertext, strlen(ciphertext));
  
  Serial.print("b64 decoded, len: ");
  Serial.print(strlen((char*) b64decoded));
  Serial.print(" [");
  for (int i=0; i<strlen((char*) b64decoded); i++) {
      Serial.print((uint8_t) b64decoded[i]);
      Serial.print(", ");
  }
  Serial.println("]");

  {
    byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint16_t olen = aesLib.decrypt(b64decoded, strlen((char*) b64decoded), (byte*) out, key, sizeof(key), iv);
  }

  Serial.print("decrypted cleartext ");
  Serial.print(" [");
  for (int i=0; i<sizeof(in); i++) {
    Serial.print((uint8_t) out[i]);
    Serial.print(", ");
  }
  Serial.println("] ");

  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
