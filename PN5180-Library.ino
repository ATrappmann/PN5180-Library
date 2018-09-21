//
// Demo usage of Arduino Uno library for PN5180-NFC Module from NXP Semiconductors
// Copyright (c) 2018 by Andreas Trappmann
//
// BEWARE: SPI with PN5180 module has to be at a level of 3.3V
// Use of logic-level converters from 5V->3.3V is absolutly neccessary on all input 
// pins of PN5180!
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

#include "PN5180.h"

#define PN5180_NSS  10
#define PN5180_BUSY 9
#define PN5180_IRQ  8
#define PN5180_RST  7

// Onboard LED
#define RF_LED LED_BUILTIN

PN5180 nfc(PN5180_NSS, PN5180_BUSY, PN5180_IRQ);

void setup() {
  pinMode(RF_LED, OUTPUT);
  pinMode(PN5180_RST, OUTPUT);

  Serial.begin(115200);
  Serial.println("Uploaded: " __DATE__ " " __TIME__);
  
  Serial.println("PN5180 Hard-Reset...");
  digitalWrite(PN5180_RST, LOW);
  delay(10);
  digitalWrite(PN5180_RST, HIGH);
  delay(10);

  Serial.print("Check IRQ signal: ");
  Serial.println((nfc.checkIRQ()?"HIGH":"LOW"));
  if (nfc.getInterrupt()) {
    Serial.print("IRQ signal is now: ");
    Serial.println((nfc.checkIRQ()?"HIGH":"LOW"));
  }
    
  /*
  Serial.println("Wakeup...");
  digitalWrite(PN5180_NSS, LOW);
  delayMicroseconds(100);
  digitalWrite(PN5180_NSS, HIGH);
  delay(10);
  */
  
  Serial.print("Reading product version... ");
  uint8_t productVersion[2];
  if (nfc.readEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion))) {
    Serial.print("0x");
    Serial.println((uint16_t)productVersion, HEX);
  }
  else {
    Serial.println(" - ERROR!");
    handleError();
  }

  Serial.print("Reading firmware version... ");
  uint8_t firmwareVersion[2];
  if (nfc.readEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion))) {
    Serial.print(firmwareVersion[1]);
    Serial.print(".");
    Serial.println(firmwareVersion[0]);
  }
  else {
    Serial.println(" - ERROR!");
    handleError();
  }
  
  Serial.print("Loading RF-Configuration... ");
  if (nfc.loadRFConfig(0x0E, 0x8E)) {  // ISO15693 parameters
    Serial.println("ok");
  }
  else {
    Serial.println(" - ERROR!");
    handleError();
  }
  
  Serial.print("Turning ON RF field... ");
  if (nfc.setRF_on()) {
    digitalWrite(RF_LED, HIGH);
    Serial.println("ok");
  }
  else {
    Serial.println(" - ERROR!");
    handleError();
  }
  
}

uint32_t loopCnt = 0;

void loop() {
/*  
  Serial.print("Loop #");
  Serial.println(loopCnt++);
  
  uint8_t uid[8];
  if (nfc.getInventory(uid)) {
#ifdef DEBUG
    PN5180DEBUG("Inventory successful, uid=");
    for (int i=0; i<8; i++) {
      PN5180DEBUG(formatHex(uid[7-i])); // LSB is first
      if (i < 2) PN5180DEBUG(":");
    }
    PN5180DEBUG("\n");
#endif
    
    uint8_t blockSize, numBlocks;
    if (nfc.getSystemInfo(uid, &blockSize, &numBlocks)) {
      PN5180DEBUG("System Info retrieved\n");

      uint8_t readBuffer[blockSize];
      for (int no=0; no<numBlocks; no++) {
        if (nfc.readSingleBlock(uid, no, readBuffer, blockSize)) {
          PN5180DEBUG("Read block #");
          PN5180DEBUG(no);
          PN5180DEBUG("\n");
        }
        else {
          PN5180DEBUG("Error in readSingleBlock\n");
          handleError();
        }
      }
    }
    else {
      PN5180DEBUG("Error in getSystemInfo\n");
      handleError();
    }
  }
  else {
    PN5180DEBUG("Error in getInventory\n");
    handleError();
  }
*/

  delay(1000);
}

void handleError() {
  uint32_t irqStatus = nfc.getInterrupt();
  Serial.print("IRQ-Status 0x");
  Serial.println(irqStatus, HEX);

  if (irqStatus & (1<<0)) Serial.println("\tRQ_IRQ_STAT");
  if (irqStatus & (1<<1)) Serial.println("\tTX_IRQ_STAT");
  if (irqStatus & (1<<2)) Serial.println("\tIDLE_IRQ_STAT");
  if (irqStatus & (1<<3)) Serial.println("\tMODE_DETECTED_IRQ_STAT");
  if (irqStatus & (1<<4)) Serial.println("\tCARD_ACTIVATED_IRQ_STAT");
  if (irqStatus & (1<<5)) Serial.println("\tSTATE_CHANGE_IRQ_STAT");
  if (irqStatus & (1<<6)) Serial.println("\tRFOFF_DET_IRQ_STAT");
  if (irqStatus & (1<<7)) Serial.println("\tRFON_DET_IRQ_STAT");
  if (irqStatus & (1<<8)) Serial.println("\tTX_RFOFF_IRQ_STAT");
  if (irqStatus & (1<<9)) Serial.println("\tTX_RFON_IRQ_STAT");
  if (irqStatus & (1<<10)) Serial.println("\tRF_ACTIVE_ERROR_IRQ_STAT");
  if (irqStatus & (1<<11)) Serial.println("\tTIMER0_IRQ_STAT");
  if (irqStatus & (1<<12)) Serial.println("\tTIMER1_IRQ_STAT");
  if (irqStatus & (1<<13)) Serial.println("\tTIMER2_IRQ_STAT");
  if (irqStatus & (1<<14)) Serial.println("\tRX_SOF_DET_IRQ_STAT");
  if (irqStatus & (1<<15)) Serial.println("\tRX_SC_DET_IRQ_STAT");
  if (irqStatus & (1<<16)) Serial.println("\tTEMPSENS_ERROR_IRQ_STAT");
  if (irqStatus & (1<<17)) Serial.println("\tGENERAL_ERROR_IRQ_STAT");
  if (irqStatus & (1<<18)) Serial.println("\tHV_ERROR_IRQ_STAT");
  if (irqStatus & (1<<19)) Serial.println("\tLPCD_IRQ_STAT");
  
}
