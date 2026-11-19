#include "stm32g4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef BOOST_SIM_SERIAL_NUMBER
#define BOOST_SIM_SERIAL_NUMBER 0x4E554300UL
#endif

#define CAN_ID_COORD_HEARTBEAT 0x100U
#define CAN_ID_BOOST_COMMAND 0x140U
#define CAN_ID_BOOST_TELEMETRY 0x300U
#define CAN_ID_NODE_HEARTBEAT 0x620U
#define CAN_ID_NODE_IDENTITY 0x720U
#define CAN_ID_COMMAND_ACK 0x760U

#define NODE_TYPE_BOOST 0x02U
#define BOOST_INSTANCE 0x00U

#define COORDINATOR_TIMEOUT_MS 500U
#define TELEMETRY_PERIOD_MS 50U
#define NODE_HEARTBEAT_PERIOD_MS 100U
#define NODE_IDENTITY_PERIOD_MS 1000U
#define BUS_OFF_RECOVERY_DELAY_MS 100U
#define RX_QUEUE_LENGTH 8U

#define STATUS_LED_GPIO GPIOA
#define STATUS_LED_PIN GPIO_PIN_5

#define SIMULATED_INPUT_VOLTAGE_0P05V 1920U
#define SIMULATED_STAGE_TEMPERATURE_C 38
#define SIMULATED_RUNNING_INPUT_CURRENT_0P1A 150
#define SIMULATED_AUX_SUPPLY_MV 30000U
#define SIMULATED_FIRMWARE_BUILD_ID 0x0001U
#define DEFAULT_TARGET_VOLTAGE_0P05V 7600U

typedef enum {
    NODE_STATE_OFF = 0,
    NODE_STATE_STARTUP = 1,
    NODE_STATE_READY = 2,
    NODE_STATE_RUNNING = 3,
    NODE_STATE_DEGRADED_AUTONOMOUS = 4,
    NODE_STATE_FAULT = 5,
    NODE_STATE_LOCKOUT = 6,
} node_state_t;

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
static uint32_t s_last_reported_rx_drops;
static uint32_t s_last_reported_rx_errors;
static uint32_t s_last_reported_crc_errors;
static uint32_t s_last_tx_error_log_ms;
static uint32_t s_last_coordinator_heartbeat_ms;
static uint32_t s_next_telemetry_ms;
static uint32_t s_next_node_heartbeat_ms;
static uint32_t s_next_identity_ms;
static uint32_t s_bus_recovery_deadline_ms;
static bool s_coordinator_timed_out;
static bool s_bus_recovery_pending;
static bool s_contactor_closed;
static uint8_t s_active_phases;
static int16_t s_balance_token_deciamps;
static int16_t s_secondary_limit_deciamps;
static uint16_t s_target_voltage_0p05v = DEFAULT_TARGET_VOLTAGE_0P05V;
static node_state_t s_node_state = NODE_STATE_READY;
static node_state_t s_state_before_degraded = NODE_STATE_READY;
static uint8_t s_heartbeat_counter;
static uint32_t s_command_crc_errors;

static void Error_Handler(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_FDCAN1_Init(void);
static void Console_Print(const char *message);
static void Send_Can_Frame(uint32_t identifier, const uint8_t data[8]);
static void Process_Received_Frame(const received_frame_t *frame);
static bool Try_Dequeue_Frame(received_frame_t *frame);

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

static void Write_I16_LE(uint8_t *data, int16_t value)
{
    Write_U16_LE(data, (uint16_t)value);
}

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

    for (uint32_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U
                      ? (uint8_t)((crc << 1) ^ 0x1DU)
                      : (uint8_t)(crc << 1);
        }
    }

    return (uint8_t)(crc ^ 0xFFU);
}

static void Console_Print(const char *message)
{
    if (message == NULL) {
        return;
    }
    (void)HAL_UART_Transmit(
        &s_console, (uint8_t *)message, (uint16_t)strlen(message), 20U);
}

static void Set_Status_Led(bool on)
{
    HAL_GPIO_WritePin(STATUS_LED_GPIO, STATUS_LED_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
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

    Write_U32_LE(&data[0], (uint32_t)BOOST_SIM_SERIAL_NUMBER);
    data[4] = NODE_TYPE_BOOST;
    data[5] = BOOST_INSTANCE;
    data[6] = 0x00U;
    data[7] = 0x00U;
    Send_Can_Frame(CAN_ID_NODE_IDENTITY, data);
}

static void Send_Node_Heartbeat(void)
{
    uint8_t data[8] = {0};

    data[0] = s_heartbeat_counter++;
    data[1] = NODE_TYPE_BOOST;
    data[2] = (uint8_t)s_node_state;
    data[3] = s_coordinator_timed_out ? (uint8_t)(1U << 3) : 0U;
    Write_U16_LE(&data[4], SIMULATED_AUX_SUPPLY_MV);
    Write_U16_LE(&data[6], SIMULATED_FIRMWARE_BUILD_ID);
    Send_Can_Frame(CAN_ID_NODE_HEARTBEAT, data);
}

static void Send_Boost_Telemetry(void)
{
    uint8_t data[8] = {0};
    uint16_t simulated_output_voltage_0p1v = 0U;
    int16_t simulated_input_current_0p1a = 0;
    uint8_t status = 0U;

    if (s_node_state == NODE_STATE_RUNNING ||
        (s_node_state == NODE_STATE_DEGRADED_AUTONOMOUS && s_state_before_degraded == NODE_STATE_RUNNING)) {
        simulated_output_voltage_0p1v =
            (uint16_t)((s_target_voltage_0p05v + 1U) / 2U);
        simulated_input_current_0p1a = SIMULATED_RUNNING_INPUT_CURRENT_0P1A;
    }

    if (s_active_phases > 0U) {
        status |= (uint8_t)(s_active_phases & 0x1FU);
    }
    if (s_contactor_closed) {
        status |= (uint8_t)(1U << 5);
    }
    status |= (uint8_t)(1U << 6);
    if (s_coordinator_timed_out) {
        status |= (uint8_t)(1U << 7);
    }

    Write_U16_LE(&data[0], SIMULATED_INPUT_VOLTAGE_0P05V);
    Write_U16_LE(&data[2], simulated_output_voltage_0p1v);
    Write_I16_LE(&data[4], simulated_input_current_0p1a);
    data[6] = (uint8_t)(int8_t)SIMULATED_STAGE_TEMPERATURE_C;
    data[7] = status;
    Send_Can_Frame(CAN_ID_BOOST_TELEMETRY, data);
}

static void Send_Command_Ack(uint8_t action, uint8_t status)
{
    uint8_t data[8] = {0};

    data[0] = action;
    data[1] = status;
    data[2] = (uint8_t)s_node_state;
    data[3] = 0U;
    Write_U32_LE(&data[4], HAL_GetTick());
    Send_Can_Frame(CAN_ID_COMMAND_ACK, data);
}

static void Handle_Control_Command(const uint8_t data[8])
{
    uint8_t action = data[0];
    uint8_t ack_status = 0U;

    if (Crc8_Sae_J1850(data, 7U) != data[7]) {
        ++s_command_crc_errors;
        Console_Print("Dropped Boost command: CRC-8/SAE-J1850 mismatch (no ACK sent).\r\n");
        return;
    }

    s_target_voltage_0p05v = Read_U16_LE(&data[1]);
    s_balance_token_deciamps = Read_I16_LE(&data[3]);
    s_secondary_limit_deciamps = Read_I16_LE(&data[5]);

    switch (action) {
    case 0x00U:
        return;

    case 0x01U:
        if (s_node_state == NODE_STATE_FAULT || s_node_state == NODE_STATE_LOCKOUT) {
            ack_status = 0x02U;
        } else {
            s_contactor_closed = true;
            s_node_state = NODE_STATE_READY;
        }
        break;

    case 0x02U:
        if (s_node_state == NODE_STATE_FAULT || s_node_state == NODE_STATE_LOCKOUT) {
            ack_status = 0x02U;
        } else if (!s_contactor_closed) {
            ack_status = 0x03U;
        } else {
            s_active_phases = 5U;
            s_node_state = NODE_STATE_RUNNING;
        }
        break;

    case 0x03U:
        if (s_node_state == NODE_STATE_FAULT || s_node_state == NODE_STATE_LOCKOUT) {
            ack_status = 0x02U;
        } else {
            s_active_phases = 0U;
            s_contactor_closed = false;
            s_node_state = NODE_STATE_READY;
        }
        break;

    case 0x05U:
        s_node_state = NODE_STATE_READY;
        s_active_phases = 0U;
        s_contactor_closed = false;
        break;

    case 0x04U:
    case 0x06U:
    case 0x07U:
    default:
        ack_status = 0x01U;
        break;
    }

    Send_Command_Ack(action, ack_status);
    char message[128];
    (void)snprintf(
        message,
        sizeof(message),
        "Simulated command %u: Vset=%u, token=%d, secondary=%d, ACK=%u.\r\n",
        (unsigned int)action,
        (unsigned int)s_target_voltage_0p05v,
        (int)s_balance_token_deciamps,
        (int)s_secondary_limit_deciamps,
        (unsigned int)ack_status);
    Console_Print(message);
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
        if (s_coordinator_timed_out) {
            s_coordinator_timed_out = false;
            if (s_node_state == NODE_STATE_DEGRADED_AUTONOMOUS) {
                s_node_state = s_state_before_degraded;
            }
            Console_Print("Coordinator heartbeat restored.\r\n");
        }
        return;
    }

    if (frame->identifier == CAN_ID_BOOST_COMMAND) {
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
        if (s_node_state != NODE_STATE_FAULT && s_node_state != NODE_STATE_LOCKOUT) {
            s_state_before_degraded = s_node_state;
            s_node_state = NODE_STATE_DEGRADED_AUTONOMOUS;
        }
        Console_Print("Coordinator timeout: reporting simulated degraded state.\r\n");
    }
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
        return;
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
    char message[96];
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
    s_next_node_heartbeat_ms = now;
    s_next_identity_ms = now;

    Console_Print("\r\nMicrogrid Boost 0 CAN simulator ready.\r\n");
    Console_Print("Classical CAN 500 kbit/s; no power-stage outputs are configured.\r\n");
    Console_Print("Use an isolated CAN transceiver and an isolated communications bench.\r\n");

    while (true) {
        received_frame_t frame;
        while (Try_Dequeue_Frame(&frame)) {
            Process_Received_Frame(&frame);
        }

        now = HAL_GetTick();
        Update_Coordinator_Watchdog(now);
        Service_Bus_Off_Recovery(now);
        Report_Queue_Counters();

        if (Time_Reached(now, s_next_telemetry_ms)) {
            Send_Boost_Telemetry();
            s_next_telemetry_ms = now + TELEMETRY_PERIOD_MS;
        }
        if (Time_Reached(now, s_next_node_heartbeat_ms)) {
            Send_Node_Heartbeat();
            s_next_node_heartbeat_ms = now + NODE_HEARTBEAT_PERIOD_MS;
            HAL_GPIO_TogglePin(STATUS_LED_GPIO, STATUS_LED_PIN);
        }
        if (Time_Reached(now, s_next_identity_ms)) {
            Send_Node_Identity();
            s_next_identity_ms = now + NODE_IDENTITY_PERIOD_MS;
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
    Set_Status_Led(false);
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
    filter.FilterID1 = CAN_ID_BOOST_COMMAND;
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
