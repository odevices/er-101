#include <asf.h>
#include <meta.h>
#include <wallclock.h>
#include <extdac.h>
#include <display.h>
#include <ui_events.h>
#include <sequencer.h>
#include <interrupt_handlers.h>
#include <event_handlers.h>
#include <voltages.h>
#include <bridge.h>

volatile wallclock_t display_freeze = 0;

static void configure_filtered_input(uint32_t pin)
{
  gpio_configure_pin(pin, GPIO_DIR_INPUT);
  gpio_enable_pin_glitch_filter(pin);
}

static void pre_init(void)
{
  // unused expansion pins
  gpio_configure_pin(PIN_EXPANSION_7, GPIO_DIR_INPUT | GPIO_PULL_DOWN);
  gpio_configure_pin(PIN_EXPANSION_9, GPIO_DIR_INPUT | GPIO_PULL_DOWN);
  gpio_configure_pin(PIN_EXPANSION_10, GPIO_DIR_INPUT | GPIO_PULL_DOWN);
  gpio_configure_pin(PIN_EXPANSION_11, GPIO_DIR_INPUT | GPIO_PULL_DOWN);
  gpio_configure_pin(PIN_EXPANSION_12, GPIO_DIR_INPUT | GPIO_PULL_DOWN);
  gpio_configure_pin(PIN_EXPANSION_15, GPIO_DIR_INPUT | GPIO_PULL_DOWN);
  gpio_configure_pin(PIN_EXPANSION_16, GPIO_DIR_INPUT | GPIO_PULL_DOWN);

  gpio_configure_pin(PIN_BLANK_SEGDISP, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_BLANK_ORANGE, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);

  gpio_configure_pin(PIN_SELECT_SEGDISP, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_SELECT_ORANGE, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);

  gpio_configure_pin(PIN_TRACK1_GATE, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_TRACK2_GATE, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_TRACK3_GATE, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_TRACK4_GATE, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);

  gpio_configure_pin(PIN_SELECT_DAC, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_ENABLE_DACSUPPLY, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_ENABLE_VREF, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_LATCH_DAC, GPIO_DIR_OUTPUT | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_CLEAR_DAC, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_RESET_DAC, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);

  gpio_configure_pin(PIN_COPY_LED, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_SMOOTH_LED, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_LSTART_LED, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_LEND_LED, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_COMMIT_LED, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INIT_HIGH);
  gpio_configure_pin(PIN_PAUSE_LED, GPIO_DIR_OUTPUT | GPIO_OPEN_DRAIN | GPIO_INIT_HIGH);

  gpio_configure_pin(PIN_EXPANSION_SYNC, GPIO_DIR_OUTPUT | GPIO_INIT_LOW);
  gpio_configure_pin(PIN_EXPANSION_ALIVE, GPIO_DIR_INPUT | GPIO_PULL_DOWN);

  // gpio irq 0
  configure_filtered_input(PIN_LEND_BUTTON);
  configure_filtered_input(PIN_LSTART_BUTTON);

  // gpio irq 1
  configure_filtered_input(PIN_MODE_A);
  configure_filtered_input(PIN_MODE_B);
  configure_filtered_input(PIN_RESET_INPUT);
  configure_filtered_input(PIN_COMMIT_BUTTON);

  // gpio irq 2
  configure_filtered_input(PIN_CLOCK_INPUT);

  // gpio irq 4
  configure_filtered_input(PIN_VOLTAGE_BUTTON);
  configure_filtered_input(PIN_INDEX_BUTTON);

  // gpio irq 6
  configure_filtered_input(PIN_DURATION_BUTTON);
  configure_filtered_input(PIN_GATE_BUTTON);

  // gpio irq 7
  configure_filtered_input(PIN_STEP_BUTTON);
  configure_filtered_input(PIN_SNAPSHOT_BUTTON);

  // gpio irq 10
  configure_filtered_input(PIN_PATTERN_BUTTON);
  configure_filtered_input(PIN_CVB_BUTTON);
  configure_filtered_input(PIN_MATH_BUTTON);
  configure_filtered_input(PIN_COPY_BUTTON);
  configure_filtered_input(PIN_LOAD_BUTTON);
  configure_filtered_input(PIN_SAVE_BUTTON);
  configure_filtered_input(PIN_PAUSE_BUTTON);
  configure_filtered_input(PIN_RESET_BUTTON);

  // gpio irq 12
  configure_filtered_input(PIN_INSERT_BUTTON);
  configure_filtered_input(PIN_DELETE_BUTTON);

  // gpio irq 13
  configure_filtered_input(PIN_TABLE_A);
  configure_filtered_input(PIN_TABLE_B);

  // gpio irq 14
  configure_filtered_input(PIN_TRACK_BUTTON);
  configure_filtered_input(PIN_SMOOTH_BUTTON);

  // gpio irq 15
  configure_filtered_input(PIN_CVA_BUTTON);

  static const gpio_map_t function_map =
      {
          {PIN_DAC_MOSI, FUNCTION_DAC_MOSI},
          {PIN_DAC_CLK, FUNCTION_DAC_CLK},
          {PIN_DISPLAY_MOSI, FUNCTION_DISPLAY_MOSI},
          {PIN_DISPLAY_CLK, FUNCTION_DISPLAY_CLK},
          {PIN_QDEC0_A, FUNCTION_QDEC0_A},
          {PIN_QDEC0_B, FUNCTION_QDEC0_B},
          {PIN_QDEC1_A, FUNCTION_QDEC1_A},
          {PIN_QDEC1_B, FUNCTION_QDEC1_B},
          {PIN_EXPANSION_MISO, AVR32_SPI1_MISO_FUNCTION},
          {PIN_EXPANSION_MOSI, AVR32_SPI1_MOSI_FUNCTION},
          {PIN_EXPANSION_SCK, AVR32_SPI1_SCK_FUNCTION}};

  gpio_enable_module(function_map, sizeof(function_map) / sizeof(function_map[0]));

  static const usart_spi_options_t usart_dac_options =
      {
          .baudrate = 12000000,
          //.baudrate     = 10000,
          .charlength = 8,
          // 1,2,3 work, 0 does not work
          .spimode = 2,
          .channelmode = USART_NORMAL_CHMODE};

  // Initialize USART in SPI mode.
  usart_init_spi_master(&USART_DAC, &usart_dac_options, PBA_HZ);

  static const usart_spi_options_t usart_display_options =
      {
          //.baudrate     = 192*1000,
          .baudrate = 50 * 1000,
          .charlength = 8,
          .spimode = 0,
          .channelmode = USART_NORMAL_CHMODE};

  // Initialize USART in SPI mode.
  usart_init_spi_master(&USART_DISPLAY, &usart_display_options, PBC_HZ);
}

static void post_init(void)
{

  static const gpio_map_t function_map =
      {
          {PIN_BLANK_ORANGE, FUNCTION_BLANK_ORANGE},
      };

  gpio_enable_module(function_map, sizeof(function_map) / sizeof(function_map[0]));

  // Options for waveform generation.
  static const tc_waveform_opt_t blank_orange =
      {
          .channel = CHANNEL_BLANK_ORANGE, // Channel selection.

          .bswtrg = TC_EVT_EFFECT_NOOP, // Software trigger effect on TIOB.
          .beevt = TC_EVT_EFFECT_NOOP,  // External event effect on TIOB.
          .bcpc = TC_EVT_EFFECT_TOGGLE, // RC compare effect on TIOB.
          .bcpb = TC_EVT_EFFECT_TOGGLE, // RB compare effect on TIOB.

          .aswtrg = TC_EVT_EFFECT_NOOP, // Software trigger effect on TIOA.
          .aeevt = TC_EVT_EFFECT_NOOP,  // External event effect on TIOA.
          .acpc = TC_EVT_EFFECT_NOOP,   // RC compare effect on TIOA: toggle.
          .acpa = TC_EVT_EFFECT_NOOP,   // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

          //.wavsel   = TC_WAVEFORM_SEL_UP_MODE,      // Waveform selection: Up mode without automatic trigger on RC compare.
          .wavsel = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
          .enetrg = false,                     // External event trigger enable.
          .eevt = TC_EXT_EVENT_SEL_XC0_OUTPUT, // External event selection.
          .eevtedg = TC_SEL_NO_EDGE,           // External event edge selection.
          .cpcdis = false,                     // Counter disable when RC compare.
          .cpcstop = false,                    // Counter clock stopped with RC compare.

          .burst = TC_BURST_NOT_GATED,   // Burst signal selection.
          .clki = TC_CLOCK_FALLING_EDGE, // Clock inversion.
          .tcclks = TC_CLOCK_SOURCE_TC2  // Internal source clock 2, connected to fPBC / 2
      };

  // Initialize the timer/counter.
  tc_init_waveform(&TC_BLANK, &blank_orange); // Initialize the timer/counter waveform.

  // Set the compare triggers.
  tc_write_rb(&TC_BLANK, CHANNEL_BLANK_ORANGE, 0x8);  // Set RB value.
  tc_write_rc(&TC_BLANK, CHANNEL_BLANK_ORANGE, 0x7F); // Set RC value.

  // Start the timer/counter.
  tc_start(&TC_BLANK, CHANNEL_BLANK_ORANGE);

  // unblank the segdisp
  gpio_set_pin_low(PIN_BLANK_SEGDISP);
}

static void encoder_init(void)
{
  // Options for QDEC timer Mode
  static const qdec_quadrature_decoder_opt_t QDEC_OPTIONS =
      {
          .filten = false,
          .idxphs = 0,
          .idxinv = 0,
          .phsinvb = 0,
          .phsinva = 0,
          .evtrge = false,
          .rcce = true,
          .pcce = true,
          .idxe = false};

  // Options for QDEC Interrupt Management
  static const qdec_interrupt_t QDEC_INTERRUPT =
      {
          .cmp = 0,  // Disable Compare Interrupt.
          .cap = 0,  // Enable Capture Interrupt.
          .pcro = 1, // Enable Position Roll-over Interrupt.
          .rcro = 0  // Enable Counter Roll-over Interrupt.
      };

  qdec_write_rc_cnt(&LEFT_ENCODER, 0);
  qdec_write_pc_cnt(&LEFT_ENCODER, 0);

  qdec_write_rc_top(&LEFT_ENCODER, 99);
  qdec_write_pc_top(&LEFT_ENCODER, 3);

  // Initialize the QDEC in quadrature decoder mode.
  qdec_init_quadrature_decoder_mode(&LEFT_ENCODER, &QDEC_OPTIONS);

  // Configure the QDEC interrupts.
  qdec_configure_interrupts(&LEFT_ENCODER, &QDEC_INTERRUPT);

  // Start the QDEC.
  qdec_software_trigger(&LEFT_ENCODER);

  qdec_write_rc_cnt(&RIGHT_ENCODER, 0);
  qdec_write_pc_cnt(&RIGHT_ENCODER, 0);

  qdec_write_rc_top(&RIGHT_ENCODER, 99);
  qdec_write_pc_top(&RIGHT_ENCODER, 3);

  // Initialize the QDEC in quadrature decoder mode.
  qdec_init_quadrature_decoder_mode(&RIGHT_ENCODER, &QDEC_OPTIONS);

  // Configure the QDEC interrupts.
  qdec_configure_interrupts(&RIGHT_ENCODER, &QDEC_INTERRUPT);

  // Start the QDEC.
  qdec_software_trigger(&RIGHT_ENCODER);
}

static void setup_pll(void)
{
  struct pll_config pcfg;

  flashc_set_wait_state(1);
  flashc_issue_command(AVR32_FLASHC_FCMD_CMD_HSEN, -1);

  osc_enable(OSC_ID_OSC0);
  pll_config_init(&pcfg, PLL_SRC_OSC0, 1, SRC_HZ / BOARD_OSC0_HZ);

  pll_enable(&pcfg, 0);
  sysclk_set_prescalers(CONFIG_SYSCLK_CPU_DIV, CONFIG_SYSCLK_PBA_DIV, CONFIG_SYSCLK_PBB_DIV, CONFIG_SYSCLK_PBC_DIV);
  pll_wait_for_lock(0);
  sysclk_set_source(SYSCLK_SRC_PLL0);
}

static void splash_blank(void)
{
  orange_clear(SEGDISP_TRACK);
  orange_clear(SEGDISP_PATTERN);
  orange_clear(SEGDISP_STEP);
  orange_clear(SEGDISP_SNAPSHOT);
  orange_clear(SEGDISP_CVA);
  orange_clear(SEGDISP_CVB);
  orange_clear(SEGDISP_DURATION);
  orange_clear(SEGDISP_GATE);
  segdisp_display[SEGDISP_TRACK] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_PATTERN] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_STEP] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  // segdisp_display[SEGDISP_INDEX] = SEGDISP_RIGHT_BLANK|SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_CVA] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_CVB] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_DURATION] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_GATE] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
}

static void splash_horizontal(unsigned long delay, int disp0, int disp1, uint16_t *sprite)
{
  for (int i = 0; i < 3; i++)
  {
    splash_blank();
    orange_set(disp0);
    orange_set(disp1);
    segdisp_display[disp0] = sprite[i];
    segdisp_display[disp1] = sprite[i];
    wait_for_display_to_refresh();
    cpu_delay_ms(delay, CPU_HZ);
  }
}

static void do_splash(void)
{
  unsigned long delay = 50;
  uint16_t up[3] = {
      SEGDISP_LEFT_seg_dn | SEGDISP_RIGHT_seg_dn,
      SEGDISP_LEFT_seg_mid | SEGDISP_RIGHT_seg_mid,
      SEGDISP_LEFT_seg_up | SEGDISP_RIGHT_seg_up};
  uint16_t dn[3] = {
      SEGDISP_LEFT_seg_up | SEGDISP_RIGHT_seg_up,
      SEGDISP_LEFT_seg_mid | SEGDISP_RIGHT_seg_mid,
      SEGDISP_LEFT_seg_dn | SEGDISP_RIGHT_seg_dn};

  splash_horizontal(delay, SEGDISP_SNAPSHOT, SEGDISP_GATE, up);
  splash_horizontal(delay, SEGDISP_STEP, SEGDISP_DURATION, up);
  splash_horizontal(delay, SEGDISP_PATTERN, SEGDISP_CVB, up);
  splash_horizontal(delay, SEGDISP_TRACK, SEGDISP_CVA, up);
  splash_horizontal(delay, SEGDISP_TRACK, SEGDISP_CVA, dn);
  splash_horizontal(delay, SEGDISP_PATTERN, SEGDISP_CVB, dn);
  splash_horizontal(delay, SEGDISP_STEP, SEGDISP_DURATION, dn);
  splash_horizontal(delay, SEGDISP_SNAPSHOT, SEGDISP_GATE, dn);

  splash_blank();
}

static void reset_mcu(void)
{
  Disable_global_interrupt();
  wdt_clear();
  // Enable the WDT with a 0s period (fastest way to get a Watchdog reset).
  wdt_opt_t opt = {
      .us_timeout_period = 100000,
      .dar = 1};
  wdt_enable(&opt);
  while (1)
    ;
}

static void standalone(void)
{
  ui_event_t evt;

  gpio_set_pin_low(PIN_COMMIT_LED);
  // init snapshots (if necessary)
  for (int i = 0; i < NUM_SNAPSHOTS; i++)
  {
    if (i == BLANK_SNAPSHOT)
      continue;
    if (!seq_snapshot_exists(i - 1))
    {
      seq_save_snapshot(i - 1);
    }
  }

  // load last snapshot as the default startup state
  seq_load_snapshot(meta_info.last_saved_snapshot);
  seq_set_slot(meta_info.last_saved_snapshot + 1);
  gpio_set_pin_high(PIN_COMMIT_LED);

  // initial state of the UI
  select_left_display(SEGDISP_TRACK);
  select_right_display(SEGDISP_CVA);
  ui_pause_button_push();
  ui_mode_switch();
  ui_table_switch();

  configure_late_interrupts(false);
  extdac_dma_start();

  if (meta_info.start_unpaused)
  {
    ui_pause_button_push();
  }

  // The Main Loop
  while (true)
  {
    // process at most 5 events
    evt = ui_q_pop();
    if (evt != UI_NONE)
    {
      evt();
      evt = ui_q_pop();
      if (evt != UI_NONE)
      {
        evt();
        evt = ui_q_pop();
        if (evt != UI_NONE)
        {
          evt();
          evt = ui_q_pop();
          if (evt != UI_NONE)
          {
            evt();
            evt = ui_q_pop();
            if (evt != UI_NONE)
            {
              evt();
            }
          }
        }
      }
    }

    if (extdac_ready)
    {
      extdac_feed();
    }

    if (segdisp_ready)
    {
      display_feed();
    }

    synthetic_clock_maintenance();
    trigger_maintenance();
    ui_timers_maintenance();
#if FORCE_STANDALONE
#else
    if (gpio_pin_is_high(PIN_EXPANSION_ALIVE))
      reset_mcu();
#endif
  }
}

static void slavemode(void)
{
  configure_late_interrupts(true);
  extdac_dma_start();
  bridge_init();

  // The Main Loop
  while (1)
  {
    while (bridge_feed())
    {
      if (extdac_ready)
      {
        extdac_feed_slavemode();
      }

      trigger_maintenance();

#if FORCE_SLAVEMODE

#else
      if (gpio_pin_is_low(PIN_EXPANSION_ALIVE))
        reset_mcu();
#endif
    }

    if (bridge_control_has_changed)
    {
      bridge_commit_control_changes();
    }

    if (extdac_ready)
    {
      extdac_feed_slavemode();
    }

    if (segdisp_ready)
    {
      if (display_freeze > 0)
      {
        if (wallclock() > display_freeze)
        {
          display_freeze = 0;
        }
      }
      else
      {
        display_feed();
      }
    }

    trigger_maintenance();
  }
}

int main(void)
{
  seq.initialized = false;
  wdt_disable();
  wdt_clear();
  sysclk_init();
  setup_pll();

  Disable_global_interrupt();

  pre_init();
  gpio_set_pin_low(PIN_SMOOTH_LED);

  extdac_init();
  display_init();
  ui_q_init();
  update_meta_info();
  if (gpio_pin_is_low(PIN_PAUSE_BUTTON))
  {
    toggle_start_unpaused();
  }

  configure_early_interrupts(false);
  display_dma_start();

  segdisp_put_version(meta_info.major, meta_info.minor, meta_info.patch);

  wait_for_display_to_refresh();
  post_init();

  do_splash();

  if (number_of_dac_bits() == 16)
  {
    splash_blank();
    segdisp_display[SEGDISP_INDEX] = segdisp_both_digits[16];
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_b | SEGDISP_RIGHT_i;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_t | SEGDISP_RIGHT_BLANK;
    wait_for_display_to_refresh();
    cpu_delay_ms(750, CPU_HZ);
  }
  else if (number_of_dac_bits() == 14)
  {
    splash_blank();
    segdisp_display[SEGDISP_INDEX] = segdisp_both_digits[14];
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_b | SEGDISP_RIGHT_i;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_t | SEGDISP_RIGHT_BLANK;
    wait_for_display_to_refresh();
    cpu_delay_ms(750, CPU_HZ);
  }

  sequencer_init();
  ui_timers_init();
  encoder_init();

  gpio_set_pin_high(PIN_SMOOTH_LED);

  Enable_global_interrupt();

  // cycle the voltage reference a few more times
  gpio_set_pin_low(PIN_ENABLE_VREF);
  cpu_delay_ms(33, CPU_HZ);
  gpio_set_pin_high(PIN_ENABLE_VREF);
  cpu_delay_ms(33, CPU_HZ);
  gpio_set_pin_low(PIN_ENABLE_VREF);
  cpu_delay_ms(33, CPU_HZ);
  gpio_set_pin_high(PIN_ENABLE_VREF);

#if FORCE_SLAVEMODE
  while (1)
  {
    slavemode();
  }
#elif FORCE_STANDALONE
  while (1)
  {
    standalone();
  }
#else
  while (1)
  {
    if (gpio_pin_is_high(PIN_EXPANSION_ALIVE))
    {
      slavemode();
    }
    else
    {
      standalone();
    }
  }
#endif
}
