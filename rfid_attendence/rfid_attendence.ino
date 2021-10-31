#include <MFRC522.h>
#include <SPI.h>
#include <SD.h>
#include <RTC.h>
#include <LiquidCrystal.h>

// define Pins for RFID
#define CS_RFID 10
#define RST_RFID 9

// define select pins for SD
#define CS_SD 8

File file;
MFRC522 rfid(CS_RFID, RST_RFID);
MFRC522::MIFARE_Key key;
DS3231 rtc;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// const int numStudents = 3;
#define numStudents 3

const long UID_ARRAY[numStudents] = {
    171819116000143, 171819116000144, 171819116000145,
};

// const int UID_BLOCK = 1;
// const int NAME_BLOCK = 2;
#define UID_BLOCK 1
#define NAME_BLOCK 2

long currentUID;
char currentName[18];

// Latest Attendence Time
// const int checkInHour = 6;
// const int checkInMinute = 55;
#define checkInHour 6
#define checkInMinute 55

// Current Student's Arrival Time
byte studentCheckInHour;
byte studentCheckInMinute;

// LED's Pins
#define RED_LED 6
#define GREEN_LED 7

void setup() {
    // Set LEDs to OUTPUT
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    
    Serial.begin(9600);
    
    SPI.begin();
    rfid.PCD_Init();
    
    Serial.println(F("Initializing SD Card"));
    
    if (!SD.begin(CS_SD)) {
        Serial.println(F("SD Card Initialization Failed"));
        while(1);
    }
    
    Serial.println(F("SD Card Initialization Done"));
    
    if (!rtc.begin()) {
        Serial.println(F("Couldn't Find RTC"));
        while(1);
        
    } else {
        // Set the date and time when the sketch was compiled
        // For the first time only, comment afterwards
        rtc.setDateTime(__DATE__, __TIME__);
        
        // Or setting it absolutely
        // rtc.setDate(2021, 9, 23);
        // rtc.setTime(4, 31, 30);
        
        // Comment out this also after setting date time...
        Serial.println(F("Date Time set!"));
    }
    
    if (!rtc.isRunning()) {
        Serial.println(F("RTC Not Running"));
    }

    lcd.begin(16, 2);
    lcd.print(F("Hello School!"));
    delay(1500);
    lcd.clear();

    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

// Used for finding the index of an long in an array
int indexOfLong(const long ar[], int len, long elem) {
    for (int i = 0; i < len; ++i)
    {
        if (elem == ar[i]) {
            return i;
        }
    }

    return -1;
}

// Reads the RFID card
void readRFID() {
        
    // Dump UID
    Serial.print(F("Card UID:"));
    for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
    }
    
    // We want to somehow store and get
    // the student UID and student Name from the Card
    // Guess Done...

    MFRC522::StatusCode status;

    lcd.clear();
    lcd.setCursor(0, 0);

    // Authenticate for UID_BLOCK
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, UID_BLOCK, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print(F("Authentication Failed: Contact Us"));
        return;
    }

    byte temp_read[18];
    // Read UID from the card
    status = rfid.MIFARE_Read(UID_BLOCK, temp_read, 18);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading UID failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print(F("Reading from Card Failed: Contact Us"));
        return;
    }

    currentUID = atol(temp_read);

    // Authenticate for NAME_BLOCK
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, NAME_BLOCK, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print(F("Authentication Failed: Contact Us"));
        return;
    }

    // Read Name from the card
    status = rfid.MIFARE_Read(NAME_BLOCK, temp_read, 18);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading Name failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print(F("Reading from Card Failed: Contact Us"));
        return;
    }

    // currentName = atol(temp_read);
    *currentName = reinterpret_cast<const char*>(temp_read);
    
    int i = indexOfLong(UID_ARRAY, numStudents, currentUID);
    if (i != -1) {
        //currentName = NAMES[i];
        digitalWrite(GREEN_LED, HIGH);
        // Print Name
        Serial.print(F("Welcome "));
        Serial.println(currentName);

        lcd.print(F("Welcome "));
        lcd.print(currentName);

        delay(1500);
        digitalWrite(GREEN_LED, LOW);

        lcd.clear();
        lcd.setCursor(0, 0);
        
        // Print UID
        Serial.print(F("Your UID is: "));
        Serial.println(currentUID);
        
        Serial.println(F("\n**End Reading**\n"));
        delay(1000); //change value if you want to read cards faster
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        
        markAttendance();
    } else {
        digitalWrite(RED_LED, HIGH);
        Serial.println(F("Sorry, We Don't know you!"));
        lcd.print(F("Sorry, We Don't know you!"));
        delay(2000);
        digitalWrite(RED_LED, LOW);
    }
}

// Marks Attendence
void markAttendance() {
    Serial.println(F("Marking You Attendance"));
    lcd.print(F("Marking Your Attendance"));
    // Enable the SD Card chip select Pin
    digitalWrite(CS_SD, LOW);
    
    studentCheckInHour = rtc.getHours();
    studentCheckInMinute = rtc.getMinutes();
    int seconds = rtc.getSeconds();

    lcd.clear();
    // Print Current Time
    lcd.print(studentCheckInHour);
    lcd.print(F(":"));
    lcd.print(studentCheckInMinute);
    lcd.print(F(":"));
    lcd.print(seconds);
    
    int date = rtc.getDay();
    int month = rtc.getMonth();
    int year = rtc.getYear();
    
    lcd.clear();
    // Print Today's Date
    lcd.print(date);
    lcd.print(F("-"));
    lcd.print(month);
    lcd.print(F("-"));
    lcd.print(year);

    char filename = "Attendance_";
    sprintf(filename, "%d_%d_%d.txt", year, month, date); 
    file = SD.open(filename, FILE_WRITE);
    
    if (file) {
        Serial.println(F("File Opened Ok"));
        delay(500);
        
        // // Write the UID to file.
        // for (uint8_t i = 0; i < 16; i++)
        // {
        //     if (currentUID[i] != 32) file.write(currentUID[i]);
        // }
        file.print(currentUID);
        file.print(F(","));
        
        // Then the Time in HH:MM:SS Format
        if (studentCheckInHour < 9) {
            file.print(0);
            file.print(studentCheckInHour);
        } else {
            file.print(studentCheckInHour);
        }
        file.print(F(":"));
        if (studentCheckInMinute < 9) {
            file.print(0);
            file.print(studentCheckInMinute);
        } else {
            file.print(studentCheckInMinute);
        }
        file.print(F(":"));
        if (seconds < 9) {
            file.print(0);
            file.print(seconds);
        } else {
            file.print(seconds);
        }
        
        file.print(F(","));

        lcd.clear();
        // Then whether the student was on time?
        if ((studentCheckInHour < checkInHour) || (studentCheckInHour == checkInHour) && (studentCheckInMinute <= checkInMinute)) {
            digitalWrite(GREEN_LED, HIGH);
            Serial.println(F("You are on Time!"));
            
            lcd.print(F("You are on Time"));
            file.println(F("True"));
            delay(500);
            digitalWrite(GREEN_LED, LOW);
            
        } else {
            digitalWrite(RED_LED, HIGH);
            Serial.println(F("You are Late!"));
            
            lcd.print(F("You are Late Today"));
            file.println(F("False"));
            delay(500);
            digitalWrite(RED_LED, LOW);
        }
        
        delay(500);
        
        file.close();
        
    } else {
        Serial.println(F("Error opening file, Contact the maintainers of this thing."));
        lcd.print(F("Error Opening File"));
    }
    
    // Disable the SD Card Chip sleect Pin
    digitalWrite(CS_SD, HIGH);
}


void loop() {
    // Look for new cards
    if (!rfid.PICC_IsNewCardPresent()) 
        return;
    // Select one of the cards
    if (!rfid.PICC_ReadCardSerial()) 
        return;

    lcd.print(F("Card Found"));
    lcd.setCursor(0, 1);
    lcd.print(F("Scanning it..."));
    
    readRFID();

    lcd.clear();
    lcd.print(F("Welcome to the Class"));
    
    delay(10);
}
