/* mifare ultralight example 
 *  
 *
 * Project school-tag-remote-station-arduino 
 * Description: Arduino Mifare MFRC522 NFC reader/writer & Ntag 213(ultralight) with arduino 
 * Authors: Vikas Singh, Aaron Roller
 * Date:10 Aug, 2019
 =======================
 *
 *   RFID-RC522 (SPI connexion)
 *   
 *   CARD RC522      Arduino (UNO)
 *     SDA  -----------  10 (Configurable, see SS_PIN constant)
 *     SCK  -----------  13
 *     MOSI -----------  11
 *     MISO -----------  12
 *     IRQ  ----------- 
 *     GND  -----------  GND
 *     RST  -----------  9 (onfigurable, see RST_PIN constant)
 *     3.3V ----------- 3.3v
 *     
 =========================
 * This program works specifically with compatible School Tags (Ntag 213 with 144 bytes).
 * The student scans the tag and this station will log the visit to the station recording:
 *  - The Station ID 
 *  - The date and time of the visit
 * 
 * When the student arrives to school, the data will be read from the tag and the game will know which stations
 * were scanned.  
 * 
 * School Tags have a single NDEF record which is the URL of the Players Scoreboard:
 * 
 * https://schooltag.org/tag/{UID} 
 * 
 * The visit data resides in binary format after the NDEF message, referenced by START_PAGE_ADDR.
 * 
 * -------------------------------
 * |      4 Byte Header          |
 * -------------------------------
 * | byte |   Description        |
 * -------------------------------
 * | 0    | Format version       |
 * -------------------------------
 * | 1    | Reserved             |
 * -------------------------------
 * | 2    | Record Count         |
 * -------------------------------
 * | 3    | Reserved             |
 * -------------------------------
 * | 4-n  | Records              | 
 * -------------------------------
 * 
 * 
 * -------------------------------
 * |     8 Byte Record           |
 * -------------------------------
 * | 0-1    | Station ID         |
 * -------------------------------
 * | 2-3    | Reserved           |
 * -------------------------------
 * | 4-7    | Unix Timestamp     |
 * -------------------------------
 * 
 * Possibilities for Reserved bytes:
 * - Station battery life
 * - Number of station scans today
 * - 
 */

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 6
#define TAG_CAPACITY_BYTES 144
#define PAGE_SIZE 4
#define ULTRALIGHT_READ_SIZE 16
#define START_PAGE_ADDR 22  // reference to the 4 byte page where school tag visits start
#define FINAL_PAGE_ADDR 37  // reference to the last 4 byte page where school tag visits are saved
#define RESET_CHAR 0xFF     //used to wipe out unused data indicating no visit data is stored
#define FORMAT_VERSION 0xBC //unique identifier for the start of the visit records

#define FORMAT_VERSION_INDEX 0
#define RECORD_COUNT_INDEX 2
#define MAX_RECORDS 5

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::StatusCode status;       //variable to get card status

byte buffer[18]; //data transfer buffer (16+2 bytes data+CRC)
byte size = sizeof(buffer);

String mostRecentTagUid;

void setup()
{
  Serial.begin(9600); // Initialize serial communications with the PC
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  Serial.println(F("Sketch has been started!"));
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial())
    return;

  //indicate tag is being read
  digitalWrite(LED_BUILTIN, HIGH); //light up LED to indicate communication

  String currentUid = uid();
  if (mostRecentTagUid == currentUid)
  {
    Serial.print(F("Already read Tag "));
    Serial.println(currentUid);
    return;
  }

  Serial.print(F("UID: "));
  Serial.println(currentUid);

  mostRecentTagUid = uid();
  bool writeSucceeded = writeVisit();
  if (!writeSucceeded)
  {
    Serial.println(F("Write visit failed"));
  }
  print();

  mfrc522.PICC_HaltA();

  digitalWrite(LED_BUILTIN, LOW); //disable LED to indicate writing is complete
}

void print()
{
  byte recordCount = recordCountFromHeader();
  if (recordCount < 0)
  {
    Serial.println(F("Failed to read record count"));
    return;
  }
  for (byte i = 0; i < recordCount; i++)
  {

    Serial.print(i, DEC);
    Serial.print(F(": "));
    byte pageAddr = START_PAGE_ADDR + 1 + (2 * i); //1 page for the header, 2 pages for each record
    readIntoBuffer(pageAddr);
    for (byte i = 0; i < 8; i++)
    {
      Serial.print(buffer[i], HEX);
    }
    Serial.println();
  }
}

void readIntoBuffer(byte pageAddr)
{
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(pageAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
}
/**
 * Reads the header potentially located at the start page and returns the number of records
 * as indicated.  If the version check does not match, then 0 is returned.
 * If there is a problem reading the tag, then -1 is returned.
 */
byte recordCountFromHeader()
{
  readIntoBuffer(START_PAGE_ADDR);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return -1;
  }
  unsigned int version = buffer[FORMAT_VERSION_INDEX];
  byte recordCount = 0;
  if (version == FORMAT_VERSION)
  {
    recordCount = buffer[RECORD_COUNT_INDEX];
    //just during testing, wipe the records when they reach the max
    if (recordCount == MAX_RECORDS)
    {
      wipe();
      recordCount = 0;
    }
  }
  return recordCount;
}
bool writeVisit()
{
  byte recordCount = recordCountFromHeader();
  if (recordCount < 0)
  {
    return false;
  }

  byte stationData[PAGE_SIZE];
  unsigned int id = stationId();
  stationData[0] = (id >> 8) & 0xFF;
  stationData[1] = id & 0xFF;
  stationData[2] = RESET_CHAR;
  stationData[3] = RESET_CHAR;
  byte pageAddr = START_PAGE_ADDR + 1 + (recordCount * 2); //1 page for head, 2 pages for every record
  bool writeSuccess = writePage(pageAddr, stationData);

  byte timestampData[PAGE_SIZE];
  unsigned long time = timestamp();
  timestampData[0] = (time >> 24) & 0xFF;
  timestampData[1] = (time >> 16) & 0xFF;
  timestampData[2] = (time >> 8) & 0xFF;
  timestampData[3] = time & 0xFF;
  writeSuccess &= writePage(pageAddr + 1, timestampData);

  //only update the header with the new record count if succeeded
  if (writeSuccess)
  {
    byte headerData[PAGE_SIZE];
    headerData[FORMAT_VERSION_INDEX] = FORMAT_VERSION;
    headerData[1] = RESET_CHAR;
    headerData[RECORD_COUNT_INDEX] = recordCount + 1; //increment for this record
    headerData[3] = RESET_CHAR;
    writeSuccess &= writePage(START_PAGE_ADDR, headerData);
  }
  return writeSuccess;
}
bool wipe()
{
  byte pageData[PAGE_SIZE];
  for (byte i = 0; i < PAGE_SIZE; i++)
  {
    pageData[i] = RESET_CHAR;
  }

  bool succeeded = true;
  for (uint8_t page = START_PAGE_ADDR; page <= FINAL_PAGE_ADDR; page++)
  {
    succeeded &= writePage(page, pageData);
  }
  return succeeded;
}
/**Writes a 4 byte buffer to the page of storage starting at the given page address. 
 */
bool writePage(uint8_t pageAddr, byte *pageData)
{
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Ultralight_Write(pageAddr, pageData, PAGE_SIZE);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}
/**Called when a tag has been detected.  This returns the UID of the tag, in uppercase HEX string.
 * 
 */
String uid()
{
  char dataString[15]; //7 byte / 14 characters + 1 terminating character?
  sprintf(dataString, "%02X%02X%02X%02X%02X%02X%02X",
          mfrc522.uid.uidByte[0],
          mfrc522.uid.uidByte[1],
          mfrc522.uid.uidByte[2],
          mfrc522.uid.uidByte[3],
          mfrc522.uid.uidByte[4],
          mfrc522.uid.uidByte[5],
          mfrc522.uid.uidByte[6]);

  return String(dataString);
}

/** The Unix timestamp.
 * https://www.unixtimestamp.com/index.php
 */
unsigned long timestamp()
{
  return millis() / 1000; //placeholder until clock is provided
}

/**
 * The identifier of this station that the student is visiting.
 */
unsigned int stationId()
{
  return 0x5456;
}
