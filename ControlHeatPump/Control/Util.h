/*
 * Copyright (c) 2016-2021 by Vadim Kulakov vad7@yahoo.com, vad711
 * &                       by Pavel Panfilov <firstlast2007@gmail.com> pav2000
 * "Народный контроллер" для тепловых насосов.
 * Данное програмноое обеспечение предназначено для управления
 * различными типами тепловых насосов для отопления и ГВС.
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */
// --------------------------------------- функции общего использования -------------------------------------------
#ifndef Util_h
#define Util_h

#include <Arduino.h>

#define sign(a) (a > 0 ? 1 : a < 0 ? -1 : 0)
#define signm(a,t) ((a > 0 ? 1 : a < 0 ? -1 : 0) * (abs(a) / t + 1))
#define MIN(a,b) (a < b ? a : b)

struct type_SEMAPHORE {
	volatile bool xSemaphore;
	uint16_t BusyCnt;
};
void SemaphoreCreate(type_SEMAPHORE &_sem);
bool SemaphoreTake(type_SEMAPHORE &_sem, uint32_t wait_time);// Захватить семафор с проверкой, что шедуллер работает
inline void SemaphoreGive(type_SEMAPHORE &_sem) { _sem.xSemaphore = false; };
uint16_t calc_crc16(unsigned char * pcBlock, unsigned short len, uint16_t crc = 0xFFFF);
void int_to_dec_str(int32_t value, int32_t div, char **ret, uint8_t maxfract);
uint8_t calc_bits_in_mask(uint32_t mask);
int32_t round_div_int32(int32_t value, int16_t div);
void getIDchip(char *outstr);
char* _itoa( int value, char *string);
uint32_t i32toa(int32_t value, char *string, uint8_t zero_pad);
uint8_t _ftoa(char *outstr, float val, unsigned char precision);
void _dtoa(char *outstr, int val, int precision);
char* NowTimeToStr(char *buf = NULL);
char* NowDateToStr(char *buf = NULL);
void buffer_space_padding(char * buf, int add);

// Структура для хранения переменных для паролей
struct type_WebSecurity
{
	uint8_t len;
	char *hash;
};
void calc_WebSec_hash(type_WebSecurity *ws, char *login, char *pass, char *buf);

extern uint8_t PWMEnabled;
extern uint8_t TCChanEnabled[];
#define sizeof_TCChanEnabled 9

#ifdef WATTROUTER
#ifdef WR_ONE_PERIOD_PWM
#define PWM_WRITE_OUT_FREQUENCY	PWM_WRITE_OUT_FREQ_DEFAULT	// PWM freq for PWM_Write() function
#else
#define PWM_WRITE_OUT_FREQUENCY	WR.PWM_Freq					// PWM freq for PWM_Write() function
#endif
void WR_Init(void);
void WR_Switch_Load(uint8_t idx, boolean On);
void WR_Change_Load_PWM(uint8_t idx, int16_t delta);
inline int16_t WR_Adjust_PWM_delta(uint8_t idx, int16_t delta);
#ifdef HTTP_MAP_Read_MPPT
// 0 - Oшибка, 1 - Нет свободной энергии, 2 - Нужна пауза, 3 - Есть свободная энергия
int8_t WR_Check_MPPT(void);
#endif
#ifdef HTTP_MAP_Read_MAP
int16_t WR_Read_MAP(void);
#endif
#ifdef PWM_CALC_POWER_ARRAY
void WR_Calc_Power_Array_NewMeter(int32_t power);
#endif
void WR_ReadPowerMeter(void);

#else //WATTROUTER

#define PWM_WRITE_OUT_FREQUENCY	PWM_FREQUENCY	// PWM freq for PWM_Write() function
#ifndef PWM_WRITE_OUT_RESOLUTION
#define PWM_WRITE_OUT_RESOLUTION 8				// bit
#endif

#endif //WATTROUTER


bool Check_I2C_bus(void);						// Проверка I2C на зависание, Возврат true если OK
void Recover_I2C_bus(bool _reset);				// Борьба с зависшими устройствами на шине  I2C
void repower(void);								// Сброс питания на несколько секунд

#endif
