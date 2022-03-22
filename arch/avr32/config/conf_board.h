/**
 * \file
 *
 * \brief User board configuration template
 *
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H


// External oscillator settings.
// Uncomment and set correct values if external oscillator is used.

// External oscillator frequency
//#define BOARD_XOSC_HZ          8000000

// External oscillator type.
//!< External clock signal
//#define BOARD_XOSC_TYPE        XOSC_TYPE_EXTERNAL
//!< 32.768 kHz resonator on TOSC
//#define BOARD_XOSC_TYPE        XOSC_TYPE_32KHZ
//!< 0.4 to 16 MHz resonator on XTALS
//#define BOARD_XOSC_TYPE        XOSC_TYPE_XTAL

// External oscillator startup time
//#define BOARD_XOSC_STARTUP_US  500000

#define BOARD_OSC0_HZ 16000000
#define BOARD_OSC0_STARTUP_US  36000
#define BOARD_OSC0_IS_XTAL        true

#define GPIO_FUNCTION_A        0
#define GPIO_FUNCTION_B        1
#define GPIO_FUNCTION_C        2
#define GPIO_FUNCTION_D        3
#define GPIO_FUNCTION_E        4

#define SRC_HZ				   96000000
#define CPU_HZ                 (SRC_HZ/(1 << CONFIG_SYSCLK_CPU_DIV))
#define PBA_HZ                 (SRC_HZ/(1 << CONFIG_SYSCLK_PBA_DIV))
#define PBB_HZ                 (SRC_HZ/(1 << CONFIG_SYSCLK_PBB_DIV))
#define PBC_HZ                 (SRC_HZ/(1 << CONFIG_SYSCLK_PBC_DIV))

#define USART_DAC              AVR32_USART0
#define PIN_DAC_MOSI           AVR32_PIN_PC16
#define FUNCTION_DAC_MOSI      GPIO_FUNCTION_D
#define PIN_DAC_CLK            AVR32_PIN_PC14
#define FUNCTION_DAC_CLK       GPIO_FUNCTION_B
#define EXTDAC_PDCA_CHANNEL	   2

#define USART_DISPLAY          AVR32_USART1
#define PIN_DISPLAY_MOSI       AVR32_PIN_PD11
#define FUNCTION_DISPLAY_MOSI  GPIO_FUNCTION_A
#define PIN_DISPLAY_CLK        AVR32_PIN_PD13
#define FUNCTION_DISPLAY_CLK   GPIO_FUNCTION_B
#define DISPLAY_PDCA_CHANNEL   3

#define LEFT_ENCODER           AVR32_QDEC0
#define PIN_QDEC0_A            AVR32_PIN_PD09
#define FUNCTION_QDEC0_A       GPIO_FUNCTION_D
#define PIN_QDEC0_B            AVR32_PIN_PD10
#define FUNCTION_QDEC0_B       GPIO_FUNCTION_D

#define RIGHT_ENCODER          AVR32_QDEC1
#define PIN_QDEC1_A            AVR32_PIN_PD24
#define FUNCTION_QDEC1_A       GPIO_FUNCTION_D
#define PIN_QDEC1_B            AVR32_PIN_PD23
#define FUNCTION_QDEC1_B       GPIO_FUNCTION_D

#define TC_BLANK               AVR32_TC0

#define CHANNEL_BLANK_SEGDISP  0
#define PIN_BLANK_SEGDISP      AVR32_PIN_PD28
#define FUNCTION_BLANK_SEGDISP GPIO_FUNCTION_D

#define PIN_BLANK_ORANGE       AVR32_PIN_PD03
#define CHANNEL_BLANK_ORANGE   2
#define FUNCTION_BLANK_ORANGE  GPIO_FUNCTION_B

// outputs
#define PIN_SMOOTH_LED      AVR32_PIN_PD01
#define PIN_COPY_LED           AVR32_PIN_PC31
#define PIN_PAUSE_LED          AVR32_PIN_PC05
#define PIN_COMMIT_LED         AVR32_PIN_PC03
#define PIN_LEND_LED           AVR32_PIN_PC01
#define PIN_LSTART_LED         AVR32_PIN_PC00
#define PIN_SELECT_SEGDISP     AVR32_PIN_PD02
#define PIN_SELECT_ORANGE      AVR32_PIN_PB03

// obsolete
#define PIN_SHARP_LED          AVR32_PIN_PB02
#define PIN_FLAT_LED           AVR32_PIN_PD30

#define PIN_CLEAR_DAC          AVR32_PIN_PC07
#define PIN_RESET_DAC          AVR32_PIN_PC06
#define PIN_SELECT_DAC         AVR32_PIN_PC04
#define PIN_ENABLE_DACSUPPLY   AVR32_PIN_PC13
#define PIN_ENABLE_VREF        AVR32_PIN_PC12
#define PIN_LATCH_DAC          AVR32_PIN_PC11
#define PIN_TRACK1_GATE        AVR32_PIN_PA14
#define PIN_TRACK3_GATE        AVR32_PIN_PA13
#define PIN_TRACK2_GATE        AVR32_PIN_PA12
#define PIN_TRACK4_GATE        AVR32_PIN_PA11

// inputs

// gpio irq 0
#define PIN_LEND_BUTTON        AVR32_PIN_PA07
#define PIN_LSTART_BUTTON      AVR32_PIN_PA06
#define PIN_SNAPSHOT_BUTTON    AVR32_PIN_PA04

// gpio irq 1
#define PIN_MODE_A             AVR32_PIN_PA10
#define PIN_MODE_B             AVR32_PIN_PA09
#define PIN_RESET_INPUT        AVR32_PIN_PA15
#define PIN_COMMIT_BUTTON      AVR32_PIN_PA08

// gpio irq 2
#define PIN_CLOCK_INPUT        AVR32_PIN_PA16

// gpio irq 4
#define PIN_VOLTAGE_BUTTON     AVR32_PIN_PB01
#define PIN_INDEX_BUTTON       AVR32_PIN_PB00

// gpio irq 6
#define PIN_DURATION_BUTTON    AVR32_PIN_PB23
#define PIN_GATE_BUTTON        AVR32_PIN_PB22

// gpio irq 7
//#define PIN_POSITION_BUTTON    AVR32_PIN_PB31
//#define PIN_SNAPSHOT_BUTTON    AVR32_PIN_PB30

// gpio irq 8
#define PIN_STEP_BUTTON    AVR32_PIN_PC02

// gpio irq 10
#define PIN_CVB_BUTTON         AVR32_PIN_PC23
#define PIN_MATH_BUTTON      AVR32_PIN_PC22
#define PIN_COPY_BUTTON        AVR32_PIN_PC21
#define PIN_LOAD_BUTTON        AVR32_PIN_PC20
#define PIN_SAVE_BUTTON        AVR32_PIN_PC19
#define PIN_PAUSE_BUTTON       AVR32_PIN_PC18
#define PIN_RESET_BUTTON       AVR32_PIN_PC17

// gpio irq 11
#define PIN_PATTERN_BUTTON     AVR32_PIN_PC24

// gpio irq 12
#define PIN_INSERT_BUTTON       AVR32_PIN_PD07
#define PIN_DELETE_BUTTON      AVR32_PIN_PD00

// gpio irq 13
#define PIN_TABLE_A            AVR32_PIN_PD14
#define PIN_TABLE_B            AVR32_PIN_PD08

// gpio irq 14
#define PIN_TRACK_BUTTON       AVR32_PIN_PD22
#define PIN_SMOOTH_BUTTON   AVR32_PIN_PD21

// gpio irq 15
#define PIN_CVA_BUTTON         AVR32_PIN_PD29

// expansion pins
#define PIN_EXPANSION_4        AVR32_PIN_PB04
#define PIN_EXPANSION_5        AVR32_PIN_PB05
#define PIN_EXPANSION_6        AVR32_PIN_PB06
#define PIN_EXPANSION_14       AVR32_PIN_PB19
#define PIN_EXPANSION_15       AVR32_PIN_PB20
#define PIN_EXPANSION_16       AVR32_PIN_PB21
#define PIN_EXPANSION_7        AVR32_PIN_PA20
#define PIN_EXPANSION_8        AVR32_PIN_PA21
#define PIN_EXPANSION_9        AVR32_PIN_PA22
#define PIN_EXPANSION_10       AVR32_PIN_PA23
#define PIN_EXPANSION_11       AVR32_PIN_PA24
#define PIN_EXPANSION_12       AVR32_PIN_PA25

#define PIN_EXPANSION_MOSI    PIN_EXPANSION_4
#define PIN_EXPANSION_MISO    PIN_EXPANSION_5
#define PIN_EXPANSION_SCK     PIN_EXPANSION_6
#define PIN_EXPANSION_ALIVE   PIN_EXPANSION_8
#define PIN_EXPANSION_SYNC    PIN_EXPANSION_14

#define EXPANSION_SPI            (&AVR32_SPI1)
#define BRIDGE_PDCA_RX_CHANNEL 1
#define BRIDGE_PDCA_TX_CHANNEL 0
#define BRIDGE_PDCA_RX_IRQ		AVR32_PDCA_IRQ_1
#define BRIDGE_PDCA_TX_IRQ		AVR32_PDCA_IRQ_0

#endif // CONF_BOARD_H
