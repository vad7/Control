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
	set.Modbus_Attempts = 1;
}

void devHeater::check_link(void)
{
	target_temp = 0;
	target_boiler_temp = 0;
	if(!GETBIT(set.setup_flags, fHeater_Opentherm)) return;
	uint16_t d;
	int8_t _err = ERR_CONFIG;
	REPEAT_N(HP.dHeater.set.Modbus_Attempts, _err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_SET_T_FlowOut, &d, READ_HOLDING));
	if(_err) {
		err_num_total++;
		journal.jprintf("Heater read #%X error: %d\n", HM_SET_T_FlowOut, _err);
		return;
	}
	target_temp = d / 10;
	REPEAT_N(HP.dHeater.set.Modbus_Attempts, _err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_SET_T_BOILER, &d, READ_HOLDING));
	if(_err) {
		err_num_total++;
		journal.jprintf("Heater read #%X error: %d\n", HM_SET_T_BOILER, _err);
		return;
	}
	target_boiler_temp = d;
//	_err = Modbus.readHoldingRegisters16(HEATER_MODBUS_ADDR, HM_SET_POWER, &d);
//	if(_err) {
//		journal.jprintf("Heater read #%X error: %d\n", HM_SET_T_BOILER, _err);
//		return;
//	}
//	PowerMaxCurrent = d;
//	uint8_t t = (HP.Prof.Heat.tempPID + 50) / 100;
//	if(GETBIT(HP.Prof.SaveON.flags, fHeat_UseHeater) && curr_temp != t) {
//		_err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_T_FlowOut, t * 10);
//		if(_err) {
//			err_num_total++;
//			journal.jprintf("Heater write #%X error: %d\n", HM_SET_T_FlowOut, _err);
//			return;
//		}
//		curr_temp = HP.Prof.Heat.tempPID;
//	}
//	t = (HP.Prof.Boiler.TempTarget + 50) / 100;
//	if(GETBIT(set.setup_flags, fHeater_BoilerModeByExtTemp) && GETBIT(HP.Prof.SaveON.flags, fBoiler_UseHeater) && curr_boiler_temp != t) {
//		_err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_T_BOILER, t);
//		if(_err) {
//			err_num_total++;
//			journal.jprintf("Heater write #%X error: %d\n", HM_SET_T_BOILER, _err);
//			return;
//		}
//		curr_boiler_temp = t;
//	}
	SETBIT1(fwork, fHeater_LinkAdapterOk);
}

// rise_error = false, не генерить ошибку, если что
void devHeater::Heater_Stop(bool rise_error)
{
#ifdef USE_HEATER
#ifdef RHEATER
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RHEATER)) HP.dRelay[RHEATER].set_OFF();
#endif
	int8_t err = ERR_CONFIG;
#ifdef HEATER_MODBUS_RELAY_ID
	if(GETBIT(HP.work_flags, fHP_HeaterOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus)) {
			err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, 0);
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
		uint16_t d = 0;
		REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_SET_FLAGS, &d, WRITE_MULTIPLE));
		if(err && testMode == NORMAL) {
			err_num_total++;
			if(rise_error) set_Error(err, (char*)"HeaterStop"); else journal.jprintf("ERROR Stop Heater: %d\n", err);
			return;
		}
	}
#endif
    journal.jprintf(" %s[%s] OFF\n", HEATER_NAME, (char *)codeRet[HP.get_ret()]);
	HP.stopHeater = rtcSAM3X8.unixtime();
	if(GETBIT(HP.work_flags, fHP_HeaterOn)) SETBIT1(HP.work_flags, fHP_HeaterWasOn); else SETBIT0(HP.work_flags, fHP_HeaterWasOn);
	HP.work_flags &= ~((1<<fHP_HeaterOn) | (1<<fHP_CompressorWasOn));
#endif
}

// Только вколючение Котла, кран не переключается
void devHeater::Heater_Start()
{
#ifdef USE_HEATER
	bool _ok = false;
#ifdef RHEATER
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RHEATER)) {
		HP.dRelay[RHEATER].set_ON();
		_ok = true;
	}
#endif
	int8_t err = ERR_CONFIG;
	uint16_t d;
#ifdef HEATER_MODBUS_RELAY_ID
	if(!(GETBIT(HP.work_flags, fHP_HeaterOn)) {
		if(dHeater.set.setup_flags & fHeater_USE_Relay_Modbus) {
			d = 1;
			REPEAT_N(set.Modbus_Attempts, err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, &d, WRITE_COIL);
			if(err) {
				err_num_total++;
				set_Error(err, (char*)__FUNCTION__);
				return;
			} else _ok = true;
		}
	}
#endif
#ifdef HEATER_MODBUS_ADDR
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		d = (HP.get_modWork() & pBOILER) && GETBIT(set.setup_flags, fHeater_BoilerModeByExtTemp) ? (1<<HM_SET_FLAGS_BOILER) : (1<<HM_SET_FLAGS_HEATING);
		REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_SET_FLAGS, &d, WRITE_MULTIPLE));
		if(err && testMode == NORMAL) {
			err_num_total++;
			set_Error(err, (char*)"HeaterStart");
			return;
		} else _ok = true;
	}
#endif
	if(!_ok) {
		set_Error(ERR_CONFIG, (char*)"HeaterStart");
	} else {
		journal.jprintf(" %s[%s] ON\n", HEATER_NAME, (char *)codeRet[HP.get_ret()]);
		HP.startHeater = burner_time_last = rtcSAM3X8.unixtime();
		SETBIT0(HP.work_flags, fHP_CompressorWasOn);
		SETBIT1(HP.work_flags, fHP_HeaterOn);
	}
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
			int8_t err;
			uint16_t d = 1;
			REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, &d, WRITE_COIL);
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
	#ifndef BOILER_R3WAY_BEFORE_HEATER_3WAY
	if(HP.is_heater_on()) {
		Heater_Stop(true);
		journal.jprintf("Heater is ON -> Turn OFF\n");
	}
	WaitPumpOff();
	#endif
#ifdef RH_3WAY
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) HP.dRelay[RH_3WAY].set_OFF();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(GETBIT(HP.work_flags, fHP_HeaterValveOn)) {
		if(GETBIT(dHeater.set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) {
			int8_t err;
			uint16_t d = 0;
			REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, &d, WRITE_COIL);
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
	int8_t err = OK;
	uint16_t r;
	if(group == 0 || !GETBIT(fwork, fHeater_LinkAdapterOk) || !GETBIT(fwork, fHeater_LinkHeaterOk)) { // группа 0 или если нет связи
//		if(curr_temp == 0 && GETBIT(fwork, fHeater_LinkHeaterOk)) {
//			err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, HM_SET_T_FlowOut, 1, &r);
//			if(err == OK) curr_temp = r / 10;
//		} else
		if(GETBIT(fwork, fHeater_ReadErrorFlags)) {
			err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_HEATER_fERRORS, &err_flags, READ_HOLDING);
			if(err == OK) err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_HEATER_ERROR2, &Heater_Error2, READ_HOLDING);
			if(err == OK) SETBIT0(fwork, fHeater_ReadErrorFlags);
		} else {
			REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_ADAPTER_FLAGS, &r, READ_HOLDING));
			if(err == OK) {
				if(GETBIT(r, HM_ADAPTER_FLAGS_bLINK)) SETBIT0(fwork, fHeater_fNotAnswerOnCmd); else SETBIT1(fwork, fHeater_fNotAnswerOnCmd);
				if(GETBIT(r, HM_ADAPTER_FLAGS_bLINK) || testMode != NORMAL) {
					SETBIT0(fwork, fHeater_CmdNotResponse);
					SETBIT1(fwork, fHeater_LinkHeaterOk);
				} else {
					if(GETBIT(set.setup_flags, fHeater_Log) && GETBIT(fwork, fHeater_LinkHeaterOk)) journal.jprintf_time("#Heater: NO LINK (%d)\n", r);
					if(GETBIT(fwork, fHeater_CmdNotResponse)) {
						SETBIT0(fwork, fHeater_LinkHeaterOk);
						if(HP.is_heater_on()) {
							set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
						}
					} else SETBIT1(fwork, fHeater_CmdNotResponse);
				}
				if(data.Error) SETBIT1(fwork, fHeater_ReadErrorFlags);
				else {
					Heater_Error2 = 0;
					err_flags = 0;
				}
			}
		}
	} else {
		err = devModbus::ReadHoldingRegisters2(HEATER_MODBUS_ADDR, HM_START_DATA, sizeof(data)/sizeof(uint16_t), (uint16_t*)&data);
		if(err == OK) {
			uint16_t _status = (HP.get_modWork() & pBOILER) && GETBIT(set.setup_flags, fHeater_BoilerModeByExtTemp) ? GETBIT(data.Status, HM_STATUS_bBOILER) : GETBIT(data.Status, HM_STATUS_bHEATING);
			if(HP.is_heater_on()) {
				uint32_t t = rtcSAM3X8.unixtime();
				if(!_status) {
					if(t - HP.startHeater > HEATER_WAIT_CMD_COMPLETION) {
						set_Error(ERR_HEATER_STOP, (char*)__FUNCTION__);
						return ERR_HEATER_STOP;
					}
				} else if(GETBIT(data.Status, HM_STATUS_bBURNER)) {
					burner_time_last = t;
				} else if(t - burner_time_last > HEATER_WAIT_BURNER_TIME_MAX){ // проверить включился ли нагрев
					set_Error(ERR_HEATER_NOT_BURN, (char*)__FUNCTION__);
				}
			} else if(_status && rtcSAM3X8.unixtime() - HP.stopHeater > HEATER_WAIT_CMD_COMPLETION) { // Работает, а не должен
				if(HP.get_State() != pOFF_HP) {
					r = 0;
					REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_ADDR, HM_SET_FLAGS, &r, WRITE_MULTIPLE));
					journal.jprintf_time("Heater is ON! Turn OFF!\n");
				}
			}
		}
	}
	if(err) {
		if(testMode != NORMAL && testMode != HARD_TEST) {
			SETBIT1(fwork, fHeater_LinkAdapterOk);
			err = OK;
		} else {
			if(GETBIT(set.setup_flags, fHeater_Log)) journal.jprintf_time("#Heater(%d): Error %d\n", group, err);
			err_num_total++;
			if(++err_num > HEATER_ADAPTER_ERRORS_MAX) {
				SETBIT0(fwork, fHeater_LinkAdapterOk);
				if(HP.is_heater_on()) {
					set_Error(ERR_HEATER_ADAPTER_LINK, (char*)__FUNCTION__);
				}
			}
		}
	} else {
		SETBIT1(fwork, fHeater_LinkAdapterOk);
		err_num = 0;
	}
	return err;
}

// Установить целевую температуру в сотых градусах (целевые регистры зависят от задачи нагрев/бойлер)
int8_t devHeater::set_target(int16_t temp)
{
	if(temp == 0 || GETBIT(set.setup_flags, fHeater_DontSetFlowTemp) || (testMode != NORMAL && testMode != HARD_TEST)) return OK;
	temp = (temp + 50) / 100;
	int8_t err = OK;
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		uint16_t reg1 = 0;
		switch (testMode) // РЕАЛЬНЫЕ Действия в зависимости от режима
		{
		case NORMAL:
		case HARD_TEST:
			if(GETBIT(set.setup_flags, fHeater_BoilerModeByExtTemp) && (HP.get_modWork() & pBOILER)) {
				if(target_boiler_temp == temp) return OK;
				reg1 = HM_SET_T_BOILER;
				journal.jprintf(" Set Heater%s: %d C\n", " boiler", temp);
			} else {
				if(target_temp == temp) return OK;
				reg1 = HM_SET_T_FlowOut; // регистр в десятых градуса
				journal.jprintf(" Set Heater%s: %d C\n", "", temp);
				temp *= 10;
			}
			REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_ADDR, reg1, &temp, WRITE_MULTIPLE));
			if(err) {
				err_num_total++;
				set_Error(err, (char*)"HeaterTarget");
				break;
			}
//			int16_t status;
//			_delay(HEATER_ADAPTER_WAIT_WRITE); // ожидание
//			REPEAT_N(set.Modbus_Attempts, err = devModbus::Process2(HEATER_MODBUS_ADDR, 0x30 + reg1, (uint16_t*)&status, READ_HOLDING);	// Получить регистр состояния записи
//			if(err) {
//				err_num_total++;
//				set_Error(err, (char*)"HeaterR");
//				break;
//			}
//			if(status != 0) {
//				journal.jprintf("OT write #%d err: %d\n", reg1, status);
//				set_Error(ERR_HEATER_ADAPTER_LINK, (char*)__FUNCTION__);
//			} else curr_temp = temp;
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
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		return GETBIT(data.Status, HM_STATUS_bBURNER);
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
		if(data.P_OUT) buf += m_snprintf(buf, 256, "1A|Давление в контуре, бар|%.1d;", data.P_OUT);
		if(data.F_Boiler) buf += m_snprintf(buf, 256, "1B|Текущий расход ГВС, л/м|%.1d;", data.F_Boiler);
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
		strcat(buf, "||;");
		buf += strlen(buf);
		if(GETBIT(HP.work_flags, fHP_Heater_Heating_pipes)) buf += m_snprintf(buf, 256, "|Идет прогрев трассы|;");
		if(GETBIT(HP.work_flags, fHP_Heater_HeatFloorDelayed)) buf += m_snprintf(buf, 256, "|Идет ожидание ТП|;");
	}
}

void devHeater::DumpJournal(void)
{
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		journal.jprintf(" Heater:%X M:%d%% tF:%d tB:%d ", data.Status, data.Power, data.T_FlowOut / 10, data.T_Boiler / 10);
		journal.jprintf("P:%.1d E:%d,%d\n", data.P_OUT, data.Error, Heater_Error2);
	}
}

// Получить параметр в виде строки, результат ДОБАВЛЯЕТСЯ в ret, если ошибка возврат false
bool devHeater::get_param(char *var, char *ret)
{
	ret += strlen(ret);
	if(strcmp(var, WHeater_LinkHeaterOk)==0)			{ if(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(fwork, fHeater_LinkHeaterOk)) {
																strcat(ret, "Ok");
																if(GETBIT(fwork, fHeater_CmdNotResponse)) strcat(ret, "?");
																if(GETBIT(fwork, fHeater_fNotAnswerOnCmd)) strcat(ret, "!");
															} else strcat(ret, "Нет"); } else
	if(strcmp(var, WHeater_fLinkAdapterOk)==0)			{ _itoa(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(fwork, fHeater_LinkAdapterOk), ret); } else
	if(strcmp(var, WHeater_is_on)==0) 					{ _itoa(GETBIT(set.setup_flags, fHeater_Opentherm) && GETBIT(fwork, fHeater_LinkHeaterOk) ? GETBIT(data.Status, HM_STATUS_bBURNER) : GETBIT(HP.work_flags, fHP_HeaterOn), ret); } else
	if(strcmp(var, WHeater_is_on_set)==0) 				{ _itoa(GETBIT(HP.work_flags, fHP_HeaterOn), ret); } else
	if(strcmp(var, option_Control_Period)==0) 			{ _itoa(set.Control_Period, ret); } else
	if(strcmp(var, WHeater_3way)==0) 					{ strcat(ret, HP.is_heater_active() ? "Котел" : "ТН"); } else
	if(strcmp(var, WHeater_T_FlowOut)==0) 				{ _dtoa(ret, data.T_FlowOut / 10, 0); strcat(ret, " ("); _itoa(target_temp, ret); strcat(ret, ")"); } else
	if(strcmp(var, WHeater_Power)==0) 					{ if(!GETBIT(fwork, fHeater_LinkHeaterOk)) strcat(ret, "-"); else if(data.Power==0) strcat(ret, "Выкл"); else { _itoa(data.Power, ret); strcat(ret, "%"); } } else
	if(strcmp(var, WHeater_err_num_total)==0)			{ _itoa(err_num_total, ret); } else
	if(strcmp(var, WHeater_fHeater_Opentherm)==0)		{ if(GETBIT(set.setup_flags, fHeater_Opentherm)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_RHEATER)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RHEATER)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_RH_3WAY)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_Modbus)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_Heating_Pipes)==0){ if(GETBIT(set.setup_flags, fHeater_Heating_Pipes)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_Heating_Pipes_Temp)==0){ if(GETBIT(set.setup_flags, fHeater_Heating_Pipes_Temp)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_BoilerModeByExtTemp)==0){ if(GETBIT(set.setup_flags, fHeater_BoilerModeByExtTemp)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_DontSetFlowTemp)==0)	{ if(GETBIT(set.setup_flags, fHeater_DontSetFlowTemp)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_fHeater_Log)==0)				{ if(GETBIT(set.setup_flags, fHeater_Log)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, WHeater_wait_heating_pipes_time)==0)	{ _itoa(set.wait_heating_pipes_time * 4, ret); } else
	if(strcmp(var, WHeater_wait_heating_pipes_time_max)==0){ _itoa(set.wait_heating_pipes_time_max * 4, ret); } else
	if(strcmp(var, WHeater_HeatingPipesAddTemp)==0)		{ _itoa(set.HeatingPipesAddTemp, ret); } else
	if(strcmp(var, WHeater_HeatFloorAddTemp)==0)		{ _itoa(set.HeatFloorAddTemp, ret); } else
	if(strcmp(var, WHeater_target_temp)==0)				{ _itoa(target_temp, ret); } else
//	if(strcmp(var, WHeater_heat_tempout)==0) 			{ _itoa(set.heat_tempout, ret); } else
//	if(strcmp(var, WHeater_heat_power_min)==0) 			{ _itoa(set.heat_power_min, ret); } else
//	if(strcmp(var, WHeater_heat_power_max)==0) 			{ _itoa(set.heat_power_max, ret); } else
//	if(strcmp(var, WHeater_heat_protect_temp_dt)==0)	{ _dtoa(ret, set.heat_protect_temp_dt, 1); } else
//	if(strcmp(var, WHeater_boiler_tempout)==0)			{ _itoa(set.boiler_tempout, ret); } else
//	if(strcmp(var, WHeater_power_boiler_min)==0) 		{ _itoa(set.boiler_power_min, ret); } else
//	if(strcmp(var, WHeater_power_boiler_max)==0) 		{ _itoa(set.boiler_power_max, ret); } else
//	if(strcmp(var, WHeater_boiler_protect_temp_dt)==0)	{ _dtoa(ret, set.boiler_protect_temp_dt, 1); } else
	if(strcmp(var, WHeater_pump_work_time_after_stop)==0){ _itoa(set.pump_work_time_after_stop * 10, ret); } else
	if(strcmp(var, WHeater_fHP_Heater_Heating_pipes)==0){ if(GETBIT(HP.work_flags, fHP_Heater_Heating_pipes)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, option_ModbusMinTimeBetweenTransaction)==0){ _itoa(set.ModbusMinTimeBetweenTransaction, ret); } else
	if(strcmp(var, option_ModbusResponseTimeout)==0){ _itoa(set.ModbusResponseTimeout, ret); } else
	if(strcmp(var, option_ModbusWriteResponseTimeout)==0){ _itoa(set.ModbusWriteResponseTimeout, ret); } else
	if(strcmp(var, W_Modbus_Attempts)==0){ _itoa(set.Modbus_Attempts, ret); } else
	return false;

	return true;
}

// Установить параметр из строки из web, возврат ошибки
int8_t devHeater::set_param(char *var, float f)
{
	int32_t x = f;
	if(strcmp(var, WHeater_fHeater_Opentherm)==0)		{ if(x) SETBIT1(set.setup_flags, fHeater_Opentherm); else SETBIT0(set.setup_flags, fHeater_Opentherm); return OK;} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_RHEATER)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RHEATER); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RHEATER); return OK;} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_RH_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RH_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RH_3WAY); return OK;} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_Modbus)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus); return OK;} else
	if(strcmp(var, WHeater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); return OK;} else
	if(strcmp(var, WHeater_fHeater_Heating_Pipes)==0)	{ if(x) SETBIT1(set.setup_flags, fHeater_Heating_Pipes); else SETBIT0(set.setup_flags, fHeater_Heating_Pipes); return OK;} else
	if(strcmp(var, WHeater_fHeater_Heating_Pipes_Temp)==0){ if(x) SETBIT1(set.setup_flags, fHeater_Heating_Pipes_Temp); else SETBIT0(set.setup_flags, fHeater_Heating_Pipes_Temp); return OK;} else
	if(strcmp(var, WHeater_fHeater_BoilerModeByExtTemp)==0){ if(x) SETBIT1(set.setup_flags, fHeater_BoilerModeByExtTemp); else SETBIT0(set.setup_flags, fHeater_BoilerModeByExtTemp); return OK;} else
	if(strcmp(var, WHeater_fHeater_DontSetFlowTemp)==0)	{ if(x) SETBIT1(set.setup_flags, fHeater_DontSetFlowTemp); else SETBIT0(set.setup_flags, fHeater_DontSetFlowTemp); return OK;} else
	if(strcmp(var, WHeater_fHeater_Log)==0){ if(x) SETBIT1(set.setup_flags, fHeater_Log); else SETBIT0(set.setup_flags, fHeater_Log); return OK;} else
	if(strcmp(var, WHeater_wait_heating_pipes_time)==0)	{ set.wait_heating_pipes_time = x / 4; return OK; } else
	if(strcmp(var, WHeater_wait_heating_pipes_time_max)==0){ set.wait_heating_pipes_time_max = x / 4; return OK; } else
	if(strcmp(var, WHeater_HeatingPipesAddTemp)==0)		{ set.HeatingPipesAddTemp = x; return OK; } else
	if(strcmp(var, WHeater_HeatFloorAddTemp)==0)		{ set.HeatFloorAddTemp = x; return OK; } else
	if(strcmp(var, option_Control_Period)==0)			{ if(x>0){ set.Control_Period = x; return OK; } } else
//	if(strcmp(var, WHeater_heat_tempout)==0)			{ if(x>=0 && x<=100){ set.heat_tempout = x; return OK; } } else
//	if(strcmp(var, WHeater_heat_power_min)==0)			{ if(x>=0 && x<=100){ set.heat_power_min = x; return OK; } } else
//	if(strcmp(var, WHeater_heat_power_max)==0)			{ if(x>=0 && x<=100){ set.heat_power_max = x; return OK; } } else
//	if(strcmp(var, WHeater_heat_protect_temp_dt)==0)	{ set.heat_protect_temp_dt = rd(x, 10); return OK; } else
//	if(strcmp(var, WHeater_boiler_tempout)==0)			{ if(x>=0 && x<=100){ set.boiler_tempout = x; return OK; } } else
//	if(strcmp(var, WHeater_power_boiler_min)==0)		{ if(x>=0 && x<=100){ set.boiler_power_min = x; return OK; } } else
//	if(strcmp(var, WHeater_power_boiler_max)==0)		{ if(x>=0 && x<=100){ set.boiler_power_max = x; return OK; } } else
//	if(strcmp(var, WHeater_boiler_protect_temp_dt)==0)	{ set.boiler_protect_temp_dt = rd(x, 10); return OK; } else
	if(strcmp(var, WHeater_pump_work_time_after_stop)==0){ set.pump_work_time_after_stop = x / 10; return OK; } else
	if(strcmp(var, option_ModbusMinTimeBetweenTransaction)==0){ set.ModbusMinTimeBetweenTransaction = RS485_2.ModbusMinTimeBetweenTransaction = x; return OK; } else
	if(strcmp(var, option_ModbusResponseTimeout)==0)	{ set.ModbusResponseTimeout = RS485_2.ModbusResponseTimeout = x; return OK; } else
	if(strcmp(var, option_ModbusWriteResponseTimeout)==0){ set.ModbusWriteResponseTimeout = x; return OK; } else
	if(strcmp(var, WHeater_is_on_set)==0){ if(x && !GETBIT(HP.work_flags, fHP_HeaterOn)) Heater_Start(); else if(!x && GETBIT(HP.work_flags, fHP_HeaterOn)) Heater_Stop(true); return OK;} else
	if(strcmp(var, WHeater_fHP_Heater_Heating_pipes)==0){ if(x) SETBIT1(HP.work_flags, fHP_Heater_Heating_pipes); else SETBIT0(HP.work_flags, fHP_Heater_Heating_pipes); return OK; } else
	if(strcmp(var, WHeater_target_temp)==0)	{ if(x != target_temp) { SemaphoreGive(xWebThreadSemaphore); set_target(x*100); SemaphoreTake(xWebThreadSemaphore, W5200_TIME_WAIT); } return OK; } else
	if(strcmp(var, W_Modbus_Attempts)==0){ set.Modbus_Attempts = x > 0 ? x : 1; return OK; }

	return 27;
}

#endif
