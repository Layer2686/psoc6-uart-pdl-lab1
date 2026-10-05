#include "cy_pdl.h"
#include "uart_pdl.h"

#define UART_HW             SCB5
#define UART_OVERSAMPLE     (16UL)
#define UART_DIVIDER_TYPE   CY_SYSCLK_DIV_16_5_BIT
#define UART_DIVIDER_NUMBER (0UL)

static const cy_stc_scb_uart_config_t uart_config =
{
    .uartMode = CY_SCB_UART_STANDARD,
    .enableMutliProcessorMode = false,
    .smartCardRetryOnNack = false,
    .irdaInvertRx = false,
    .irdaEnableLowPowerReceiver = false,
    .oversample = UART_OVERSAMPLE,
    .enableMsbFirst = false,
    .dataWidth = 8UL,
    .parity = CY_SCB_UART_PARITY_NONE,
    .stopBits = CY_SCB_UART_STOP_BITS_1,
    .enableInputFilter = false,
    .breakWidth = 11UL,
    .dropOnFrameError = false,
    .dropOnParityError = false,
    .receiverAddress = 0UL,
    .receiverAddressMask = 0UL,
    .acceptAddrInFifo = false,
    .enableCts = false,
    .ctsPolarity = CY_SCB_UART_ACTIVE_LOW,
    .rtsRxFifoLevel = 0UL,
    .rtsPolarity = CY_SCB_UART_ACTIVE_LOW,
    .rxFifoTriggerLevel = 0UL,
    .rxFifoIntEnableMask = 0UL,
    .txFifoTriggerLevel = 0UL,
    .txFifoIntEnableMask = 0UL,
};

bool uart_init(void)
{
    /* cybsp_init() has already configured clk_peri. No HAL resource manager. */
    const uint32_t clock_hz = Cy_SysClk_ClkPeriGetFrequency();
    const uint32_t target_hz = UART_BAUDRATE * UART_OVERSAMPLE;

    /* Round the divider to the nearest 1/32: D = integer + fraction / 32. */
    const uint32_t divider32 = (uint32_t)
        (((uint64_t)clock_hz * 32UL + target_hz / 2UL) / target_hz);

    if ((divider32 < 32UL) || (divider32 > (65536UL * 32UL + 31UL)))
    {
        return false;
    }

    if ((CY_SYSCLK_SUCCESS != Cy_SysClk_PeriphDisableDivider(
            UART_DIVIDER_TYPE, UART_DIVIDER_NUMBER)) ||
        (CY_SYSCLK_SUCCESS != Cy_SysClk_PeriphSetFracDivider(
            UART_DIVIDER_TYPE, UART_DIVIDER_NUMBER,
            divider32 / 32UL - 1UL, divider32 % 32UL)) ||
        (CY_SYSCLK_SUCCESS != Cy_SysClk_PeriphAssignDivider(
            PCLK_SCB5_CLOCK, UART_DIVIDER_TYPE, UART_DIVIDER_NUMBER)) ||
        (CY_SYSCLK_SUCCESS != Cy_SysClk_PeriphEnableDivider(
            UART_DIVIDER_TYPE, UART_DIVIDER_NUMBER)))
    {
        return false;
    }

    /* Route the physical pins to SCB5. TX idles high; RX is a digital input. */
    Cy_GPIO_Pin_FastInit(GPIO_PRT5, 1UL, CY_GPIO_DM_STRONG_IN_OFF,
                        1UL, P5_1_SCB5_UART_TX);
    Cy_GPIO_Pin_FastInit(GPIO_PRT5, 0UL, CY_GPIO_DM_HIGHZ,
                        1UL, P5_0_SCB5_UART_RX);

    /* NULL context: low-level FIFO APIs, with no UART interrupt or DMA. */
    if (CY_SCB_UART_SUCCESS != Cy_SCB_UART_Init(UART_HW, &uart_config, NULL))
    {
        return false;
    }

    Cy_SCB_UART_Enable(UART_HW);
    return true;
}

bool uart_read_byte(uint8_t *byte)
{
    if (0UL == Cy_SCB_UART_GetNumInRxFifo(UART_HW))
    {
        return false;
    }

    *byte = (uint8_t)Cy_SCB_UART_Get(UART_HW);
    return true;
}

bool uart_rx_has_error(void)
{
    const uint32_t error_mask = CY_SCB_UART_RX_OVERFLOW |
        CY_SCB_UART_RX_ERR_FRAME | CY_SCB_UART_RX_ERR_PARITY |
        CY_SCB_UART_RX_BREAK_DETECT;
    const uint32_t errors = Cy_SCB_UART_GetRxFifoStatus(UART_HW) & error_mask;

    if (0UL != errors)
    {
        Cy_SCB_UART_ClearRxFifoStatus(UART_HW, errors);
        return true;
    }

    return false;
}

void uart_write_string(const char *text)
{
    /* Wait for a free TX FIFO entry and enqueue each byte ourselves. */
    while ('\0' != *text)
    {
        while (0UL == Cy_SCB_UART_Put(UART_HW, (uint8_t)*text))
        {
        }
        ++text;
    }

    /* Empty FIFO alone is insufficient: the last stop bit must also leave TX. */
    while (!Cy_SCB_UART_IsTxComplete(UART_HW))
    {
    }
}
