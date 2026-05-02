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
	fwork = 0;

	set.setup_flags = (0<<fHeater_Opentherm) | (1<<fHeater_USE_Relay_RHEATER) | (1<<fHeater_USE_Relay_RH_3WAY) | (0<<fHeater_USE_Relay_Modbus) | (0<<fHeater_USE_Relay_Modbus_3WAY);
//	set.heat_tempout = 100;
//	set.heat_power_max = 100;
//	set.boiler_tempout = 100;
//	set.boiler_power_max = 100;
	set.pump_work_time_after_stop = 18;			// 3 минуты
	set.ModbusMinTimeBetweenTransaction = HEATER_MODBUS_MIN_TIME_BETWEEN_TRNS;
	set.ModbusResponseTimeout = HEATER_MODBUS_TIMEOUT;
}

void devHeater::check_link(void)
{
	curr_temp = 0;
	curr_boiler_temp = 0;
	if(!GETBIT(set.setup_flags, fHeater_Opentherm)) return;
	uint16_t d;
	int8_t _err = Modbus.readHoldingRegisters16(HEATER_MODBUS_ADDR, HM_SET_T_Flow, &d);
	if(_err) {
		journal.jprintf("Heater read #%X error: %d\n", HM_SET_T_Flow, _err);
		return;
	}
	curr_temp = d / 10;
	_err = Modbus.readHoldingRegisters16(HEATER_MODBUS_ADDR, HM_SET_T_BOILER, &d);
	if(_err) {
		journal.jprintf("Heater read #%X error: %d\n", HM_SET_T_BOILER, _err);
		return;
	}
	curr_boiler_temp = d;
//	_err = Modbus.readHoldingRegisters16(HEATER_MODBUS_ADDR, HM_SET_POWER, &d);
//	if(_err) {
//		journal.jprintf("Heater read #%X error: %d\n", HM_SET_T_BOILER, _err);
//		return;
//	}
//	PowerMaxCurrent = d;
	SETBIT1(fwork, fHeater_LinkAdapterOk);
}

// rise_error = false, не генерить ошибку, если что
void devHeater::Heater_Stop(bool rise_error)
{
#ifdef USE_HEATER
#ifdef RHEATER
	HP.dRelay[RHEATER].set_OFF();
#endif
#ifdef HEATER_MODBUS_RELAY_ID
	if(GETBIT(HP.work_flags, fHP_HeaterOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus)) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, 0);
			if(err) {
				err_num_total++;
				if(rise_error) {
					set_Error(err, (char*)__FUNCTION__);
					return;
				}
			}
		}
	}
#endif
#ifdef HEATER_MODBUS_ADDR
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_FLAGS, 0);
		if(err) {
			err_num_total++;
			if(rise_error) set_Error(err, (char*)"HeaterSW"); else journal.jprintf("ERROR Stop Heater: %d\n", err);
			return;
		}
		if(rise_error) {
			_delay(HEATER_ADAPTER_WAIT_WRITE); // ожидание
			int16_t status;
			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + HM_SET_FLAGS, 1, (uint16_t*)&status);
			if(err) {
				err_num_total++;
				set_Error(err, (char*)"HeaterSR");
				return;
			}
			if(status != 0) {
				journal.jprintf("OT write #%d err: %d\n", HM_SET_FLAGS, status);
				set_Error(ERR_HEATER_ADAPTER_LINK, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
	HP.work_flags = (HP.work_flags & ~((1<<fHP_HeaterOn) | (1<<fHP_HeaterWasOn) | (1<<fHP_CompressorWasOn))) | (GETBIT(HP.work_flags, fHP_HeaterOn)<<fHP_HeaterWasOn);
	HP.stopHeater = rtcSAM3X8.unixtime();
    journal.jprintf(" %s[%s] OFF\n", HEATER_NAME, (char *)codeRet[HP.get_ret()]);
#endif
}

// Только вколючение Котла, кран не переключается
void devHeater::Heater_Start()
{
#ifdef USE_HEATER
#ifdef RHEATER
	HP.dRelay[RHEATER].set_ON();
#endif
#ifdef HEATER_MODBUS_RELAY_ID
	if(!(GETBIT(HP.work_flags, fHP_HeaterOn)) {
		if(dHeater.set.setup_flags & fHeater_USE_Relay_Modbus) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, 1);
			if(err) {
				err_num_total++;
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
			err_num_total++;
			set_Error(err, (char*)"HeaterBW");
			return;
		}
		_delay(HEATER_ADAPTER_WAIT_WRITE); // ожидание
		int16_t status;
		err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + HM_SET_FLAGS, 1, (uint16_t*)&status);	// Получить регистр состояния записи
		if(err) {
			err_num_total++;
			set_Error(err, (char*)"HeaterBR");
			return;
		}
		if(status != 0) {
			journal.jprintf("OT write #%d err: %d\n", HM_SET_FLAGS, status);
			set_Error(ERR_HEATER_ADAPTER_LINK, (char*)__FUNCTION__);
			return;
		}
	}
#endif
	SETBIT1(HP.work_flags, fHP_HeaterOn);
	HP.startHeater = rtcSAM3X8.unixtime();
    journal.jprintf(" %s[%s] ON\n", HEATER_NAME, (char *)codeRet[HP.get_ret()]);
#endif
}

// Переключиться на котел, если еще нет
void devHeater::HeaterValve_On()
{
#ifdef USE_HEATER
#ifdef RH_3WAY
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) HP.dRelay[RH_3WAY].set_ON();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(!GETBIT(HP.work_flags, fHP_HeaterValveOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, 1);
			if(err) {
				err_num_total++;
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
			if(GETBIT(HP.Option.flags2, f2modWorkLog)) journal.jprintf("Heater valve ON\n");
		}
	}
#endif
#if defined(RH_3WAY) || defined(HEATER_MODBUS_3WAY_ID)
	if(!GETBIT(HP.work_flags, fHP_HeaterValveOn)) {
		SETBIT1(HP.work_flags, fHP_HeaterValveOn);
		_delay(HP.Option.SwitchHeaterHPTime * 1000);	// Ожидание переключения
	}
#endif
#endif
}

// Переключиться на ТН, если еще нет
void devHeater::HeaterValve_Off()
{
#ifdef USE_HEATER
	if(HP.is_heater_on()) {
		Heater_Stop(true);
		journal.jprintf("Heater is ON -> Turn OFF\n");
	}
	WaitPumpOff();
#ifdef RH_3WAY
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) HP.dRelay[RH_3WAY].set_OFF();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(GETBIT(HP.work_flags, fHP_HeaterValveOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, 0);
			if(err) {
				err_num_total++;
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
			if(GETBIT(HP.Option.flags2, f2modWorkLog)) journal.jprintf("Heater valve OFF\n");
		}
	}
#endif
#if defined(RH_3WAY) || defined(HEATER_MODBUS_3WAY_ID)
	if(GETBIT(HP.work_flags, fHP_HeaterValveOn)) {
		SETBIT0(HP.work_flags, fHP_HeaterValveOn);
		_delay(HP.Option.SwitchHeaterHPTime * 1000);	// Ожидание переключения
	}
#endif
#endif
}

// Ожидать постциркуляцию насоса
void devHeater::WaitPumpOff()
{
	int32_t d = set.pump_work_time_after_stop * 10;
	d -= rtcSAM3X8.unixtime() - (HP.stopHeater ? HP.stopHeater : HP.get_startDT());
	if(d > 0) {
		journal.jprintf("Wait Heater pump stop: %d s\n", d);
		HP.DelaySec(d);
	}
}

// Прочитать (внутренние переменные обновляются) состояние Котла, возвращает или ОК или ошибку
// Вызывается из задачи чтения датчиков, group: 0 - состояние линка с котлом, 1 - данные
int8_t devHeater::read_state(uint8_t group)
{
	if(group == 0 || !GETBIT(fwork, fHeater_LinkAdapterOk) || !GETBIT(fwork, fHeater_LinkHeaterOk)) { // группа 0 или если нет связи
		uint16_t r;
//		if(curr_temp == 0 && GETBIT(fwork, fHeater_LinkHeaterOk)) {
//			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_SET_T_Flow, 1, &r);
//			if(err == OK) curr_temp = r / 10;
//		} else
		if(GETBIT(fwork, fHeater_ReadErrorFlags)) {
			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_HEATER_fERRORS, 1, &err_flags);
			if(err == OK) Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_HEATER_ERROR2, 1, &Heater_Error2);
			SETBIT0(fwork, fHeater_ReadErrorFlags);
		} else {
			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_ADAPTER_FLAGS, 1, &r);
			if(err == OK) {
				if(GETBIT(r, HM_ADAPTER_FLAGS_bLINK)) SETBIT0(fwork, fHeater_fNotAnswerOnCmd); else SETBIT1(fwork, fHeater_fNotAnswerOnCmd);
				if(GETBIT(r, HM_ADAPTER_FLAGS_bLINK) || testMode != NORMAL) {
					err_num = 0;
					SETBIT0(fwork, fHeater_CmdNotResponse);
					SETBIT1(fwork, fHeater_LinkHeaterOk);
				} else {
					SETBIT1(fwork, fHeater_CmdNotResponse);
					//if(err_num >= HEATER_ADAPTER_NOT_RESPONSE_MAX) SETBIT0(fwork, fHeater_LinkHeaterOk); else err_num++;
				}
				if(data.Error) SETBIT1(fwork, fHeater_ReadErrorFlags);
				else {
					Heater_Error2 = 0;
					err_flags = 0;
				}
			}
		}
	} else {
		err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_START_DATA, sizeof(data), (uint16_t*)&data);
	}
	if(err) {
		if(testMode != NORMAL && testMode != HARD_TEST) {
			SETBIT1(fwork, fHeater_LinkAdapterOk);
			err = OK;
		} else {
			err_num_total++;
//			if(HP.IsWorkingNow()) {
//				journal.jprintf("%s Modbus error %d\n", HEATER_NAME, err);
//				set_Error(ERR_HEATER_ADAPTER_LINK, (char*)__FUNCTION__);
//			}
			SETBIT0(fwork, fHeater_LinkAdapterOk);
		}
	} else {
		SETBIT1(fwork, fHeater_LinkAdapterOk);
		if(group && (testMode == NORMAL || testMode == HARD_TEST)) {
			uint16_t _status = (HP.get_modWork() & pBOILER) ? GETBIT(data.Status, HM_STATUS_bBOILER) : GETBIT(data.Status, HM_STATUS_bHEATING);
			if(HP.is_heater_on()) {
				if(!GETBIT(fwork, fHeater_LinkHeaterOk)) {
					set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
				} else if(!_status && rtcSAM3X8.unixtime() - HP.startHeater > HEATER_WAIT_CMD_COMPLETION) {
					set_Error(err = ERR_HEATER_STOP, (char*)__FUNCTION__);
				}
	//		} else if(_status) {
	//			journal.jprintf("%s is working!\n", HEATER_NAME);
	//			DumpJournal();
	//			//Heater_Stop(true);
			}
		}
	}
	return err;
}

// Установить целевую температуру в градусах (целевые регистры зависят от задачи нагрев/бойлер)
// Если temp = 0, то установка только макс. мощности
int8_t devHeater::set_target(uint16_t temp)
{
	if(temp == 0 || GETBIT(set.setup_flags, fHeater_DontSetFlowTemp) || (testMode != NORMAL && testMode != HARD_TEST)) return OK;
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		uint16_t reg1 = 0;
		switch (testMode) // РЕАЛЬНЫЕ Действия в зависимости от режима
		{
		case NORMAL:
		case HARD_TEST:
			if(GETBIT(set.setup_flags, fHeater_BoilerInHeatingMode) && (HP.get_modWork() & pBOILER)) {
				if(curr_boiler_temp == temp) return OK;
				reg1 = HM_SET_T_BOILER;
			} else {
				if(curr_temp == temp) return OK;
				reg1 = HM_SET_T_Flow; // регистр в десятых градуса
				temp *= 10;			// регистр состояния тоже в десятых
			}
			journal.jprintf(" Set Heater%s: %d C\n", reg1 == HM_SET_T_Flow ? "" : " boiler", temp);
			err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, reg1, temp);
			if(err) {
				err_num_total++;
				set_Error(err, (char*)"HeaterW");
				break;
			}
			int16_t status;
			_delay(HEATER_ADAPTER_WAIT_WRITE); // ожидание
			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + reg1, 1, (uint16_t*)&status);	// Получить регистр состояния записи
			if(err) {
				err_num_total++;
				set_Error(err, (char*)"HeaterR");
				break;
			}
			if(status != 0) {
				journal.jprintf("OT write #%d err: %d\n", reg1, status);
				set_Error(ERR_HEATER_ADAPTER_LINK, (char*)__FUNCTION__);
			} else curr_temp = temp;
			break;
		default:
			break; //  Ничего не включаем
		}
	}
	return err;
}

// Проверка работает ли котел
bool devHeater::CheckIsHeaterOn(void)
{
	if(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(fwork, fHeater_LinkHeaterOk)) {
		return (data.Status & HM_STATUS_bBURNER) ?  true : false;
	} else return HP.is_heater_on();
}

// Получить информацию
void devHeater::get_info(char* buf)
{
	buf += strlen(buf);
	if(!GETBIT(fwork, fHeater_LinkAdapterOk)) {   // Нет связи
		strcat(buf, "|Данные не доступны - нет связи|;");
	} else {
		buf += m_snprintf(buf, 256, "1D|Состояние: %X|%s %s %s;", data.Status, !data.Status ? "Выкл" : (data.Status & 1) ? "Вкл" : "", (data.Status & 2) ? "Отопление" : "", (data.Status & 4) ? "ГВС" : "");
//		buf += m_snprintf(buf, 256, "1C|Модуляция, %%|%d;", (int8_t)data.Power);
		buf += m_snprintf(buf, 256, "1A|Давление в контуре, бар|%.1d;", data.P_OUT);
		strcat(buf, "1E|Код ошибки (дополнительный)|");
		if(data.Error) {
			buf += m_snprintf(buf += strlen(buf), 32, "%d (%d);", data.Error, Heater_Error2);
			uint16_t d = err_flags;
			buf += m_snprintf(buf, 256, "23|Флаги ошибок|%d;", d);
			if(GETBIT(d, HM_ERROR_SERVICE)) buf += m_snprintf(buf, 256, "|%s|;", HM_ERROR_SERVICE_S);
			if(GETBIT(d, HM_ERROR_BLOCKED)) buf += m_snprintf(buf, 256, "|%s|;", HM_ERROR_BLOCKED_S);
			if(GETBIT(d, HM_ERROR_LOW_PRESSURE)) buf += m_snprintf(buf, 256, "|%s|;", HM_ERROR_LOW_PRESSURE_S);
			if(GETBIT(d, HM_ERROR_IGN_ERROR)) buf += m_snprintf(buf, 256, "|%s|;", HM_ERROR_IGN_ERROR_S);
			if(GETBIT(d, HM_ERROR_LOW_AIR)) buf += m_snprintf(buf, 256, "|%s|;", HM_ERROR_LOW_AIR_S);
			if(GETBIT(d, HM_ERROR_OVERHEAT)) buf += m_snprintf(buf, 256, "|%s|;", HM_ERROR_OVERHEAT_S);
		} else strcat(buf, "-;");
	}
}

void devHeater::DumpJournal(void)
{
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		journal.jprintf(" Heater:%X M:%d%% tF:%.1d tB:%.1d ", data.Status, data.Power, data.T_Flow, data.T_Boiler);
		journal.jprintf("P:%.1d E:%d,%d\n", data.P_OUT, data.Error, Heater_Error2);
	}
}

// Получить параметр в виде строки, результат ДОБАВЛЯЕТСЯ в ret, если ошибка возврат false
bool devHeater::get_param(char *var, char *ret)
{
	ret += strlen(ret);
	if(strcmp(var, Wheater_LinkHeaterOk)==0)			{ if(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(fwork, fHeater_LinkAdapterOk) && GETBIT(fwork, fHeater_LinkHeaterOk)) {
																strcat(ret, "Ok");
																if(GETBIT(fwork, fHeater_CmdNotResponse)) strcat(ret, "?");
																if(GETBIT(fwork, fHeater_fNotAnswerOnCmd)) strcat(ret, "!");
															} else strcat(ret, "Нет"); } else
	if(strcmp(var, Wheater_fLinkAdapterOk)==0)			{ _itoa(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(fwork, fHeater_LinkAdapterOk), ret); } else
	if(strcmp(var, Wheater_is_on)==0) 					{ _itoa(GETBIT(HP.work_flags, fHP_HeaterOn), ret); } else
	if(strcmp(var, option_Control_Period)==0) 			{ _itoa(set.Control_Period, ret); } else
	if(strcmp(var, Wheater_3way)==0) 					{ strcat(ret, HP.is_heater_active() ? "Котел" : "ТН"); } else
	if(strcmp(var, Wheater_T_Flow)==0) 					{ _dtoa(ret, data.T_Flow, 1); strcat(ret, " ("); _itoa(curr_temp, ret); strcat(ret, ")"); } else
	if(strcmp(var, Wheater_Power)==0) 					{ _itoa(data.Power, ret); } else
	if(strcmp(var, Wheater_Errors)==0) 					{ _itoa(err_num_total, ret); } else
	if(strcmp(var, Wheater_fHeater_Opentherm)==0)		{ if(GETBIT(set.setup_flags, fHeater_Opentherm)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RHEATER)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RHEATER)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RH_3WAY)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_Heating_Pipes)==0){ if(GETBIT(set.setup_flags, fHeater_Heating_Pipes)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_Heating_Pipes_Temp)==0){ if(GETBIT(set.setup_flags, fHeater_Heating_Pipes_Temp)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_BoilerInHeatingMode)==0){ if(GETBIT(set.setup_flags, fHeater_BoilerInHeatingMode)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_DontSetFlowTemp)==0){ if(GETBIT(set.setup_flags, fHeater_DontSetFlowTemp)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_wait_heating_pipes_time)==0){ _itoa(set.wait_heating_pipes_time * 4, ret); } else
	if(strcmp(var, Wheater_wait_heating_pipes_time_max)==0){ _itoa(set.wait_heating_pipes_time_max * 4, ret); } else
	if(strcmp(var, Wheater_HeatingPipesSubTemp)==0){ _itoa(set.HeatingPipesSubTemp, ret); } else
//	if(strcmp(var, Wheater_heat_tempout)==0) 			{ _itoa(set.heat_tempout, ret); } else
//	if(strcmp(var, Wheater_heat_power_min)==0) 			{ _itoa(set.heat_power_min, ret); } else
//	if(strcmp(var, Wheater_heat_power_max)==0) 			{ _itoa(set.heat_power_max, ret); } else
//	if(strcmp(var, Wheater_heat_protect_temp_dt)==0)	{ _dtoa(ret, set.heat_protect_temp_dt, 1); } else
//	if(strcmp(var, Wheater_boiler_tempout)==0)			{ _itoa(set.boiler_tempout, ret); } else
//	if(strcmp(var, Wheater_power_boiler_min)==0) 		{ _itoa(set.boiler_power_min, ret); } else
//	if(strcmp(var, Wheater_power_boiler_max)==0) 		{ _itoa(set.boiler_power_max, ret); } else
//	if(strcmp(var, Wheater_boiler_protect_temp_dt)==0)	{ _dtoa(ret, set.boiler_protect_temp_dt, 1); } else
	if(strcmp(var, Wheater_pump_work_time_after_stop)==0){ _itoa(set.pump_work_time_after_stop * 10, ret); } else
	if(strcmp(var, option_ModbusMinTimeBetweenTransaction)==0){ _itoa(set.ModbusMinTimeBetweenTransaction, ret); } else
	if(strcmp(var, option_ModbusResponseTimeout)==0){ _itoa(set.ModbusResponseTimeout, ret); } else
	if(strcmp(var, W_Modbus_Attempts)==0){ _itoa(set.Modbus_Attempts, ret); } else
	return false;

	return true;
}

// Установить параметр из строки, возврат ошибки
int8_t devHeater::set_param(char *var, float f)
{
	int32_t x = f;
	if(strcmp(var, Wheater_fHeater_Opentherm)==0)		{ if(x) SETBIT1(set.setup_flags, fHeater_Opentherm); else SETBIT0(set.setup_flags, fHeater_Opentherm); return OK;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RHEATER)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RHEATER); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RHEATER); return OK;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RH_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RH_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RH_3WAY); return OK;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus); return OK;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); return OK;} else
	if(strcmp(var, Wheater_fHeater_Heating_Pipes)==0){ if(x) SETBIT1(set.setup_flags, fHeater_Heating_Pipes); else SETBIT0(set.setup_flags, fHeater_Heating_Pipes); return OK;} else
	if(strcmp(var, Wheater_fHeater_Heating_Pipes_Temp)==0){ if(x) SETBIT1(set.setup_flags, fHeater_Heating_Pipes_Temp); else SETBIT0(set.setup_flags, fHeater_Heating_Pipes_Temp); return OK;} else
	if(strcmp(var, Wheater_fHeater_BoilerInHeatingMode)==0){ if(x) SETBIT1(set.setup_flags, fHeater_BoilerInHeatingMode); else SETBIT0(set.setup_flags, fHeater_BoilerInHeatingMode); return OK;} else
	if(strcmp(var, Wheater_fHeater_DontSetFlowTemp)==0){ if(x) SETBIT1(set.setup_flags, fHeater_DontSetFlowTemp); else SETBIT0(set.setup_flags, fHeater_DontSetFlowTemp); return OK;} else
	if(strcmp(var, Wheater_wait_heating_pipes_time)==0){ set.wait_heating_pipes_time = x / 4; return OK; } else
	if(strcmp(var, Wheater_wait_heating_pipes_time_max)==0){ set.wait_heating_pipes_time_max = x / 4; return OK; } else
	if(strcmp(var, Wheater_HeatingPipesSubTemp)==0){ set.HeatingPipesSubTemp = x; return OK; } else
	if(strcmp(var, option_Control_Period)==0)			{ if(x>0){ set.Control_Period = x; return OK; } } else
//	if(strcmp(var, Wheater_heat_tempout)==0)			{ if(x>=0 && x<=100){ set.heat_tempout = x; return OK; } } else
//	if(strcmp(var, Wheater_heat_power_min)==0)			{ if(x>=0 && x<=100){ set.heat_power_min = x; return OK; } } else
//	if(strcmp(var, Wheater_heat_power_max)==0)			{ if(x>=0 && x<=100){ set.heat_power_max = x; return OK; } } else
//	if(strcmp(var, Wheater_heat_protect_temp_dt)==0)	{ set.heat_protect_temp_dt = rd(x, 10); return OK; } else
//	if(strcmp(var, Wheater_boiler_tempout)==0)			{ if(x>=0 && x<=100){ set.boiler_tempout = x; return OK; } } else
//	if(strcmp(var, Wheater_power_boiler_min)==0)		{ if(x>=0 && x<=100){ set.boiler_power_min = x; return OK; } } else
//	if(strcmp(var, Wheater_power_boiler_max)==0)		{ if(x>=0 && x<=100){ set.boiler_power_max = x; return OK; } } else
//	if(strcmp(var, Wheater_boiler_protect_temp_dt)==0)	{ set.boiler_protect_temp_dt = rd(x, 10); return OK; } else
	if(strcmp(var, Wheater_pump_work_time_after_stop)==0){ set.pump_work_time_after_stop = x / 10; return OK; } else
	if(strcmp(var, option_ModbusMinTimeBetweenTransaction)==0){ set.ModbusMinTimeBetweenTransaction = x; return OK; } else
	if(strcmp(var, option_ModbusResponseTimeout)==0){ set.ModbusResponseTimeout = x; return OK; }
	if(strcmp(var, W_Modbus_Attempts)==0){ set.Modbus_Attempts = x; return OK; }
    return 27;
}

#endif
