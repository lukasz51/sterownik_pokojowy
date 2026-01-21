#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "uart_dma.h"
#include <stdio.h>
#include <string.h>

/* ===== ZMIENNE ZEWNĘTRZNE ===== */
extern int temperature;          // temperatura w ×10 (np. 234 = 23.4°C)
extern uint32_t voltage_mv;

/* ===== STATUSY ===== */
uint8_t bat_char;
uint8_t fenix_stat = 0;
uint8_t lqi = 0;
uint8_t co_stat = 0;
uint8_t cwu_stat = 0;

/* ===== DEFINICJE ===== */
#define NEXTION_MSG_MAX 256

/* ===== FUNKCJE POMOCNICZE ===== */

static uint8_t battery_level_from_mv(uint32_t mv)
{
    if (mv < 3300)       return 0;
    else if (mv < 3600)  return 1;
    else if (mv < 3800)  return 2;
    else                 return 3;
}

/* ===== WYSYŁANIE TEMPERATURY TXT Z °C ===== */
static int build_nextion_msg(const char *comp, int temp, uint8_t *outbuf)
{
    if (temp < 0)
        return -1;

    int i = 0;

    /* nazwa komponentu */
    while (*comp && i < NEXTION_MSG_MAX)
        outbuf[i++] = *comp++;

    const char suffix[] = ".txt=\"";
    for (int k = 0; k < (int)sizeof(suffix) - 1 && i < NEXTION_MSG_MAX; k++)
        outbuf[i++] = suffix[k];

    /* temp /10 */
    int integer = temp / 10;

    char numbuf[8];
    int idx = 0;

    if (integer == 0)
        numbuf[idx++] = '0';
    else
    {
        int t = integer;
        while (t > 0 && idx < (int)sizeof(numbuf))
        {
            numbuf[idx++] = '0' + (t % 10);
            t /= 10;
        }
    }

    for (int j = idx - 1; j >= 0 && i < NEXTION_MSG_MAX; j--)
        outbuf[i++] = numbuf[j];

    /* °C */
    if (i < NEXTION_MSG_MAX) outbuf[i++] = 0xB0;
    if (i < NEXTION_MSG_MAX) outbuf[i++] = 'C';

    if (i < NEXTION_MSG_MAX) outbuf[i++] = '"';

    /* terminator Nextion */
    if (i + 3 <= NEXTION_MSG_MAX)
    {
        outbuf[i++] = 0xFF;
        outbuf[i++] = 0xFF;
        outbuf[i++] = 0xFF;
    }
    else
        return -1;

    return i;
}

/* ===== WYSYŁANIE TXT (LICZBA W CUDZYSŁOWACH) ===== */
static int append_nextion_txt_value(const char *name,
                                    int value,
                                    uint8_t *buf,
                                    int offset)
{
    int i = offset;

    while (*name && i < NEXTION_MSG_MAX)
        buf[i++] = *name++;

    const char *suf = ".txt=\"";
    while (*suf && i < NEXTION_MSG_MAX)
        buf[i++] = *suf++;

    buf[i++] = '0' + value;
    buf[i++] = '"';

    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;

    return i - offset;
}

/* ===== WYSYŁANIE VAL (BEZ ZMIAN) ===== */
static int append_nextion_val(const char *name,
                              int value,
                              uint8_t *buf,
                              int offset)
{
    int i = offset;

    while (*name && i < NEXTION_MSG_MAX)
        buf[i++] = *name++;

    const char *suf = ".val=";
    while (*suf && i < NEXTION_MSG_MAX)
        buf[i++] = *suf++;

    buf[i++] = '0' + value;

    buf[i++] = 0xFF;
    buf[i++] = 0xFF;
    buf[i++] = 0xFF;

    return i - offset;
}

/* ===== GŁÓWNA FUNKCJA ===== */
void SendDataNextion(void)
{
    uint16_t len = 0;
    uint8_t buf[128];

    /* temperatura */
    int l = build_nextion_msg("tTemp2", temperature, &buf[len]);
    if (l > 0)
        len += l;

    /* bateria — TERAZ JAK TXT Z CUDZYSŁOWAMI */
    uint8_t bat_lvl = battery_level_from_mv(voltage_mv);
    len += append_nextion_txt_value("t_v", bat_lvl, buf, len);

    /* statusy – bez zmian */
    len += append_nextion_val("nCharge", bat_char,   buf, len);
    len += append_nextion_val("nFenix",  fenix_stat, buf, len);
    len += append_nextion_val("nLqi",    lqi,        buf, len);
    len += append_nextion_val("nCO",     co_stat,   buf, len);
    len += append_nextion_val("nCWU",    cwu_stat,  buf, len);

    /* TX */
    uart_tx_write(buf, len);
}
