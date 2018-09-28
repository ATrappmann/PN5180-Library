// NAME: PN5180.h
//
// DESC: NFC Communication with NXP Semiconductors PN5180 module for Arduino.
//
#ifndef PN5180_H
#define PN5180_H

#include <SPI.h>

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
#define EEPROM_VERSION      (0x14)
#define IRQ_PIN_CONFIG      (0x1A)

enum PN5180TransceiveStat {
  PN5180_TS_Idle = 0,
  PN5180_TS_WaitTransmit = 1,
  PN5180_TS_Transmitting = 2,
  PN5180_TS_WaitReceive = 3,
  PN5180_TS_WaitForData = 4,
  PN5180_TS_Receiving = 5,
  PN5180_TS_LoopBack = 6,
  PN5180_TS_RESERVED = 7
};

// PN5180 IRQ_STATUS
#define RX_IRQ_STAT         (1<<0)  // End of RF rececption IRQ
#define TX_IRQ_STAT         (1<<1)  // End of RF transmission IRQ
#define IDLE_IRQ_STAT       (1<<2)  // IDLE IRQ
#define RFOFF_DET_IRQ_STAT  (1<<6)  // RF Field OFF detection IRQ
#define RFON_DET_IRQ_STAT   (1<<7)  // RF Field ON detection IRQ
#define TX_RFOFF_IRQ_STAT   (1<<8)  // RF Field OFF in PCD IRQ
#define TX_RFON_IRQ_STAT    (1<<9)  // RF Field ON in PCD IRQ
#define RX_SOF_DET_IRQ_STAT (1<<14) // RF SOF Detection IRQ

class PN5180 {
private:
  uint8_t PN5180_NSS;   // active low
  uint8_t PN5180_BUSY;
  uint8_t PN5180_RST;
  uint8_t PN5180_IRQ;

  SPISettings PN5180_SPI_SETTINGS;
  
public:
  PN5180(uint8_t SSpin, uint8_t BUSYpin, uint8_t RSTpin = 0, uint8_t IRQpin = 0);
  
  void begin();
  void end();
  
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
  bool readEEprom(uint8_t addr, uint8_t *buffer, uint8_t len);

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
  void reset();

  bool isIRQ();
  uint32_t getIRQStatus();
  bool clearIRQStatus(uint32_t irqMask);
  
  PN5180TransceiveStat getTransceiveState();
     
  /*
   * Private methods, called within an SPI transaction
   */
private:
  bool transceiveCommand(uint8_t *sendBuffer, size_t sendBufferLen, uint8_t *recvBuffer = 0, size_t recvBufferLen = 0);

};

#endif /* PN5180_H */
