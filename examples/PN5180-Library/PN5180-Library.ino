//
// PN5180-NFC Module from NXP Semiconductors
//
// BEWARE: SPI with PN5180 module has to be at a level of 3.3V
// use of logic-level converters from 5V->3.3V is absolutly neccessary on all input pins of PN5180!
// 
// Arduino <-> Level Converter <-> PN5180
// 5V             <-->             5V
// 3,3V           <-->             3,3V
// GND            <-->             GND

// 5V      <-> HV              
// GND     <-> GND (HV)
//             LV              <-> 3.3V
//             GND (LV)        <-> GND
//
// UNO
// SCLK,13 <-> HV1 - LV1       --> SCLK
// MISO,12        <---         <-- MISO
// MOSI,11 <-> HV3 - LV3       --> MOSI
// SS,10   <-> HV4 - LV4       --> NSS (=Not SS -> active LOW)
// BUSY,9         <---             BUSY
// IRQ,8          <---             IRQ
// Reset,7 <-> HV2 - LV2       --> RST
// 

/*
 * Pins on ICODE2 Reader Writer:
 * 
 *   ICODE2   |    PN5180
 * pin  label |pin  I/O name
 * 1    +5V     
 * 2    +3,3V
 * 3    RST     10   I    RESET_N (low active)
 * 4    NSS     1    I    SPI NSS
 * 5    MOSI    3    I    SPI MOSI
 * 6    MISO    5    O    SPI MISO
 * 7    SCK     7    I    SPI Clock
 * 8    BUSY    8    O    Busy Signal
 * 9    GND     9  Supply VSS - Ground
 * 10   GPIO    38   O    GPO1 - Control for external DC/DC
 * 11   IRQ     39   O    IRQ
 * 12   AUX     40   O    AUX1 - Analog/Digital test signal
 * 13   REQ     2?  I/O   AUX2 - Analog test bus or download
 * 
 */
 
// ESP-32 <-> PN5180 pin mapping: 
//
// SCLK, 18                     --> SCLK
// MISO, 19                     <-- MISO
// MOSI, 23                     --> MOSI
// SS, 16                       --> NSS (=Not SS -> active LOW)
// BUSY, 5                      <-- BUSY
// IRQ, not used
// Reset, 17                    --> RST
// 

#include <PN5180.h>
#include <PN5180ISO15693.h>

// Arduino UNO
#define PN5180_NSS  10
#define PN5180_BUSY 9
#define PN5180_IRQ  8
#define PN5180_RST  7

/*
  // ESP-32 
  #define PN5180_NSS  16
  #define PN5180_BUSY 5
  #define PN5180_IRQ  -1
  #define PN5180_RST  17
*/ 

// Onboard LED
#define RF_LED LED_BUILTIN

PN5180 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST, PN5180_IRQ);
PN5180ISO15693 isoNfc(&nfc);

void setup() {
  pinMode(RF_LED, OUTPUT);
  pinMode(PN5180_RST, OUTPUT);

  Serial.begin(115200);
  Serial.println(F("=================================="));
  Serial.println(F("Uploaded: " __DATE__ " " __TIME__));

  nfc.begin();

  Serial.println(F("----------------------------------"));
  Serial.println(F("PN5180 Hard-Reset..."));
  nfc.reset();
  if (nfc.isIRQ()) {
    showIRQStatus();
    nfc.end();
    Serial.println(F("ERROR: Card not responding!?"));
    Serial.println(F("Done. -- Press RESET to continue..."));
    Serial.flush();
    Serial.end();
    exit(0);
  }
  
  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading product version..."));
  uint8_t productVersion[2];
  if (nfc.readEEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion))) {
    Serial.print(F("Product version="));
    Serial.print(productVersion[1]);
    Serial.print(".");
    Serial.println(productVersion[0]);
  }
  else showIRQStatus();

  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading firmware version..."));
  uint8_t firmwareVersion[2];
  if (nfc.readEEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion))) {
    Serial.print(F("Firmware version="));
    Serial.print(firmwareVersion[1]);
    Serial.print(".");
    Serial.println(firmwareVersion[0]);
  }
  else showIRQStatus();

  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading EEPROM version..."));
  uint8_t eepromVersion[2];
  if (nfc.readEEprom(EEPROM_VERSION, eepromVersion, sizeof(eepromVersion))) {
    Serial.print(F("EEPROM version="));
    Serial.print(eepromVersion[1]);
    Serial.print(".");
    Serial.println(eepromVersion[0]);
  }
  else showIRQStatus();

  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading IRQ pin config..."));
  uint8_t irqConfig;
  if (nfc.readEEprom(IRQ_PIN_CONFIG, &irqConfig, 1)) {
    Serial.print(F("IRQ_PIN_CONFIG=0x"));
    Serial.println(irqConfig, HEX);
  }
  else showIRQStatus();

  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading IRQ_ENABLE register..."));
  uint32_t irqEnable;
  if (nfc.readRegister(IRQ_ENABLE, &irqEnable)) {
    Serial.print(F("IRQ_ENABLE=0x"));
    Serial.println(irqConfig, HEX);
  }
  else showIRQStatus();

  Serial.println(F("----------------------------------"));
  Serial.println(F("Enable RF field..."));
  isoNfc.setupRF();
  digitalWrite(RF_LED, HIGH);
}

uint32_t loopCnt = 0;

void loop() {
  Serial.println(F("----------------------------------"));
  Serial.print(F("Loop #"));
  Serial.println(loopCnt++);

  Serial.print(F("Check IRQ signal: "));
  Serial.println((nfc.isIRQ()?"HIGH":"LOW"));
  if (!nfc.isIRQ()) {
    uint8_t uid[8];
    bool rc = isoNfc.getInventory(uid);
    uint32_t irqStatus = nfc.getIRQStatus();
    if (0 == irqStatus) {
      Serial.print(F("Inventory successful, uid="));
      for (int i=0; i<8; i++) {
        Serial.print(uid[7-i], HEX); // LSB is first
        if (i < 2) Serial.print(":");
      }
      Serial.println();
  
      uint8_t blockSize, numBlocks;
      if (isoNfc.getSystemInfo(uid, &blockSize, &numBlocks)) {
        Serial.print(F("System Info retrieved: blockSize="));
        Serial.print(blockSize);
        Serial.print(F(", numBlocks="));
        Serial.println(numBlocks);
  
        uint8_t readBuffer[blockSize];
        for (int no=0; no<numBlocks; no++) {
          if (isoNfc.readSingleBlock(uid, no, readBuffer, blockSize)) {
            Serial.print(F("Read block #"));
            Serial.print(no);
            Serial.print(": ");
            for (int i=0; i<blockSize; i++) {
              if (readBuffer[i] < 16) Serial.print("0");
              Serial.print(readBuffer[i], HEX);
              Serial.print(" ");
            }
            Serial.print(" ");
            for (int i=0; i<blockSize; i++) {
              if (isprint(readBuffer[i])) {
                Serial.print((char)readBuffer[i]);
              }
              else Serial.print(".");
            }
            Serial.println();          
          }
          else {
            Serial.println(F("Error in readSingleBlock!"));
            showIRQStatus();
            nfc.clearIRQStatus(0xffffffff);
            break;
          }
        }  
      }
      else {
        Serial.println(F("Error in getSystemInfo!"));
        showIRQStatus();
        nfc.clearIRQStatus(0xffffffff);
      }
    }
    else if (0 == (RX_SOF_DET_IRQ_STAT & irqStatus)) { // no card detected
      Serial.println(F("No card!"));
      nfc.clearIRQStatus(RX_SOF_DET_IRQ_STAT | TX_IRQ_STAT | IDLE_IRQ_STAT);
      delay(2000);
    }
    else {
      Serial.println(F("Error in getInventory!"));
      showIRQStatus();
      nfc.reset();
      isoNfc.setupRF();
      delay(2000);
    }
  }
  else {
      showIRQStatus();
      nfc.reset();
      isoNfc.setupRF();
      delay(2000);
  }
  
  delay(1000);
}

void showIRQStatus() {
  uint32_t irqStatus = nfc.getIRQStatus();
  Serial.print("IRQ-Status 0x");
  Serial.println(irqStatus, HEX);

  if (irqStatus & (1<<0)) Serial.println(F("\tRQ_IRQ_STAT"));
  if (irqStatus & (1<<1)) Serial.println(F("\tTX_IRQ_STAT"));
  if (irqStatus & (1<<2)) Serial.println(F("\tIDLE_IRQ_STAT"));
  if (irqStatus & (1<<3)) Serial.println(F("\tMODE_DETECTED_IRQ_STAT"));
  if (irqStatus & (1<<4)) Serial.println(F("\tCARD_ACTIVATED_IRQ_STAT"));
  if (irqStatus & (1<<5)) Serial.println(F("\tSTATE_CHANGE_IRQ_STAT"));
  if (irqStatus & (1<<6)) Serial.println(F("\tRFOFF_DET_IRQ_STAT"));
  if (irqStatus & (1<<7)) Serial.println(F("\tRFON_DET_IRQ_STAT"));
  if (irqStatus & (1<<8)) Serial.println(F("\tTX_RFOFF_IRQ_STAT"));
  if (irqStatus & (1<<9)) Serial.println(F("\tTX_RFON_IRQ_STAT"));
  if (irqStatus & (1<<10)) Serial.println(F("\tRF_ACTIVE_ERROR_IRQ_STAT"));
  if (irqStatus & (1<<11)) Serial.println(F("\tTIMER0_IRQ_STAT"));
  if (irqStatus & (1<<12)) Serial.println(F("\tTIMER1_IRQ_STAT"));
  if (irqStatus & (1<<13)) Serial.println(F("\tTIMER2_IRQ_STAT"));
  if (irqStatus & (1<<14)) Serial.println(F("\tRX_SOF_DET_IRQ_STAT"));
  if (irqStatus & (1<<15)) Serial.println(F("\tRX_SC_DET_IRQ_STAT"));
  if (irqStatus & (1<<16)) Serial.println(F("\tTEMPSENS_ERROR_IRQ_STAT"));
  if (irqStatus & (1<<17)) Serial.println(F("\tGENERAL_ERROR_IRQ_STAT"));
  if (irqStatus & (1<<18)) Serial.println(F("\tHV_ERROR_IRQ_STAT"));
  if (irqStatus & (1<<19)) Serial.println(F("\tLPCD_IRQ_STAT"));
}
