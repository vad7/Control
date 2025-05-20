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

int8_t devHeater::init()
{
	err = OK;
	err_num = 0;
	err_num_total = 0;
	Target = 0;
	work_flags = 0;

	set.setup_flags = (0<<fHeater_Opentherm) | (1<<fHeater_USE_Relay_RHEATER) | (1<<fHeater_USE_Relay_RH_3WAY) | (0<<fHeater_USE_Relay_Modbus) | (0<<fHeater_USE_Relay_Modbus_3WAY);
	set.power_start = 100;
	set.power_max = 100;
	set.power_boiler_start = 100;
	set.power_boiler_max = 100;
	set.pump_work_time_after_stop = 18;			// 3 минуты

	/*
	state = 0;
	CheckLinkStatus();
	if(err != OK) return err;
	uint8_t i = 3;
	while((flags & fHeater_Work))// Если работает, то остановить его
	{
		stop();
		journal.jprintf("Wait stop %s...\n", name);
		_delay(3000);
		CheckLinkStatus(); //  Получить состояние частотника
	}
	if(i == 0) { // Не останавливается
		err = ERR_MODBUS_STATE;
		set_Error(err, name);
	}
	*/
	return err;
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
			journal.jprintf("OT write err: %d\n", status);
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
		err = Modbus.readHoldingRegistersNNR(HEATER_MODBUS_ADDR, 0x30 + HM_SET_FLAGS, 1, (uint16_t*)&status);
		if(err) {
			set_Error(err, (char*)__FUNCTION__);
			return;
		}
		if(status != 0) {
			journal.jprintf("OT write err: %d\n", status);
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
	HP.dRelay[RH_3WAY].set_ON();
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
	if(!GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) {
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
	HP.dRelay[RH_3WAY].set_OFF();
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
	if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) {
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
// Вызывается из задачи чтения датчиков период
int8_t devHeater::read_state()
{
/*
	if(testMode != NORMAL && testMode != HARD_TEST) {
		err = OK;
	} else {
		state = read_0x03_16(FC_STATUS); // прочитать состояние
		if(err) // Ошибка
		{
			state = ERR_LINK_FC; // признак потери связи с инвертором
			power = FC_curr_freq = 0;
			if(!GETBIT(flags, fOnOff)) return err; // Если не работаем, то и ошибку не генерим
			SETBIT1(flags, fErrFC); // Блок инвертора
			set_Error(err, name); // генерация ошибки
			return err; // Возврат
		}
		if(GETBIT(flags, fOnOff)) { // ТН включил компрессор, проверяем состояние инвертора
			if(state & FC_S_FLT) { // Действующий отказ
				uint16_t reg = read_0x03_16(FC_ERROR);
				journal.jprintf("%s, Fault: %s(%d) - ", name, reg ? get_fault_str(reg) : cError, reg ? reg : err);
				err = ERR_FC_FAULT;
				if(HP.NO_Power) return err;
				if(GETBIT(HP.Option.flags, fBackupPower)) {
//					HP.sendCommand(pWAIT);
//					SETBIT1(HP.flags, fHP_BackupNoPwrWAIT);
//					return err;
					// Нужно перезапустить
					if(HP.is_compressor_on()) {
						if(get_present()) stop_FC(); else HP.dRelay[RCOMP].set_OFF();
						HP.set_stopCompressor();
					}
					HP.sendCommand(pREPEAT_FAST);
					return err;
				}
			} else if(get_startTime() && rtcSAM3X8.unixtime() - get_startTime() > FC_ACCEL_TIME / 100 && ((state & (FC_S_RDY | FC_S_RUN | FC_S_DIR)) != (FC_S_RDY | FC_S_RUN))) {
				err = ERR_MODBUS_STATE;
			}
			if(err) {
				set_Error(err, name); // генерация ошибки
				return err;
			}
		} else if(state & FC_S_RUN) { // Привод работает, а не должен - останавливаем через модбас
			if(rtcSAM3X8.unixtime() - HP.get_stopCompressor() > FC_DEACCEL_TIME / 100) {
				journal.jprintf_time("Compressor running - stop via Modbus!\n");
				err = write_0x06_16(FC_CONTROL, FC_C_STOP); // подать команду ход/стоп через модбас
				if(err) return err;
			}
		}
		// Состояние прочитали и оно правильное все остальное читаем
		{
			FC_curr_freq = read_0x03_16(FC_FREQ) * FC_FREQ_MUL; 	// прочитать текущую частоту
			if(err == OK) {
				power = read_0x03_16(FC_POWER); 	// прочитать мощность
				if(err == OK) {
					FC_Temp = read_0x03_16(FC_TEMP);
					if(err == OK && _data.FC_MaxTemp && FC_Temp > _data.FC_MaxTemp) {
						set_Error(err = ERR_MODBUS_VACON_TEMP, name); // генерация ошибки
						return err;
					}
					if(_data.FC_TargetTemp) {
						if(FC_Temp > _data.FC_TargetTemp) write_0x06_16(FC_CONTROL, FC_C_COOLER_FAN); // подать команду
						else if(FC_Temp < _data.FC_TargetTemp - FC_COOLER_FUN_HYSTERESIS) write_0x06_16(FC_CONTROL, 0); // подать команду
					}
				}
				if(GETBIT(_data.setup_flags,fLogWork) && GETBIT(flags, fOnOff)) {
					journal.jprintf_time("FC: %Xh,%.2dHz,%.3d\n", state, FC_curr_freq, get_power());
				}
			}
		}
		if(GETBIT(flags, fOnOff) && err == OK) {
			err = write_0x06_16((uint16_t) FC_SET_SPEED, FC_target);
		}
	}
	*/
	return err;
}

// Установить целевую мощность в %
int8_t devHeater::set_target(uint8_t start, uint8_t max)
{
	if(GETBIT(set.setup_flags, fHeater_Opentherm)) {
		switch (testMode) // РЕАЛЬНЫЕ Действия в зависимости от режима
		{
		case NORMAL:
		case HARD_TEST:

			break;
		default:
			break; //  Ничего не включаем
		}
		journal.jprintf(" Set Heater: %.2d%%..%.2d%%\n", start, max);
	}
	return err;
}

// Команда начала нагрева
// Может быть подана команда через реле и через модбас в зависимости от ключей компиляции
int8_t devHeater::start()
{
	/*
    if(testMode != NORMAL && testMode != HARD_TEST) {
        err = OK;
    	goto xStarted;
    }
	if(!get_present() || GETBIT(flags, fErrFC)) return err = ERR_MODBUS_BLOCK; // выходим если нет инвертора или он заблокирован по ошибке
    err = OK;
    // set_target(FC_START_FC,true);  // Запись в регистр инвертора стартовой частоты  НЕ всегда скорость стартовая - супербойлер
	if((state & FC_S_FLT)) { // Действующий отказ
		uint16_t reg = read_0x03_16(FC_ERROR);
		journal.jprintf("%s, Fault: %s(%d) - ", name, reg ? get_fault_str(reg) : cError, reg ? reg : err);
		if(GETBIT(_data.setup_flags, fAutoResetFault)) { // Автосброс не критичного сбоя инвертора
			if(!err) { // код считан успешно
				uint8_t i = 0;
				for(; i < sizeof(FC_NonCriticalFaults); i++) if(FC_NonCriticalFaults[i] == reg) break;
				if(i < sizeof(FC_NonCriticalFaults))
				{
					if(reset_errorFC()) {
						if((state & FC_S_FLT)) {
							if(reset_errorFC() && (state & FC_S_FLT)) err = ERR_FC_FAULT; else journal.jprintf("Reseted(2).\n");
						} else journal.jprintf("Reseted.\n");
					}
					if(err) journal.jprintf("reset failed!\n");
				} else {
					journal.jprintf("Critical!\n");
					err = ERR_FC_FAULT;
				}
			}
		} else {
			journal.jprintf("skip start!\n");
			err = ERR_FC_FAULT;
		}
	}
	if(err == OK) {
	    set_nominal_power();
  #ifdef FC_USE_RCOMP // Использовать отдельный провод для команды ход/стоп
		HP.dRelay[RCOMP].set_ON();
  #else
		uint16_t ctrl = read_0x03_16(FC_CONTROL);
		if(!err) err = write_0x06_16(FC_CONTROL, (ctrl & FC_C_COOLER_FAN) | FC_C_RUN); // Команда Ход
  #endif
	}
    if(err == OK) {
xStarted:
    	SETBIT1(flags, fOnOff);
        set_startCompressor();
        journal.jprintf(" %s[%s] ON\n", name, (char *)codeRet[HP.get_ret()]);
    } else {
        SETBIT1(flags, fErrFC);
        set_Error(err, name);  // генерация ошибки
    }
//  FC_ANALOG_CONTROL
    if((testMode == NORMAL) || (testMode == HARD_TEST)) //   Режим работа и хард тест, все включаем,
    {
  #ifdef FC_USE_RCOMP // Использовать отдельный провод для команды ход/стоп
        HP.dRelay[RCOMP].set_ON(); // ПЛОХО через глобальную переменную
  #else
        err = ERR_FC_CONF_ANALOG;
        SETBIT1(flags, fErrFC);
        set_Error(err, name); // Ошибка конфигурации
  #endif
  #ifdef FC_ANALOG_OFF_SET_0
        dac = (int32_t)FC_target * (_data.level100 - _data.level0) / (100*100) + _data.level0;
        analogWrite(pin, dac);
  #endif
    }
xStarted:
    SETBIT1(flags, fOnOff);
        set_startCompressor();
    journal.jprintf(" %s ON\n", name);
    */
    return err;
}

// Команда стоп на инвертор Обратно код ошибки
int8_t devHeater::stop()
{
	/*
    if((testMode == NORMAL) || (testMode == HARD_TEST)) // Режим работа и хард тест, все включаем,
    {
    	if(!get_present()) {
            SETBIT0(flags, fOnOff);
            startCompressor = 0;
            return err; // выходим если нет инвертора или нет связи
    	}
  #ifdef FC_USE_RCOMP // Использовать отдельный провод для команды ход/стоп
        HP.dRelay[RCOMP].set_OFF();
  #endif
    	if(state == ERR_LINK_FC) {
            SETBIT0(flags, fOnOff);
            startCompressor = 0;
            return err; // выходим если нет инвертора или нет связи
    	}
  #ifndef FC_USE_RCOMP
		uint16_t ctrl = read_0x03_16(FC_CONTROL);
		if(!err) err = write_0x06_16(FC_CONTROL, (ctrl & FC_C_COOLER_FAN) | FC_C_STOP); // подать команду ход/стоп через модбас
  #else
        err = OK;
  #endif
    }
    if(err == OK || err == ERR_NO_POWER_WHILE_WORK) {
        journal.jprintf(" %s[%s] OFF\n", name, (char *)codeRet[HP.get_ret()]);
        SETBIT0(flags, fOnOff);
        startCompressor = 0;
    } else {
        SETBIT1(flags, fErrFC);
        set_Error(err, name);
    } // генерация ошибки
// FC_ANALOG_CONTROL
    if((testMode == NORMAL) || (testMode == HARD_TEST)) // Режим работа и хард тест, все включаем,
    {
  #ifdef FC_USE_RCOMP // Использовать отдельный провод для команды ход/стоп
        HP.dRelay[RCOMP].set_OFF();
  #else // подать команду ход/стоп через модбас
        err = ERR_FC_CONF_ANALOG;
        SETBIT1(flags, fErrFC);
        set_Error(err, name); // Ошибка конфигурации
  #endif
  #ifdef FC_ANALOG_OFF_SET_0
        analogWrite(pin, FC_target = dac = 0);
  #endif
    }
    SETBIT0(flags, fOnOff);
    startCompressor = 0;
    journal.jprintf(" %s OFF\n", name);
    */
	return err;
}

// Получить информацию
void devHeater::get_info(char* buf)
{
	buf += strlen(buf);
	if(!GETBIT(work_flags, fHeater_LinkOk)) {   // Нет связи
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
	if(strcmp(var, Wheater_power_on)==0) 					{ _itoa( GETBIT(set.setup_flags, fHeater_Opentherm) ? GETBIT(work_flags, fHeater_PowerOn) : 1, ret); } else
	if(strcmp(var, Wheater_is_on)==0) 					    { _itoa(GETBIT(HP.work_flags, fHP_HeaterOn), ret); } else
	if(strcmp(var, Wheater_3way)==0) 					    { strcat(ret, GETBIT(HP.work_flags, fHP_HeaterValveOn) ? "Котел" : "ТН"); } else
	if(strcmp(var, Wheater_T_Flow)==0) 					    { if(GETBIT(work_flags, fHeater_LinkOk)) _dtoa(ret, data.Power, 1); else strcat(ret, "-"); } else
	if(strcmp(var, Wheater_Power)==0) 					    { if(GETBIT(work_flags, fHeater_LinkOk)) _itoa(data.T_Flow, ret); else strcat(ret, "-"); } else
	if(strcmp(var, Wheater_Errors)==0) 					    { _itoa(err_num_total, ret); } else
	if(strcmp(var, Wheater_fHeater_Opentherm)==0)			{ if(GETBIT(set.setup_flags, fHeater_Opentherm)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RHEATER)==0)	{ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RHEATER)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RH_3WAY)==0)	{ if(GETBIT(set.setup_flags, fHeater_USE_Relay_RH_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus)==0)	{ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(GETBIT(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY)) strcat(ret,(char*)cOne); else strcat(ret,(char*)cZero);} else
	if(strcmp(var, Wheater_power_start)==0) 				{ _itoa(set.power_start, ret); } else
	if(strcmp(var, Wheater_power_max)==0) 					{ _itoa(set.power_max, ret); } else
	if(strcmp(var, Wheater_power_boiler_start)==0)			{ _itoa(set.power_boiler_start, ret); } else
	if(strcmp(var, Wheater_power_boiler_max)==0) 			{ _itoa(set.power_boiler_max, ret); } else
	if(strcmp(var, Wheater_pump_work_time_after_stop)==0) 	{ _itoa(set.pump_work_time_after_stop * 10, ret); } else
	if(strcmp(var, "INFO")==0) 					    		get_info(ret); else

    strcat(ret,(char*)cInvalid);
}

// Установить параметр из строки
boolean devHeater::set_param(char *var, float f)
{
	int32_t x = f;
	if(strcmp(var, Wheater_fHeater_Opentherm)==0)		{ if(x) SETBIT1(set.setup_flags, fHeater_Opentherm); else SETBIT0(set.setup_flags, fHeater_Opentherm); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RHEATER)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RHEATER); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RHEATER); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_RH_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_RH_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_RH_3WAY); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus); return true;} else
	if(strcmp(var, Wheater_fHeater_USE_Relay_Modbus_3WAY)==0){ if(x) SETBIT1(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); else SETBIT0(set.setup_flags, fHeater_USE_Relay_Modbus_3WAY); return true;} else
	if(strcmp(var, Wheater_power_start)==0)				{ if(x>=0 && x<=100){ set.power_start = x; return true; } } else
	if(strcmp(var, Wheater_power_max)==0)				{ if(x>=0 && x<=100){ set.power_max = x; return true; } } else
	if(strcmp(var, Wheater_power_boiler_start)==0)		{ if(x>=0 && x<=100){ set.power_boiler_start = x; return true; } } else
	if(strcmp(var, Wheater_power_boiler_max)==0)		{ if(x>=0 && x<=100){ set.power_boiler_max = x; return true; } } else
	if(strcmp(var, Wheater_pump_work_time_after_stop)==0){ if(x>=0 && x<=100){ set.pump_work_time_after_stop = x / 10; return true; } }

    return false;
}

#endif
