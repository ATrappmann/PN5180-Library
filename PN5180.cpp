// NAME: PN5180.cpp
//
// DESC:
//
//#define DEBUG 1

#include <Arduino.h>
#include "PN5180.h"
#include "Debug.h"

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
  PN5180DEBUG("PN5180 class initialized\n");
  
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
  // use defaults: init_AlwaysInline(4000000, MSBFIRST, SPI_MODE0);
  PN5180_SPI_SETTINGS = SPISettings(4000000, MSBFIRST, SPI_MODE0);
 
  pinMode(PN5180_NSS, OUTPUT);
  pinMode(PN5180_BUSY, INPUT);
  pinMode(PN5180_IRQ, INPUT);

  digitalWrite(PN5180_NSS, HIGH); // disable
  SPI.begin();
}

/*
 * WRITE_REGISTER - 0x00
 * This command is used to write a 32-bit value (little endian) to a configuration register.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool PN5180::writeRegister(uint8_t reg, uint32_t value) {
  uint8_t *p = (uint8_t*)&value;

#ifdef DEBUG
  PN5180DEBUG(F("Write Register 0x"));
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(F(", value (LSB first)=0x"));
  for (int i=0; i<4; i++) {
    PN5180DEBUG(formatHex(p[i]));
  }
  PN5180DEBUG("\n");
#endif

  /*
  For all 4 byte command parameter transfers (e.g. register values), the payload
  parameters passed follow the little endian approach (Least Significant Byte first). 
   */
  uint8_t buf[6] = { PN5180_WRITE_REGISTER, reg, p[0], p[1], p[2], p[3] };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);  
  sendSPIFrame(buf, 6);
  SPI.endTransaction();
  
  return !isIRQ();
}

/*
 * WRITE_REGISTER_OR_MASK - 0x01
 * This command modifies the content of a register using a logical OR operation. The
 * content of the register is read and a logical OR operation is performed with the provided
 * mask. The modified content is written back to the register.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool PN5180::writeRegisterWithOrMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;

#ifdef DEBUG
  PN5180DEBUG(F("Write Register 0x"));
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(F(" with OR mask (LSB first)=0x"));
  for (int i=0; i<4; i++) {
    PN5180DEBUG(formatHex(p[i]));
  }
  PN5180DEBUG("\n");
#endif

  uint8_t buf[6] = { PN5180_WRITE_REGISTER_OR_MASK, reg, p[0], p[1], p[2], p[3] };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);  
  sendSPIFrame(buf, 6);
  SPI.endTransaction();
  
  return !isIRQ();
}

/*
 * WRITE _REGISTER_AND_MASK - 0x02
 * This command modifies the content of a register using a logical AND operation. The
 * content of the register is read and a logical AND operation is performed with the provided
 * mask. The modified content is written back to the register.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool PN5180::writeRegisterWithAndMask(uint8_t reg, uint32_t mask) {
  uint8_t *p = (uint8_t*)&mask;

#ifdef DEBUG  
  PN5180DEBUG(F("Write Register 0x"));
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(F(" with AND mask (LSB first)=0x"));
  for (int i=0; i<4; i++) {
    PN5180DEBUG(formatHex(p[i]));
  }
  PN5180DEBUG("\n");
#endif

  uint8_t buf[6] = { PN5180_WRITE_REGISTER_AND_MASK, reg, p[0], p[1], p[2], p[3] };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);  
  sendSPIFrame(buf, 6);
  SPI.endTransaction();

  return !isIRQ();
}

/*
 * READ_REGISTER - 0x04
 * This command is used to read the content of a configuration register. The content of the
 * register is returned in the 4 byte response.
 * The address of the register must exist. If the condition is not fulfilled, an exception is
 * raised.
 */
bool PN5180::readRegister(uint8_t reg, uint32_t *value) {
  PN5180DEBUG(F("Reading register 0x"));
  PN5180DEBUG(formatHex(reg));
  PN5180DEBUG(F("...\n"));
  
  uint8_t cmd[2] = { PN5180_READ_REGISTER, reg };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 2);
  recvSPIFrame((uint8_t*)value, 4);
  SPI.endTransaction();
  
  PN5180DEBUG(F("Register value=0x"));
  PN5180DEBUG(formatHex(*value));
  PN5180DEBUG("\n");

  return !isIRQ();
}

/*
 * READ_EEPROM - 0x07
 * This command is used to read data from EEPROM memory area. The field 'Address'
 * indicates the start address of the read operation. The field Length indicates the number
 * of bytes to read. The response contains the data read from EEPROM (content of the
 * EEPROM); The data is read in sequentially increasing order starting with the given
 * address.
 * EEPROM Address must be in the range from 0 to 254, inclusive. Read operation must
 * not go beyond EEPROM address 254. If the condition is not fulfilled, an exception is
 * raised.
 */
bool PN5180::readEprom(uint8_t addr, uint8_t *buffer, uint8_t len) {
  if ((addr > 254) || ((addr+len) > 254)) {
    PN5180DEBUG(F("ERROR: Reading beyond addr 254!\n"));
    return false;
  }
  
  PN5180DEBUG(F("Reading EEPROM at 0x"));
  PN5180DEBUG(formatHex(addr));
  PN5180DEBUG(F(", size="));
  PN5180DEBUG(len);
  PN5180DEBUG(F("...\n"));
  
  uint8_t cmd[3] = { PN5180_READ_EEPROM, addr, len };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 3);
  recvSPIFrame(buffer, len);
  SPI.endTransaction();

#ifdef DEBUG
  PN5180DEBUG(F("EEPROM values: "));
  for (int i=0; i<len; i++) {
    PN5180DEBUG(formatHex(buffer[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif
  
  return !isIRQ();
}

/*
 * SEND_DATA - 0x09
 * This command writes data to the RF transmission buffer and starts the RF transmission.
 * The parameter ‘Number of valid bits in last Byte’ indicates the exact number of bits to be
 * transmitted for the last byte (for non-byte aligned frames).
 * Precondition: Host shall configure the Transceiver by setting the register
 * SYSTEM_CONFIG.COMMAND to 0x3 before using the SEND_DATA command, as
 * the command SEND_DATA is only writing data to the transmission buffer and starts the
 * transmission but does not perform any configuration.
 * The size of ‘Tx Data’ field must be in the range from 0 to 260, inclusive (the 0 byte length
 * allows a symbol only transmission when the TX_DATA_ENABLE is cleared).‘Number of
 * valid bits in last Byte’ field must be in the range from 0 to 7. The command must not be
 * called during an ongoing RF transmission. Transceiver must be in ‘WaitTransmit’ state
 * with ‘Transceive’ command set. If the condition is not fulfilled, an exception is raised.
 */
bool PN5180::sendData(uint8_t *data, uint8_t len) {
  if (len > 260) {
    PN5180DEBUG(F("ERROR: sendData with more than 260 bytes is not supported!\n"));
    return false;
  }
  
#ifdef DEBUG
  PN5180DEBUG(F("Send data (len="));
  PN5180DEBUG(len);
  PN5180DEBUG(F("): "));
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

  writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom Command
  writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);   // Transceive Command
  /*
   * Transceive command; initiates a transceive cycle.
   * Note: Depending on the value of the Initiator bit, a
   * transmission is started or the receiver is enabled
   * Note: The transceive command does not finish 
   * automatically. It stays in the transceive cycle until
   * stopped via the IDLE/StopCom command
   */

  PN5180TransceiveStat transceiveState = getTransceiveState();
#ifdef DEBUG  
  if (PN5180_TS_WaitTransmit != transceiveState) {
    PN5180DEBUG(F("*** ERROR: Transceiver not in state WaitTransmit!?"));    
  }
#endif
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(buffer, len+2);
  SPI.endTransaction();
  
  return !isIRQ();
}

/*
 * READ_DATA - 0x0A
 * This command reads data from the RF reception buffer, after a successful reception.
 * The RX_STATUS register contains the information to verify if the reception had been
 * successful. The data is available within the response of the command. The host controls
 * the number of bytes to be read via the SPI interface.
 * The RF data had been successfully received. In case the instruction is executed without
 * preceding an RF data reception, no exception is raised but the data read back from the
 * reception buffer is invalid. If the condition is not fulfilled, an exception is raised.
 */
bool PN5180::readData(uint8_t *buffer, uint16_t len) {
  if (len > 508) {
    PN5180DEBUG("\nERROR: Reading more than 508 bytes is not supported!\n");
    return false;
  }
  
  PN5180DEBUG(F("Reading Data (len="));
  PN5180DEBUG(len);
  PN5180DEBUG(F(")...\n"));

  uint8_t cmd[2] = { PN5180_READ_DATA, 0x00 };
  
  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 2);
  recvSPIFrame(buffer, len);
  SPI.endTransaction();

#ifdef DEBUG
  PN5180DEBUG(F("Data read: "));
  for (int i=0; i<len; i++) {
    PN5180DEBUG(formatHex(buffer[i]));
    PN5180DEBUG(" ");
  }
  PN5180DEBUG("\n");
#endif  

  return !isIRQ();
}

/*
 * LOAD_RF_CONFIG - 0x11
 * Parameter 'Transmitter Configuration' must be in the range from 0x0 - 0x1C, inclusive. If
 * the transmitter parameter is 0xFF, transmitter configuration is not changed.
 * Field 'Receiver Configuration' must be in the range from 0x80 - 0x9C, inclusive. If the
 * receiver parameter is 0xFF, the receiver configuration is not changed. If the condition is
 * not fulfilled, an exception is raised.
 * The transmitter and receiver configuration shall always be configured for the same
 * transmission/reception speed. No error is returned in case this condition is not taken into
 * account.
 * 
 * Transmitter: RF   Protocol          Speed     Receiver: RF    Protocol    Speed 
 * configuration                       (kbit/s)  configuration               (kbit/s)
 * byte (hex)                                    byte (hex)
 * ----------------------------------------------------------------------------------------------
 * ->0D              ISO 15693 ASK100  26        8D              ISO 15693   26
 *   0E              ISO 15693 ASK10   26        8E              ISO 15693   53
 */
bool PN5180::loadRFConfig(uint8_t txConf, uint8_t rxConf) {
  PN5180DEBUG(F("Load RF-Config: txConf="));
  PN5180DEBUG(formatHex(txConf));
  PN5180DEBUG(F(", rxConf="));
  PN5180DEBUG(formatHex(rxConf));
  PN5180DEBUG("\n");
  
  uint8_t cmd[3] = { PN5180_LOAD_RF_CONFIG, txConf, rxConf };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 3);
  SPI.endTransaction();

  return !isIRQ();
}

/*
 * RF_ON - 0x16
 * This command is used to switch on the internal RF field. If enabled the TX_RFON_IRQ is
 * set after the field is switched on.
 */
bool PN5180::setRF_on() {
  PN5180DEBUG(F("Set RF ON\n"));
  
  uint8_t cmd[2] = { PN5180_RF_ON, 0x00 };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 2);
  SPI.endTransaction();
 
  return !isIRQ();
}

/*
 * RF_OFF - 0x17
 * This command is used to switch off the internal RF field. If enabled, the TX_RFOFF_IRQ
 * is set after the field is switched off.
 */
bool PN5180::setRF_off() {
  PN5180DEBUG(F("Set RF OFF\n"));
  
  uint8_t cmd[2] { PN5180_RF_OFF, 0x00 };

  SPI.beginTransaction(PN5180_SPI_SETTINGS);
  sendSPIFrame(cmd, 2);
  SPI.endTransaction();
  
  return !isIRQ();
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
 * A Host Interface Command consists of either 1 or 2 SPI frames depending whether the
 * host wants to write or read data from the PN5180. An SPI Frame consists of multiple
 * bytes.
 * All commands are packed into one SPI Frame. An SPI Frame consists of multiple bytes.
 * No NSS toggles allowed during sending of an SPI frame.
 * For all 4 byte command parameter transfers (e.g. register values), the payload
 * parameters passed follow the little endian approach (Least Significant Byte first).
 * The BUSY line is used to indicate that the system is BUSY and cannot receive any data
 * from a host. Recommendation for the BUSY line handling by the host:
 * 1. Assert NSS to Low
 * 2. Perform Data Exchange
 * 3. Wait until BUSY is high
 * 4. Deassert NSS
 * 5. Wait until BUSY is low
 * If there is a parameter error, the IRQ is set to ACTIVE and a GENERAL_ERROR_IRQ is set.
 */
bool PN5180::sendSPIFrame(uint8_t *sendBuffer, uint8_t sendBufferLen) {
#ifdef DEBUG  
  PN5180DEBUG(F("Sending SPI frame: '"));
  for (uint8_t i=0; i<sendBufferLen; i++) {
    if (i > 0) PN5180DEBUG(" ");
    PN5180DEBUG(formatHex(sendBuffer[i]));
  }
  PN5180DEBUG("'\n");
#endif
  
  // 1.
  digitalWrite(PN5180_NSS, LOW);
  // 2.
  for (uint8_t i=0; i<sendBufferLen; i++) {
    SPI.transfer(sendBuffer[i]);
  }
  // 3.
  while(LOW == digitalRead(PN5180_BUSY));  // wait until BUSY is high
  // 4.
  digitalWrite(PN5180_NSS, HIGH);
  // 5.
  while (HIGH == digitalRead(PN5180_BUSY)); // wait unitl BUSY is low

  return !isIRQ();
}

/*
In order read data from the PN5180, "dummy reads" shall be performed.
 */
bool PN5180::recvSPIFrame(uint8_t *recvBuffer, uint8_t recvBufferLen) {
  PN5180DEBUG(F("Receiving SPI frame...\n"));

  // 1.
  digitalWrite(PN5180_NSS, LOW);
  // 2.
  for (uint8_t i=0; i<recvBufferLen; i++) {
    recvBuffer[i] = SPI.transfer(0xff);
  }
  // 3.
  while(LOW == digitalRead(PN5180_BUSY));  // wait until BUSY is high
  // 4.
  digitalWrite(PN5180_NSS, HIGH);
  // 5.
  while(HIGH == digitalRead(PN5180_BUSY));  // wait until BUSY is low

#ifdef DEBUG  
  PN5180DEBUG(F("Received: "));
  for (uint8_t i=0; i<recvBufferLen; i++) {
    if (i > 0) PN5180DEBUG(" ");
    PN5180DEBUG(formatHex(recvBuffer[i]));
  }
  PN5180DEBUG("'\n");
#endif

  return !isIRQ();
}

/*
 * Check signaled interrupt on IRQ pin
 */
bool PN5180::isIRQ() {
  if (HIGH == digitalRead(PN5180_IRQ)) {
    return true;
  }
  else return false; // no IRQ
}

/**
 * @name  getInterrrupt
 * @desc  read interrupt status register and clear interrupt status
 */
uint32_t PN5180::getIRQStatus() {
  PN5180DEBUG(F("Read IRQ-Status register...\n"));
  
  uint32_t irqStatus;
  readRegister(IRQ_STATUS, &irqStatus);
  PN5180DEBUG(F("IRQ-Status=0x"));
  PN5180DEBUG(formatHex(irqStatus));
  PN5180DEBUG("\n");
  
  // clear IRQ status
  //writeRegister(IRQ_CLEAR, irqStatus);

  return irqStatus;
}

/*
 * Get TRANSCEIVE_STATE from RF_STATUS register
 */
PN5180TransceiveStat PN5180::getTransceiveState() {
  PN5180DEBUG(F("Get Transceive state...\n"));

  uint32_t rfStatus;
  if (!readRegister(RF_STATUS, &rfStatus)) {
    PN5180DEBUG(F("ERROR reading RF_STATUS register.\n"));
    return 0;
  }

  /*
   * TRANSCEIVE_STATEs:
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
  PN5180DEBUG(F("TRANSCEIVE_STATE=0x"));
  PN5180DEBUG(formatHex(state));
  PN5180DEBUG("\n");

  return state;
}
