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
	flags = 0;
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

	HP.stopHeater = rtcSAM3X8.unixtime();
#endif
}

void devHeater::Heater_Start()
{
#ifdef USE_HEATER
	if(!GETBIT(HP.work_flags, fHP_HeaterValveOn)) HeaterValve_On();
#ifdef RHEATER
	if(set.setup_flags & fHeater_USE_Relay_RHEATER) HP.dRelay[RHEATER].set_ON();
#endif
#ifdef HEATER_MODBUS_RELAY_ID
	if(!(work_flags & fHP_HeaterOn)) {
		if(dHeater.set.setup_flags & fHeater_USE_Relay_Modbus) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_RELAY_ADDR, HEATER_MODBUS_RELAY_ID, 1);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
	if((set.setup_flags & fHeater_Opentherm)) {
		int8_t err = Modbus.writeHoldingRegistersN1R(HEATER_MODBUS_ADDR, HM_SET_FLAGS, HM_SET_FLAGS_HEATING);
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
			journal.jprintf("OT write err: %d", status);
			set_Error(ERR_HEATER_LINK, (char*)__FUNCTION__);
			return;
		}
	}
	SETBIT1(HP.work_flags, fHP_HeaterOn);
	HP.startHeater = rtcSAM3X8.unixtime();
#endif
}

// Переключиться на котел, если еще нет
void devHeater::HeaterValve_On()
{
#ifdef RH_3WAY
	if(set.setup_flags & fHeater_USE_Relay_RH_3WAY) HP.dRelay[RH_3WAY].set_ON();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(!(work_flags & fHP_HeaterValveOn)) {
		if(dHeater.set.setup_flags & fHeater_USE_Relay_Modbus_3WAY) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, 1);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
#if defined(RH_3WAY) || defined(HEATER_MODBUS_3WAY_ID)
	_delay(HP.Option.SwitchHeaterHPTime * 1000);	// Ожидание переключения
	SETBIT1(HP.work_flags, fHP_HeaterValveOn);
#endif
}

// Переключиться на ТН, если еще нет
void devHeater::HeaterValve_Off()
{
#ifdef USE_HEATER
	int32_t d = set.pump_work_time_after_stop * 10;
	if(HP.is_heater_on()) {
		Heater_Stop();
		journal.jprintf("Heater is ON -> Turn off");
	} else {
		d = set.pump_work_time_after_stop * 10 - (rtcSAM3X8.unixtime() - HP.stopHeater);
		if(d > 0) journal.jprintf("Wait Heater pump stop");
	}
	for(; d > 0; d--) { // задержка перед включением компрессора
		_delay(1000);
		if(HP.get_errcode() || HP.is_next_command_stop() || HP.get_State() == pSTOPING_HP) return; // прерваться по ошибке или по команде
	}
#ifdef RH_3WAY
	if(set.setup_flags & fHeater_USE_Relay_RH_3WAY) HP.dRelay[RH_3WAY].set_OFF();
#endif
#ifdef HEATER_MODBUS_3WAY_ID
	if(work_flags & fHP_HeaterValveOn) {
		if(dHeater.set.setup_flags & fHeater_USE_Relay_Modbus_3WAY) {
			int8_t err = Modbus.writeSingleCoilR(HEATER_MODBUS_3WAY_ADDR, HEATER_MODBUS_3WAY_ID, 0);
			if(err) {
				set_Error(err, (char*)__FUNCTION__);
				return;
			}
		}
	}
#endif
#if defined(RH_3WAY) || defined(HEATER_MODBUS_3WAY_ID)
	_delay(HP.Option.SwitchHeaterHPTime * 1000);	// Ожидание переключения
	SETBIT0(HP.work_flags, fHP_HeaterValveOn);
#endif
#endif
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
        startCompressor = rtcSAM3X8.unixtime();
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
    startCompressor = rtcSAM3X8.unixtime();
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

// Получить параметр в виде строки, результат ДОБАВЛЯЕТСЯ в ret
void devHeater::get_param(char *var, char *ret)
{

	ret += m_strlen(ret);
    if(strcmp(var,fc_ON_OFF)==0)                { if (GETBIT(flags,fOnOff))  strcat(ret,(char*)cOne);else  strcat(ret,(char*)cZero); } else

    strcat(ret,(char*)cInvalid);
}

// Установить параметр из строки
boolean devHeater::set_param(char *var, float f)
{
	int16_t x = f;
    if(strcmp(var, fc_ON_OFF)==0)                { if(x == 0) stop(); else start(); return true; } else

	x = rd(f, 100);
//    	if(strcmp(var,fc_DT_COMP_TEMP)==0)          { if(x>=0 && x<2500){set.setup_flags=x;return true; } else return false; } else // градусы


    return false;
}

// Получить информацию
void devHeater::get_info(char* buf)
{
	buf += m_strlen(buf);
	if(testMode == NORMAL || testMode == HARD_TEST) {
		if(!GETBIT(flags, fHeater_LinkOk)) {   // Нет связи
			strcat(buf, "|Данные не доступны - нет связи|;");
		} else {
			/*
			strcat(buf, "16-00|Состояние инвертора: ");
			get_infoFC_status(buf + m_strlen(buf), state);
			buf += m_snprintf(buf += m_strlen(buf), 256, "|0x%X;", state);
			if(err == OK) {
				buf += m_snprintf(buf, 256, "16-01|Фактическая скорость|%d%%;16-10|Выходная мощность (кВт)|%d;", read_0x03_16(FC_SPEED), get_power());
				buf += m_snprintf(buf, 256, "16-17|Обороты (об/м)|%d;", (int16_t)read_0x03_16(FC_RPM));
				buf += m_snprintf(buf, 256, "16-22|Крутящий момент|%d%%;", (int16_t)read_0x03_16(FC_TORQUE));
				buf += m_snprintf(buf, 256, "16-12|Выходное напряжение (В)|%.1d;", (int16_t)read_0x03_16(FC_VOLTAGE));
				buf += m_snprintf(buf, 256, "16-30|Напряжение шины постоянного тока (В)|%d;", (int16_t)read_0x03_16(FC_VOLTAGE_DC));
				buf += m_snprintf(buf, 256, "16-34|Температура радиатора (°С)|%d;", read_tempFC());
				buf += m_snprintf(buf, 256, "15-00|Время включения инвертора (часов)|%d;", read_0x03_32(FC_POWER_HOURS));
				buf += m_snprintf(buf, 256, "15-01|Время работы компрессора (часов)|%d;", read_0x03_32(FC_RUN_HOURS));
				buf += m_snprintf(buf, 256, "15-02|Итого выход кВтч|%d;", read_0x03_16(FC_CNT_POWER));
			}
			*/
		}
	} else {
		m_snprintf(buf, 256, "|Режим тестирования!|;");
	}
}

#endif
