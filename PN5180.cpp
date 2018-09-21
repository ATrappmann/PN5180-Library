// NAME: PN5180.cpp
//
// DESC:
//
#define DEBUG 1

#include <Arduino.h>
#include "PN5180.h"

// PN5180 1-Byte Direct Commands
// see 11.4.3.3 Host Interface Command List
#define PN5180_WRITE_REGISTER           (0x00)
#define PN5180_WRITE_REGISTER_OR_MASK   (0x01)
#define PN5180_WRITE_REGISTER_AND_MASK  (0x02)
#define PN5180_READ_REGISTER            (0x04)
#define PN5180_READ_EEPROM              (0x07)
#define PN5180_SEND_DATA                (0x09)
#define PN5180_READ_DATA                (0x0A)
#define PN5180_SWITCH_MODE              (0x0B)
#define PN5180_LOAD_RF_CONFIG           (0x11)
#define PN5180_RF_ON                    (0x16)
#define PN5180_RF_OFF                   (0x17)

PN5180::PN5180(uint8_t SSpin, uint8_t BUSYpin, uint8_t IRQpin) {
  PN5180_NSS = SSpin;
  PN5180_BUSY = BUSYpin;
  PN5180_IRQ = IRQpin;

  /*
   * 11.4.1 Physical Host Interface
   * The interface of the PN5180 to a host microcontroller is based on a SPI interface,
   * extended by signal line BUSY. The maximum SPI speed is 7 Mbps and fixed to CPOL
   * = 0 and CPHA = 0.
   */
  // Settings for PN5180: 7Mbps, MSB first, SPI_MODE0 (CPOL=0, CPHA=0) 
  //const SPISettings PN5180_SPI_SETTINGS(7000000, MSBFIRST, SPI_MODE0);
  // use defaults: init_AlwaysInline(4000000, MSBFIRST, SPI_MODE0);
  PN5180_SPI_SETTINGS = SPISettings(7000000, MSBFIRST, SPI_MODE0);
 
  pinMode(PN5180_NSS, OUTPUT);
  pinMode(PN5180_BUSY, INPUT);
  pinMode(PN5180_IRQ, INPUT);

  digitalWrite(PN5180_NSS, HIGH); // disable
  SPI.begin();
}


/*  
 * Inventory, code=01
 * 
 * Request format: SOF, Flags, Inventory, Opt.AFI, Mask len, Mask value, CRC16, EOF
 * Response format: SOF, Flags, DSFID, UID, CRC16, EOF
 * 
 */
bool PN5180::getInventory(uint8_t *uid) {  
  //                     Flags,  CMD, maskLen
  uint8_t inventory[] = { 0x26, 0x01, 0x00 };
  //                        |\- inventory flag + high data rate
  //                        \-- 1 slot: only one card, no AFI field present
  PN5180DEBUG("Get Inventory: cmd=");
#ifdef DEBUG  
  for (int i=0; i<sizeof(inventory); i++) {
    PN5180DEBUG(formatHex(inventory[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif

  uint8_t *readBuffer;
  if (!issueISO15693Command(inventory, sizeof(inventory), &readBuffer)) {
    PN5180DEBUG("no answer\n");
    return false;
  }
  PN5180DEBUG("sizeof readBuffer=");
  PN5180DEBUG(sizeof(readBuffer));
  PN5180DEBUG("\n");
  
  PN5180DEBUG("Response flags: ");
  PN5180DEBUG(formatHex(readBuffer[0]));
  PN5180DEBUG("\n");
  
  PN5180DEBUG("Data Storage Format ID: ");
  PN5180DEBUG(formatHex(readBuffer[1]));
  PN5180DEBUG("\n");
  
  PN5180DEBUG("UID: ");
  for (int i=0; i<8; i++) {
    uid[i] = readBuffer[2+i]; 
    PN5180DEBUG(formatHex(uid[9-i])); // LSB comes first
    if (i<2) PN5180DEBUG(":");
  }
  PN5180DEBUG("\n");

  free(readBuffer);
  return true;
}

/*
 * Read single block, code=20
 * 
 * Request format: SOF, Flags, ReadSingleBlock, UID, BlockNumber, CRC16, EOF
 * Response format: SOF, Flags, BlockSecurityStatus, Data, CRC16, EOF
 */
bool PN5180::readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *readBuffer, uint8_t blockSize) {
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
  return true;
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
bool PN5180::getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks) {
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
  return true;
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
bool PN5180::issueISO15693Command(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr) {
#ifdef DEBUG  
  PN5180DEBUG("Issue Command 0x");
  for (int i=0; i<cmdLen; i++) {
    PN5180DEBUG(formatHex(cmd[i]));
  }
  PN5180DEBUG("\n");
#endif

  /*
  uint8_t isoCmd[cmdLen+1];
  isoCmd[0] = 0x00; // ISO Command Configuration Byte
  for (int i=0; i<cmdLen; i++) {
    isoCmd[i+1] = cmd[i];
  }
  sendData(isoCmd, sizeof(isoCmd));
  */
  sendData(cmd, cmdLen);
  
  while (!checkIRQ()); // wait for RX_IRQ to raise
  
  uint32_t irqStatus = getInterrupt();
  if (irqStatus & (1<<0)) {
    Serial.println("\tRQ_IRQ_STAT detected.");
  }
  else {
    Serial.println("\tUNKNOWN IRQ state detected!");
  }

  uint32_t rxStatus;
  readRegister(RX_STATUS, &rxStatus);
  PN5180DEBUG("RX-Status=");
  PN5180DEBUG(formatHex(rxStatus));

  uint8_t frames = (uint8_t)((rxStatus >> 9) & 0x0000000f);
  PN5180DEBUG(", frames=");
  PN5180DEBUG(frames);
  
  uint16_t len = (uint16_t)(rxStatus & 0x000001ff);  
  PN5180DEBUG(", len=");
  PN5180DEBUG(len);
  PN5180DEBUG("\n");
  if (0 == len) return false;
  
  *resultPtr = malloc(len);
  readData(*resultPtr, len);    // TODO: Ist readData() hier erforderlich oder gleich
                                // direct recvSPIFrame() ???

  uint8_t responseFlags = *resultPtr[0];
  if (responseFlags & (1<<0)) { // error flag
    PN5180DEBUG("ERROR code=");
    PN5180DEBUG(formatHex(*resultPtr[1]));
    PN5180DEBUG(" - ");
    switch (*resultPtr[1]) {
      case 0x01:
        PN5180DEBUG("The command is not supported");
        break;
      case 0x02:
        PN5180DEBUG("The command is not recognised");
        break;
      case 0x03:
        PN5180DEBUG("The option is not supported");
        break;
      case 0x0f:
        PN5180DEBUG("Unknown error");
        break;
      case 0x10:
        PN5180DEBUG("The specified block is not available (doesn’t exist)");
        break;
      case 0x11:
        PN5180DEBUG("The specified block is already locked and thus cannot be locked again");
        break;
      case 0x12:
        PN5180DEBUG("The specified block is locked and its content cannot be changed");
        break;
      case 0x13:
        PN5180DEBUG("The specified block was not successfully programmed");
        break;
      case 0x14:
        PN5180DEBUG("The specified block was not successfully locked");
        break;
      default:
        PN5180DEBUG("Custom command error code");
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

//---------------------------------------------------------------------------------------------

/*
 * WRITE_REGISTER - 0x00
 */
bool PN5180::writeRegister(uint8_t reg, uint32_t value) {
  uint8_t *p = (uint8_t*)&value;

#ifdef DEBUG
  PN5180DEBUG("Write Register 0x");
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(", value=");
  PN5180DEBUG(formatHex(value));
  PN5180DEBUG(", (LSB first): ");
  for (int i=0; i<4; i++) {
    PN5180DEBUG(formatHex(p[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif

  /*
  For all 4 byte command parameter transfers (e.g. register values), the payload
  parameters passed follow the little endian approach (Least Significant Byte first). 
   */
  SPI.beginTransaction(PN5180_SPI_SETTINGS);  
  uint8_t buf[6] = { PN5180_WRITE_REGISTER, reg, p[0], p[1], p[2], p[3] };
  sendSPIFrame(buf, 6);
  SPI.endTransaction();
  return !checkIRQ();
}

/*
 * WRITE_REGISTER_OR_MASK - 0x01
 */
bool PN5180::writeRegisterWithOrMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;

#ifdef DEBUG
  PN5180DEBUG("Write Register 0x");
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(" with OR mask=");
  PN5180DEBUG(formatHex(mask));
  PN5180DEBUG(", (LSB first): ");
  for (int i=0; i<4; i++) {
    PN5180DEBUG(formatHex(p[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);  
  uint8_t buf[6] = { PN5180_WRITE_REGISTER_OR_MASK, reg, p[0], p[1], p[2], p[3] };
  sendSPIFrame(buf, 6);
  SPI.endTransaction();
  return !checkIRQ();
}

/*
 * WRITE _REGISTER_AND_MASK - 0x02
 */
bool PN5180::writeRegisterWithAndMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;

#ifdef DEBUG  
  PN5180DEBUG("Write Register 0x");
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(" with AND mask=");
  PN5180DEBUG(formatHex(mask));
  for (int i=0; i<4; i++) {
    PN5180DEBUG(formatHex(p[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif
      
  SPI.beginTransaction(PN5180_SPI_SETTINGS);  
  uint8_t buf[6] = { PN5180_WRITE_REGISTER_AND_MASK, reg, p[0], p[1], p[2], p[3] };
  sendSPIFrame(buf, 6);
  SPI.endTransaction();
  return !checkIRQ();
}

/*
 * READ_REGISTER - 0x04
 */
bool PN5180::readRegister(uint8_t reg, uint32_t *value) {
#ifdef DEBUG
  PN5180DEBUG("Reading register 0x");
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(": value=");
#endif
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  uint8_t cmd[2] = { PN5180_READ_REGISTER, reg };
  sendSPIFrame(cmd, 2);
  recvSPIFrame((uint8_t*)value, 4);
  SPI.endTransaction();
  
  PN5180DEBUG(formatHex(*value));
  PN5180DEBUG("\n");

  return !checkIRQ();
}

/*
 * READ_EEPROM - 0x07
 */
bool PN5180::readEprom(uint8_t addr, uint8_t *buffer, uint8_t len) {
  PN5180DEBUG("Read EEPROM at 0x");
  PN5180DEBUG(formatHex(addr));

  if ((addr > 254) || ((addr+len) > 254)) {
    PN5180DEBUG("\nERROR: Reading beyond addr 254!\n");
    return false;
  }
  
  PN5180DEBUG(", size=");
  PN5180DEBUG(len);
  PN5180DEBUG(": ");
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  uint8_t cmd[3] = { PN5180_READ_EEPROM, addr, len };
  sendSPIFrame(cmd, 3);
  recvSPIFrame(buffer, len);
  SPI.endTransaction();

#ifdef DEBUG
  for (int i=0; i<len; i++) {
    PN5180DEBUG(formatHex(buffer[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif
  
  return !checkIRQ();
}

/*
 * SEND_DATA - 0x09
 * Precondition: Host shall configure the Transceiver by setting the register
 * SYSTEM_CONFIG.COMMAND to 0x3 before using the SEND_DATA command, as
 * the command SEND_DATA is only writing data to the transmission buffer and starts the
 * transmission but does not perform any configuration.
 */
bool PN5180::sendData(uint8_t *data, uint8_t len) {
  PN5180DEBUG("Send data (len=");
  PN5180DEBUG(len);
  PN5180DEBUG("): ");

  if (len > 260) {
    PN5180DEBUG("\nERROR: Sending more than 260 bytes is not supported!\n");
    return false;
  }
  
#ifdef DEBUG
  for (int i=0; i<len; i++) {
    PN5180DEBUG(formatHex(data[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif  

  uint8_t buffer[len+2];
  buffer[0] = PN5180_SEND_DATA;
  buffer[1] = 0; // all bits of last byte are transmitted
  for (uint8_t i=0; i<len; i++) {
    buffer[2+i] = data[i];
  }

  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(buffer, len+2);
  SPI.endTransaction();
  return !checkIRQ();
}

/*
 * READ_DATA - 0x0A
 */
bool PN5180::readData(uint8_t *buffer, uint16_t len) {
  PN5180DEBUG("Read Data (len=");
  PN5180DEBUG(len);
  PN5180DEBUG("): ");

  if (len > 508) {
    PN5180DEBUG("\nERROR: Reading more than 508 bytes is not supported!\n");
    return false;
  }
  
  uint8_t cmd[2] = { PN5180_READ_DATA, 0x00 };
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 2);
  recvSPIFrame(buffer, len);
  SPI.endTransaction();

#ifdef DEBUG
  for (int i=0; i<len; i++) {
    PN5180DEBUG(formatHex(buffer[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif  

  return !checkIRQ();
}

/*
 * LOAD_RF_CONFIG - 0x11
 * 
 * The parameter 0xFF has to be used if the corresponding configuration shall not be changed.
 *
 * Transmitter: RF   Protocol          Speed     Receiver: RF    Protocol    Speed 
 * configuration                       (kbit/s)  configuration               (kbit/s)
 * byte (hex)                                    byte (hex)
 * ----------------------------------------------------------------------------------------------
 *   0D              ISO 15693 ASK100  26        8D              ISO 15693   26
 * ->0E              ISO 15693 ASK10   26        8E              ISO 15693   53
 */
bool PN5180::loadRFConfig(uint8_t txConf, uint8_t rxConf) {
  PN5180DEBUG("Load RF-Config: txConf=");
  PN5180DEBUG(formatHex(txConf));
  PN5180DEBUG(", rxConf=");
  PN5180DEBUG(formatHex(rxConf));
  PN5180DEBUG("\n");
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  uint8_t cmd[3] = { PN5180_LOAD_RF_CONFIG, txConf, rxConf };
  sendSPIFrame(cmd, 3);
  SPI.endTransaction();
  return !checkIRQ();
}

/*
 * RF_ON - 0x16
 */
bool PN5180::setRF_on() {
  PN5180DEBUG("Set RF ON\n");
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  uint8_t cmd[2] = { PN5180_RF_ON, 0x00 };
  sendSPIFrame(cmd, 2);
  SPI.endTransaction();
  return !checkIRQ();
}

/*
 * RF_OFF - 0x17
 */
bool PN5180::setRF_off() {
  PN5180DEBUG("Set RF OFF\n");
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  uint8_t cmd[2] { PN5180_RF_OFF, 0x00 };
  sendSPIFrame(cmd, 2);
  SPI.endTransaction();
  return !checkIRQ();
}

//---------------------------------------------------------------------------------------------

/*
11.4.3.1 A Host Interface Command consists of either 1 or 2 SPI frames depending whether the
host wants to write or read data from the PN5180. An SPI Frame consists of multiple
bytes.

All commands are packed into one SPI Frame. An SPI Frame consists of multiple bytes.
No NSS toggles allowed during sending of an SPI frame.

For all 4 byte command parameter transfers (e.g. register values), the payload
parameters passed follow the little endian approach (Least Significant Byte first).

Direct Instructions are built of a command code (1 Byte) and the instruction parameters
(max. 260 bytes). The actual payload size depends on the instruction used. 
Responses to direct instructions contain only a payload field (no header).
All instructions are bound to conditions. If at least one of the conditions is not fulfilled, an exception is
raised. In case of an exception, the IRQ line of PN5180 is asserted and corresponding interrupt
status register contain information on the exception.
*/
 
/*
The BUSY line is used to indicate that the system is BUSY and cannot receive any data
from a host. Recommendation for the BUSY line handling by the host:
1. Assert NSS to Low
2. Perform Data Exchange
3. Wait until BUSY is high
4. Deassert NSS
5. Wait until BUSY is low

If there is a parameter error, the IRQ is set to ACTIVE and a
GENERAL_ERROR_IRQ is set.
*/
bool PN5180::sendSPIFrame(uint8_t *sendBuffer, uint8_t sendBufferLen) {
  PN5180DEBUG("Sending SPI frame: ");

  // 1.
  digitalWrite(PN5180_NSS, LOW);
  // 2.
  for (uint8_t i=0; i<sendBufferLen; i++) {
    SPI.transfer(sendBuffer[i]);
    PN5180DEBUG(formatHex(sendBuffer[i]));
  }
  // 3.
  while(LOW == digitalRead(PN5180_BUSY));  // wait until BUSY is high
  // 4.
  digitalWrite(PN5180_NSS, HIGH);
  // 5.
  while (HIGH == digitalRead(PN5180_BUSY)); // wait unitl BUSY is low
  PN5180DEBUG("\n");

  return !checkIRQ();
}

/*
In order read data from the PN5180, "dummy reads" shall be performed.
 */
bool PN5180::recvSPIFrame(uint8_t *recvBuffer, uint8_t recvBufferLen) {
  PN5180DEBUG("Reading SPI frame: ");

  // 1.
  digitalWrite(PN5180_NSS, LOW);
  // 2.
  for (uint8_t i=0; i<recvBufferLen; i++) {
    recvBuffer[i] = SPI.transfer(0xff);
    PN5180DEBUG(formatHex(recvBuffer[i]));
  }
  // 3.
  while(LOW == digitalRead(PN5180_BUSY));  // wait until BUSY is high
  // 4.
  digitalWrite(PN5180_NSS, HIGH);
  // 5.
  while(HIGH == digitalRead(PN5180_BUSY));  // wait until BUSY is low
  PN5180DEBUG("\n");

  return !checkIRQ();
}

bool PN5180::checkIRQ() {
  // check IRQ for error
  if (HIGH == digitalRead(PN5180_IRQ)) {
    PN5180DEBUG("ERROR detected: IRQ is set!\n");
    return true;
  }
  else return false; // no IRQ
}

/**
 * @name  getInterrrupt
 * @desc  read interrupt status register and clear interrupt status
 */
uint32_t PN5180::getInterrupt() {
  PN5180DEBUG("Read IRQ-Status register: ");
  
  uint32_t irqStatus;
  readRegister(IRQ_STATUS, &irqStatus);
  PN5180DEBUG(formatHex(irqStatus));
  PN5180DEBUG("\n");
  
  // clear IRQ status
  writeRegister(IRQ_CLEAR, irqStatus);

  return irqStatus;
}

uint8_t PN5180::getTransceiveState() {
  PN5180DEBUG("Get Transceive state...\n");

  uint32_t rfStatus;
  if (!readRegister(RF_STATUS, &rfStatus)) {
    PN5180DEBUG("ERROR reading RF_STATUS register.\n");
    return 0;
  }

  /*
   * TRANSCEIVE_STATE
   *  0 - idle
   *  1 - wait transmit 
   *  2 - transmitting
   *  3 - wait receive
   *  4 - wait for data
   *  5 - receiving
   *  6 - loopback
   *  7 - reserved
   */
  uint8_t state = ((rfStatus >> 24) & 0x07);
  PN5180DEBUG("state=");
  PN5180DEBUG(formatHex(state));
  PN5180DEBUG("\n");

  return state;
}

//---------------------------------------------------------------------------------------------

#ifdef DEBUG
String PN5180::formatHex(uint8_t val) {
  const char hexChar[] = "0123456789ABCDEF";
  
  uint8_t hi = (val & 0xf0) >> 4;
  uint8_t lo = (val & 0x0f);
  
  String s = ""; 
  s += hexChar[hi];
  s += hexChar[lo];

  return s;
}

String PN5180::formatHex(uint16_t val) {
  uint8_t hi = (val & 0xff00) >> 8;
  uint8_t lo = (val & 0x00ff);

  String s = "";
  s += formatHex(hi);
  s += formatHex(lo);

  return s;
}

String PN5180::formatHex(uint32_t val) {
  uint16_t hi = (val & 0xffff0000) >> 16;
  uint16_t lo = (val & 0x0000ffff);

  String s = "";
  s += formatHex(hi);
  s += formatHex(lo);

  return s;
}
#endif /* DEBUG */
