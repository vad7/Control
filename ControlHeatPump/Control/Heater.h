/*
 * Copyright (c) 2024-2025 by Vadim Kulakov vad7@yahoo.com, vad711
 * Котел
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
#ifndef _Heater_h
#define _Heater_h

#include "Constant.h"
#ifdef USE_HEATER

#define HEATER_NAME	"Heater"
// Регистры Modbus RTU адаптера ectoControl v2 (19200 bps)
// Для чтения
#define HM_ADAPTER_FLAGS				0x0010
#define HM_ADAPTER_VER					0x0011
#define HM_ADAPTER_TIME					0x0012	// 4 bytes
#define HM_ADAPTER_RESET				0x0080	// 2  - перезагрузка адаптера, 3 - сброс ошибок котла
#define HM_ADAPTER_CMD_INFO				0x0081	// Ответ на команду:
												// -5 – ошибка выполнения команды
												// -4 – неподдерживаемая котлом команда
												// -3 – не поддерживаемый котлом идентификатор устройства
												// -2 – не поддерживается данный адаптером
												// -1 – не получен ответ за отведенное врекмя
												// 0 – команда выполнена успешно
												// 1 – не было команды (значение по умолчанию)
												// 2 – идет обработка команды (обмен данными)
#define HM_LIMIT_LOW					0x0014	// Нижний предел уставки теплоносителя (0 - 100 С)
#define HM_LIMIT_HIGH					0x0015	// Верхний предел уставки теплоносителя (0 - 100 С)
#define HM_LIMIT_BOILER_LOW				0x0016	// Нижний предел уставки ГВС (0 - 100 С)
#define HM_LIMIT_BOILER_HIGH			0x0017	// Верхний предел уставки ГВС (0 - 100 С)
#define HM_HEATER_MAKER					0x0021	// Код производителя котла
#define HM_HEATER_MODEL					0x0022	// Код модели котла
#define HM_HEATER_ERRORS				0x0023	// Флаги ошибок
#define HM_START_DATA					0x0018

struct type_heater_read {
	int16_t	 T_Flow;							// 0x18, Текущая температура теплоносителя (-100.0 - 100.0), десятые градуса
	uint16_t T_Boiler;							// 0x19, Текущая температура ГВС (0.0 - 100.0), десятые градуса
	uint16_t P_OUT;								// 0x1A, Текущее Давление в контуре (0.0 - 5.0), десятые бара
	uint16_t F_Boiler;							// 0x1B, Текущий расход ГВС (0.0 - 25.5), десятые литра
	uint16_t Power;								// 0x1C, Модуляция (0 - 100%, 0xFF - неопределено)
	uint16_t Status;							// 0x1D, биты статуса
	uint16_t Error;								// 0x1E, ошибка котла
	uint16_t Error2;							// 0x1F, ошибка котла дополнительная
	int16_t  T_OUT;								// 0x20, Температура уличного датчика котла (-65 - 100), градусы
} __attribute__((packed));

#define HM_ERROR_SERVICE				0		// Необходимо обслуживание
#define HM_ERROR_BLOCKED				1		// Котел заблокирован
#define HM_ERROR_LOW_PRESSURE			2		// Низкое давление в отопительном контуре
#define HM_ERROR_IGN_ERROR				3		// Ошибка розжига
#define HM_ERROR_LOW_AIR				4		// Низкое давление воздуха
#define HM_ERROR_OVERHEAT				5		// Перегрев теплоносителя в контуре

#define HM_STATUS_BURNER				0		// горелка вкл/выкл
#define HM_STATUS_HEATING				1		// отопление вкл/выкл
#define HM_STATUS_BOILER				2		// ГВС вкл/выкл

// Регистры для записи
#define HM_CONN_TYPE					0x0030	// Тип внешних подключений (0 - адаптер подключен к котлу, 1 - котел подключен к внешнему устройству (панель или перемычка)
#define HM_SET_T_Flow					0x0031	// Уставка теплоносителя (0.0 - 100.0), десятые градуса
#define HM_SET_T_Flow_NC				0x0032	// Уставка теплоносителя в случае отсутствия связи, десятые градуса
#define HM_SET_LIMIT_LOW				0x0033	// Нижний предел уставки теплоносителя (0 - 100 C)
#define HM_SET_LIMIT_HIGH				0x0034	// Верхний предел уставки теплоносителя (0 - 100 C)
#define HM_SET_LIMIT_BOILER_LOW			0x0035	// Нижний предел уставки ГВС (0 - 100 C)
#define HM_SET_LIMIT_BOILER_HIGH		0x0036	// Верхний предел уставки ГВС (0 - 100 C)
#define HM_SET_T_BOILER					0x0037	// Уставка ГВС (0 - 100 C)
#define HM_SET_POWER					0x0038	// Уставка максимальной модуляции (0 - 100 %)
#define HM_SET_FLAGS					0x0039	// Флаги битовые:
#define HM_SET_FLAGS_HEATING			0		// режим контура отопления (вкл/выкл)
#define HM_SET_FLAGS_BOILER				1		// режим контура ГВС (вкл/выкл)
#define HM_SET_FLAGS_SECOND				2		// второй контур (вкл/выкл)

// Флаги setup_flags
#define fHeater_Opentherm				0		// Обмен с котлом по Opentherm (через Modbus)
#define fHeater_USE_Relay_RHEATER		1		// Использовать обычное реле RHEATER для запуска Котла
#define fHeater_USE_Relay_RH_3WAY		2		// Использовать обычное реле RH_3WAY для переключения Котел - ТН
#define fHeater_USE_Relay_Modbus		3		// Использовать Modbus реле для запуска Котла
#define fHeater_USE_Relay_Modbus_3WAY	4		// Использовать Modbus реле для переключения Котел - ТН

struct type_HeaterSettings {
	uint16_t setup_flags;						// флаги настройки
	uint8_t  power_start;						// Стартовая мощность для отопления, %
	uint8_t  power_max;							// Максимальная мощность для отопления, %
	uint8_t  power_boiler_start;				// Стартовая мощность для бойлера, %
	uint8_t  power_boiler_max;					// Максимальная мощность для бойлера, %
	uint8_t  pump_work_time_after_stop;			// Время работы циркуляцонного насоса котла после останова, /10 секунд
};												// Структура для сохранения настроек

// Рабочие флаги (flags)
#define fHeater_Work					0		// Котел работает
#define fHeater_LinkOk					1		// Есть связь по Modbus


class devHeater
{
public:
	int8_t	init();									// Инициализация
	int16_t	CheckLinkStatus(void);					// Проверка связи
	int8_t	read_state();							// Текущее состояние
	uint8_t	get_target() { return Target; }			// Получить целевую мощность
	int8_t	set_target(uint8_t start, uint8_t max); // Установить целевую и максимальную мощность
	int8_t	start();								// Команда начала нагрева
	int8_t	stop();									// Команда останова нагрева
	uint8_t	*get_save_addr(void) { return (uint8_t *)&set; }	// Адрес структуры сохранения
	uint16_t get_save_size(void) { return sizeof(set); }	// Размер структуры сохранения
	void	get_param(char *var, char *ret);		// Получить параметр в виде строки - get_pFC('x')
	boolean	set_param(char *var, float p);			// Установить параметр из строки - set_pFC('x')
	void 	get_info(char* buf);					// Получить информацию
	inline type_HeaterSettings *get_settings() { return &set; };	// Вернуть структуру настроек

	void 	Heater_Start();							// Включить котел
	void 	Heater_Stop();								// Выключить котел
	void 	HeaterValve_On();							// Переключиться на котел
	void 	HeaterValve_Off();							// Переключиться на ТН

	int8_t   err;									// ошибка
	uint8_t  err_num;								// число ошибок чтение по модбасу подряд
	uint16_t err_num_total;							// число ошибок чтение по модбасу
	uint8_t  Target;								// Целевая мощность (модуляция), %
	type_HeaterSettings set;						// Структура для сохранения настроек

private:
	uint16_t flags;									// рабочие флаги
 };

#endif
#endif
