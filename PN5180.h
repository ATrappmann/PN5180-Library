// NAME: PN5180.h
//
// DESC: NFC Communication with NXP Semiconductors PN5180 module for Arduino.
//
#ifndef PN5180_H
#define PN5180_H

#include <SPI.h>

#ifdef DEBUG
#define PN5180DEBUG(msg) Serial.print(msg)
#else
#define PN5180DEBUG(msg)
#endif

// PN5180 Registers
#define SYSTEM_CONFIG       (0x00)
#define IRQ_ENABLE          (0x01)
#define IRQ_STATUS          (0x02)
#define IRQ_CLEAR           (0x03)
#define TRANSCEIVE_CONTROL  (0x04)
#define TIMER1_RELOAD       (0x0c)
#define TIMER1_CONFIG       (0x0f)
#define RX_WAIT_CONFIG      (0x11)
#define CRC_RX_CONFIG       (0x12)
#define RX_STATUS           (0x13)
#define RF_STATUS           (0x1d)
#define SYSTEM_STATUS       (0x24)
#define TEMP_CONTROL        (0x25)

// PN5180 EEPROM Addresses
#define DIE_IDENTIFIER      (0x00)
#define PRODUCT_VERSION     (0x10)
#define FIRMWARE_VERSION    (0x12)

class PN5180 {
private:
  uint8_t PN5180_NSS;   // active low
  uint8_t PN5180_BUSY;
  uint8_t PN5180_IRQ;

  SPISettings PN5180_SPI_SETTINGS;
  
public:
  PN5180(uint8_t SSpin, uint8_t BUSYpin, uint8_t IRQpin);

  /*
   * PN5180 direct commands with host interface
   */
public:   
  /* cmd 0x00 */
  bool writeRegister(uint8_t reg, uint32_t value);
  /* cmd 0x01 */
  bool writeRegisterWithOrMask(uint8_t addr, uint32_t mask);
  /* cmd 0x02 */
  bool writeRegisterWithAndMask(uint8_t addr, uint32_t mask);

  /* cmd 0x04 */
  bool readRegister(uint8_t reg, uint32_t *value);

  /* cmd 0x07 */
  bool readEprom(uint8_t addr, uint8_t *buffer, uint8_t len);

  /* cmd 0x09 */
  bool sendData(uint8_t *data, uint8_t len);
  /* cmd 0x0a */
  bool readData(uint8_t *buffer, uint16_t len);

  /* cmd 0x11 */
  bool loadRFConfig(uint8_t txConf, uint8_t rxConf);
  
  /* cmd 0x16 */
  bool setRF_on();
  /* cmd 0x17 */
  bool setRF_off();

  /*
   * Helper functions
   */
public:
  bool checkIRQ();
  uint32_t getInterrupt();
  uint8_t getTransceiveState();
     
  /*
   * ISO 15693 commands with tag
   */
private:
  bool issueISO15693Command(uint8_t *cmd, uint8_t cmdLen, uint8_t **resultPtr);

public:   
  bool getInventory(uint8_t *uid);
  bool readSingleBlock(uint8_t *uid, uint8_t blockNo, uint8_t *readBuffer, uint8_t blockSize);
  bool getSystemInfo(uint8_t *uid, uint8_t *blockSize, uint8_t *numBlocks);

  /*
   * Private methods, called within an SPI transaction
   */
private:
  bool sendSPIFrame(uint8_t *sendBuffer, uint8_t sendBufferLen);
  bool recvSPIFrame(uint8_t *recvBuffer, uint8_t recvBufferLen);

#ifdef DEBUG
  /*
   * Helper methods for debugging
   */
private:
  String formatHex(uint8_t val);
  String formatHex(uint16_t val);
  String formatHex(uint32_t val);
#endif  
};

#endif /* PN5180_H */
