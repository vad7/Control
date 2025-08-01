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
// --------------------------------------------------------------------------------
// Описание базовых классов для работы Теплового Насоса
// --------------------------------------------------------------------------------
#ifndef HeatPump_h
#define HeatPump_h
#include "Constant.h"                       // Вся конфигурация и константы проекта Должен быть первым !!!!
#include "Hardware.h"
#include "Message.h"
#include "Information.h"
#include "MQTT.h"
#include "OmronFC.h"
#include "VaconFC.h"
#include "Scheduler.h"
#include "Heater.h"

extern char *MAC2String(byte* mac);

int32_t updatePID(int32_t errorPid, PID_STRUCT &pid, PID_WORK_STRUCT &pidw);
void UpdatePIDbyTime(uint16_t new_time, uint16_t curr_time, PID_STRUCT &pid);

/*/ Структура для хранения заголовка при сохранении настроек EEPROM
struct type_headerEEPROM    // РАЗМЕР 1+1+2+2=6 байт
{
//  byte magic;               // признак данных, должно быть  0xaa
//  byte zero;                // признак данных, должно быть  0x00
  uint16_t ver;             // номер версии для сохранения
  uint16_t len;             // длина данных, сколько записано байт в еепром
 };
*/

// Структура описывающая текущее состояние ТН (хранится только в памяти)
struct type_status
{
   MODE_HP modWork;         // Что делает ТН если включен (7 стадий)  0 Пауза 1 Включить отопление  2 Включить охлаждение  3 Включить бойлер   4 Продолжаем греть отопление 5 Продолжаем охлаждение 6 Продолжаем греть бойлер
   TYPE_STATE_HP State;     // Состояние теплового насоса  0 ТН выключен   1 Стартует 2 Останавливается  3 Работает 4 Ожидание ТН (расписание - пустое место) 5 Ошибка ТН  6 - Эта ошибка возникать не должна!
   int8_t ret;              // Точка выхода из алгоритма регулирования
   int8_t prev;             // Предыдущая точка выхода из алгоритма регулирования
}; 
// Структура для хранения различных счетчиков (максимальный размер 128-1 байт!!!!!)
#define fMH_ON    	0       // флаг Включения ТН (пишется внутрь счетчиков flags)

#ifndef TEST_BOARD
	#if I2C_SIZE_EEPROM >= 64
		#define I2C_COUNT_EEPROM_HEADER 0xAA
	#else
		#define I2C_COUNT_EEPROM_HEADER 0xAB
	#endif
#else
	#define I2C_COUNT_EEPROM_HEADER 0xAC
#endif
struct type_motoHour_old
{
	  byte Header;      // признак данных
	  uint8_t  flags;   // флаги
	  uint32_t H1;      // моточасы ТН ВСЕГО  [минуты]
	  uint32_t H2;      // моточасы ТН сбрасываемый счетчик (сезон) [минуты]
	  uint32_t C1;      // моточасы компрессора ВСЕГО [минуты]
	  uint32_t C2;      // моточасы компрессора сбрасываемый счетчик (сезон) [минуты]
	  uint32_t D1;      // Дата сброса общих счетчиков [юникс формат]
	  uint32_t D2;      // дата сброса сезонных счетчиков [юникс формат]
	  uint64_t E1;		// Значение потребленной энергии ВСЕГО [ватт*ч * 1000 или кВт*ч * 1000000]
	  uint64_t E2;		// Значение потребленной энергии в начале сезона [ватт*ч * 1000 или кВт*ч * 1000000]
	  uint64_t P1;      // выработанное тепло  ВСЕГО [ватт*ч * 1000 или кВт*ч * 1000000]
	  uint64_t P2;      // выработанное тепло  сбрасываемый счетчик (сезон) [ватт*ч * 1000 или кВт*ч * 1000000]
};
struct type_motoHour
{
  byte Header;      // признак данных
  uint32_t D1;      // Дата сброса общих счетчиков [юникс формат]
  uint32_t D2;      // дата сброса сезонных счетчиков [юникс формат]
  uint8_t  flags;   // флаги
  uint32_t H1;      // моточасы ТН ВСЕГО  [минуты]
  uint32_t H2;      // моточасы ТН сбрасываемый счетчик (сезон) [минуты]
  uint64_t E1;		// Значение потребленной энергии ВСЕГО [ватт*ч * 1000 или кВт*ч * 1000000]
  uint64_t E2;		// Значение потребленной энергии в начале сезона [ватт*ч * 1000 или кВт*ч * 1000000]
  uint64_t P1;      // выработанное тепло  ВСЕГО [ватт*ч * 1000 или кВт*ч * 1000000]
  uint64_t P2;      // выработанное тепло  сбрасываемый счетчик (сезон) [ватт*ч * 1000 или кВт*ч * 1000000]
  uint32_t C1;      // моточасы компрессора ВСЕГО [минуты]
  uint32_t C2;      // моточасы компрессора сбрасываемый счетчик (сезон) [минуты]
#ifdef USE_HEATER
  uint32_t R1;      // моточасы котла ВСЕГО [минуты]
  uint32_t R2;      // моточасы котла сбрасываемый счетчик (сезон) [минуты]
#endif
};

TEST_MODE testMode;					// Значение режима тестирования

int32_t motohour_OUT_work = 0; 		// рабочий для счетчиков - энергия отопления, Вт
int32_t motohour_IN_work = 0;  		// рабочий для счетчиков - энергия потребленная, Вт
boolean TarifNightNow = false;		// Тариф дневной или ночной (TARIF_NIGHT_START - END)
uint16_t task_updstat_chars = 0;
#ifdef CHART_ONLY_COMP_ON
boolean Charts_when_comp_on = true;	// Графики в памяти только во время работы компрессора
#else
boolean Charts_when_comp_on = false;
#endif
uint8_t Request_LowConsume = 0xFF;
uint8_t Calc_COP_skip_timer = 0;	// Пропустить расчет COP на время *TIME_READ_SENSOR
uint32_t DailySwitch_on = 0;		// Битовый массив включенных дневных реле, b24..31 - выключить HTTP реле
#define DailySwitch_on_MASK_OFF 0xFF000000

#ifdef WATTROUTER
#define  WR_fActive				1				// Ваттроутер включен
#define  WR_fLog				2				// Логирование ваттроутера
#define  WR_fLogFull			3				// Логирование ваттроутера полное
#define  WR_fAverage			4				// Усреднение
#define  WR_fMedianFilter		5				// Медианный фильтр на 3 значения
#define  WR_fPeriod_1sec		6				// Период работы 1 сек, иначе TIME_READ_SENSOR
#define  WR_fLoadMask			((1<<WR_NumLoads)-1) // для WR_Refresh
#define  WR_fTYPE				uint8_t
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

#ifdef WEATHER_FORECAST
uint8_t WF_BoilerTargetPercent = 100;
#endif
#ifdef WR_LOG_DAYS_POWER_EXCESS
int32_t WR_Power_Excess = 0;			// Излишки ваттроутера
#endif
#endif //WATTROUTER
#ifdef WR_PowerMeter_Modbus
bool WR_PowerMeter_New = false;
#else
unsigned long Web0_FreqTime;
#endif

type_WebSecurity WebSec_user;				// хеш паролей
type_WebSecurity WebSec_admin;				// хеш паролей
#ifdef HTTP_MAP_Server
type_WebSecurity WebSec_Microart;			// хеш паролей
#endif

// Рабочие флаги ТН (work_flags)
#define fHP_BoilerTogetherHeat	0			// Идет нагрев бойлера вместе с отоплением
#define fHP_SunNotInited		1			// Солнечный коллектор не инициализирован
#define fHP_SunSwitching		2			// Солнечный коллектор переключается
#define fHP_SunReady			3			// Солнечный коллектор открыт
#define fHP_SunWork 			4			// Солнечный коллектор работает
#define fHP_BackupNoPwrWAIT		5			// Нет 3-х фаз питания - ТН в режиме ожидания, если SGENERATOR == ALARM
#define fHP_HeaterOn			6			// Идет работа котла
#define fHP_HeaterValveOn		7			// Положение крана Котла - ТН: 0 - ТН, 1 - Котел
#define fHP_CompressorWasOn		8			// Последний цикл работы - Компрессор
#define fHP_HeaterWasOn			9			// Последний цикл работы - Котел
#define fHP_ProfileSetByError	10			// Текущий профиль установлен по переключению из-за ошибки
#define fHP_NewCommand			11			// Новая команда(ы) для отработки

// Флаги настроек, Option.flags:
#define fDelayPumpsStopOnError	0				// При ошибке останавливать насосы с задержкой
#define fBeep					1               // флаг Использование звука
#define fNextion				2               // флаг Использование nextion дисплея
#define fHistory				3               // флаг записи истории на карту памяти
#define fSaveON					4               // флаг записи в EEPROM включения ТН
#define f_reserved_2			5               //
#define f1Wire1TSngl			6				// На 1-ой шине 1-Wire только один датчик
#define f1Wire2TSngl			7				// На 2-ой шине 1-Wire(DS2482) только один датчик
#define f1Wire3TSngl			8				// На 3-ей шине 1-Wire(DS2482) только один датчик
#define f1Wire4TSngl			9				// На 4-ей шине 1-Wire(DS2482) только один датчик
#define fSunRegenerateGeo		10				// Использовать солнечный коллектор для регенерации геоконтура в простое
#define fNextionOnWhileWork		11				// Включать дисплей, когда компрессор работает
#define fWebStoreOnSPIFlash		12				// флаг, что веб морда лежит на SPI Flash, иначе на SD карте
#define fLogWirelessSensors		13				// Логировать обмен между беспроводными датчиками
#define fBackupPower			14				// Использование резервного питания от генератора (ограничение мощности)
#define fModbusLogErrors		15              // флаг писать в лог нерегулярные ошибки Modbus
//  type_optionHP.flags2:
#define f2BackupPowerAuto		0               // Автоматически определять работу от генератора (через датчик SGENERATOR)
#define f2NextionGenFlashing	1				// Моргать картинкой на дисплее, если работаем от генератора
#define f2AutoStartGenerator	2				// Автозапуск генератора по специальному гистерезису генератора
#define f2NextionLog			3				// Логировать обмен Nextion
#define f2modWorkLog			4				// Логировать обмен modWork
#define f2RelayLog				5				// Логировать доп. инфу по реле
#define f2LogEnergy				6				// Логировать потребленную энергию со счетчика при переходах тарифного плана и дня

// Структура для хранения опций теплового насоса.
struct type_optionHP
{
 uint8_t  ver;							// номер версии для сохранения
 int8_t   numProf;						// Номер профиля включаемый при загрузке, выбор вручную только через веб "set_listProf", 0..I2C_PROFIL_NUM
 uint16_t flags;						// Флаги опций до 16 флагов
 uint8_t  nStart;						// Число попыток начала/продолжения работы
 uint8_t  sleep;						// Время засыпания дисплея минуты
 uint8_t  dim;							// Яркость дисплея %
 uint8_t  DailySwitchHysteresis;			// Гистерезис для переключения реле по температуре, десятые градуса
 uint16_t tChart;						// период сбора статистики в секундах!!
 uint16_t delayOnPump;					// Задержка включения компрессора/котла после включения насосов (сек).
 uint16_t delayOffPump;					// Задержка выключения насосов отопления при ошибке или при нагреве бойлера, сек
 uint16_t delayStartRes;				// Задержка включения ТН после внезапного сброса контроллера (сек.)
 uint16_t delayRepeadStart;				// Задержка перед повторным включениме ТН при ошибке (попытки пуска) секунды
 uint16_t delayDefrostOn;				// ДЛЯ ВОЗДУШНОГО ТН Задержка после срабатывания датчика перед включением разморозки (секунды)
 uint16_t delayDefrostOff;				// ДЛЯ ВОЗДУШНОГО ТН Задержка перед выключением разморозки (секунды)
 uint16_t delayR4WAY;					// Задержка между переключением 4-х ходового клапана и включением компрессора, для выравнивания давлений (сек). Если включены эти опции (переключение тепло-холод)
 uint16_t delayBoilerSW;                // Пауза (сек) после переключение ГВС - выравниваем температуру в контуре отопления/ГВС что бы сразу защиты не сработали
 uint16_t delayBoilerOff;               // Время (сек) на сколько блокируются защиты при переходе с ГВС на отопление и охлаждение слишком горяче после ГВС
 int16_t  SunRegGeoTemp;				// Солнечный коллектор - Температура начала регенерации геоконтура с помощью СК, в сотых градуса
 uint8_t  SwitchHeaterHPTime;			// Время переключения Котел - ТН, сек
 uint8_t  Modbus_Attempts;				// Modbus - попыток чтения/записи при ошибке (кроме инвертора и счетчика)
 int16_t  SunTDelta;					// Солнечный коллектор - Дельта температур для включения, сотые градуса
 int16_t  SunGTDelta;					// Солнечный коллектор - Дельта температур жидкости для выключения, сотые градуса
 int16_t  SunRegGeoTempGOff;			// Температура жидкости для выключения регенерации
 uint16_t maxBackupPower;		     	// Максимальная мощность при питании от генератора (Вт)
 int16_t  SunTempOn;					// Солнечный коллектор - Температура выше которой открывается СК
 int16_t  SunTempOff;					// Солнечный коллектор - Температура ниже которой закрывается СК
 uint16_t MinCompressorOn;              // Минимальное время работы компрессора в секундах
 uint16_t flags2;						// Флаги #2 до 16 флагов
 uint16_t SunMinWorktime;				// Солнечный коллектор - минимальное время работы, после которого будут проверятся границы, сек
 uint16_t SunMinPause;					// Солнечный коллектор - минимальное время паузы после останова СК, сек
#ifdef DEFROST
 int16_t  DefrostTempLimit;				// температура TEVAIN выше которой разморозка не включается, сотые градуса
 int16_t  DefrostStartDTemp;			// Разница температур TOUT-TEVAIN более которой начнется оттайка, сотые градуса
 int16_t  DefrostTempSteam;				// температура ниже которой оттаиваем паром, сотые градуса
 int16_t  DefrostTempEnd;				// температура окончания отттайки, сотые градуса
#endif
 char     Microart_pass[PASS_LEN+1];	// Пароль для Микроарт Малины
#ifdef WEATHER_FORECAST
 int8_t   WF_MinTemp;					// Минимальная прогнозируемая температура по ощущению для использования прогноза, градусы
 char     WF_ReqServer[24];				// Сервер прогноза погоды по протоколу http
 char     WF_ReqText[128];				// Тело GET запроса
#endif
 uint16_t Generator_Start_Time;			// Время запуска генератора
 uint8_t  ModbusMinTimeBetweenTransaction;
 uint8_t  ModbusResponseTimeout;
 uint8_t  nStartNextProf;				// Число попыток начала/продолжения работы на новом профиле
 uint8_t  Control_Period;				// Период управления тепловым насосом (в режиме Гистерезис и Паузе), сек
};// __attribute__((packed));


//  Работа с отдельными флагами type_DateTimeHP
#define fDT_Update        0                // флаг Обновление часов c сервером раз в сутки и при старте
#define fDT_UpdateI2C     1                // флаг Обновление часов раз в час с I2C  часами
#define fDT_UpdateByHTTP  2                // флаг Обновление по HTTP - спец страница: define HTTP_TIME_REQUEST
// Структура для хранения настроек времени, для удобного сохранения.
struct type_DateTimeHP
{
    uint8_t flags;                        //  Флаги опций до 8 флагов
    int8_t timeZone;                      //  Часовой пояс (не используется)
    char serverNTP[NTP_SERVER_LEN+1];     //  Адрес NTP сервера
    uint32_t saveTime;                    //  дата и время сохранения настроек в eeprom
};

//  Работа с отдельными флагами type_NetworkHP
#define fDHCP         0                  // флаг Использование DHCP
#define fPass         1                  // флаг Использование паролей
#define fInitW5200    2                  // флаг Ежеминутный контроль SPI для сетевого чипа
#define fNoAck        4                  // флаг Не ожидать ответа ACK
#define fNoPing       5                  // флаг Запрет пинга контроллера
#define fWebLogError  6					// Логировать ошибки
#define fWebFullLog   7					// Полный лог

// Структура для хранения сетевых настроек, для удобного сохранения.
struct type_NetworkHP
{
    uint16_t flags;                       // !save! Флаги
    IPAddress ip;                         // !save! ip адрес
    IPAddress sdns;                       // !save! сервер dns
    IPAddress gateway;                    // !save! шлюз
    IPAddress subnet;                     // !save!подсеть
    byte mac[6];                          // !save! mac адрес
    uint16_t resSocket;                   // !save! Время очистки сокетов секунды
    uint32_t resW5200;                    // !save! Время сброса чипа     секунды
    char passUser[PASS_LEN+1];            // !save! Пароль пользователя
    char passAdmin[PASS_LEN+1];           // !save! Пароль администратора
    uint16_t sizePacket;                  // !save! Размер пакета для отправки в байтах
    uint16_t port;                        // !save! порт веб сервера
    uint8_t delayAck;                     // !save! задержка мсек перед отправкой пакета
    char pingAdr[40];                     // !save! адрес для пинга, может быть в любом виде
    uint16_t pingTime;                    // !save! время пинга в секундах
};

// Структура для хранения состояния ТН в момент работы.
struct type_statusHP
{
 uint32_t comp_ON;                        // Время включения компрессора
 uint32_t comp_OFF;                       // Время выключения компрессора
 uint32_t pumpCO_ON;                      // Время включения насоса системы отопления
 uint32_t pumpCO_OFF;                     // Время выключения насоса системы отопления
};

#if defined(R3WAY)
#define BOILER_HEATING_NOW HP.dRelay[R3WAY].get_Relay()
#elif defined(RPUMPBH)
#define IS_BOILER_HEATING (HP.dRelay[RPUMPBH].get_Relay() && !HP.dRelay[PUMP_OUT].get_Relay())
#else
#define IS_BOILER_HEATING false
#endif

// Признак запуска задачи насос:
#define StartPump_Stop		0	// останов задачи
#define StartPump_Start		1	// запуск
#define StartPump_Work_Off	2	// в работе (выкл)
#define StartPump_Work_On	3	// в работе (вкл)
#define StartPump_AfterWork	4	// отработка после останова компрессора/котла (вкл)
const char *StartPump_STR[] = { "Stop", "Start", "Work Off", "Work On", "Wait Off" };

#define HeaterNeedOn (((Status.modWork & pHEAT) && GETBIT(Prof.SaveON.flags, fHeat_UseHeater)) || ((Status.modWork & pBOILER) && GETBIT(Prof.SaveON.flags, fBoiler_UseHeater)))

// ------------------------- ОСНОВНОЙ КЛАСС --------------------------------------
class HeatPump
{
public:
	void initHeatPump();                                     // Конструктор
// Информационные функции определяющие состояние ТН
	__attribute__((always_inline)) inline MODE_HP get_modWork()     {return Status.modWork;}  // (переменная) Получить что делает сейчас ТН
	__attribute__((always_inline)) inline TYPE_STATE_HP get_State() {return Status.State;}    // (переменная) Получить состяние теплового насоса [0-Выключен 1 Стартует 2 Останавливается  3 Работает 4 Ожидание ТН (расписание - пустое место) 5 Ошибка ТН 6 - Эта ошибка возникать не должна!]
	__attribute__((always_inline)) inline int8_t get_ret()          {return Status.ret;}      // (переменная) Точка выхода из алгоритма регулирования (причина (условие) нахождения в текущем положении modWork)

	__attribute__((always_inline)) inline  MODE_HP get_modeHouse()   {return Prof.SaveON.mode;}// (настройка) Получить режим работы ДОМА (охлаждение/отопление/выключено) ЭТО НАСТРОЙКА через веб морду!
	//inline  type_settingHP *get_modeHouseSettings() {return Prof.SaveON.mode == pCOOL ? &Prof.Cool : &Prof.Heat; } // Настройки для режима отопление или охлаждение
	void set_mode(MODE_HP b) {Prof.SaveON.mode=b;}           // Установить режим работы отопления
	void set_nextMode();                                     // Переключение на следующий режим работы отопления (последовательный перебор режимов)

	#ifdef R4WAY
	__attribute__((always_inline)) inline  boolean is_HP_Heating() { return !dRelay[R4WAY].get_Relay(); } 	// true = Режим нагрева отопления или бойлера, false = охлаждение
	#else
	__attribute__((always_inline)) inline  boolean is_HP_Heating() { return true; } 	// true = Режим нагрева отопления или бойлера, false = охлаждение
	#endif
	boolean IsWorkingNow() { return !(get_State() == pOFF_HP && PauseStart == 0); }				// Включен ТН или нет
	boolean CheckAvailableWork();							// проверить есть ли работа для ТН

	void vUpdate();                                          // Итерация по управлению всем ТН - старт-стоп
	void calculatePower();                                   // Вычисление мощностей контуров и КОП
	void eraseError();                                       // стереть последнюю ошибку
	void process_error(void);

	__attribute__((always_inline)) inline int8_t get_errcode(){return error;} // Получить код последней ошибки
	char    *get_lastErr(){return note_error;}// Получить описание последней ошибки, которая вызвала останов ТН, при удачном запуске обнуляется
	void     scan_OneWire(char *result_str);  // Сканирование шины OneWire на предмет датчиков
	boolean  get_onBoiler(){return onBoiler;} // Получить состояние трехходового точнее если true то идет нагрев бойлера
	uint8_t  get_fSD() { return fSD;}         // Получить флаг наличия РАБОТАЮЩЕЙ СД карты
	void     set_fSD(uint8_t f) { fSD=f; }    // Установить флаг наличия РАБОТАЮЩЕЙ СД карты
	uint8_t  get_fSPIFlash() { return fSPIFlash;}    // Получить флаг наличия РАБОТАЮЩЕГО флеш диска
	void     set_fSPIFlash(uint8_t f) {fSPIFlash=f;} // Установить флаг наличия РАБОТАЮЩЕГО флеш диска
	TYPE_SOURSE_WEB get_SourceWeb();                 // Получить источник загрузки веб морды
	uint32_t get_errorReadDS18B20();    // Получить число ошибок чтения датчиков температуры
	void     Reset_TempErrors();		// Сбросить счетчик ошибок всех датчиков
	void     resetPID();				// Инициализировать переменные ПИД регулятора

	void     sendCommand(TYPE_COMMAND c);// Послать команду на управление ТН
	__attribute__((always_inline)) inline TYPE_COMMAND isCommand()  {return command;}  // Получить текущую команду выполняемую ТН
	void     runCommand(void);              // Выполнить команду по управлению ТН
	char *get_command_name(TYPE_COMMAND c) { return (char*)hp_commands_names[c < pCOMAND_END ? c : pCOMAND_END]; }
	boolean is_next_command_stop() { return next_command == pSTOP || next_command == pREPEAT; }
	uint8_t is_pause();					// Возвращает 1, если ТН в паузе
	inline boolean is_compressor_on() { return dRelay[RCOMP].get_Relay() || dFC.isfOnOff(); } // Компрессор работает?
	inline boolean is_heater_on() { return GETBIT(work_flags, fHP_HeaterOn); } // Котел работает?
	inline boolean is_comp_or_heater_on() { return GETBIT(work_flags, fHP_HeaterOn) || dRelay[RCOMP].get_Relay() || dFC.isfOnOff(); }// Проверка работает ли котел или компрессор
	void 	relayAllOFF();              // Все реле выключить
	void	HandleNoPower(void);		// Обработать пропадание питания

// Строковые функции
	char *StateToStr();                 // Получить состояние ТН в виде строки
	char *StateToStrEN();               // Получить состояние ТН в виде английской строки
	void get_StateModworkStr(char *strReturn);
	char *TestToStr();                  // Получить режим тестирования
	int8_t save_DumpJournal(boolean f); // Записать состояние теплового насоса в журнал

	int32_t save(void); 		        // Записать настройки в eeprom i2c на входе адрес с какого, на выходе код ошибки (меньше нуля) или количество записанных  байт
	int32_t load(uint8_t *buffer, uint8_t from_RAM); // Считать настройки из i2c или RAM, на выходе код ошибки (меньше нуля)
	int8_t loadFromBuf(int32_t adr,byte *buf);// Считать настройки из буфера на входе адрес с какого, на выходе код ошибки (меньше нуля)
	int8_t save_motoHour();             // запись счетчиков теплового насоса в ЕЕПРОМ
	int8_t load_motoHour();             // чтение счетчиков теплового насоса в ЕЕПРОМ

//  ===================  К Л А С С Ы  ===========================
// Датчики
	sensorTemp sTemp[TNUMBER];          // Датчики температуры

#ifdef SENSOR_IP                    // Получение данных удаленного датчика
	sensorIP sIP[IPNUMBER];           // Массив удаленных датчиков
#endif
	sensorADC sADC[ANUMBER];            // Датчик аналоговый
	sensorDiditalInput sInput[INUMBER]; // Контактные датчики
	sensorFrequency sFrequency[FNUMBER]; // Частотные датчики

// Устройства  исполнительные
	devRelay dRelay[RNUMBER];           // Реле
#ifdef EEV_DEF
	devEEV dEEV;                        // ЭРВ
#endif
#ifdef FC_VACON
	devVaconFC dFC;						// Частотник VACON
#else
	devOmronMX2 dFC;					// Частотник OMRON
#endif
#ifdef USE_ELECTROMETER_SDM
	devSDM dSDM;						// Счетчик
#endif
#ifdef USE_HEATER
	devHeater dHeater;					// Котел
#endif

// Сервис
	Message message;                     // Класс уведомления
	Profile Prof;                        // Текуший профиль
	Scheduler Schdlr;					// Расписание

	#ifdef MQTT
	  clientMQTT clMQTT;                // MQTT клиент
	#endif
// Сетевые настройки
	boolean set_network(char *var, char *c);        // Установить параметр из строки
	char*   get_network(char *var,char *ret);       // Получить параметр из строки
	//  inline uint16_t get_sizePacket() {return Network.sizePacket;} // Получить размер пакета при передаче
	inline uint16_t get_sizePacket() {return 2048;} // Получить размер пакета при передаче

// Дата время
	boolean set_datetime(char *var, char *c);              //  Установить параметр дата и время из строки
	void    get_datetime(char *var,char *ret);             //  Получить параметр дата и время из строки
	IPAddress get_ip() { return Network.ip;}               //  Получить ip адрес
	IPAddress get_sdns() { return Network.sdns;}           //  Получить sdns адрес
	IPAddress get_subnet() { return Network.subnet;}       //  Получить subnet адрес
	IPAddress get_gateway() { return Network.gateway;}     //  Получить gateway адрес
	void set_ip(IPAddress ip) {Network.ip=ip;}             //  Установить ip адрес
	void set_sdns(IPAddress sdns) {Network.sdns=sdns;}     //  Установит sdns адрес
	void set_subnet(IPAddress subnet) {Network.subnet=subnet;}  //  Установит subnet адрес
	void set_gateway(IPAddress gateway) {Network.gateway=gateway;}//  Установит gateway адрес
	uint16_t get_port() {return Network.port;}             //  получить порт вебсервера
	__attribute__((always_inline)) inline uint16_t get_NetworkFlags() { return Network.flags; }
	boolean get_NoAck() { return GETBIT(Network.flags,fNoAck);}  //  Получить флаг Не ожидать ответа ACK
	uint8_t get_delayAck() {return Network.delayAck;}      //  получить задержку перед отсылкой следующего пакета
	uint16_t get_pingTime() {return Network.pingTime;}     //  получить вермя пингования сервера, 0 если не надо
	char *  get_pingAdr() {return Network.pingAdr;}         //  получить адрес сервера для пингования
	boolean get_NoPing() { return GETBIT(Network.flags,fNoPing);} //  Получить флаг блокировки пинга
	char *  get_netMAC() {return MAC2String(Network.mac);}  //  получить мас адрес контроллера
	boolean get_DHCP() { return GETBIT(Network.flags,fDHCP);}    //  Получить использование DHCP
	byte *get_mac() { return Network.mac;}                 //  Получить mac адрес
	uint32_t socketRes() {return countResSocket;}          //  Получить число сбросов сокетов
	void add_socketRes() {countResSocket++;}               //  Добавить 1 к счетчику число сбросов сокетов
	uint32_t time_socketRes() {return Network.resSocket;}  //  Получить период сбросов сокетов
	uint32_t time_resW5200() {return Network.resW5200;}    //  Получить период сбросов W5200
	boolean get_fPass() { return GETBIT(Network.flags,fPass);}   //  Получить флаг необходимости идентификации
	boolean get_fInitW5200() { return GETBIT(Network.flags,fInitW5200);}  //  Получить флаг Контроля w5200
	inline  char* get_passUser() { return Network.passUser; }
	inline  char* get_passAdmin() { return Network.passAdmin; }

// Параметры ТН
	boolean set_optionHP(char *var, float x);                // Установить опции ТН из числа (float)
	char*   get_optionHP(char *var, char *ret);              // Получить опции ТН
	uint16_t get_delayRepeadStart(){return Option.delayRepeadStart;} // Получить время между повторными попытками старта
	void SwitchToProfile(uint8_t _profile);					// Переключиться на другой профиль

	RULE_HP get_ruleCool(){return Prof.Cool.Rule;}           // Получить алгоритм охлаждения
	RULE_HP get_ruleHeat(){return Prof.Heat.Rule;}           // Получить алгоритм отопления
	boolean get_TargetCool(){return GETBIT(Prof.Cool.flags,fTarget);}  // Получить цель 0 - Дом 1 - Обратка
	boolean get_TargetHeat(){return GETBIT(Prof.Heat.flags,fTarget);}  // Получить цель 0 - Дом 1 - Обратка
	uint16_t get_timeCool(){return Prof.Cool.pid_time;}      // Получить время интегрирования охлаждения
	uint16_t get_timeHeat(){return Prof.Heat.pid_time;}      // Получить время интегрирования отопления
	uint16_t get_timeBoiler(){return Prof.Boiler.pid_time;}  // Получить время интегрирования ГВС

	int16_t get_targetTempCool();                           // Получить целевую температуру Охлаждения
	int16_t get_targetTempHeat();                           // Получить целевую температуру Отопления
	void    getTargetTempStr(char *rstr);					// Целевая температура в строку
	void    getTargetTempStr2(char *rstr);					// Целевая температура в строку, 2 знака после запятой
	int16_t setTargetTemp(int16_t dt);                      // ИЗМЕНИТЬ целевую температуру
	int16_t setTempTargetBoiler(int16_t dt);                // ИЗМЕНИТЬ целевую температуру бойлера
	int16_t CalcTargetPID_Heat();							// Рассчитать подачу для PID нагрева
	int16_t CalcTargetPID_Cool();							// Рассчитать подачу для PID охлаждения
	boolean scheduleBoiler();                               // Проверить расписание бойлера true - нужно греть false - греть не надо

// Опции ТН
	uint16_t get_pausePump() { return Status.modWork & pCOOL ? Prof.Cool.pausePump : Prof.Heat.pausePump; }// Время паузы  насоса при выключенном компрессоре, секунды
	uint16_t get_workPump() { return Status.modWork & pCOOL ? Prof.Cool.workPump : Prof.Heat.workPump; }  // Время работы  насоса при выключенном компрессоре, секунды
	void     pump_in_pause_set(bool ONOFF);					// Переключение насосов при выключенном компрессоре
	void     pump_in_pause_wait_off();                      // ждем пока насосы остановятся
	uint8_t  get_Beep() {return GETBIT(Option.flags,fBeep);}    // подача звуковых сигналов
	uint8_t  get_SaveON() {return GETBIT(Option.flags,fSaveON);}// получить флаг записи состояния
	uint8_t  get_WebStoreOnSPIFlash() {return GETBIT(Option.flags,fWebStoreOnSPIFlash);}// получить флаг хранения веб морды на флеш диске
	boolean  set_WebStoreOnSPIFlash(boolean f) {if(f)SETBIT1(Option.flags,fWebStoreOnSPIFlash);else SETBIT0(Option.flags,fWebStoreOnSPIFlash);return GETBIT(Option.flags,fWebStoreOnSPIFlash);}// установить флаг хранения веб морды на флеш диске
	uint16_t get_maxBackupPower() {return Option.maxBackupPower;};      // Максимальная мощность при питании от генератора (Вт)
	uint8_t  get_BackupPower() {return GETBIT(Option.flags,fBackupPower);}// получить флаг использования генератора (ограничение мощности)
	boolean  set_BackupPower(boolean f) {if(f)SETBIT1(Option.flags,fBackupPower);else SETBIT0(Option.flags,fBackupPower);return GETBIT(Option.flags,fBackupPower);}// установить флаг использования генератора (ограничение мощности)
	#ifdef SGENERATOR
	void     check_fBackupPower(void) { Option.flags = (Option.flags & ~(1<<fBackupPower)) | ((sInput[SGENERATOR].get_Input() == sInput[SGENERATOR].get_alarmInput())<<fBackupPower); };
	#endif

	uint8_t  get_nStart() {return Option.nStart;};                      // получить максимальное число попыток пуска ТН
	uint8_t  get_sleep() {return Option.sleep;}                         //
	uint16_t get_flags() { return Option.flags; }					  // Все флаги
	void     updateNextion(bool need_init);                             // Обновить настройки дисплея

	void set_HP_error_state() { Status.State = pERROR_HP; }
	inline void  set_HP_OFF(){SETBIT0(motoHour.flags,fMH_ON);Status.State=pOFF_HP;}// Сброс флага включения ТН
	inline void  set_HP_ON(){SETBIT1(motoHour.flags,fMH_ON);Status.State=pWORK_HP;}// Установка флага включения ТН
	inline bool  get_HP_ON() {return GETBIT(motoHour.flags,fMH_ON);}           // Получить значение флага включения ТН

	void     set_BoilerOFF(){SETBIT0(Prof.SaveON.flags,fBoilerON);}          // Выключить бойлер
	void     set_BoilerON() {SETBIT1(Prof.SaveON.flags,fBoilerON);}          // Включить бойлер
	bool     get_BoilerON() {return GETBIT(Prof.SaveON.flags,fBoilerON);}    // Получить флаг включения бойлера

// Бойлер ТН
	int16_t get_boilerTempTarget();					          // Получить целевую температуру бойлера с учетом корректировки
	__attribute__((always_inline)) inline int16_t Boiler_Target_AddHeating() { return Prof.Boiler.tempRBOILER - (onBoiler || HeatBoilerUrgently ? 0 : Prof.Boiler.dAddHeat); }
	boolean get_Circulation(){return GETBIT(Prof.Boiler.flags,fCirculation);} // Нужно ли управлять циркуляционным насосом болйлера
	uint16_t get_CirculWork(){ return Prof.Boiler.Circul_Work; }            // Время  работы насоса ГВС секунды (fCirculation)
	uint16_t get_CirculPause(){ return Prof.Boiler.Circul_Pause;}           // Пауза в работе насоса ГВС  секунды (fCirculation)

// Времена
	void set_countNTP(uint32_t b) {countNTP=b;}             // Установить текущее время обновления по NTP, (секундах)
	uint32_t get_countNTP() {return countNTP;}              // Получить время последнего обновления по NTP (секундах)
	boolean get_updateNTP() { return GETBIT(DateTime.flags,fDT_Update); }// Получить флаг возможности синхронизации по NTP
	unsigned long get_saveTime(){return  DateTime.saveTime;}// Получить время сохранения текущих настроек
	char* get_serverNTP() {return DateTime.serverNTP;}      // Получить адрес сервера
	void updateDateTime(int32_t  dTime);                    // После любого изменения часов необходимо пересчитать все времна которые используются
	boolean  get_updateI2C(){return GETBIT(DateTime.flags,fDT_UpdateI2C);}// Получить необходимость обновления часов I2C
	inline bool get_UpdateByHTTP() { return GETBIT(DateTime.flags, fDT_UpdateByHTTP); }
	unsigned long timeNTP;                                  // Время обновления по NTP в тиках (0-сразу обновляемся)

	__attribute__((always_inline)) inline uint32_t get_uptime() {return rtcSAM3X8.unixtime()-timeON;} // Получить время с последенй перезагрузки в секундах
	uint32_t get_startDT(){return timeON;}                  // Получить дату и время последеней перезагрузки
	inline uint32_t get_startCompressor(){return startCompressor;} // Получить дату и время пуска компрессора! нужно для старта ЭРВ
	inline uint32_t get_stopCompressor(){return stopCompressor;} // Получить дату и время останова компрессора!
	inline void set_startCompressor() { startCompressor = rtcSAM3X8.unixtime(); SETBIT0(work_flags, fHP_HeaterWasOn); }
	inline void set_stopCompressor() { stopCompressor = rtcSAM3X8.unixtime(); SETBIT0(work_flags, fHP_HeaterWasOn); SETBIT1(work_flags, fHP_CompressorWasOn); }
	inline uint32_t get_command_completed(){ return command_completed; } // Время выполнения команды
	inline type_motoHour *get_motoHour(){ return &motoHour; }// Получить счетчики

	void resetCount(boolean full);                          // Сборос сезонного счетчика моточасов
	void updateCount();                                     // Обновление счетчиков моточасов

	void set_uptime(unsigned long ttime){timeON=ttime;}     // Установить текущее время как начало старта контроллера

// Переменные
	uint8_t CPU_LOAD;                                      // загрузка CPU
	uint32_t mRTOS;                                        // Память занимаемая задачами
	uint32_t startRAM;                                     // Свободная память при старте FREE Rtos - пытаемся определить свободную память при работе

	int16_t  lastEEV;                                      // + значение шагов ЭРВ перед выключением  -1 - первое включение
	uint8_t num_repeat;                                   // + текущее число повторов пуска ТН
	uint8_t num_repeat_prof;
	uint16_t num_resW5200;                                 // + текущее число сброса сетевого чипа
	uint16_t num_resMutexWEB;                              // + текущее число сброса митекса WEB
	uint16_t num_resMutexI2C;                              // + текущее число сброса митекса I2C
	uint16_t num_resMQTT;                                  // + число повторных инициализация MQTT клиента
	uint16_t num_resPing;                                  // + число не прошедших пингов

	uint16_t AdcVcc;                                       // напряжение питания
//	uint16_t AdcTempSAM3x;                                 // температура чипа

	uint8_t PauseStart;                                    // 1/2 - ТН в отложенном запуске, 0 - нет, начать отсчет времени с начала при отложенном старте

	uint8_t startPump;                                     // Признак запуска задачи насос - StartPump_*
	boolean safeNetwork;                                   // Режим работы safeNetwork (сеть по умолчанию, паролей нет)


// Графики в памяти
	uint16_t get_tChart(){return Option.tChart;}           // Получить время накопления графиков в секундах
	void updateChart();                                    // обновить графики в памяти
	void clearChart();                                     // Очистить графики в памяти
	void get_listChart(char* ret, const char *delimiter);	       // получить список доступных графиков
	void get_Chart(int index, char *str);                  // получить данные графика
	statChart Charts[sizeof(ChartsModSetup) / sizeof(ChartsModSetup[0]) + sizeof(ChartsConstSetup) / sizeof(ChartsConstSetup[0])];
	uint8_t Chart_PressTemp_PCON;
	uint8_t Chart_Temp_TCONOUT;
	uint8_t Chart_Temp_TCOMP;
	uint8_t Chart_Temp_TCONOUTG;
	uint8_t Chart_Temp_TCONING;
	uint8_t Chart_Temp_TEVAING;
	uint8_t Chart_Temp_TEVAOUTG;
	uint8_t Chart_Flow_FLOWCON;
	uint8_t Chart_Flow_FLOWEVA;

	int32_t powerOUT;									// Мощность выходная, кроме реле бойлера, Вт
	int32_t powerGEO;									// Мощность системы GEO, Вт
	int32_t power220;									// Мощность системы 220, кроме тэна бойлера, Вт
	int32_t power_RBOILER;								// Мощность нагрева бойлера тэном, Вт
	int32_t power_BOILER;								// Мощность нагрева бойлера, итого, включая тэн, Вт
	int32_t fullCOP;									// Полный СОР, сотые

// Удаленные датчики
	#ifdef SENSOR_IP
	boolean updateLinkIP();                    // Обновить ВСЕ привязки удаленных датчиков
	#endif


	TaskHandle_t xHandleUpdate;                         // Заголовок задачи "Обновление ТН"
	#ifdef EEV_DEF
	TaskHandle_t xHandleUpdateEEV;                      // Заголовок задачи "Обновление ЭРВ"
	#endif
	TaskHandle_t xHandleReadSensor;                     // Заголовок задачи "Чтение датчиков"
	TaskHandle_t xHandleSericeHP;						// Задача обслуживания ТН:
	TaskHandle_t xHandleUpdateWeb0;                     // Заголовок задачи "Веб сервер" в зависимости от потоков
	#if    W5200_THREAD > 1
	TaskHandle_t xHandleUpdateWeb1;                     // Заголовок задачи "Веб сервер"
	#endif
	#if    W5200_THREAD > 2
	TaskHandle_t xHandleUpdateWeb2;                     // Заголовок задачи "Веб сервер"
	#endif
	#if    W5200_THREAD > 3
	TaskHandle_t xHandleUpdateWeb3;                     // Заголовок задачи "Веб сервер"
	#endif
	#ifdef LCD2004
	TaskHandle_t xHandleKeysLCD;
	#endif

	type_SEMAPHORE xCommandSemaphore;    // Семафор команды
	boolean Task_vUpdate_run;				// задача vUpdate работает
	void SetTask_vUpdate(bool onoff);		// Пуск/Останов задачи vUpdate

	void Pumps(boolean b);					// Включение/выключение насосов
	void Pump_HeatFloor(boolean On);		// Включить/выключить насос ТП
	void Sun_ON(void);						// Включить СК
	void Sun_OFF(void);						// Выключить СК
	uint16_t work_flags;					// Рабочие флаги ТН

	int16_t get_temp_condensing(void);	    // Расчитать температуру конденсации
	int16_t get_temp_evaporating(void);		// Получить температуру кипения
	int16_t get_overcool(void);			    // Расчитать переохлаждение
	int8_t	Prepare_Temp(uint8_t bus);		// Запуск преобразования температуры
// Настройки опций
	type_optionHP Option;                  // Опции теплового насоса

	uint16_t pump_in_pause_timer;			// sec
	uint32_t time_Sun;                    // тики солнечного коллектора
	uint8_t  NO_Power;					  // Нет питания основных узлов, 2 - нужно запустить после восстановления
	uint8_t  NO_Power_delay;
	uint16_t fBackupPowerOffDelay;			// задержка выключения флага работы от резервного питания
	boolean  HeatBoilerUrgently;		  // Срочно нужно ГВС
	void     set_HeatBoilerUrgently(boolean onoff);
#ifdef RHEAT
	uint16_t RHEAT_timer = 0;
	int16_t  RHEAT_prev_temp = STARTTEMP;
#endif
	uint32_t startCompressor;             // время пуска компрессора (для обеспечения минимального времени работы)
	uint32_t stopCompressor;              // время останова компрессора (для опеспечения паузы)
	uint32_t startHeater;                 // время включения котла
	uint32_t stopHeater;                  // время выключения котла

private:
	void    StartResume(boolean start);    // Функция Запуска/Продолжения работы ТН - возвращает ок или код ошибки
	void    StopWait(boolean stop);        // Функция Останова/Ожидания ТН  - возвращает код ошибки
	int8_t  ResetFC();                     // Сброс инвертора, если он стоит в ошибке, проверяется наличие инвертора и проверка ошибки
	int16_t getPower(void);               // Получить мощность потребления ТН (нужно при ограничении мощности при питании от резерва)

	MODE_HP get_Work();                   // Что надо делать
	boolean configHP();                   // Концигурация 4-х, 3-х ходового крана и включение насосов, выход - разрешение запуска компрессора
	boolean Switch_R4WAY(boolean fCool);  // Переключение реверсивного 4-х ходового клапана (true - охлаждение, false - нагрев), возврат true если переключили
	void    defrost();                    // Все что касается разморозки воздушника

	void resetSettingHP();                // Функция сброса настроек охлаждения и отопления
	boolean boilerAddHeat();              // Проверка на необходимость греть бойлер дополнительным теном (true - надо греть)
	boolean switchBoiler(boolean b);      // Переключение на нагрев бойлера ТН true-бойлер false-отопление/охлаждение
	boolean checkEVI();                   // Проверка и если надо включение EVI если надо то выключение возвращает состояние реле
	MODE_COMP UpdateHeat();               // Итерация нагрев  вход true - делаем, false - ТОЛЬКО проверяем выход что сделано или надо сделать
	MODE_COMP UpdateCool();               // Итерация охлаждение вход true - делаем, false - ТОЛЬКО проверяем выход что сделано или надо сделать
	MODE_COMP UpdateBoiler();             // Итерация управления бойлером возвращает что делать компрессору
	void compressorON();                  // попытка включить компрессор с учетом всех защит
	void compressorOFF();                 // попытка выключить компрессор с учетом всех защит
	void heaterON();                      // попытка включить котел с учетом всех защит
	void heaterOFF();                     // попытка выключить котел с учетом всех защит
	boolean check_start_pause();          // проверка на паузу между включениями
	int8_t check_crc16_eeprom(int32_t addr, uint16_t size);// Проверить контрольную сумму в EEPROM для данных на выходе ошибка, длина определяется из заголовка
	boolean setState(TYPE_STATE_HP st);   // установить состояние теплового насоса

	type_motoHour motoHour;               // Структура для хранения счетчиков запись каждый час
	type_motoHour motoHour_saved;
	TYPE_COMMAND command;                 // Текущая команда управления ТН
	TYPE_COMMAND next_command;            // Следующая команда управления ТН
	type_status Status;                   // Описание состояния ТН

// Ошибки и описания
	int8_t error;                         // Код ошибки
	char   note_error[160+1];             // Строка c описанием ошибки формат "время источник:описание"
	uint8_t fSD;                          // Признак наличия SD карты: 0 - нет, 1 - есть, но пустая, 2 - есть, веб в наличии
	uint8_t fSPIFlash;                    // Признак наличия (физического) SPI флеш: 0 - нет, 1 - есть, но пустая, 2 - есть, веб в наличии
	boolean startWait;                    // Начало работы с ожидания

// Различные времена
	type_DateTimeHP DateTime;             // структура где хранится все что касается времени и даты
	uint32_t timeON;                      // время включения контроллера для вычисления UPTIME
	uint32_t countNTP;                    // число секунд с последнего обновления по NTP
	uint32_t offBoiler;                   // время выключения нагрева ГВС ТН (необходимо для переключения на другие режимы на ходу)
	uint32_t startDefrost;                // время срабатывания датчика разморозки
	uint32_t startLegionella;             // время начала обеззараживания
	uint32_t command_completed;			  // Время отработки команды
	boolean  compressor_in_pause;         // Компрессор в паузе
	boolean  heater_in_pause;             // Котел в паузе

// Сетевые настройки
	type_NetworkHP Network;                 // Структура для хранения сетевых настроек
	uint32_t countResSocket;                // Число сбросов сокетов

// Переменные ПИД регулятора Отопление
	PID_WORK_STRUCT pidw;
	unsigned long updatePidTime;          // время обновления ПИДа отопления

// Переменные ПИД регулятора ГВС
	unsigned long updatePidBoiler;        // время обновления ПИДа ГВС
	boolean flagRBOILER;                  // true - идет или скоро может быть пойдет цикл догрева бойлера
	boolean onBoiler;                     // Если true то идет нагрев бойлера ТН (не ТЭНом)
	boolean onLegionella;                 // Если true то идет Обеззараживание

	friend void set_Error(int8_t err, char *nam );// Установка критической ошибки для класса ТН
};

#endif
