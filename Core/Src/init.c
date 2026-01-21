#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "init.h"
#include "nrf24l01p.h"
#include "delay_us.h"

#define RX_BUF_SIZE 20
extern uint32_t adc[3];
//extern uint8_t uart_rx_byte;      // DMA odbiór po 1 bajcie
extern uint8_t rx_data[NRF24L01P_PAYLOAD_LENGTH];
extern uint8_t rx_buf[RX_BUF_SIZE];



void init(void)
{
	HAL_TIM_Base_Start_IT(&htim11);
    HAL_ADC_Start(&hadc);
	HAL_ADC_Start_DMA(&hadc, adc, 3);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_buf, RX_BUF_SIZE);
	nrf24l01p_rx_init(2476, _250kbps);
	delay_us_init();


}


