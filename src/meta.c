/*
 * version.c
 *
 * Created: 9/22/2013 11:12:08 PM
 *  Author: clarkson
 */

#include <asf.h>
#include <meta.h>

meta_info_t meta_info;

#if DIAGNOSTIC_FIRMWARE
#define MAJOR_VERSION 9
#define MINOR_VERSION 9
#define PATCH 0
#else
#define MAJOR_VERSION 2
#define MINOR_VERSION 10
#define PATCH 0
#endif

void update_meta_info(void)
{
  bool need_write = false;
  nvm_read(INT_FLASH, META_DESCRIPTOR_ADDRESS, (void *)&meta_info, sizeof(meta_info_t));
  if (meta_info.magic != META_DESCRIPTOR_MAGIC)
  {
    meta_info.magic = META_DESCRIPTOR_MAGIC;
    meta_info.last_saved_snapshot = 0;
    need_write = true;
  }
  if (meta_info.major != MAJOR_VERSION)
  {
    meta_info.major = MAJOR_VERSION;
    need_write = true;
  }
  if (meta_info.minor != MINOR_VERSION)
  {
    if (meta_info.minor < 10)
    {
      meta_info.last_saved_snapshot = 0;
    }
    meta_info.minor = MINOR_VERSION;
    need_write = true;
  }
  if (meta_info.patch != PATCH)
  {
    meta_info.patch = PATCH;
    need_write = true;
  }
  if (meta_info.last_saved_snapshot > 15)
  {
    meta_info.last_saved_snapshot = 0;
    need_write = true;
  }
  if (need_write)
    nvm_write(INT_FLASH, META_DESCRIPTOR_ADDRESS, (void *)&meta_info, sizeof(meta_info_t));
}

void set_last_saved_snapshot(uint8_t slot)
{
  meta_info.last_saved_snapshot = slot;
  nvm_write(INT_FLASH, META_DESCRIPTOR_ADDRESS, (void *)&meta_info, sizeof(meta_info_t));
}

void toggle_start_unpaused()
{
  meta_info.start_unpaused = !meta_info.start_unpaused;
  nvm_write(INT_FLASH, META_DESCRIPTOR_ADDRESS, (void *)&meta_info, sizeof(meta_info_t));
}

int number_of_dac_bits(void)
{
  dac_info_t dac_info;
  nvm_read(INT_USERPAGE, DAC_DESCRIPTOR_ADDRESS, (void *)&dac_info, sizeof(dac_info_t));
  if (dac_info.magic == DAC_DESCRIPTOR_MAGIC)
  {
    return dac_info.nbits;
  }
  return 12;
}