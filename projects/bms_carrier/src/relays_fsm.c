#include "relays_fsm.h"

#include "gpio.h"
// check current sense CS_FAULT pin
// AFE cell conversions, wait 10ms by reading start time
// Peform current sense read and check DONE
// Read AFE Cells and do checks

FSM(bms_relays, NUM_RELAY_STATES, TASK_STACK_512);
static RelaysStateId fsm_prev_state = RELAYS_OPEN;

static CurrentStorage s_currentsense_storage;
static LtcAfeStorage s_ltc_store;

static const LtcAfeSettings s_afe_settings = {
  // Settings pending hardware validation
  .mosi = { .port = GPIO_PORT_B, .pin = 15 },
  .miso = { .port = GPIO_PORT_B, .pin = 14 },
  .sclk = { .port = GPIO_PORT_B, .pin = 13 },
  .cs = { .port = GPIO_PORT_B, .pin = 12 },

  .spi_port = SPI_PORT_2,
  .spi_baudrate = 750000,

  .adc_mode = LTC_AFE_ADC_MODE_7KHZ,

  .cell_bitset = { 0xFFF, 0xFFF, 0xFFF, 0xFFF, 0xFFF },
  .aux_bitset = { 0 },

  .num_devices = 2,
  .num_cells = 12,
  .num_thermistors = 12,
};

static const I2CSettings i2c_settings = {
  .speed = I2C_SPEED_STANDARD,
  .sda = BMS_PERIPH_I2C_SDA_PIN,
  .scl = BMS_PERIPH_I2C_SCL_PIN,
};

const GpioAddress pos_relay_en = { .port = GPIO_PORT_B, .pin = 8 };
const GpioAddress neg_relay_en = { .port = GPIO_PORT_B, .pin = 4 };
const GpioAddress solar_relay_en = { .port = GPIO_PORT_C, .pin = 13 };

void close_relays() {
  // 150 MS GAP BETWEEN EACH RELAY
  gpio_set_state(&pos_relay_en, GPIO_STATE_HIGH);
  gpio_set_state(&neg_relay_en, GPIO_STATE_HIGH);
  gpio_set_state(&solar_relay_en, GPIO_STATE_HIGH);
}

void open_relays() {
  gpio_set_state(&pos_relay_en, GPIO_STATE_LOW);
  gpio_set_state(&neg_relay_en, GPIO_STATE_LOW);
  gpio_set_state(&solar_relay_en, GPIO_STATE_LOW);
}

static void prv_bms_fault_ok_or_transition(Fsm *fsm) {
  LOG_DEBUG("Bms fault check \n");
  StatusCode status = STATUS_CODE_OK;
  uint16_t max_voltage = 0;

  // status |= current_sense_fault_check();

  if (status != STATUS_CODE_OK) {
    fsm_transition(fsm, RELAYS_FAULT);
    return;
  }

  // Retry Mechanism
  if(ltc_afe_impl_trigger_cell_conv(&s_ltc_store)) {
    // If this has failed, try once more after a short delay
    delay_ms(5);
    status |= ltc_afe_impl_trigger_cell_conv(&s_ltc_store);
  }

  delay_ms(10);
  if (status != STATUS_CODE_OK) {
    LOG_DEBUG("status (cell_conv failed): %d\n", status);
    fsm_transition(fsm, RELAYS_FAULT);
    return;
  }

  prv_fuel_gauge_read();

  status |= ltc_afe_impl_read_cells(&s_ltc_store);
  for (size_t cell = 0; cell < (s_afe_settings.num_devices * s_afe_settings.num_cells); cell++) {
    LOG_DEBUG("CELL %d: %d\n\r", cell,
              s_ltc_store.cell_voltages[s_ltc_store.cell_result_lookup[cell]]);
    max_voltage = s_ltc_store.cell_voltages[s_ltc_store.cell_result_lookup[cell]] > max_voltage
                      ? s_ltc_store.cell_voltages[s_ltc_store.cell_result_lookup[cell]]
                      : max_voltage;
    delay_ms(1);
  }
  LOG_DEBUG("MAX VOLTAGE: %d\n", max_voltage);
  set_battery_vt_voltage(max_voltage);
  status |= ltc_afe_impl_fault_check();

  if (status != STATUS_CODE_OK) {
    LOG_DEBUG("status (fault_check or read_cells failed): %d\n", status);
    fsm_transition(fsm, RELAYS_FAULT);
    return;
  }
}

static void prv_relays_open_output(void *context) {
  LOG_DEBUG("Transitioned to RELAYS_OPEN\n");
  open_relays();
  set_battery_relay_info_state(EE_RELAY_STATE_OPEN);
  fsm_prev_state = RELAYS_OPEN;
}

static void prv_relays_open_input(Fsm *fsm, void *context) {
  prv_bms_fault_ok_or_transition(fsm);
  RelaysStateId relay_event = get_bms_relays_relays_state();
  if (relay_event == RELAYS_CLOSED) {
    fsm_transition(fsm, RELAYS_CLOSED);
  }
}

static void prv_relays_closed_output(void *context) {
  LOG_DEBUG("Transitioned to RELAYS_CLOSED\n");
  close_relays();
  set_battery_relay_info_state(EE_RELAY_STATE_CLOSE);
  fsm_prev_state = RELAYS_CLOSED;
}

static void prv_relays_closed_input(Fsm *fsm, void *context) {
  prv_bms_fault_ok_or_transition(fsm);
  RelaysStateId relay_event = get_bms_relays_relays_state();
  if (relay_event == RELAYS_OPEN) {
    fsm_transition(fsm, RELAYS_OPEN);
  }
}

static void prv_relays_fault_output(void *context) {
  LOG_DEBUG("Transitioned to RELAYS_FAULT\n");
  open_relays();
  set_battery_relay_info_state(EE_RELAY_STATE_FAULT);
  fsm_prev_state = RELAYS_FAULT;
}

static void prv_relays_fault_input(Fsm *fsm, void *context) {}

// Relays FSM declaration for states and transitions
static FsmState s_relays_state_list[NUM_RELAY_STATES] = {
  STATE(RELAYS_OPEN, prv_relays_open_input, prv_relays_open_output),
  STATE(RELAYS_CLOSED, prv_relays_closed_input, prv_relays_closed_output),
  STATE(RELAYS_FAULT, prv_relays_fault_input, prv_relays_fault_output)
};

static bool s_relays_transitions[NUM_RELAY_STATES][NUM_RELAY_STATES] = {
  TRANSITION(RELAYS_OPEN, RELAYS_CLOSED), TRANSITION(RELAYS_OPEN, RELAYS_FAULT),
  TRANSITION(RELAYS_CLOSED, RELAYS_OPEN), TRANSITION(RELAYS_CLOSED, RELAYS_FAULT)
};

StatusCode init_bms_relays(void) {
  i2c_init(BMS_PERIPH_I2C_PORT, &i2c_settings);
  ltc_afe_init(&s_ltc_store, &s_afe_settings);
  delay_ms(10);
  current_sense_init(&s_currentsense_storage, &i2c_settings, FUEL_GAUGE_CYCLE_TIME_MS);
  gpio_init_pin(&pos_relay_en, GPIO_OUTPUT_PUSH_PULL, GPIO_STATE_LOW);
  gpio_init_pin(&neg_relay_en, GPIO_OUTPUT_PUSH_PULL, GPIO_STATE_LOW);
  gpio_init_pin(&solar_relay_en, GPIO_OUTPUT_PUSH_PULL, GPIO_STATE_LOW);
  fsm_init(bms_relays, s_relays_state_list, s_relays_transitions, RELAYS_OPEN, NULL);
  return STATUS_CODE_OK;
}
