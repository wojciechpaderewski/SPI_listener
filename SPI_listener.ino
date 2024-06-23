#include <SPI.h>

volatile uint8_t rxBuffer[1024];
volatile uint8_t rxBufferIndex = 0;
volatile bool dataReady = false;

constexpr uint8_t mySOF = 0xAA;
constexpr uint8_t myEOF = 0xBB;
constexpr uint8_t expectedDataSize = 24;
constexpr uint8_t expectedFrameSize = 1 + expectedDataSize + 1 + 1; // SOF, data, CRC, EOF

ISR (SPI_STC_vect) {
  uint8_t received = SPDR;

  if (rxBufferIndex == 0 && received != 0xC0) {
    return;
  } else if (rxBufferIndex == 1 && received != 0x02) {
    rxBufferIndex = 0;
    return;
  } else if (rxBufferIndex == 2 && received != 0x03) {
    rxBufferIndex = 0;
    return;
  } else if (rxBufferIndex == 17 && received != 0x31) {
    rxBufferIndex = 0;
    return;
  } else if (rxBufferIndex == 18 && received != 0x42) {
    rxBufferIndex = 0;
    return;
  }

  rxBuffer[rxBufferIndex] = received;

  if (rxBufferIndex == expectedDataSize - 1) {
    dataReady = true;
  } else {
    rxBufferIndex++;
  }
}

void setup() {
  Serial.begin(115200);
  SPCR |= _BV(SPE);      // Turn on SPI in Slave Mode
  SPI.attachInterrupt(); // Activate SPI Interrupt
  SPI.setDataMode(SPI_MODE3);
}

uint8_t calcCrc(uint8_t *buffer, uint8_t size) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < size; i++) {
    crc += buffer[i];
  }
  return crc;
}


void loop() {
  if (dataReady) {
    // Critical section to handle shared variables
    noInterrupts();
    uint8_t crc = calcCrc((uint8_t *)rxBuffer, expectedDataSize);
    dataReady = false; // Reset dataReady flag before re-enabling interrupts
    interrupts();

    Serial.write(mySOF);
    for (uint8_t i = 0; i < expectedDataSize; i++) {
      Serial.write(rxBuffer[i]);
    }
    Serial.write(crc);
    Serial.write(myEOF);

    // Reset buffer index after processing
    noInterrupts();
    rxBufferIndex = 0;
    interrupts();
  }
}
