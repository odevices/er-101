#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Values from uc3c1512c.h
#define AVR32_FLASH_SIZE                   0x00080000
#define AVR32_FLASH_PAGE_SIZE              0x00000200

typedef uint64_t wallclock_t;

typedef struct
{
  wallclock_t end_time;
  bool running;
} walltimer_t;

#include "sequencer.h"

int main() {
    printf("running tests...\n");

    printf("sizeof(sequencer_state_t): %lu\n", sizeof(sequencer_data_t));
}