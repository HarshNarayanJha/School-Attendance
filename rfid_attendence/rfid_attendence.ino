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

const int numStudents = 3;

const byte UID_ARRAY[numStudents][16] = {
    171819116000143, 171819116000144, 171819116000145,
};

const byte UID_BLOCK = 1;
const byte NAME_BLOCK = 2;

byte currentUID[18];
byte currentName[18];

// Latest Attendence Time
const int checkInHour = 6;
const int checkInMinute = 55;

// Current Student's Arrival Time
int studentCheckInHour;
int studentCheckInMinute;

// LED's Pins
const int RED_LED = 6;
const int GREEN_LED = 7;

void setup() {
    // Set LEDs to OUTPUT
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    
    Serial.begin(9600);
    
    SPI.begin();
    rfid.PCD_Init();
    
    Serial.println("Initializing SD Card");
    
    if (!SD.begin(CS_SD)) {
        Serial.println("SD Card Initialization Failed");
        while(1);
    }
    
    Serial.println("SD Card Initialization Done");
    
    if (!rtc.begin()) {
        Serial.println("Couldn't Find RTC");
        while(1);
        
    } else {
        // Set the date and time when the sketch was compiled
        // For the first time only, comment afterwards
        rtc.setDateTime(__DATE__, __TIME__);
        
        // Or setting it absolutely
        // rtc.setDate(2021, 9, 23);
        // rtc.setTime(4, 31, 30);
        
        // Comment out this also after setting date time...
        Serial.println("Date Time set!");
    }
    
    if (!rtc.isRunning()) {
        Serial.println("RTC Not Running");
    }

    lcd.begin(16, 2);
    lcd.print("Hello School!");
    delay(1500);
    lcd.clear();

    // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

// Pads singal digits with a 0 to the start
String pad(int val) {
    if (val > 9) {
        return String(val);
    } else {
        return String("0" + val);
    }
}

// Used for finding the index of an byte array in an array
int indexOfArray(byte ar[][16], byte elem[16]) {
    int index = -1;
    int len = sizeof(ar) / sizeof(ar[0]);
    for (int i = 0; i < len; ++i)
    {
        if (elem == ar[i]) {
            index = i;
            break;
        }
    }

    return index;
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
        lcd.print("Authentication Failed: Contact Us");
        return;
    }

    // Read UID from the card
    status = rfid.MIFARE_Read(UID_BLOCK, currentUID, 18);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading UID failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print("Reading from Card Failed: Contact Us");
        return;
    }

    // Authenticate for NAME_BLOCK
    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, NAME_BLOCK, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print("Authentication Failed: Contact Us");
        return;
    }

    // Read Name from the card
    status = rfid.MIFARE_Read(NAME_BLOCK, currentName, 18);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading Name failed: "));
        Serial.println(rfid.GetStatusCodeName(status));
        lcd.print("Reading from Card Failed: Contact Us");
        return;
    }
    
    int i = indexOfArray(UID_ARRAY, currentUID);
    if (i != -1) {
        //currentName = NAMES[i];
        digitalWrite(GREEN_LED, HIGH);
        // Print Name
        Serial.print(F("Welcome "));
        lcd.print("Welcome ");
        for (uint8_t i = 0; i < 16; i++)
        {
            if (currentName[i] != 32) {
                Serial.write(currentName[i]);
                lcd.write(currentName[i]);
            }
        }
        delay(1500);
        digitalWrite(GREEN_LED, LOW);

        lcd.clear();
        lcd.setCursor(0, 0);
        
        // Print UID
        Serial.print(F("Your UID is: "));
        for (uint8_t i = 0; i < 16; i++)
        {
            if (currentUID[i] != 32) Serial.write(currentUID[i]);
        }
        
        Serial.println(F("\n**End Reading**\n"));
        delay(1000); //change value if you want to read cards faster
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        
        markAttendance();
    } else {
        digitalWrite(RED_LED, HIGH);
        Serial.println("Sorry, We Don't know you!");
        delay(2000);
        digitalWrite(RED_LED, LOW);
    }
}

// Marks Attendence
void markAttendance() {
    Serial.println(F("Marking You Attendance"));
    lcd.print("Marking Your Attendance");
    // Enable the SD Card chip select Pin
    digitalWrite(CS_SD, LOW);
    
    studentCheckInHour = rtc.getHours();
    studentCheckInMinute = rtc.getMinutes();
    int seconds = rtc.getSeconds();

    lcd.clear();
    // Print Current Time
    //lcd.print(String(studentCheckInHour).concat(":").concat(studentCheckInMinute).concat(":").concat(seconds));
    
    int date = rtc.getDay();
    int month = rtc.getMonth();
    int year = rtc.getYear();
    
    lcd.clear();
    // Print Today's Date
    //lcd.print(String(date).concat("-").concat(month).concat("-").concat(year));

    char filename = "Attendance_";
    sprintf(filename, "%d_%d_%d.txt", year, month, date); 
    file = SD.open(filename, FILE_WRITE);
    
    if (file) {
        Serial.println("File Opened Ok");
        delay(500);
        
        // Write the UID to file.
        for (uint8_t i = 0; i < 16; i++)
        {
            if (currentUID[i] != 32) file.write(currentUID[i]);
        }
        // file.print(currentUID);
        file.print(",");
        
        // Then the Time in HH:MM:SS Format
        file.print(pad(studentCheckInHour));
        file.print(":");
        file.print(pad(studentCheckInMinute));
        file.print(":");
        file.print(pad(seconds));
        
        file.print(",");

        lcd.clear();
        // Then whether the student was on time?
        if ((studentCheckInHour < checkInHour) || (studentCheckInHour == checkInHour) && (studentCheckInMinute <= checkInMinute)) {
            digitalWrite(GREEN_LED, HIGH);
            Serial.println("You are on Time!");
            
            lcd.print("You are on Time");
            file.print("True");
            delay(500);
            digitalWrite(GREEN_LED, LOW);
            
        } else {
            digitalWrite(RED_LED, HIGH);
            Serial.println("You are Late!");
            
            lcd.print("You are Late Today");
            file.print("False");
            delay(500);
            digitalWrite(RED_LED, LOW);
        }
        
        file.print("\n");
        delay(500);
        
        file.close();
        
    } else {
        Serial.println("Error opening file, Contact the maintainers of this thing.");
        lcd.print("Error Opening File");
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

    lcd.print("Card Found");
    lcd.setCursor(0, 1);
    lcd.print("Scanning it...");
    
    readRFID();

    lcd.clear();
    lcd.print("Welcome to the Class");
    
    delay(10);
}
