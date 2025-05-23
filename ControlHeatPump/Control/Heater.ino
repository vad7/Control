/*
 * Copyright (c) 2024-2025 by Vadim Kulakov vad7@yahoo.com, vad711
 * Частотный преобразователь Vacon 10
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
#include "Heater.h"
#ifdef USE_HEATER

void devHeater::init()
{
	err = OK;
	err_num = 0;
	err_num_total = 0;
	Target = 0;
	work_flags = 0;
	prev_temp = -1;
	prev_boiler_temp = -1;
	prev_power = -1;

	set.setup_flags = (0<<fHeater_Opentherm) | (1<<fHeater_USE_Relay_RHEATER) | (1<<fHeater_USE_Relay_RH_3WAY) | (0<<fHeater_USE_Relay_Modbus) | (0<<fHeater_USE_Relay_Modbus_3WAY);
	set.heat_tempout = 100;
	set.heat_power_max = 100;
	set.boiler_tempout = 100;
	set.boiler_power_max = 100;
	set.pump_work_time_after_stop = 18;			// 3 минуты
}

void devHeater::Heater_Stop()
{
#ifdef USE_HEATER
#ifdef RHEATER
	HP.dRelay[RHEATER].set_OFF();
#endif
#ifdef HEATER_MODBUS_RELAY_ID
	if(GETBIT(work_flags, fHP_HeaterOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus)) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, 0);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
#ifdef HEATER_MODBUS_ADDR
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_FLAGS, 0);
		if(err) {
			set_Error(err, (char*)__FUNCTION__);
			return;
		}
		int16_t status;
		err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + HM_SET_FLAGS, 1, (uint16_t*)&status);
		if(err) {
			set_Error(err, (char*)__FUNCTION__);
			return;
		}
		if(status != 0) {
			journal.jprintf("OT write #%d err: %d\n", HM_SET_FLAGS, status);
			set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
			return;
		}
	}
#endif
	HP.work_flags = (HP.work_flags & ~((1<<fHP_HeaterOn) | (1<<fHP_HeaterWasOn) | (1<<fHP_CompressorWasOn))) | (GETBIT(HP.work_flags, fHP_HeaterOn)<<fHP_HeaterWasOn);
	HP.stopHeater = rtcSAM3X8.unixtime();
#endif
}

void devHeater::Heater_Start()
{
#ifdef USE_HEATER
	HeaterValve_On();
#ifdef RHEATER
	HP.dRelay[RHEATER].set_ON();
#endif
#ifdef HEATER_MODBUS_RELAY_ID
	if(!(GETBIT(work_flags, fHP_HeaterOn)) {
		if(dHeater.set.setup_flags & fHeater_USE_Relay_Modbus) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, 1);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
#ifdef HEATER_MODBUS_ADDR
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_FLAGS, 1<<HM_SET_FLAGS_HEATING);
		if(err) {
			set_Error(err, (char*)__FUNCTION__);
			return;
		}
		int16_t status;
		err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + HM_SET_FLAGS, 1, (uint16_t*)&status);	// Получить регимстр состояния записи
		if(err) {
			set_Error(err, (char*)__FUNCTION__);
			return;
		}
		if(status != 0) {
			journal.jprintf("OT write #%d err: %d\n", HM_SET_FLAGS, status);
			set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
			return;
		}
	}
#endif
	SETBIT1(HP.work_flags, fHP_HeaterOn);
	HP.startHeater = rtcSAM3X8.unixtime();
#endif
}

// Переключиться на котел, если еще нет
void devHeater::HeaterValve_On()
{
#ifdef USE_HEATER
#ifdef RH_3WAY
	if(!GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) HP.dRelay[RH_3WAY].set_ON();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(!GETBIT(work_flags, fHP_HeaterValveOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, 1);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
#if defined(RH_3WAY) || defined(HEATER_MODBUS_3WAY_ID)
	if(!GETBIT(work_flags, fHP_HeaterValveOn)) {
		journal.jprintf("Heater valve ON, wait\n");
		_delay(HP.Option.SwitchHeaterHPTime * 1000);	// Ожидание переключения
	}
#endif
	SETBIT1(HP.work_flags, fHP_HeaterValveOn);
#endif
}

// Переключиться на ТН, если еще нет
void devHeater::HeaterValve_Off()
{
#ifdef USE_HEATER
	if(HP.is_heater_on()) {
		Heater_Stop();
		journal.jprintf("Heater is ON -> Turn OFF\n");
	}
	WaitPumpOff();
#ifdef RH_3WAY
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) HP.dRelay[RH_3WAY].set_OFF();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(GETBIT(work_flags, fHP_HeaterValveOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, 0);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
#if defined(RH_3WAY) || defined(HEATER_MODBUS_3WAY_ID)
	if(GETBIT(work_flags, fHP_HeaterValveOn)) {
		journal.jprintf("Heater valve OFF, wait\n");
		_delay(HP.Option.SwitchHeaterHPTime * 1000);	// Ожидание переключения
	}
#endif
	SETBIT0(HP.work_flags, fHP_HeaterValveOn);
#endif
}

// Ожидать постциркуляцию насоса
void devHeater::WaitPumpOff()
{
	int32_t d = set.pump_work_time_after_stop * 10;
	if(GETBIT(HP.work_flags, fHP_HeaterWasOn)) d -= rtcSAM3X8.unixtime() - HP.stopHeater;
	if(d > 0) journal.jprintf("Wait Heater pump stop: %ds\n", d);
	for(; d > 0; d--) { // задержка после выкл котла (постциркуляция насоса)
		_delay(1000);
		if(HP.get_errcode() || HP.is_next_command_stop() || HP.get_State() == pSTOPING_HP) return; // прерваться по ошибке или по команде
	}
}

// Прочитать (внутренние переменные обновляются) состояние Котла, возвращает или ОК или ошибку
// Вызывается из задачи чтения датчиков, group: 0 - состояние линка с котлом, 1 - данные
int8_t devHeater::read_state(uint8_t group)
{
	if(testMode != NORMAL && testMode != HARD_TEST) {
		// todo
		err = OK;
	} else {
		if(group == 0 || GETBIT(work_flags, fHeater_LinkHeaterOk)) {
			uint16_t r;
			if((err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_ADAPTER_FLAGS, 1, &r))) {
				if(HP.IsWorkingNow()) set_Error(err, (char*)__FUNCTION__);
				SETBIT0(work_flags, fHeater_LinkAdapterOk);
			} else {
				SETBIT1(work_flags, fHeater_LinkAdapterOk);
				if(GETBIT(r, HM_ADAPTER_FLAGS_bLINK)) SETBIT1(work_flags, fHeater_LinkHeaterOk); else SETBIT0(work_flags, fHeater_LinkHeaterOk);
			}
		} else {
			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_START_DATA, sizeof(data), (uint16_t*)&data);
		}
		if(err) {
			if(HP.IsWorkingNow()) set_Error(err, (char*)__FUNCTION__);
			SETBIT0(work_flags, fHeater_LinkAdapterOk);
		} else {
			SETBIT1(work_flags, fHeater_LinkAdapterOk);

		}
	}
	return err;
}

// Установить целевую температуру и макс. мощность % (целевые регистры зависят от задачи нагрев/бойлер)
int8_t devHeater::set_target(uint8_t temp, uint8_t power_max)
{
	uint16_t reg1 = 0;
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		journal.jprintf(" Set Heater: %.2d%, %.2d%%\n", temp, power_max);
		switch (testMode) // РЕАЛЬНЫЕ Действия в зависимости от режима
		{
		case NORMAL:
		case HARD_TEST:
			if(prev_temp != temp) {
				reg1 = HP.get_modWork() & pBOILER ? HM_SET_T_BOILER : HM_SET_T_Flow;
				int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, reg1, temp);
				if(err) {
					set_Error(err, (char*)__FUNCTION__);
					break;
				}
			}
			if(prev_power != power_max) {
				int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_POWER, power_max);
				if(err) {
					set_Error(err, (char*)__FUNCTION__);
					break;
				}
			}
			int16_t status;
			if(prev_temp != temp) {
				err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + reg1, 1, (uint16_t*)&status);	// Получить регистр состояния записи
				if(err) {
					set_Error(err, (char*)__FUNCTION__);
					break;
				}
				if(status != 0) {
					journal.jprintf("OT write #%d err: %d\n", reg1, status);
					set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
				} else prev_temp = temp;
			}
			if(prev_power != power_max) {
				err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + HM_SET_POWER, 1, (uint16_t*)&status);	// Получить регистр состояния записи
				if(err) {
					set_Error(err, (char*)__FUNCTION__);
					break;
				}
				if(status != 0) {
					journal.jprintf("OT write #%d err: %d\n", HM_SET_POWER, status);
					set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
				} else prev_power = power_max;
			}
			break;
		default:
			break; //  Ничего не включаем
		}
	}
	return err;
}

// Получить информацию
void devHeater::get_info(char* buf)
{
	buf += strlen(buf);
	if(!GETBIT(work_flags, fHeater_LinkHeaterOk)) {   // Нет связи
		strcat(buf, "|Данные не доступны - нет связи|;");
	} else {
		buf += m_snprintf(buf, 256, "1D|Статус: %X|%s %s %s;", data.Status, (data.Status & 1) ? "Вкл" : "", (data.Status & 2) ? "Отопление" : "", (data.Status & 4) ? "ГВС" : "");
		buf += m_snprintf(buf, 256, "1E|Код ошибки (дополнительный)|%d(%d);", data.Error, data.Error2);
		//HM_HEATER_ERRORS
		buf += m_snprintf(buf, 256, "1A|Давление в контуре, бар|%.1d;", data.P_OUT);
	}
}

// Получить параметр в виде строки, результат ДОБАВЛЯЕТСЯ в ret
void devHeater::get_param(char *var, char *ret)
{
	ret += strlen(ret);
	if(strcmp(var, Wheater_LinkHeaterOk)==0)				{ _itoa(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(work_flags, fHeater_LinkHeaterOk), ret); } else
	if(strcmp(var, Wheater_fLinkAdapterOk)==0)			    { _itoa(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(HP.work_flags, fHeater_LinkAdapterOk), ret); } else
	if(strcmp(var, Wheater_is_on)==0) 					    { _itoa(GETBIT(HP.work_flags, fHP_HeaterOn), ret); } else
	if(strcmp(var, Wheater_3way)==0) 					    { strcat(ret, GETBIT(HP.work_flags, fHP_HeaterValveOn) ? "Котел" : "ТН"); } else
	if(strcmp(var, Wheater_T_Flow)==0) 					    { if(GETBIT(work_flags, fHeater_LinkHeaterOk)) _dtoa(ret, data.Power, 1); else strcat(ret, "-"); } else
	if(strcmp(var, Wheater_Power)==0) 					    { if(GETBIT(work_flags, fHeater_LinkHeaterOk)) _itoa(data.T_Flow, ret); else strcat(ret, "-"); } else
	if(strcmp(var, Wheater_Errors)==0) 					    { _itoa(err_num_total, ret); } else
	if(strcmp(var, Wheater_fHeater_Opentherm)==0)			{ if(GETBIT(set.setup_flags, fHeater_Opentherm)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RHEATER)==0)	{ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RHEATER)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RH_3WAY)==0)	{ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus)==0)	{ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_heat_tempout)==0) 				{ _itoa(set.heat_tempout, ret); } else
	if(strcmp(var, Wheater_heat_power_max)==0) 					{ _itoa(set.heat_power_max, ret); } else
	if(strcmp(var, Wheater_boiler_tempout)==0)			{ _itoa(set.boiler_tempout, ret); } else
	if(strcmp(var, Wheater_power_boiler_max)==0) 			{ _itoa(set.boiler_power_max, ret); } else
	if(strcmp(var, Wheater_pump_work_time_after_stop)==0) 	{ _itoa(set.pump_work_time_after_stop * 10, ret); } else
	if(strcmp(var, "INFO")==0) 					    		get_info(ret); else

    strcat(ret,(char*)cInvalid);
}

// Установить параметр из строки, возврат ошибки
int8_t devHeater::set_param(char *var, float f)
{
	int32_t x = f;
	if(strcmp(var, Wheater_fHeater_Opentherm)==0)		{ if(x) SETBIT1(set.setup_flags, fHeater_Opentherm); else SETBIT0(set.setup_flags, fHeater_Opentherm); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RHEATER)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RHEATER); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RHEATER); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RH_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RH_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RH_3WAY); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); return true;} else
	if(strcmp(var, Wheater_heat_tempout)==0)				{ if(x>=0 && x<=100){ set.heat_tempout = x; return true; } } else
	if(strcmp(var, Wheater_heat_power_max)==0)				{ if(x>=0 && x<=100){ set.heat_power_max = x; return true; } } else
	if(strcmp(var, Wheater_boiler_tempout)==0)		{ if(x>=0 && x<=100){ set.boiler_tempout = x; return true; } } else
	if(strcmp(var, Wheater_power_boiler_max)==0)		{ if(x>=0 && x<=100){ set.boiler_power_max = x; return true; } } else
	if(strcmp(var, Wheater_pump_work_time_after_stop)==0){ if(x>=0 && x<=100){ set.pump_work_time_after_stop = x / 10; return true; } } else
	if(var[0] == 'W'){ // set_HT(Wn), где n номер регистра в HEX
		uint16_t adr = strtol(var + 1, NULL, 16);
		if(x != LONG_MAX) {
			int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, adr, x);
			if(err) return err;
			int16_t status;
			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + adr, 1, (uint16_t*)&status);	// Получить регистр состояния записи
			if(err) return err;
			return status;
		}
	}

    return false;
}

#endif
