/*
 * meta.h
 *
 * Created: 9/22/2013 11:12:40 PM
 *  Author: clarkson
 */

#ifndef META_H_
#define META_H_

#define PROGRAM_START_OFFSET 0x00002000
#define PROGRAM_START_ADDRESS (AVR32_FLASH_ADDRESS + PROGRAM_START_OFFSET)
#define MAX_PROGRAM_SIZE (176 * AVR32_FLASH_PAGE_SIZE)
#define META_DESCRIPTOR_MAGIC 0xABCDABCD
#define META_DESCRIPTOR_ADDRESS (PROGRAM_START_ADDRESS + MAX_PROGRAM_SIZE - 32)
#define DAC_DESCRIPTOR_MAGIC 0xABCDDCBA
#define DAC_DESCRIPTOR_ADDRESS 0x808001F0

#define DIAGNOSTIC_FIRMWARE 0
#define FORCE_SLAVEMODE 0
#define FORCE_STANDALONE 0

typedef struct
{
  uint32_t magic;
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
  uint8_t last_saved_snapshot;
  uint8_t start_unpaused;
} meta_info_t;

typedef struct
{
  uint32_t magic;
  uint32_t nbits;
} dac_info_t;

extern meta_info_t meta_info;

void update_meta_info(void);
void set_last_saved_snapshot(uint8_t slot);
void toggle_start_unpaused(void);
int number_of_dac_bits(void);

#endif /* META_H_ */