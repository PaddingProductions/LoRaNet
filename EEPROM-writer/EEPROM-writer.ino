/*
  This program is for setting the EEPROM memory of Nodes & Gateways.
  It can be used to assign fixed ID's or Blacklists for testing.
*/

#include <EEPROM.h>
#include <LoRaNet.h>

void setup () {
  // put your setup code here, to run once:
  Serial.begin(9600);

  setID(42069);

  uint16_t list[] = {};
  setBlacklist(list, sizeof(list)/sizeof(list[0]));
}

void setID (uint16_t id_in) {
  Serial.print("Writing ID to EEPROM @ addr = 0x");
  Serial.println(EEPROM_ADDR_ID, HEX);

  EEPROM.put(EEPROM_ADDR_ID, id_in);

  delay(2000);

  Serial.println("Testing read...");
  uint16_t id_out = 0;
  EEPROM.get(EEPROM_ADDR_ID, id_out);
  Serial.print("ID retrived from EEPROM: ");
  Serial.println(id_out); 
}

void setBlacklist (uint16_t* ptr, uint16_t len) {
  Serial.print("Writing Blacklist to EEPROM @ addr = 0x");
  Serial.println(EEPROM_ADDR_BLACKLIST, HEX);

  for (int i=0; i<min(10, len); i++) 
    EEPROM.write(EEPROM_ADDR_BLACKLIST + i*2, ptr[i]);
  
  delay (2000);

  Serial.println("Testing read...");

  uint16_t out[10];
  for (int i=0; i<10; i++) {
    EEPROM.get(EEPROM_ADDR_BLACKLIST + i*2, out[i]);
    if (out[i] == 0) return;
  }
  Serial.print("Blacklist retrieved from EEPROM: [");
  for (int i=0; i<10 && out[i] != 0; i++) {
    Serial.print(out[i]);
    Serial.print(", ")
  }
  Serial.println("]");
}

void loop () {

}