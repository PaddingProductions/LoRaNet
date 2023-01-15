#include "AESLib.h"
#include <Arduino.h>

AESLib aesLib;

char cleartext[64] = {0}; // THIS IS INPUT BUFFER (FOR TEXT)
char ciphertext[64] = {0}; // THIS IS OUTPUT BUFFER (FOR BASE64-ENCODED ENCRYPTED DATA)

char readBuffer[] = { 27, 216, 0, 99, 10, 50, 0, 0 };

// AES Encryption Key (same as in node-js example)
byte aes_key[] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

uint16_t encrypt_to_ciphertext(char * msg, uint16_t msgLen) {
  byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
  return aesLib.encrypt((byte*)msg, msgLen, (byte*)ciphertext, aes_key, sizeof(aes_key), iv);
}

uint16_t decrypt_to_cleartext(byte msg[], uint16_t msgLen) {
  byte iv[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
  return aesLib.decrypt(msg, msgLen, (byte*)cleartext, aes_key, sizeof(aes_key), iv);
}

void setup() {
  Serial.begin(9600);
  delay(2000);
  aesLib.set_paddingmode((paddingMode)0);
}

void loop() {
  {
    Serial.print("readBuffer length: "); 
    Serial.println(sizeof(readBuffer));
    memcpy(cleartext, readBuffer, sizeof(readBuffer));
    Serial.flush();

    uint16_t msgLen = sizeof(readBuffer);
    uint16_t encLen = encrypt_to_ciphertext((char*)cleartext, msgLen);
    Serial.print("Encrypted length = "); Serial.println(encLen );

    Serial.println("Encrypted. Decrypting..."); Serial.println(encLen ); Serial.flush();
    Serial.print("Ecrypted ciphertext (b64): [");
    for (int i=0; i<encLen; i++) {
      Serial.print((uint8_t) ciphertext[i]);
      Serial.print(", ");
    }
    Serial.println("]");

    char base64decoded[50] = {0};
    base64_decode((char*)base64decoded, (char*)ciphertext, encLen);

    Serial.print("Raw ciphertext, len:");
    Serial.print(strlen(base64decoded));
    Serial.print(", [");
    for (int i=0; i<strlen(base64decoded); i++) {
      Serial.print((uint8_t) base64decoded[i]);
      Serial.print(", ");
    }
    Serial.println("]");

    uint16_t decLen = decrypt_to_cleartext(base64decoded, strlen((char*)base64decoded));
    Serial.print("Decrypted cleartext of length: "); Serial.println(decLen);
    Serial.print("Decrypted cleartext: [");
    for (int i=0; i<decLen; i++) {
      Serial.print((uint8_t) cleartext[i]);
      Serial.print(", ");
    }
    Serial.println("]");

    if (strcmp((char*)readBuffer, (char*)cleartext) == 0) {
      Serial.println("Decrypted correctly.");
    } else {
      Serial.println("Decryption test failed.");
    }
    Serial.println("---");
  }
  {
    /*
    Serial.println("== Testing encryption ==");

    char readBuffer[] = "HELLO!";//{ 37, 166, 0, 20, 24, 51, 0, 0 };
    uint16_t msgLen = sizeof(readBuffer);
    Serial.print("readBuffer length: "); 
    Serial.println(msgLen);
    
    char buf[50];
    memcpy(buf, readBuffer, msgLen);

    Serial.flush();
    byte iv1[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint16_t encLen = aesLib.encrypt(buf, msgLen, cipherbuf, aes_key, sizeof(aes_key), iv1);
    Serial.print("Encrypted length = "); Serial.println(encLen);
    Serial.print("Ciphertext: ");
    Serial.println((char *) cipherbuf);
    Serial.println("Decrypting...");

    unsigned char decoded[50] = {0};
    base64_decode(decoded, cipherbuf, encLen);

    byte iv2[N_BLOCK] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    uint16_t decLen = aesLib.decrypt(decoded, strlen(decoded), cipherbuf, aes_key, sizeof(aes_key), iv2);

    Serial.print("Decrypted cleartext of length: "); Serial.println(decLen);
    Serial.print("Decrypted cleartext: [");
    for (int i=0; i<decLen; i++) {
      Serial.print((uint8_t) cipherbuf[i]);
      Serial.print(", ");
    }
    Serial.println("]");

    if (strcmp((char*)readBuffer, (char*)cipherbuf) == 0) {
      Serial.println("Decrypted correctly.");
    } else {
      Serial.println("Decryption test failed.");
    }
    Serial.println("---");
    */
  }
  delay(3000);
}