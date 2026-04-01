/*
 * Ваттроутер - нагрев бойлера и других потребителей от солнечного инвертора, 
 *   работающего в режиме подкачки в сеть
 * 
 * Copyright (c) 2016-2026 by Vadim Kulakov vad7@yahoo.com, vad711
 * Автор vad711, vad7@yahoo.com
 *
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
//
#ifndef _Wattrouter_h
#define _Wattrouter_h

#include "Constant.h"

#define  WR_fActive				1				// Ваттроутер включен
#define  WR_fLog				2				// Логирование ваттроутера
#define  WR_fLogFull			3				// Логирование ваттроутера полное
#define  WR_fAverage			4				// Усреднение
#define  WR_fMedianFilter		5				// Медианный фильтр на 3 значения
#define  WR_fPeriod_1sec		6				// Период работы 1 сек, иначе TIME_READ_SENSOR
#define  WR_fLoadMask			((1<<WR_NumLoads)-1) // для WR_Refresh
#define  WR_fTYPE				uint8_t

struct {
	WR_fTYPE Loads;						// Биты активирования нагрузки
	WR_fTYPE PWM_Loads;					// Биты нагрузки PWM
	uint16_t Flags;						// Флаги
	int16_t  MinNetLoad;				// Сколько минимально можно брать из сети, Вт
	int16_t  LoadHist;					// Гистерезис нагрузки, Вт
	int16_t  LoadAdd;					// Увеличение нагрузки PWM за один шаг, Вт
	uint16_t NextSwitchPause;			// Задержка следующего переключения реле, секунды
	uint16_t TurnOnPause;				// Задержка включения реле после его выключения, секунды
	uint16_t TurnOnMinTime;				// Минимальное время включения реле, секунды
	uint16_t PWM_Freq;					// Гц
	uint8_t  PWM_FullPowerTime;			// Время работы на максимальной мощности для PWM и время паузы после, 0 - выкл, минут
	uint8_t  PWM_FullPowerLimit;		// Процент ограничения мощности после времени максимальной работы, %
	uint8_t  WF_Time;					// Время получения прогноза погоды, hh * 100 + mm / 10
	uint8_t  MinNetLoadSunDivider;		// Увеличение минимальной мощности из сети в зависимости от выработки MPPT (MinNetLoad += SunPower / n)
	uint8_t  MinNetLoadHyst;			// Гистерезис минимальной мощности из сети, если находимся в нем (MinNetLoad .. MinNetLoad-Hyst), то ничего не делаем с нагрузкой, Вт
	int8_t   DeltaUbatmin;				// Разница Ubuf - Ubatmin, в этом диапазоне идет подкачка и определяется наличие свободного солнца, десятые V
	int16_t  LoadPower[WR_NumLoads];	// Мощности нагрузки, Вт
} WR;

int32_t  WR_Pnet = -32768;
#ifdef WR_PowerMeter_Modbus
	#ifdef WR_PowerMeter_DDS238
volatile int32_t WR_PowerMeter_Power = 0;		// W
	#else
volatile int32_t WR_PowerMeter_Power = 0;		// W
	#endif
volatile uint8_t WR_Error_Read_PowerMeter = 0;
#endif
#if WR_PNET_AVERAGE == 1
int32_t  WR_Pnet_avg;
#elif WR_PNET_AVERAGE > 1
int32_t  WR_Pnet_avg[WR_PNET_AVERAGE];
uint8_t  WR_Pnet_avg_idx = 0;
int32_t  WR_Pnet_avg_sum = 0;
#endif
boolean  WR_Pnet_avg_init = true;
WR_fTYPE WR_Refresh = 0;
WR_fTYPE WR_Loads;						// зависим от профиля
int16_t  WR_LoadRun[WR_NumLoads];		// Включенная мощность, Вт
int32_t  WR_LoadRunStats = 0;
uint32_t WR_SwitchTime[WR_NumLoads];
uint32_t WR_LastSwitchTime = 0;
uint8_t  WR_TestLoadStatus = 0; 		// >0 - идет тестирование нагрузки
uint8_t  WR_TestLoadIndex;
int32_t  WR_LastSunPowerOut = 0;		// Вт
uint8_t  WR_LastSunPowerOutCnt = 0;		// Счетчик задержки отсутствия свободной энергии
uint8_t  WR_LastSunSign = 0;			// 0 - Выключен или ошибка(!), 1 - мало или нет энергии(), 2 - сканирование MPPT(*), 3 - избыток энергии(+)

#define WR_fWF_Read_MPPT	1			// Прочитать данные с солнечного контроллера MPPT
#define WR_fWF_Charging_BAT	2			// Идет заряд АКБ
uint8_t  WR_WorkFlags = 0;
int16_t  WR_MAP_Ubat = 0;
int16_t  WR_MAP_Ubuf = WR_DEFAULT_MAP_Ubuf;	// Буферное напряжение на АКБ, десятые V
#ifdef RSOLINV
int16_t  WR_Invertor2_off_cnt = 0;		// Счетчик до выключения
#endif

#ifdef PWM_CALC_POWER_ARRAY
// Вычисление массива точного расчета мощности
#define PWM_fCalcNow			1
#define PWM_fCalcRelax			2
uint8_t PWM_CalcFlags = 0;
int8_t  PWM_CalcLoadIdx;
int32_t PWM_AverageSum;
uint8_t PWM_AverageCnt; // +1
int32_t PWM_StandbyPower;
int16_t *PWM_CalcArray;
uint16_t PWM_CalcIdx;
#endif

#ifdef WEATHER_FORECAST
uint8_t WF_BoilerTargetPercent = 0;		// Процент от максимальной температуры нагрева бойлера по погоде (100 - греть макс)
#endif
#ifdef WR_LOG_DAYS_POWER_EXCESS
int32_t WR_Power_Excess = 0;			// Излишки ваттроутера
#endif

void WR_Process(void);
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

#endif
