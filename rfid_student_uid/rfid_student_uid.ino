#include <MFRC522.h>
#include <SPI.h>

// define Pins for RFID
#define CS_RFID 10
#define RST_RFID 9

MFRC522 rfid(CS_RFID, RST_RFID);
MFRC522::MIFARE_Key key;

const int UID_BLOCK = 1;
const int NAME_BLOCK = 2;

void setup() {
    
    Serial.begin(9600);
    
    SPI.begin();
    rfid.PCD_Init();

    Serial.println(F("This Program Writes the Student's UID and Name to his/her RFID Cards on by one."));
    Serial.println(F("Just bring the Card closer, keep it still, enter the UID and Name, and data will be written onto the card."));

    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
    // Look for new cards
    if (!rfid.PICC_IsNewCardPresent()) 
        return;
        
    // Select one of the cards
    if (!rfid.PICC_ReadCardSerial()) 
        return;

    Serial.println(F("Card Detected... Keep it still until writing the data..."));
    MFRC522::StatusCode status;
    

    // Dump UID
    Serial.print(F("Card UID:"));
    for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
    }
    
    byte buffer[34];
    
    Serial.setTimeout(20000L);              // wait until 20 seconds for input from serial
    
    // Get Student UID
    Serial.println(F("Enter the UID of current Student (end it with a #)"));
    byte len = Serial.readBytesUntil('#', (char *) buffer, 30) ; // read UID from serial
    for (byte i = len; i < 30; i++) buffer[i] = ' ';        // pad with spaces

    Serial.print(F("UID: "));
    for (byte i = 0; i < 34; i++) {
        Serial.print(buffer[i]);
    }
    Serial.println();
    
    // Authenticate for UID_BLOCK
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, UID_BLOCK, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        return;
    }
    
    // Write UID to card
    status = rfid.MIFARE_Write(UID_BLOCK, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: Writing UID failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        return;
    } else {
        Serial.println(F("MIFARE_Write() success: Writing UID Success"));
    }
    delay(10);
    
    Serial.setTimeout(20000L);              // wait until 20 seconds for input from serial
    
    // Get Student Name
    Serial.println(F("Enter the Name of current Student (end it with a #)"));
    len = Serial.readBytesUntil('#', (char *) buffer, 30) ; // read UID from serial
    for (byte i = len; i < 30; i++) buffer[i] = ' ';        // pad with spaces

    Serial.print(F("Name: "));
    for (byte i = 0; i < 34; i++) {
        Serial.println(buffer[i]);
    }

    Serial.println();

    // Authenticate for NAME_BLOCK
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, NAME_BLOCK, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        return;
    }

    // Write Name
    status = rfid.MIFARE_Write(NAME_BLOCK, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: Writing Name failed"));
        Serial.println(rfid.GetStatusCodeName(status));
        return;
    } else {
        Serial.println(F("MIFARE_Write() success: Writing Name Success"));
    }

    rfid.PICC_HaltA(); // Halt PICC
    rfid.PCD_StopCrypto1();  // Stop encryption on PCD

    Serial.println(F("Next Student Please..."));
}
