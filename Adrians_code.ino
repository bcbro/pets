/*
   Initial Author: ryand1011 (https://github.com/ryand1011)

   Reads data written by a program such as "rfid_write_personal_data.ino"

   See: https://github.com/miguelbalboa/rfid/tree/master/examples/rfid_write_personal_data

   Uses MIFARE RFID card using RFID-RC522 reader
   Uses MFRC522 - Library
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <stdlib.h>
#include <MFRC522.h>
#include <Servo.h>

#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above

// Add  '//' in front of Serial to stop debug serial writes  like this below
//  ->  #define Serial_println //Serial.println
#define Serial_println //Serial.println
#define Serial_print //Serial.print
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
Servo myservo;

//*****************************************************************************************//
void setup() {
  Serial.begin(9600);                                           // Initialize serial communications with the PC
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                              // Init MFRC522 card
  Serial_println(F("Where is Muffun?"));    //shows in serial that it is ready to read
  myservo.attach(9);
}

//*****************************************************************************************//
void loop() {

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;

  //-------------------------------------------

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial_println(F("**Card Detected:**"));

  //-------------------------------------------

  //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex

  //-------------------------------------------

  Serial_print(F("Name: "));

  byte buffer1[18];

  block = 4;
  len = 18;

  //------------------------------------------- GET FIRST NAME
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial_print(F("Authentication failed: "));
    Serial_println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial_print(F("Reading failed: "));
    Serial_println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //PRINT FIRST NAME
  for (uint8_t i = 0; i < 16; i++)
  {
    if (buffer1[i] != 32)
    {
      Serial_print((char)buffer1[i]);
    }
  }
  Serial_print(" ");


  //---------------------------------------- GET LAST NAME
  unsigned long id = getID();
  String cardID = "uid =";
  cardID = cardID + String(mfrc522.uid.uidByte[0], HEX) + String(mfrc522.uid.uidByte[1], HEX) + String(mfrc522.uid.uidByte[2], HEX) + String(mfrc522.uid.uidByte[3], HEX) + "&rftext=";
  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial_print(F("Authentication failed: "));
    Serial_println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial_print(F("Reading failed: "));
    Serial_println(mfrc522.GetStatusCodeName(status));
    return;
  }


  //PRINT LAST NAME
  for (uint8_t i = 0; i < 16; i++) {
    Serial_print((char)buffer2[i] );
  }

  Serial_println("");
  char writeBuffer[50];
  buffer1[17]=0;
  buffer2[17]=0;

  sprintf(writeBuffer,"rfid=%lu&rftext=%s_%s",id, (char*)buffer1, (char*)buffer2);
  Serial.println(writeBuffer);
  //----------------------------------------

  Serial_println(F("\n**End Reading**\n"));


  char incomingChar = ReadResponse();

    if (incomingChar == 'Y') {
      myservo.write(100);
      delay(2000);
      myservo.write(180);
      Serial_println("Hello");
    }else{
      if (incomingChar == 'N') {
        Serial_println("Go away");
      }
      else {
        Serial_println("Bleh");
      }
    }

  delay(1000); //change value if you want to read cards faster

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
//*****************************************************************************************//
const int maxcounter = 50;

char  ReadResponse() {
  int counter = 0;
  while ( counter < maxcounter) {
    if (Serial.available()) {
      //Serial.println(F("available true"));
      char ch = (char) Serial.read();
      if (rcvdResponse(Serial.read())){
        return ch;
      }
    }else{
      delay(100);
    }
    counter++;
  }
  return 'N';
}

boolean rcvdResponse(char resp) {
  char buf[15];
  sprintf(buf,"Response:%c",resp);
  Serial_println(buf);

  if (resp == 'Y' ||  resp == 'N' || resp =='I'){
    return true;
  }
}

unsigned long getID(){
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0];
  hex_num *= 256;
  hex_num += mfrc522.uid.uidByte[1];
  hex_num *= 256;
  hex_num += mfrc522.uid.uidByte[2];
  hex_num *= 256;
  hex_num += mfrc522.uid.uidByte[3];
  return hex_num;
}