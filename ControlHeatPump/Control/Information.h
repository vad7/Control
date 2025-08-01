/*
 * Copyright (c) 2016-2021 by Vadim Kulakov vad7@yahoo.com, vad711
 * &                       by Pavel Panfilov <firstlast2007@gmail.com> pav2000
 * "Народный контроллер" для тепловых насосов.
 * Данное програмное обеспечение предназначено для управления
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
//  Описание вспомогательных лассов данных, предназначенных для получения информации о ТН
#ifndef Information_h
#define Information_h
#include "Constant.h"                       // Вся конфигурация и константы проекта Должен быть первым !!!!
#include "utility/w5100.h"
#include "utility/socket.h"

// --------------------------------------------------------------------------------------------------------------- 
//  Класс системный журнал пишет в консоль и в память ------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------- 
//  Необходим для получения лога контроллера
//  http://arduino.ru/forum/programmirovanie/etyudy-dlya-nachinayushchikh-interfeis-printable
//  Для записи ТОЛЬКО в консоль использовать функции printf
//  Для записи в консоль И в память (журнал) использовать jprintf
//  По умолчанию журнал пишется в RAM размер JOURNAL_LEN
//  Если включена опция I2C_EEPROM_64KB то журнал пишется в I2C память. Должен быть устанвлен чип размером более 4кБ, адрес начала записи 0x0fff до I2C_STAT_EEPROM
//  Размер журнала I2C_STAT_EEPROM-I2C_JOURNAL_START

#define PRINTF_BUF 256                           // размер буфера для одной строки - большаяя длина нужна при отправке уведомлений, там длинные строки (видел 178)

extern uint16_t sendPacketRTOS(uint8_t thread, const uint8_t * buf, uint16_t len,uint16_t pause);
const char *errorReadI2C =    {"$ERROR - read I2C memory\n"};
const char *errorWriteI2C =   {"$ERROR - write I2C memory\n"};
const char *promtUser={"> "};   

// для прекращения логирования
enum {
	fLog_HTTP_RelayError			= 0,			// Ошибка Send_HTTP_Request
	fLog_DNS_Lookup
};
uint8_t Logflags = 0;								// fLog_*

#define I2C_JOURNAL_HEAD   		0x01 					// Признак головы журнала
#define I2C_JOURNAL_TAIL   		0x02					// Признак хвоста журнала
#define I2C_JOURNAL_FORMAT 		0xFF					// Символ которым заполняется журнал при форматировании
#define I2C_JOURNAL_READY  		0x55AA					// Признак создания журнала - если его нет по адресу I2C_JOURNAL_START-2 то надо форматировать журнал (первичная инициализация)

class Journal :public Print
{
public:
  void Init();                                            // Инициализация
  void printf(const char *format, ...) ;                  // Печать только в консоль
  void jprintf(const char *format, ...);                  // Печать в консоль и журнал возвращает число записанных байт
  void jprintf_only(const char *format, ...);             // Печать ТОЛЬКО в журнал возвращает число записанных байт для использования в критических секциях кода
  void jprintf_time(const char *format, ...);			// +Time, далее печать в консоль и журнал
  void jprintf_date(const char *format, ...);			// +DateTime, далее печать в консоль и журнал
  int32_t send_Data(uint8_t thread);                     // отдать журнал в сеть клиенту  Возвращает число записанных байт
  int32_t available(void);                               // Возвращает размер журнала
  int8_t   get_err(void) { return err; };
  bool check_wait_semaphore(void);
  virtual size_t write (uint8_t c);                       // чтобы print работал для это класса
  #ifdef I2C_EEPROM_64KB                                  // Если журнал находится в i2c
  void Format(void);                           		    // форматирование журнала в еепром
  #else
  void Clear(){bufferTail=0;bufferHead=0;full=false;err=OK;} // очистка журнала в памяти
  #endif
  type_SEMAPHORE Semaphore;                    			// Семафор
private:
  int8_t err;                                             // ошибка журнала
  int32_t bufferHead, bufferTail;                        // Начало и конец
  uint8_t full;                                           // признак полного буфера
  void _write(char *dataPtr);                            // Записать строку в журнал
   // Переменные
  char pbuf[PRINTF_BUF+2];                                // Буфер для одной строки + маркеры
  #ifndef I2C_EEPROM_64KB                                 // Если журнал находится в памяти
    char _data[JOURNAL_LEN+1];                            // Буфер журнала
  #else
    void writeTAIL();                                     // Записать символ "конец" значение bufferTail должно быть установлено
    void writeHEAD();                                     // Записать символ "начало" значение bufferHead должно быть установлено
    void writeREADY();                                    // Записать признак "форматирования" журнала - журналом можно пользоваться
    boolean checkREADY();                                 // Проверить наличие журнала
  #endif
 };

// ---------------------------------------------------------------------------------
//  Класс Профиль ТН    ------------------------------------------------------------
// ---------------------------------------------------------------------------------
// Класс предназначен для работы с настройками ТН. в пямяти хранится текущий профиль, ав еепром можно хранить до 10 профилей, и загружать их по одному в пямять
// также есть функции по сохраннию и удалению из еепром
// номер текущего профиля хранится в структуре type_SaveON
#define PROFILE_MAGIC 0xAA     // Признак начала профиля
#define PROFILE_EMPTY 0x55     // Признак пустого профиля

// --------- внутренние структуры --------------
// Структура для хранения статуса работы ТН EEPROM  - часть профиля!!!
//  Определение флагов в  type_SaveON
#define fBoilerON 				1		// флаг Включения бойлера, true - работает нагрев бойлера
#define fAutoSwitchProf_mode	2		// флаг автопереключения Отопление/Охлаждение по температуре, если цель температура дома
#define fHeat_UseHeater			3		// флаг Использовать Котел для отопления
#define fBoiler_UseHeater		4		// флаг Использовать Котел для нагрева бойлера
struct type_SaveON {
	byte magic;							// признак данных, должно быть  0x55
	uint8_t flags;						// флаги состояния ТН
	MODE_HP mode;						// текущий и сохраненный режим работы отопление|охлаждение
	uint8_t WR_Loads;					// Биты активирования нагрузки ваттроутера
	uint32_t reserved;					//
	uint32_t bTIN;						// Разрешения для датчиков температуры участвовать в расчете температуры дома (TIN), максимально кол-во датчиков = 32!
};

// Структуры для хранения настроек бойлера
//  Определение флагов в type_boilerHP: Boiler.flags
#define fScheduleAddHeat 	0           // флаг Расписание только для ТЭНа
#define fSchedule        	1           // флаг Использование расписания
#define fLegionella      	2           // флаг легионелла раз внеделю греть бойлер
#define fCirculation     	3           // флаг Управления циркуляционным насосом ГВС
#define fResetHeat       	4           // флаг Сброса лишнего тепла в СО
#define fTurboBoiler     	5           // флаг Ускоренный нагрев бойлера (ТН + ТЭН)
#define fAddHeating      	6           // флаг ДОГРЕВА ГВС ТЭНом
#define fBoilerTogetherHeat	7			// флаг одновременного нагрева бойлера с отоплением до температуры догрева
#define fBoiler_reserved	8			// резерв
#define fBoilerUseSun		9		  	// флаг использования солнечного коллектора
#define fAddHeatingForce	10			// флаг Включать догрев, если компрессор не нагрел бойлер до температуры догрева
#define fBoilerOnGenerator  11			// Греть бойлер на генераторе
#define fBoilerHeatElemSchPri 12		// Приоритет нагрева бойлера тэном по расписанию
#define fBoilerCircSchedule 13		  	// флаг Рециркуляция ГВС по расписанию
#define fBoilerPID			14			// Использовать ПИД

struct type_boilerHP {
	uint16_t flags;                    // Флаги
	uint8_t DischargeDelta;            // Сброс тепла в отопление, если температура подачи/конденсации приблизилась к максимуму/догреву, в десятых градуса
	uint8_t pid_time; 				   // Период расчета ПИД в секундах
	int16_t TempTarget;                // Целевая температура бойлера
	int16_t dTemp;                     // гистерезис целевой температуры
	int16_t tempInLim;                 // Tемпература подачи максимальная/минимальная если охлаждение ЗАЩИТА
	uint32_t Schedule[7];              // Расписание бойлера
	uint16_t Circul_Work;              // Время  работы насоса ГВС секунды (fCirculation)
	uint16_t Circul_Pause;             // Пауза в работе насоса ГВС  секунды (fCirculation)
	uint16_t Reset_Time;               // время сброса излишков тепла в секундах (fResetHeat)
	uint16_t delayOffPump;			   // Задержка выключения насоса после выключения компрессора (сек).
	PID_STRUCT pid;                    // Настройки и переменные ПИД регулятора
	int16_t dAddHeat;                  // Гистерезис нагрева бойлера до температуры догрева, в сотых градуса
	int16_t tempRBOILER;               // Температура догрева бойлера
	int16_t add_delta_temp; 	       // Добавка температуры к установке бойлера, в сотых градусах
	uint8_t add_delta_hour;	           // Начальный Час добавки температуры к установке бойлера
	uint8_t add_delta_end_hour; 	   // Конечный Час добавки температуры к установке
	int16_t tempPID;                   // Целевая температура ПИД
	int16_t WF_MinTarget;              // Температура цели при нулевой облачности, сотые градуса
	int16_t WR_Target;                 // Температура цели при нагреве от ваттроутера (солнца), сотые градуса
	uint16_t WorkPause;                // Минимальное время простоя, секунды
};

// Структуры для хранения настроек Отопления и Охлаждения (одинаковые)
// Работа с отдельными флагами type_settingHP
#define fTarget      			0	// Что является целью  - 0 (температура в доме), 1 (температура обратки).
#define fWeather     			1	// флаг Погодозависмости
#define fHeatFloor   			2	// флаг использования теплого пола
#define fUseSun      			3	// флаг использования солнечного коллектора
#define fP_ContinueAfterBoiler	4	// Продолжить работу по нагреву/охлаждению после нагрева бойлера
#define fAddHeat1				5	// Использование дополнительного тэна при нагреве (битовое поле)
#define fAddHeat2				6	// 0 - нет, 1 - по дому, 2 - по улице, 3 - интеллектуально
#define fUseAdditionalTargets	7	// Использовать дополнительные целевые датчики температуры
//#define f						8	// резерв

#define DS_TimeOn_Extended 236
struct type_DailySwitch {
	uint8_t Device;					// Реле, если >=RNUMBER, то дистанционные реле; 0 - нет и конец массива
	uint8_t TimeOn;					// Время включения hh:m0, или если >= 236, то TOUT: b0=0: T<, b0=1: T>, b1=1: ночью
	uint8_t TimeOff;				// Время выключения hh:m0, или градусы, если TimeOn >= 236
} __attribute__((packed));

struct type_setting_heat {
	uint16_t flags;					// Флаги опций
	RULE_HP Rule;					// алгоритм работы отопления.
	uint8_t _reserved_;				//
	int16_t Temp1;					// Целевая температура дома
	int16_t Temp2;					// Целевая температура обратки
	int16_t dTemp;					// Гистерезис целевой температуры
	uint16_t pid_time;				// Период расчета ПИД в секундах
	PID_STRUCT pid;					// Настройки и переменные ПИД регулятора
	int16_t dTempGen;				// Гистерезис целевой температуры от генератора
	int16_t tempInLim;				// Tемпература подачи (минимальная для охлажления или максимальная для нагрева)
	int16_t tempOutLim;				// Tемпература обратки (максимальная для охлажления или минимальная для нагрева)
	uint16_t WorkPause;				// Минимальное время простоя, секунды
	int16_t MaxDeltaTempOut;		// Максимальная разность температур входа-выхода отопления.
	int16_t kWeatherPID;			// Коэффициент погодозависимости PID, в ТЫСЯЧНЫХ градуса на градус
	int8_t WeatherBase;				// Базовая температура для погодозависимости, градусы
	uint8_t WeatherTargetRange;		// Предел изменения цели, десятые градусы
	int16_t add_delta_temp;			// Добавка температуры к установке отопления, в сотых градусах
	uint8_t add_delta_hour;			// Начальный Час добавки температуры к установке
	uint8_t add_delta_end_hour;		// Конечный Час добавки температуры к установке
	int16_t tempPID;				// Целевая темперaтура ПИД
	int16_t kWeatherTarget;			// Коэффициент погодозависимости цели (дом/обратка), в ТЫСЯЧНЫХ градуса на градус
	int16_t FC_FreqLimit;			// Максимальная скорость инвертора
	uint8_t FC_FreqLimitHour;		// с 00:00 до этого часа ограничивается скорость инвертора
	uint8_t timeRHEAT;				// Для интеллектуального режима догрева тэном - время за которое должно цель должна нагреться на tempRHEAT, минут
	int16_t tempRHEAT;				// Значение температуры для управления дополнительным ТЭН для нагрева СО
	uint16_t pausePump;				// Время паузы  насоса при выключенном компрессоре СЕКУНДЫ
	uint16_t workPump;				// Время работы насоса при выключенном компрессоре СЕКУНДЫ
	uint16_t delayOffPump;			// Задержка выключения насосов после выключения компрессора (сек).
	uint16_t HeatTargetSchedulerL;	// Расписание проверки дополнительных целевых датчиков температуры, час - битовое поле (b0..b15)
	uint8_t HeatTargetSchedulerH;	// Расписание проверки дополнительных целевых датчиков температуры, час - битовое поле (b15..b23)
	uint8_t MaxTargetRise;			// Максимальное превышение цели температуры, десятые градуса
};
struct type_setting_cool {
	uint16_t flags;					// Флаги опций
	RULE_HP Rule;					// алгоритм работы отопления.
	uint8_t _reserved_;				//
	int16_t Temp1;					// Целевая температура дома
	int16_t Temp2;					// Целевая температура обратки
	int16_t dTemp;					// Гистерезис целевой температуры
	uint16_t pid_time;				// Период расчета ПИД в секундах
	PID_STRUCT pid;					// Настройки и переменные ПИД регулятора
	int16_t dTempGen;				// Гистерезис целевой температуры от генератора
	int16_t tempInLim;				// Tемпература подачи (минимальная для охлажления или максимальная для нагрева)
	int16_t tempOutLim;				// Tемпература обратки (максимальная для охлажления или минимальная для нагрева)
	uint16_t WorkPause;				// Минимальное время простоя, секунды
	int16_t MaxDeltaTempOut;		// Максимальная разность температур входа-выхода отопления.
	int16_t kWeatherPID;			// Коэффициент погодозависимости PID, в ТЫСЯЧНЫХ градуса на градус
	int8_t WeatherBase;				// Базовая температура для погодозависимости, градусы
	uint8_t WeatherTargetRange;		// Предел изменения цели, десятые градусы
	//int16_t add_delta_temp;			// Добавка температуры к установке отопления, в сотых градусах
	//uint8_t add_delta_hour;			// Начальный Час добавки температуры к установке
	//uint8_t add_delta_end_hour;		// Конечный Час добавки температуры к установке
	int16_t tempPID;				// Целевая темперaтура ПИД
	int16_t kWeatherTarget;			// Коэффициент погодозависимости цели (дом/обратка), 0 - нет, в ТЫСЯЧНЫХ градуса на градус
	//int16_t FC_FreqLimit;			// Максимальная скорость инвертора
	//uint8_t FC_FreqLimitHour;		// с 00:00 до этого часа ограничивается скорость инвертора
	//uint8_t timeRHEAT;				// Для интеллектуального режима догрева тэном - время за которое должно цель должна нагреться на tempRHEAT, минут
	//int16_t tempRHEAT;				// Значение температуры для управления дополнительным ТЭН для нагрева СО
	uint16_t pausePump;				// Время паузы  насоса при выключенном компрессоре СЕКУНДЫ
	uint16_t workPump;				// Время работы насоса при выключенном компрессоре СЕКУНДЫ
	uint16_t delayOffPump;			// Задержка выключения насосов после выключения компрессора (сек).
	//uint16_t HeatTargetSchedulerL;	// Расписание проверки дополнительных целевых датчиков температуры, час - битовое поле (b0..b15)
	//uint8_t HeatTargetSchedulerH;	// Расписание проверки дополнительных целевых датчиков температуры, час - битовое поле (b15..b23)
	//uint8_t MaxTargetRise;			// Максимальное превышение цели температуры, десятые градуса
};

#define LEN_PROFILE_NAME        	26  // Длина имени профиля + 1 (конец строки)
// dataprofile.flags, биты:
#define fEnabled                	0   // Разрешение данного профайла использоваться в коротком списке
#define fPro_Main					1	// Основной профиль (нельзя авто-переключаться по расписанию)
#define fSwitchProfileNext_OnError	2	// Переключать на ProfileNext при ошибке
#define fSwitchProfileNext_ByTime	3	// Переключать на ProfileNext по времени (когда не рабочее время профиля)

struct type_dataProfile               // Хранение общих данных
{
	uint8_t 	flags;                  // Флаги профайла (2 элемент структуры!)
	uint8_t 	ProfileNext;			// Профиль на который будет переключение при ошибке или по времени [1..I2C_PROFIL_NUM], 0 - нет
	uint8_t 	TimeStart;             	// Время начала работы профиля, с hh:m0
	uint8_t 	TimeEnd;             	// Время окончания работы профиляm, до hh:m0
	uint32_t 	saveTime;               // Время сохранения профиля
	char 		name[LEN_PROFILE_NAME]; // Имя профайла
	char 		note[85]; // Описание профайла кодирование описания профиля 40 русских букв (одна буква 6 байт (двойное кодирование) это входная строка)  описание переводится и хранится в utf8 по 2 байта символ
};


class Profile                         // Класс профиль
{
 public:
    type_SaveON SaveON;                                     // Структура для хранения состояния ТН в ЕЕПРОМ
   // Настройки для отопления, охлаждения, бойлера
    type_setting_heat Heat;                                 // Настройки для режима отопления
    type_setting_cool Cool;                                 // Настройки для режима охлаждение
    type_boilerHP Boiler;                                   // Настройка бойлера
	type_DailySwitch DailySwitch[DAILY_SWITCH_MAX]; 		// дневное периодическое включение
	uint32_t DailySwitchStateT;
    type_dataProfile dataProfile;                           // данные профиля
    
     // Функции работы с профилем
    void initProfile();                                     // инициализация профиля
    char *get_list(char *c /*,int8_t num*/);                // ДОБАВЛЯЕТ к строке с - список возможных конфигураций num - текущий профиль
    int8_t set_list(int8_t num);                            // Устанавливает текущий профиль из номера списка, ДОБАВЛЯЕТ к строке с
    int8_t update_list(int8_t num);                         // обновить список имен профилей

    char *get_info(char *c,int8_t num);                     // ДОБАВЛЯЕТ к строке с - описание профиля num
    int16_t save(int8_t num);                               // Записать профайл в еепром под номерм num
    int32_t load(int8_t num);                               // загрузить профайл num из еепром память
//    int8_t  loadFromBuf(uint32_t adr,byte *buf);            // Считать настройки из буфера на входе адрес с какого, на выходе код ошибки (меньше нуля)
    int8_t  convert_to_new_version(void);
    int32_t erase(int8_t num);                              // стереть профайл num из еепром  (ставится признак пусто)
    boolean set_paramProfile(char *var,char *c);            // Профиль Установить параметры ТН из строки
    char*   get_paramProfile(char *var,char *ver);          // профиль Получить параметр
    inline  int8_t get_idProfile(){return id;}             // получить номер текущего профиля
    int8_t  check_DailySwitch(uint8_t i, uint32_t hhmm);
    uint8_t	check_switch_to_ProfileNext_byTime(type_dataProfile *dp);// проверка нужно ли переключиться на ProfileNext, возвращает номер профиля+1 или 0, если нет

    // Установка параметров
    boolean set_paramCoolHP(char *var, float x);            // Охлаждение Установить параметры ТН из числа (float)
    char*   get_paramCoolHP(char *var, char *ret);          // Охлаждение Получить параметр
    boolean set_paramHeatHP(char *var, float x);            // Отопление  Установить параметры ТН из числа (float)
    char*   get_paramHeatHP(char *var, char *ret);          // отопление  Получить параметр
    boolean set_boiler(char *var, char *c);                 // Установить параметр из строки
    char*   get_boiler(char *var, char *ret);               // Получить параметр из строки по имени var, результат ДОБАВЛЯЕТСЯ в строку ret
 
    char list[I2C_PROFIL_NUM*(LEN_PROFILE_NAME+6-1)+1];       // текущий список конфигураций + "n. " + ":?;"
    inline uint32_t get_sizeProfile() { // определить длину данных
    	return 1 + 2 + //sizeof(magic) + sizeof(crc16) +
				// данные контрольная сумма считается с этого места
				sizeof(dataProfile) + sizeof(SaveON) + sizeof(Cool) + sizeof(Heat) + sizeof(Boiler)	+ sizeof(DailySwitch);
    };
    int8_t id;												// Номер профайла 0..I2C_PROFIL_NUM-1
 private:
    uint16_t get_crc16_mem();                               // Расчитать контрольную сумму
    int8_t   check_crc16_eeprom(int8_t num);                // Проверить контрольную сумму ПРОФИЛЯ в EEPROM для данных на выходе ошибка, длина определяется из заголовка
    int8_t   check_crc16_buf(int32_t adr, byte* buf);       // Проверить контрольную сумму в буфере для данных на выходе ошибка, длина определяется из заголовка
    int8_t err;                                             // Ошибка
};

#endif
