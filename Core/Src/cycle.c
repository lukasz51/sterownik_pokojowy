

#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "thermistor.h"
#include <stdint.h>
#include <string.h> // memset
#include <math.h>   // opcjonalnie, jeśli używasz math w innych plikach
#include "nrf24l01p.h"
#include "nextion_com.h"

extern uint8_t uart_tx_flag;
uint8_t tx_data[NRF24L01P_PAYLOAD_LENGTH] = {25, 1, 2, 3, 4, 5, 6, 7};
uint32_t adc[3];

extern volatile uint8_t rf_flag;
// ---------- Konfiguracja ----------
#ifndef FILTER_SIZE
#define FILTER_SIZE   8     // długość filtra (pierścieniowy bufor)
#endif
// Bufory filtrów
static uint16_t adc_buffer[4][FILTER_SIZE];
static uint32_t adc_sum[4] = {0};
static uint8_t adc_index[4] = {0};
static uint8_t adc_filled[4] = {0};


volatile int temperature;        // jedna temperatura
uint32_t voltage_mv;    // napięcie w mV po przeliczeniu i korekcji ×2

#define ADC_MAX     4095
#define ADC_REF_MV  3300   // zmień jeśli masz inną referencję
#define DIV_FACTOR  2      // mnożnik korekcyjny dzielnika

// ---- Filtr średniej kroczącej ----
static uint16_t adc_filter(uint8_t ch, uint16_t new_sample)
{
    adc_sum[ch] -= adc_buffer[ch][adc_index[ch]];
    adc_buffer[ch][adc_index[ch]] = new_sample;
    adc_sum[ch] += new_sample;

    adc_index[ch]++;
    if (adc_index[ch] >= FILTER_SIZE)
        adc_index[ch] = 0;

    if (adc_filled[ch] < FILTER_SIZE)
        adc_filled[ch]++;

    return (uint16_t)(adc_sum[ch] / adc_filled[ch]);
}

// ---- Przeliczanie temperatury i napięcia ----
void process_adc_values(void)
{
    // --- temperatura z adc[0] ---
    uint16_t filtered_t = adc_filter(0, (uint16_t)adc[0]);
    temperature = NTC_ADC2Temperature(filtered_t);

    // --- napięcie z adc[1] ---
    uint16_t filtered_v = adc_filter(1, (uint16_t)adc[1]);

    // ADC -> mV
    uint32_t mv = ((uint32_t)filtered_v * ADC_REF_MV) / ADC_MAX;

    // uwzględnienie dzielnika napięcia (×2)
    voltage_mv = mv * DIV_FACTOR;
}
void cycle(void)
{

	if(uart_tx_flag == 1)
	{
		SendDataNextion();
		uart_tx_flag = 0;
	}
	process_adc_values();
	temperature = temperature - 40;
	tx_data[0] = temperature/10;
	tx_data[1] = temperature% 10;
	tx_data[2] = 24;
	if (rf_flag == 1)
	{

		nrf24l01p_switch_rx_to_tx();
		HAL_Delay(100);
		nrf24l01p_tx_transmit(tx_data);
		HAL_Delay(50);
		nrf24l01p_switch_tx_to_rx();
		rf_flag = 0;
	}




}
