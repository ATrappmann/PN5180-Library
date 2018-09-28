// NAME: PN5180ISO15693.h
//
// DESC: ISO15693 protocol on NXP Semiconductors PN5180 module for Arduino.
//
//#define DEBUG 1

#include <inttypes.h>
#include "PN5180ISO15693.h"
#include "Debug.h"
#include <Arduino.h>

PN5180ISO15693::PN5180ISO15693(PN5180 *nfc) {
  this->nfc = nfc;
}

/*  
 * Inventory, code=01
 * 
 * Request format: SOF, Flags, Inventory, Opt.AFI, Mask len, Mask value, CRC16, EOF
 * Response format: SOF, Flags, DSFID, UID, CRC16, EOF
 * 
 */
bool PN5180ISO15693::getInventory(uint8_t *uid) {  
  //                     Flags,  CMD, maskLen
  uint8_t inventory[] = { 0x26, 0x01, 0x00 };
  //                        |\- inventory flag + high data rate
  //                        \-- 1 slot: only one card, no AFI field present
  PN5180DEBUG(F("Get Inventory...\n"));

  uint8_t *readBuffer;
  if (!issueISO15693Command(inventory, sizeof(inventory), &readBuffer)) {
    PN5180DEBUG("no answer\n");
    return false;
  }

  uint32_t irqStatus = nfc->getIRQStatus();
  if (0 == (RX_SOF_DET_IRQ_STAT & irqStatus)) { // no card detected
     nfc->clearIRQStatus(TX_IRQ_STAT | IDLE_IRQ_STAT);
     return false;
  }
  
  PN5180DEBUG(F("Response flags: "));
  PN5180DEBUG(formatHex(readBuffer[0]));
  PN5180DEBUG(F(", Data Storage Format ID: "));
  PN5180DEBUG(formatHex(readBuffer[1]));
  PN5180DEBUG(F(", UID: "));
  for (int i=0; i<8; i++) {
    uid[i] = readBuffer[2+i]; 
    PN5180DEBUG(formatHex(uid[7-i])); // LSB comes first
    if (i<2) PN5180DEBUG(":");
  }
  PN5180DEBUG("\n");

  free(readBuffer);
  nfc->clearIRQStatus(RX_SOF_DET_IRQ_STAT | IDLE_IRQ_STAT | TX_IRQ_STAT | RX_IRQ_STAT);
  return !nfc->isIRQ();
}

/*
 * Read single block, code=20
 * 
 * Request format: SOF, Flags, ReadSingleBlock, UID, BlockNumber, CRC16, EOF
 * Response format: SOF, Flags, BlockSecurityStatus, Data, CRC16, EOF
 */
bool PN5180ISO15693::readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *readBuffer, uint8_t blockSize) {
  //                              flags, cmd, uid,             blockNo  
  uint8_t readSingleBlock[] = { 0x62, 0x20, 1,2,3,4,5,6,7,8, blockNo }; // UID has LSB first!
  //                              |\- high data rate
  //                              \-- options, addressed by UID
  for (int i=0; i<8; i++) {
    readSingleBlock[2+i] = uid[i];
  }
  PN5180DEBUG("Read Single Block #");
  PN5180DEBUG(blockNo); 
  PN5180DEBUG(", size=");
  PN5180DEBUG(blockSize);
  PN5180DEBUG(": ");
#ifdef DEBUG  
  for (int i=0; i<sizeof(readSingleBlock); i++) {
    PN5180DEBUG(" ");
    PN5180DEBUG(formatHex(readSingleBlock[i]));
  }
  PN5180DEBUG("\n");
#endif

  uint8_t *resultPtr;
  if (!issueISO15693Command(readSingleBlock, sizeof(readSingleBlock), &resultPtr)) {
    return false;
  }

  uint32_t irqStatus = nfc->getIRQStatus();
  if (0 == (RX_SOF_DET_IRQ_STAT & irqStatus)) { // no card detected
     nfc->clearIRQStatus(TX_IRQ_STAT | IDLE_IRQ_STAT);
     return false;
  }

  uint8_t flags = readBuffer[0];
  if (flags & 0x01) { // check error flag
    PN5180DEBUG(F("ERROR_FLAG is set!"));
  }
  uint8_t secStatus = readBuffer[1];  // wirklich??
  PN5180DEBUG("Value=");
  for (int i=0; i<blockSize; i++) {
    readBuffer[i] = resultPtr[2+i];
    PN5180DEBUG(formatHex(readBuffer[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG(" ");
  for (int i=0; i<blockSize; i++) {  
    char c = readBuffer[i];
#ifdef DEBUG    
    if (isPrintable(c)) {
      PN5180DEBUG(c);
    }
    else PN5180DEBUG(".");
#endif
  }
  PN5180DEBUG("\n");

  free(resultPtr);
  nfc->clearIRQStatus(RX_SOF_DET_IRQ_STAT | IDLE_IRQ_STAT | TX_IRQ_STAT | RX_IRQ_STAT);
  return !nfc->isIRQ();
}

/*
 * Get System Information, code=2B
 * 
 * Request format: SOF, Flags, GetSysInfo, Opt.UID, CRC16, EOF
 * Response format: 
 *  when NO error: SOF, Flags, InfoFlags, UID, DSFID, AFI, Other fields, CRC16, EOF
 *  when error flag is set: SOF, Flags, Error code, CRC16, EOF
 * 
 */
bool PN5180ISO15693::getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks) {
  uint8_t sysInfo[] = { 0x22, 0x2b, 1,2,3,4,5,6,7,8 };  // UID has LSB first!
  for (int i=0; i<8; i++) {
    sysInfo[2+i] = uid[i];
  }
  PN5180DEBUG("Get System Information"); 
#ifdef DEBUG
  for (int i=0; i<sizeof(sysInfo); i++) {
    PN5180DEBUG(" ");
    PN5180DEBUG(formatHex(sysInfo[i]));
  }
#endif
  PN5180DEBUG("\n");

  uint8_t *readBuffer;
  if (!issueISO15693Command(sysInfo, sizeof(sysInfo), &readBuffer)) {
    return false;
  }

  uint32_t irqStatus = nfc->getIRQStatus();
  if (0 == (RX_SOF_DET_IRQ_STAT & irqStatus)) { // no card detected
     nfc->clearIRQStatus(TX_IRQ_STAT | IDLE_IRQ_STAT);
     return false;
  }

  for (int i=0; i<8; i++) {
    uid[i] = readBuffer[2+i];
  }  
#ifdef DEBUG
  PN5180DEBUG("UID=");
  for (int i=0; i<8; i++) {
    PN5180DEBUG(formatHex(readBuffer[9-i]));  // UID has LSB first!
    if (i<7) PN5180DEBUG(":");
  }
  PN5180DEBUG("\n");
#endif
  uint8_t *p = &readBuffer[10];
  
  uint8_t infoFlags = readBuffer[1];
  if (infoFlags & 0x01) { // DSFID flag
    uint8_t dsfid = *p++;
    PN5180DEBUG("DSDID=");  // Data storage format identifier
    PN5180DEBUG(formatHex(dsfid));
    PN5180DEBUG("\n");
  }
  if (infoFlags & 0x02) { // AFI flag
    uint8_t afi = *p++;
    PN5180DEBUG("AFI=");  // Application family identifier
    PN5180DEBUG(formatHex(afi));
    PN5180DEBUG("\n");
  }
  if (infoFlags & 0x04) { // VICC Memory size
    *numBlocks = *p++;
    *blockSize = *p++; 
    *blockSize = (*blockSize) & 0x1f;
    
    //uint16_t viccMemSize = (*blockSize << 8) | (*numBlocks << 0);
    *blockSize = *blockSize + 1;
    // neu:
    *numBlocks = *numBlocks + 1;
    uint16_t viccMemSize = (*blockSize) * (*numBlocks);
    
    PN5180DEBUG("VICC MemSize=");
    //PN5180DEBUG(formatHex(viccMemSize));
    PN5180DEBUG(viccMemSize);
    PN5180DEBUG(" BlockSize=");
    PN5180DEBUG(*blockSize);
    PN5180DEBUG(" NumBlocks=");
    PN5180DEBUG(*numBlocks);
    PN5180DEBUG("\n");
  }
  if (infoFlags & 0x08) { // IC reference
    uint8_t icRef = *p++;
    PN5180DEBUG("IC Ref=");
    PN5180DEBUG(formatHex(icRef));
    PN5180DEBUG("\n");
  }
 
  free(readBuffer);
  nfc->clearIRQStatus(RX_SOF_DET_IRQ_STAT | IDLE_IRQ_STAT | TX_IRQ_STAT | RX_IRQ_STAT);
  return !nfc->isIRQ();
}

/* 
 * ISO 15693 - Protocol
 * 
 * General Request Format:
 *  SOF, Req.Flags, Command code, Parameters, Data, CRC16, EOF
 * 
 *  Request Flags:
 *    xxxx.3210
 *         |||\_ Subcarrier flag: 0=single sub-carrier, 1=two sub-carrier
 *         ||\__ Datarate flag: 0=low data rate, 1=high data rate 
 *         |\___ Inventory flag: 0=no inventory, 1=inventory
 *         \____ Protocol extension flag: 0=no extension, 1=protocol format is extended
 *         
 *  If Inventory flag is set:
 *    7654.xxxx
 *     ||\_ AFI flag: 0=no AFI field present, 1=AFI field is present
 *     |\__ Number of slots flag: 0=16 slots, 1=1 slot
 *     \___ Option flag: 0=default, 1=meaning is defined by command description
 *     
 *  If Inventory flag is NOT set:
 *    7654.xxxx
 *     ||\_ Select flag: 0=request shall be executed by any VICC according to Address_flag
 *     ||                1=request shall be executed only by VICC in selected state 
 *     |\__ Address flag: 0=request is not addressed. UID field is not present. 
 *     |                  1=request is addressed. UID field is present. Only VICC with UID shall answer
 *     \___ Option flag: 0=default, 1=meaning is defined by command description
 *     
 * General Response Format:    
 *  SOF, Resp.Flags, Parameters, Data, CRC16, EOF
 *    
 *  Response Flags:
 *    xxxx.3210  
 *         |||\_ Error flag: 0=no error, 1=error detected, see error field
 *         ||\__ RFU: 0
 *         |\___ RFU: 0
 *         \____ Extension flag: 0=no extension, 1=protocol format is extended
 *         
 *  If Error flag is set, the following error codes are defined:
 *    01 = The command is not supported, i.e. the request code is not recognized.
 *    02 = The command is not recognized, i.e. a format error occured.
 *    03 = The option is not suppored.
 *    0F = Unknown error.
 *    10 = The specific block is not available.
 *    11 = The specific block is already locked and cannot be locked again.
 *    12 = The specific block is locked and cannot be changed.
 *    13 = The specific block was not successfully programmed.
 *    14 = The specific block was not successfully locked.
 *    A0-DF = Custom command error codes
 */ 
bool PN5180ISO15693::issueISO15693Command(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr) {
#ifdef DEBUG  
  PN5180DEBUG(F("Issue Command 0x"));
  PN5180DEBUG(formatHex(cmd[1]));
  PN5180DEBUG("...\n");
#endif

  nfc->sendData(cmd, cmdLen);
  delay(10);  
  
//  while (!isIRQ()); // wait for RX_IRQ to raise
  uint32_t irqStatus = nfc->getIRQStatus();
#ifdef DEBUG
  PN5180DEBUG(F("IRQ-Status="));
  PN5180DEBUG(formatHex(irqStatus));
  PN5180DEBUG("\n");
  if (0 == (irqStatus & (1<<0))) { // RX_IRQ_STAT
      PN5180DEBUG(F("** ERROR: Not END_of_RF_Reception_IRQ!\n"));
  }
#endif
  
  uint32_t rxStatus;
  nfc->readRegister(RX_STATUS, &rxStatus);
  PN5180DEBUG(F("RX-Status="));
  PN5180DEBUG(formatHex(rxStatus));
 
  uint16_t len = (uint16_t)(rxStatus & 0x000001ff);  
  PN5180DEBUG(", len=");
  PN5180DEBUG(len);
  PN5180DEBUG("\n");
  if (0 == len) return false;
  if (511 == len) return false;
  
  *resultPtr = (uint8_t *)malloc(len);
  nfc->readData(*resultPtr, len);
#ifdef DEBUG  
  Serial.print("Read=");
  for (int i=0; i<len; i++) {
    Serial.print(formatHex((*resultPtr)[i]));
    if (i<len-1) Serial.print(":"); 
  }
  Serial.println();
#endif
      
  uint8_t responseFlags = *resultPtr[0];
  if (responseFlags & (1<<0)) { // error flag
    PN5180DEBUG("ERROR code=");
    PN5180DEBUG(formatHex(*resultPtr[1]));
    PN5180DEBUG(" - ");
    switch (*resultPtr[1]) {
      case 0x01:
        PN5180DEBUG(F("The command is not supported"));
        break;
      case 0x02:
        PN5180DEBUG(F("The command is not recognised"));
        break;
      case 0x03:
        PN5180DEBUG(F("The option is not supported"));
        break;
      case 0x0f:
        PN5180DEBUG(F("Unknown error"));
        break;
      case 0x10:
        PN5180DEBUG(F("The specified block is not available (doesn’t exist)"));
        break;
      case 0x11:
        PN5180DEBUG(F("The specified block is already locked and thus cannot be locked again"));
        break;
      case 0x12:
        PN5180DEBUG(F("The specified block is locked and its content cannot be changed"));
        break;
      case 0x13:
        PN5180DEBUG(F("The specified block was not successfully programmed"));
        break;
      case 0x14:
        PN5180DEBUG(F("The specified block was not successfully locked"));
        break;
      default:
        PN5180DEBUG(F("Custom command error code"));
        break;
    }
    PN5180DEBUG("\n");
    free(*resultPtr);
    
    return false;
  }
  
  if (responseFlags & 0x80) { // extendsion flag
    PN5180DEBUG("Extension flag is set!\n");
  }

  return true;
}

bool PN5180ISO15693::setupRF() {
  PN5180DEBUG(F("Loading RF-Configuration...\n"));
  if (nfc->loadRFConfig(0x0d, 0x8d)) {  // ISO15693 parameters
    PN5180DEBUG(F("done."));
  }
  else return false;
  
  PN5180DEBUG(F("Turning ON RF field..."));
  if (nfc->setRF_on()) {
    PN5180DEBUG(F("done."));
  }
  else return false;

//  nfc.writeRegister(TIMER1_CONFIG, 0x00000000);
//  nfc.writeRegister(TIMER1_RELOAD, 0x00002557);
  nfc->writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom Command
  nfc->writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);   // Transceive Command
//  nfc.writeRegister(TIMER1_CONFIG, 0x00100801);

  return true;
}
