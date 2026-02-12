#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"
#include "usart.h"
#include "nextion_com.h"
#include <string.h>
#include "nrf24l01p.h"
#include "uart_cmd.h"

uint8_t uart_tx_flag = 0;

#define RX_BUF_SIZE 20

uint8_t uart_rx_byte;      // DMA odbiór po 1 bajcie
uint8_t rx_buf[RX_BUF_SIZE];
volatile uint16_t rx_len = 0;
volatile uint8_t rf_flag;


uint8_t rx_data[NRF24L01P_PAYLOAD_LENGTH];




void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if (htim->Instance == TIM11)
		{
		uart_tx_flag = 1;
		}
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == NRF24L01P_IRQ_PIN_NUMBER)
		nrf24l01p_rx_receive(rx_data);
	    rf_flag = 1;

}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART3)
    {
        uart_cmd_push(rx_buf, Size);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_buf, RX_BUF_SIZE);
    }
}



