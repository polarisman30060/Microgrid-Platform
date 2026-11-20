#include "stm32g4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(SIM_NODE_TYPE) || !defined(SIM_INSTANCE) || !defined(SIM_SERIAL_NUMBER)
#error "Select a module environment in platformio.ini."
#endif

#if SIM_NODE_TYPE != 1 && SIM_NODE_TYPE != 3 && SIM_NODE_TYPE != 4 && SIM_NODE_TYPE != 5 && SIM_NODE_TYPE != 6
#error "This project supports Battery, Solar, Generator, Grid, and Inverter nodes."
#endif

#define CAN_ID_COORD_HEARTBEAT 0x100U

#if SIM_NODE_TYPE == 1
#if SIM_INSTANCE > 4
#error "Battery instances are in the range 0..4."
#endif
#define GLOBAL_NODE_ID (0x01U + SIM_INSTANCE)
#define CAN_ID_NODE_CONTROL (0x120U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_HEARTBEAT (0x600U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_IDENTITY (0x700U + GLOBAL_NODE_ID)
#define CAN_ID_COMMAND_ACK (0x760U + GLOBAL_NODE_ID - 1U)
#elif SIM_NODE_TYPE == 3
#if SIM_INSTANCE > 31
#error "Solar segment-local instances are in the range 0..31."
#endif
#define SOLAR_LOCAL_ADDRESS (1U + SIM_INSTANCE)
#define CAN_ID_NODE_CONTROL (0x120U + SOLAR_LOCAL_ADDRESS)
#define CAN_ID_NODE_HEARTBEAT (0x600U + SOLAR_LOCAL_ADDRESS)
#define CAN_ID_NODE_IDENTITY (0x700U + SOLAR_LOCAL_ADDRESS)
#define CAN_ID_COMMAND_ACK (0x740U + SOLAR_LOCAL_ADDRESS)
#elif SIM_NODE_TYPE == 4
#if SIM_INSTANCE > 1
#error "Generator instances are in the range 0..1."
#endif
#define GLOBAL_NODE_ID (0x0BU + SIM_INSTANCE)
#define CAN_ID_NODE_CONTROL (0x120U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_HEARTBEAT (0x600U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_IDENTITY (0x700U + GLOBAL_NODE_ID)
#define CAN_ID_COMMAND_ACK (0x760U + GLOBAL_NODE_ID - 1U)
#elif SIM_NODE_TYPE == 5
#if SIM_INSTANCE > 1
#error "Grid Interface instances are in the range 0..1."
#endif
#define GLOBAL_NODE_ID (0x0DU + SIM_INSTANCE)
#define CAN_ID_NODE_CONTROL (0x120U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_HEARTBEAT (0x600U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_IDENTITY (0x700U + GLOBAL_NODE_ID)
#define CAN_ID_COMMAND_ACK (0x760U + GLOBAL_NODE_ID - 1U)
#elif SIM_NODE_TYPE == 6
#if SIM_INSTANCE > 39
#error "Inverter instances are in the range 0..39."
#endif
#define GLOBAL_NODE_ID (0x0FU + SIM_INSTANCE)
#define CAN_ID_NODE_CONTROL (0x120U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_HEARTBEAT (0x600U + GLOBAL_NODE_ID)
#define CAN_ID_NODE_IDENTITY (0x700U + GLOBAL_NODE_ID)
#define CAN_ID_COMMAND_ACK (0x760U + GLOBAL_NODE_ID - 1U)
#endif

#define COORDINATOR_TIMEOUT_MS 500U
#define BUS_OFF_RECOVERY_DELAY_MS 100U
#define RX_QUEUE_LENGTH 8U
#define SIMULATED_AUX_SUPPLY_MV 30000U
#define SIMULATED_FIRMWARE_BUILD_ID 0x0001U
#define SIMULATED_ENCLOSURE_TEMPERATURE_C 35
#define GENERATOR_CRANKING_MS 1000U
#define GENERATOR_WARMUP_MS 1000U
#define GENERATOR_COOLDOWN_MS 1000U

#if SIM_NODE_TYPE == 1
#define NODE_HEARTBEAT_PERIOD_MS 250U
#define TELEMETRY_PERIOD_MS 100U
#define CAN_ID_TELEMETRY_FAST (0x200U + (SIM_INSTANCE))
#define CAN_ID_TELEMETRY_SLOW (0x210U + (SIM_INSTANCE))
#elif SIM_NODE_TYPE == 3
#define NODE_HEARTBEAT_PERIOD_MS 100U
#define TELEMETRY_PERIOD_MS 100U
#define CAN_ID_TELEMETRY_FAST (0x400U + (SIM_INSTANCE))
#elif SIM_NODE_TYPE == 4
#define NODE_HEARTBEAT_PERIOD_MS 1000U
#define TELEMETRY_PERIOD_MS 500U
#define CAN_ID_TELEMETRY_FAST (0x410U + (SIM_INSTANCE))
#elif SIM_NODE_TYPE == 5
#define NODE_HEARTBEAT_PERIOD_MS 1000U
#define TELEMETRY_PERIOD_MS 500U
#define CAN_ID_TELEMETRY_FAST (0x420U + (SIM_INSTANCE))
#elif SIM_NODE_TYPE == 6
#define NODE_HEARTBEAT_PERIOD_MS 250U
#define TELEMETRY_PERIOD_MS 200U
#define CAN_ID_TELEMETRY_FAST (0x500U + (SIM_INSTANCE))
#endif

#define STATUS_LED_GPIO GPIOA
#define STATUS_LED_PIN GPIO_PIN_5

typedef enum {
    NODE_STATE_OFF = 0,
    NODE_STATE_STARTUP = 1,
    NODE_STATE_READY = 2,
    NODE_STATE_RUNNING = 3,
    NODE_STATE_DEGRADED_AUTONOMOUS = 4,
    NODE_STATE_FAULT = 5,
    NODE_STATE_LOCKOUT = 6,
} node_state_t;

typedef enum {
    GENERATOR_STANDBY = 0,
    GENERATOR_CRANKING = 1,
    GENERATOR_WARMUP = 2,
    GENERATOR_GENERATING = 3,
    GENERATOR_COOLDOWN = 4,
    GENERATOR_FAULT = 5,
} generator_mode_t;

typedef struct {
    uint32_t identifier;
    uint32_t id_type;
    uint32_t frame_type;
    uint32_t data_length;
    uint8_t data[8];
} received_frame_t;

static FDCAN_HandleTypeDef s_fdcan;
static UART_HandleTypeDef s_console;
static volatile received_frame_t s_rx_queue[RX_QUEUE_LENGTH];
static volatile uint8_t s_rx_head;
static volatile uint8_t s_rx_tail;
static volatile uint32_t s_rx_dropped;
static volatile uint32_t s_rx_errors;
static uint32_t s_last_coordinator_heartbeat_ms;
static uint32_t s_next_telemetry_ms;
static uint32_t s_next_slow_telemetry_ms;
static uint32_t s_next_node_heartbeat_ms;
static uint32_t s_bus_recovery_deadline_ms;
#if SIM_NODE_TYPE == 4
static uint32_t s_generator_transition_deadline_ms;
#endif
static bool s_coordinator_timed_out;
static bool s_bus_recovery_pending;
static bool s_output_connected;
static bool s_power_stage_enabled;
#if SIM_NODE_TYPE == 6
static bool s_remote_breaker_trip;
#endif
static uint8_t s_coordinator_flags;
static uint8_t s_heartbeat_counter;
static uint8_t s_generator_mode = GENERATOR_STANDBY;
static int16_t s_charge_or_import_limit_deciamps;
#if SIM_NODE_TYPE == 1
static int16_t s_discharge_or_secondary_limit_deciamps;
#endif
static node_state_t s_node_state = NODE_STATE_READY;
static node_state_t s_state_before_degraded = NODE_STATE_READY;
static uint32_t s_command_crc_errors;
static uint32_t s_last_reported_crc_errors;
static uint32_t s_last_reported_rx_drops;
static uint32_t s_last_reported_rx_errors;
static uint32_t s_last_tx_error_log_ms;

static void Error_Handler(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_FDCAN1_Init(void);

static bool Time_Reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

static uint16_t Read_U16_LE(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t Read_I16_LE(const uint8_t *data)
{
    return (int16_t)Read_U16_LE(data);
}

static void Write_U16_LE(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

#if SIM_NODE_TYPE != 3
static void Write_I16_LE(uint8_t *data, int16_t value)
{
    Write_U16_LE(data, (uint16_t)value);
}
#endif

static void Write_U32_LE(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static uint8_t Crc8_Sae_J1850(const uint8_t *data, uint32_t length)
{
    uint8_t crc = 0xFFU;

    for (uint32_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U
                      ? (uint8_t)((crc << 1) ^ 0x1DU)
                      : (uint8_t)(crc << 1);
        }
    }

    return (uint8_t)(crc ^ 0xFFU);
}

static void Console_Print(const char *message)
{
    if (message != NULL) {
        (void)HAL_UART_Transmit(
            &s_console, (uint8_t *)message, (uint16_t)strlen(message), 20U);
    }
}

static void Send_Can_Frame(uint32_t identifier, const uint8_t data[8])
{
    FDCAN_TxHeaderTypeDef header = {0};

    header.Identifier = identifier;
    header.IdType = FDCAN_STANDARD_ID;
    header.TxFrameType = FDCAN_DATA_FRAME;
    header.DataLength = FDCAN_DLC_BYTES_8;
    header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    header.BitRateSwitch = FDCAN_BRS_OFF;
    header.FDFormat = FDCAN_CLASSIC_CAN;
    header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    header.MessageMarker = 0U;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&s_fdcan, &header, (uint8_t *)data) != HAL_OK) {
        uint32_t now = HAL_GetTick();
        if (now - s_last_tx_error_log_ms >= 1000U) {
            Console_Print("CAN TX queue failed; check bus wiring, termination, and bitrate.\r\n");
            s_last_tx_error_log_ms = now;
        }
    }
}

static void Send_Node_Identity(void)
{
    uint8_t data[8] = {0};

    Write_U32_LE(&data[0], (uint32_t)SIM_SERIAL_NUMBER);
    data[4] = SIM_NODE_TYPE;
    data[5] = SIM_INSTANCE;
    Send_Can_Frame(CAN_ID_NODE_IDENTITY, data);
}

static void Send_Node_Heartbeat(void)
{
    uint8_t data[8] = {0};

    data[0] = s_heartbeat_counter++;
    data[1] = SIM_NODE_TYPE;
    data[2] = (uint8_t)s_node_state;
    if (s_coordinator_timed_out) {
        data[3] = 1U << 3;
    }
    Write_U16_LE(&data[4], SIMULATED_AUX_SUPPLY_MV);
    Write_U16_LE(&data[6], SIMULATED_FIRMWARE_BUILD_ID);
    Send_Can_Frame(CAN_ID_NODE_HEARTBEAT, data);
}

#if SIM_NODE_TYPE == 1 || SIM_NODE_TYPE == 5
static int16_t Clamp_Current_Deciamps(int16_t requested, int16_t maximum)
{
    if (requested < 0) {
        return 0;
    }
    if (maximum > 0 && requested > maximum) {
        return maximum;
    }
    return requested;
}
#endif

static void Send_Module_Telemetry(void)
{
    uint8_t data[8] = {0};

#if SIM_NODE_TYPE == 1
    uint16_t bus_voltage_0p05v = 1920U;
    uint16_t pack_voltage_0p05v = 1920U;
    int16_t pack_current_0p1a = 0;
    uint8_t battery_mode_status = 0U;

    if (s_node_state == NODE_STATE_RUNNING && !s_coordinator_timed_out) {
        if ((s_coordinator_flags & 0x01U) != 0U) {
            pack_current_0p1a = (int16_t)-Clamp_Current_Deciamps(
                s_charge_or_import_limit_deciamps, 200);
            battery_mode_status |= 0x01U;
        } else if ((s_coordinator_flags & 0x02U) != 0U) {
            pack_current_0p1a = Clamp_Current_Deciamps(
                s_discharge_or_secondary_limit_deciamps, 200);
            battery_mode_status |= 0x02U;
        }
    }
    if (s_output_connected) {
        battery_mode_status |= 1U << 2;
    }
    battery_mode_status |= (1U << 5) | (1U << 6) | (1U << 7);
    Write_U16_LE(&data[0], bus_voltage_0p05v);
    Write_U16_LE(&data[2], pack_voltage_0p05v);
    Write_I16_LE(&data[4], pack_current_0p1a);
    data[6] = 150U;
    data[7] = battery_mode_status;
#elif SIM_NODE_TYPE == 3
    uint16_t output_current_0p05a = 0U;
    uint8_t status = 0U;
    const uint16_t available_output_current_0p05a = 38U;

    if (s_power_stage_enabled) {
        output_current_0p05a = available_output_current_0p05a;
        if (s_charge_or_import_limit_deciamps > 0) {
            uint16_t limit_0p05a = (uint16_t)s_charge_or_import_limit_deciamps * 2U;
            if (output_current_0p05a > limit_0p05a) {
                output_current_0p05a = limit_0p05a;
                status |= 1U << 3;
            }
        }
        status |= 0x01U;
    }
    if (s_output_connected) {
        status |= 1U << 2;
    }
    Write_U16_LE(&data[0], 720U);
    Write_U16_LE(&data[2], 100U);
    Write_U16_LE(&data[4], output_current_0p05a);
    data[6] = (uint8_t)(int8_t)SIMULATED_ENCLOSURE_TEMPERATURE_C;
    data[7] = status;
#elif SIM_NODE_TYPE == 4
    int16_t bus_current_0p1a = 0;
    uint16_t engine_rpm = 0U;
    uint8_t status = s_generator_mode;

    if (s_generator_mode == GENERATOR_CRANKING) {
        bus_current_0p1a = -50;
        engine_rpm = 300U;
    } else if (s_generator_mode == GENERATOR_WARMUP) {
        engine_rpm = 1200U;
    } else if (s_generator_mode == GENERATOR_GENERATING) {
        int16_t current_limit = s_charge_or_import_limit_deciamps;
        bus_current_0p1a = current_limit > 0 && current_limit < 100
                               ? current_limit
                               : 100;
        engine_rpm = 1800U;
        status |= 1U << 3;
    } else if (s_generator_mode == GENERATOR_COOLDOWN) {
        engine_rpm = 1200U;
    }
    Write_U16_LE(&data[0], 1920U);
    Write_I16_LE(&data[2], bus_current_0p1a);
    Write_U16_LE(&data[4], engine_rpm);
    data[6] = (uint8_t)(int8_t)SIMULATED_ENCLOSURE_TEMPERATURE_C;
    data[7] = status;
#elif SIM_NODE_TYPE == 5
    uint8_t status = 0x03U;
    int16_t output_current_0p1a = 0;

    if (s_power_stage_enabled && !s_coordinator_timed_out) {
        output_current_0p1a = s_charge_or_import_limit_deciamps > 0
                                  ? Clamp_Current_Deciamps(s_charge_or_import_limit_deciamps, 50)
                                  : 50;
        status |= 1U << 2;
        status |= 1U << 3;
    }
    Write_U16_LE(&data[0], 1200U);
    Write_I16_LE(&data[2], output_current_0p1a);
    data[4] = 100U;
    data[5] = (uint8_t)(int8_t)SIMULATED_ENCLOSURE_TEMPERATURE_C;
    data[6] = status;
    data[7] = 98U;
#elif SIM_NODE_TYPE == 6
    uint16_t output_current_0p05a = 0U;
    int16_t active_power_w = 0;
    uint8_t breaker_status = 0U;

    if (s_remote_breaker_trip) {
        breaker_status = 4U;
    } else if (s_power_stage_enabled) {
        output_current_0p05a = 100U;
        active_power_w = 600;
        breaker_status = 1U;
    }
    if (s_power_stage_enabled && !s_coordinator_timed_out) {
        breaker_status |= 1U << 4;
    }
    Write_U16_LE(&data[0], 1200U);
    Write_U16_LE(&data[2], output_current_0p05a);
    Write_I16_LE(&data[4], active_power_w);
    data[6] = 100U;
    data[7] = breaker_status;
#endif

    Send_Can_Frame(CAN_ID_TELEMETRY_FAST, data);
}

#if SIM_NODE_TYPE == 1
static void Send_Battery_Slow_Telemetry(void)
{
    uint8_t data[8] = {0};

    Write_U16_LE(&data[0], 3300U);
    Write_U16_LE(&data[2], 3350U);
    data[4] = (uint8_t)(int8_t)27;
    data[5] = (uint8_t)(int8_t)32;
    Write_U16_LE(&data[6], 2000U);
    Send_Can_Frame(CAN_ID_TELEMETRY_SLOW, data);
}
#endif

static void Send_Command_Ack(uint8_t action, uint8_t status)
{
    uint8_t data[8] = {0};

    data[0] = action;
    data[1] = status;
    data[2] = (uint8_t)s_node_state;
    Write_U32_LE(&data[4], HAL_GetTick());
    Send_Can_Frame(CAN_ID_COMMAND_ACK, data);
}

static uint8_t Apply_Control_Action(uint8_t action, uint32_t now)
{
    if (s_node_state == NODE_STATE_LOCKOUT && action != 0x05U) {
        return 0x02U;
    }

#if SIM_NODE_TYPE == 4
    switch (action) {
    case 0x06U:
        if (s_generator_mode != GENERATOR_STANDBY &&
            s_generator_mode != GENERATOR_COOLDOWN) {
            return 0x01U;
        }
        s_generator_mode = GENERATOR_CRANKING;
        s_generator_transition_deadline_ms = now + GENERATOR_CRANKING_MS;
        s_node_state = NODE_STATE_STARTUP;
        s_output_connected = false;
        s_power_stage_enabled = false;
        return 0x00U;
    case 0x07U:
    case 0x03U:
        if (s_generator_mode == GENERATOR_CRANKING ||
            s_generator_mode == GENERATOR_WARMUP ||
            s_generator_mode == GENERATOR_GENERATING) {
            s_generator_mode = GENERATOR_COOLDOWN;
            s_generator_transition_deadline_ms = now + GENERATOR_COOLDOWN_MS;
        }
        s_power_stage_enabled = false;
        return 0x00U;
    case 0x05U:
        s_generator_mode = GENERATOR_STANDBY;
        s_node_state = s_coordinator_timed_out
                           ? NODE_STATE_DEGRADED_AUTONOMOUS
                           : NODE_STATE_READY;
        return 0x00U;
    default:
        return 0x01U;
    }
#elif SIM_NODE_TYPE == 6
    switch (action) {
    case 0x01U:
        s_output_connected = true;
        s_node_state = NODE_STATE_READY;
        return 0x00U;
    case 0x02U:
        if (!s_output_connected) {
            return 0x03U;
        }
        s_power_stage_enabled = true;
        s_remote_breaker_trip = false;
        s_node_state = NODE_STATE_RUNNING;
        return 0x00U;
    case 0x03U:
        s_power_stage_enabled = false;
        s_output_connected = false;
        s_remote_breaker_trip = false;
        s_node_state = NODE_STATE_READY;
        return 0x00U;
    case 0x04U:
        s_power_stage_enabled = false;
        s_remote_breaker_trip = true;
        s_node_state = NODE_STATE_FAULT;
        return 0x00U;
    case 0x05U:
        s_power_stage_enabled = false;
        s_output_connected = false;
        s_remote_breaker_trip = false;
        s_node_state = NODE_STATE_READY;
        return 0x00U;
    default:
        return 0x01U;
    }
#else
    switch (action) {
    case 0x01U:
        s_output_connected = true;
        s_node_state = NODE_STATE_READY;
        return 0x00U;
    case 0x02U:
        if (!s_output_connected) {
            return 0x03U;
        }
        s_power_stage_enabled = true;
        s_node_state = NODE_STATE_RUNNING;
        return 0x00U;
    case 0x03U:
        s_power_stage_enabled = false;
        s_output_connected = false;
        s_node_state = NODE_STATE_READY;
        return 0x00U;
    case 0x05U:
        s_power_stage_enabled = false;
        s_output_connected = false;
        s_node_state = NODE_STATE_READY;
        return 0x00U;
    default:
        return 0x01U;
    }
#endif
}

static void Handle_Control_Command(const uint8_t data[8])
{
    uint8_t action = data[0];
    if (Crc8_Sae_J1850(data, 7U) != data[7]) {
        ++s_command_crc_errors;
        Send_Command_Ack(action, 0x04U);
        Console_Print("Rejected draft control command: CRC-8/SAE-J1850 mismatch.\r\n");
        return;
    }

    s_charge_or_import_limit_deciamps = Read_I16_LE(&data[3]);
#if SIM_NODE_TYPE == 1
    s_discharge_or_secondary_limit_deciamps = Read_I16_LE(&data[5]);
#endif
    if (action == 0x00U) {
        return;
    }

    uint8_t ack_status = Apply_Control_Action(action, HAL_GetTick());
    Send_Command_Ack(action, ack_status);
}

static void Process_Received_Frame(const received_frame_t *frame)
{
    if (frame->id_type != FDCAN_STANDARD_ID ||
        frame->frame_type != FDCAN_DATA_FRAME ||
        frame->data_length != FDCAN_DLC_BYTES_8) {
        ++s_rx_errors;
        Console_Print("Ignored malformed/non-standard CAN frame.\r\n");
        return;
    }

    if (frame->identifier == CAN_ID_COORD_HEARTBEAT) {
        s_last_coordinator_heartbeat_ms = HAL_GetTick();
        s_coordinator_flags = frame->data[2];
        if (s_coordinator_timed_out) {
            s_coordinator_timed_out = false;
            if (s_node_state == NODE_STATE_DEGRADED_AUTONOMOUS &&
                !(SIM_NODE_TYPE == 4 && s_generator_mode == GENERATOR_COOLDOWN)) {
                s_node_state = s_state_before_degraded;
            }
            Console_Print("Coordinator heartbeat restored.\r\n");
        }
        return;
    }

    if (frame->identifier == CAN_ID_NODE_CONTROL) {
        Handle_Control_Command((const uint8_t *)frame->data);
    }
}

static bool Try_Dequeue_Frame(received_frame_t *frame)
{
    bool available = false;
    uint32_t interrupt_state = __get_PRIMASK();

    __disable_irq();
    if (s_rx_tail != s_rx_head) {
        uint8_t index = s_rx_tail;
        frame->identifier = s_rx_queue[index].identifier;
        frame->id_type = s_rx_queue[index].id_type;
        frame->frame_type = s_rx_queue[index].frame_type;
        frame->data_length = s_rx_queue[index].data_length;
        for (uint8_t byte = 0U; byte < sizeof(frame->data); ++byte) {
            frame->data[byte] = s_rx_queue[index].data[byte];
        }
        s_rx_tail = (uint8_t)((index + 1U) % RX_QUEUE_LENGTH);
        available = true;
    }
    if (interrupt_state == 0U) {
        __enable_irq();
    }
    return available;
}

static void Update_Coordinator_Watchdog(uint32_t now)
{
    if (!s_coordinator_timed_out &&
        (uint32_t)(now - s_last_coordinator_heartbeat_ms) > COORDINATOR_TIMEOUT_MS) {
        s_coordinator_timed_out = true;
        s_state_before_degraded = s_node_state;
#if SIM_NODE_TYPE == 4
        if (s_generator_mode == GENERATOR_CRANKING ||
            s_generator_mode == GENERATOR_WARMUP ||
            s_generator_mode == GENERATOR_GENERATING) {
            s_generator_mode = GENERATOR_COOLDOWN;
            s_generator_transition_deadline_ms = now + GENERATOR_COOLDOWN_MS;
        }
#elif SIM_NODE_TYPE == 5
        s_power_stage_enabled = false;
        s_output_connected = false;
#endif
        if (s_node_state != NODE_STATE_FAULT && s_node_state != NODE_STATE_LOCKOUT) {
            s_node_state = NODE_STATE_DEGRADED_AUTONOMOUS;
        }
        Console_Print("Coordinator timeout: reporting simulated degraded state.\r\n");
    }
}

static void Update_Generator_State(uint32_t now)
{
#if SIM_NODE_TYPE == 4
    if (s_generator_mode == GENERATOR_CRANKING &&
        Time_Reached(now, s_generator_transition_deadline_ms)) {
        s_generator_mode = GENERATOR_WARMUP;
        s_generator_transition_deadline_ms = now + GENERATOR_WARMUP_MS;
    } else if (s_generator_mode == GENERATOR_WARMUP &&
               Time_Reached(now, s_generator_transition_deadline_ms)) {
        s_generator_mode = GENERATOR_GENERATING;
        s_output_connected = true;
        s_power_stage_enabled = true;
        s_node_state = s_coordinator_timed_out
                           ? NODE_STATE_DEGRADED_AUTONOMOUS
                           : NODE_STATE_RUNNING;
    } else if (s_generator_mode == GENERATOR_COOLDOWN &&
               Time_Reached(now, s_generator_transition_deadline_ms)) {
        s_generator_mode = GENERATOR_STANDBY;
        s_output_connected = false;
        s_power_stage_enabled = false;
        s_node_state = s_coordinator_timed_out
                           ? NODE_STATE_DEGRADED_AUTONOMOUS
                           : NODE_STATE_READY;
    }
#else
    (void)now;
#endif
}

static void Service_Bus_Off_Recovery(uint32_t now)
{
    FDCAN_ProtocolStatusTypeDef protocol_status = {0};

    if (s_bus_recovery_pending) {
        if (Time_Reached(now, s_bus_recovery_deadline_ms)) {
            if (HAL_FDCAN_Start(&s_fdcan) != HAL_OK ||
                HAL_FDCAN_ActivateNotification(
                    &s_fdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U) != HAL_OK) {
                Error_Handler();
            }
            s_bus_recovery_pending = false;
            Console_Print("FDCAN restarted after 100 ms bus-off backoff.\r\n");
        }
        return;
    }

    if (HAL_FDCAN_GetProtocolStatus(&s_fdcan, &protocol_status) != HAL_OK) {
        Error_Handler();
    }

    if (protocol_status.BusOff != 0U) {
        if (HAL_FDCAN_Stop(&s_fdcan) != HAL_OK) {
            Error_Handler();
        }
        s_bus_recovery_pending = true;
        s_bus_recovery_deadline_ms = now + BUS_OFF_RECOVERY_DELAY_MS;
        Console_Print("FDCAN bus-off; retry scheduled after 100 ms.\r\n");
    }
}

static void Report_Queue_Counters(void)
{
    char message[112];
    uint32_t dropped = s_rx_dropped;
    uint32_t errors = s_rx_errors;
    uint32_t crc_errors = s_command_crc_errors;

    if (dropped != s_last_reported_rx_drops ||
        errors != s_last_reported_rx_errors ||
        crc_errors != s_last_reported_crc_errors) {
        (void)snprintf(
            message,
            sizeof(message),
            "CAN RX counters: dropped=%lu invalid=%lu crc=%lu\r\n",
            (unsigned long)dropped,
            (unsigned long)errors,
            (unsigned long)crc_errors);
        Console_Print(message);
        s_last_reported_rx_drops = dropped;
        s_last_reported_rx_errors = errors;
        s_last_reported_crc_errors = crc_errors;
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_FDCAN1_Init();

    if (HAL_FDCAN_Start(&s_fdcan) != HAL_OK ||
        HAL_FDCAN_ActivateNotification(
            &s_fdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U) != HAL_OK) {
        Error_Handler();
    }

    uint32_t now = HAL_GetTick();
    s_last_coordinator_heartbeat_ms = now;
    s_next_telemetry_ms = now;
    s_next_slow_telemetry_ms = now;
    s_next_node_heartbeat_ms = now;
    Send_Node_Identity();

    Console_Print("\r\nMicrogrid draft-profile Nucleo simulator ready.\r\n");
    Console_Print("Classical CAN 500 kbit/s; power-stage outputs are not configured.\r\n");
    Console_Print("Use an isolated CAN transceiver and an isolated communications bench.\r\n");

    while (true) {
        received_frame_t frame;
        while (Try_Dequeue_Frame(&frame)) {
            Process_Received_Frame(&frame);
        }

        now = HAL_GetTick();
        Update_Coordinator_Watchdog(now);
        Update_Generator_State(now);
        Service_Bus_Off_Recovery(now);
        Report_Queue_Counters();

        if (Time_Reached(now, s_next_telemetry_ms)) {
            Send_Module_Telemetry();
            s_next_telemetry_ms = now + TELEMETRY_PERIOD_MS;
        }
#if SIM_NODE_TYPE == 1
        if (Time_Reached(now, s_next_slow_telemetry_ms)) {
            Send_Battery_Slow_Telemetry();
            s_next_slow_telemetry_ms = now + 1000U;
        }
#endif
        if (Time_Reached(now, s_next_node_heartbeat_ms)) {
            Send_Node_Heartbeat();
            s_next_node_heartbeat_ms = now + NODE_HEARTBEAT_PERIOD_MS;
            HAL_GPIO_TogglePin(STATUS_LED_GPIO, STATUS_LED_PIN);
        }
    }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t interrupt_flags)
{
    if (hfdcan != &s_fdcan || (interrupt_flags & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
        return;
    }

    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U) {
        FDCAN_RxHeaderTypeDef header = {0};
        uint8_t data[8] = {0};

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &header, data) != HAL_OK) {
            ++s_rx_errors;
            return;
        }

        uint8_t next_head = (uint8_t)((s_rx_head + 1U) % RX_QUEUE_LENGTH);
        if (next_head == s_rx_tail) {
            ++s_rx_dropped;
            continue;
        }

        uint8_t index = s_rx_head;
        s_rx_queue[index].identifier = header.Identifier;
        s_rx_queue[index].id_type = header.IdType;
        s_rx_queue[index].frame_type = header.RxFrameType;
        s_rx_queue[index].data_length = header.DataLength;
        for (uint8_t byte = 0U; byte < sizeof(data); ++byte) {
            s_rx_queue[index].data[byte] = data[byte];
        }
        s_rx_head = next_head;
    }
}

void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&s_fdcan);
}

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan->Instance != FDCAN1) {
        return;
    }

    __HAL_RCC_FDCAN_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) {
        return;
    }

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = STATUS_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_GPIO, &gpio);
    HAL_GPIO_WritePin(STATUS_LED_GPIO, STATUS_LED_PIN, GPIO_PIN_RESET);
}

static void MX_USART2_UART_Init(void)
{
    s_console.Instance = USART2;
    s_console.Init.BaudRate = 115200U;
    s_console.Init.WordLength = UART_WORDLENGTH_8B;
    s_console.Init.StopBits = UART_STOPBITS_1;
    s_console.Init.Parity = UART_PARITY_NONE;
    s_console.Init.Mode = UART_MODE_TX_RX;
    s_console.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_console.Init.OverSampling = UART_OVERSAMPLING_16;
    s_console.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    s_console.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    s_console.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&s_console) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_FDCAN1_Init(void)
{
    s_fdcan.Instance = FDCAN1;
    s_fdcan.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    s_fdcan.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    s_fdcan.Init.Mode = FDCAN_MODE_NORMAL;
    s_fdcan.Init.AutoRetransmission = ENABLE;
    s_fdcan.Init.TransmitPause = DISABLE;
    s_fdcan.Init.ProtocolException = DISABLE;
    s_fdcan.Init.NominalPrescaler = 17U;
    s_fdcan.Init.NominalSyncJumpWidth = 2U;
    s_fdcan.Init.NominalTimeSeg1 = 15U;
    s_fdcan.Init.NominalTimeSeg2 = 4U;
    s_fdcan.Init.DataPrescaler = 17U;
    s_fdcan.Init.DataSyncJumpWidth = 2U;
    s_fdcan.Init.DataTimeSeg1 = 15U;
    s_fdcan.Init.DataTimeSeg2 = 4U;
    s_fdcan.Init.StdFiltersNbr = 2U;
    s_fdcan.Init.ExtFiltersNbr = 0U;
    s_fdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    RCC_PeriphCLKInitTypeDef peripheral_clock = {0};
    peripheral_clock.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    peripheral_clock.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&peripheral_clock) != HAL_OK ||
        HAL_FDCAN_Init(&s_fdcan) != HAL_OK) {
        Error_Handler();
    }

    FDCAN_FilterTypeDef filter = {0};
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterType = FDCAN_FILTER_MASK;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID2 = 0x7FFU;

    filter.FilterIndex = 0U;
    filter.FilterID1 = CAN_ID_COORD_HEARTBEAT;
    if (HAL_FDCAN_ConfigFilter(&s_fdcan, &filter) != HAL_OK) {
        Error_Handler();
    }

    filter.FilterIndex = 1U;
    filter.FilterID1 = CAN_ID_NODE_CONTROL;
    if (HAL_FDCAN_ConfigFilter(&s_fdcan, &filter) != HAL_OK ||
        HAL_FDCAN_ConfigGlobalFilter(
            &s_fdcan,
            FDCAN_REJECT,
            FDCAN_REJECT,
            FDCAN_REJECT_REMOTE,
            FDCAN_REJECT_REMOTE) != HAL_OK) {
        Error_Handler();
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK) {
        Error_Handler();
    }

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    oscillator.PLL.PLLM = RCC_PLLM_DIV4;
    oscillator.PLL.PLLN = 85U;
    oscillator.PLL.PLLP = RCC_PLLP_DIV2;
    oscillator.PLL.PLLQ = RCC_PLLQ_DIV2;
    oscillator.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        Error_Handler();
    }

    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV1;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

static void Error_Handler(void)
{
    __disable_irq();
    while (true) {
        HAL_GPIO_TogglePin(STATUS_LED_GPIO, STATUS_LED_PIN);
        for (volatile uint32_t delay = 0U; delay < 1500000U; ++delay) {
            __NOP();
        }
    }
}
