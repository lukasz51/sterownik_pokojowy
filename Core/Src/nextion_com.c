
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "uart_dma.h"
#include <stdio.h>
#include <string.h>

extern int temperature;
extern uint32_t voltage_mv;
uint8_t bat_char;
uint8_t fenix_stat = 0;
uint8_t lqi = 0;
uint8_t co_stat = 0;
uint8_t cwu_stat = 0;
// statyczne bufory dla DMA — jeden na każdą wiadomość (bezpieczne dla DMA)
#define NEXTION_MSG_MAX 256
static uint8_t dma_buf[4][NEXTION_MSG_MAX];


static uint8_t battery_level_from_mv(uint32_t mv)
{
    if (mv < 3300)       return 0;
    else if (mv < 3600)  return 1;
    else if (mv < 3800)  return 2;
    else                 return 3;
}
static int append_nextion_txt(const char *name,
                              int value,
                              uint8_t *buf,
                              int offset)
{
    int i = offset;

    // name.txt="
    while (*name && i < NEXTION_MSG_MAX)
        buf[i++] = *name++;

    const char *suf = ".txt=\"";
    while (*suf && i < NEXTION_MSG_MAX)
        buf[i++] = *suf++;

    // liczba (bez części dziesiętnej, np. 231)
    if (value >= 100) {
        buf[i++] = '0' + (value / 100);
        buf[i++] = '0' + ((value / 10) % 10);
        buf[i++] = '0' + (value % 10);
    } else if (value >= 10) {
        buf[i++] = '0' + (value / 10);
        buf[i++] = '0' + (value % 10);
    } else {
        buf[i++] = '0' + value;
    }

    buf[i++] = '"';

    // terminator Nextion
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;

    return i - offset;
}
static int append_nextion_val(const char *name,
                              int value,
                              uint8_t *buf,
                              int offset)
{
    int i = offset;

    // name.val=
    while (*name && i < NEXTION_MSG_MAX)
        buf[i++] = *name++;

    const char *suf = ".val=";
    while (*suf && i < NEXTION_MSG_MAX)
        buf[i++] = *suf++;

    buf[i++] = '0' + value;   // 0–9 wystarcza dla statusów

    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;

    return i - offset;
}
void SendDataNextion(void)
{
    uint16_t len = 0;
    uint8_t buf[128];   // lokalny bufor pakietu (dobierz rozmiar!)

    /* temperatura -> TEXT */
    len += append_nextion_txt(
                "tTemp2",
                temperature,
                buf,
                len
           );

    /* bateria 0–3 */
    uint8_t bat_lvl = battery_level_from_mv(voltage_mv);
    len += append_nextion_val(
                "t_v",
                bat_lvl,
                buf,
                len
           );

    /* statusy */
    len += append_nextion_val("nCharge", bat_char,  buf, len);
    len += append_nextion_val("nFenix",  fenix_stat,buf, len);
    len += append_nextion_val("nLqi",    lqi,       buf, len);
    len += append_nextion_val("nCO",     co_stat,  buf, len);
    len += append_nextion_val("nCWU",    cwu_stat, buf, len);

    /* wrzucenie do TX ring buffer + DMA */
    uart_tx_write(buf, len);
}
