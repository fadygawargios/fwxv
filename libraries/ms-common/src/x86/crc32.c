#include "crc32.h"  // Include the header file for CRC32 function declarations

#include "log.h"  // Include the logging utility, possibly used for debugging or status messages
#include "misc.h"
#include "stm32f10x.h"  // Include the STM32F1 series specific header file for hardware definitions
#include "stm32f10x_crc.h"  // Include the CRC library header

// Define the CRC32 polynomial and initial value
#define CRC32_POLYNOMIAL \
  0x04C11DB7  // Standard CRC32 polynomial (used in many protocols and formats)
#define CRC32_INIT_VALUE 0xFFFFFFFF  // Initial value used to start the CRC calculation

static uint32_t crc32_calculate_byte(uint32_t crc, uint32_t data) {
  crc ^= data;  // XOR the word directly into the CRC value

  // Perform the polynomial division one bit at a time for each of the 32 bits
  for (int i = 0; i < 32; i++) {
    if (crc & 0x80000000) {                 // If the top bit is set (MSB is 1)
      crc = (crc << 1) ^ CRC32_POLYNOMIAL;  // Shift left and XOR with the polynomial
    } else {
      crc <<= 1;  // Just shift left if the top bit is not set
    }
  }
  return crc;  // Return the updated CRC value after processing the word
}

uint32_t crc_calculate(const uint32_t *buffer, size_t buffer_len) {
  uint32_t crc = ~0;  // Initialize CRC with the inverted initial value (standard practice)

  // Process each byte in the buffer
  for (size_t i = 0; i < buffer_len; i++) {
    crc = crc32_calculate_byte(crc, buffer[i]);  // Update CRC for each byte
  }

  return ~crc;  // Return the inverted CRC32 value (final CRC value needs to be inverted)
}

// no use for x86
StatusCode crc_init(void) {
  return STATUS_CODE_OK;
}
