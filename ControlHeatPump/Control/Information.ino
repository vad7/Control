/*
 * Copyright (c) 2016-2021 by Vadim Kulakov vad7@yahoo.com, vad711
 * &                       by Pavel Panfilov <firstlast2007@gmail.com> pav2000
 *
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

//  описание вспомогательных Kлассов данных, предназначенных для получения информации о ТН
#include "Information.h"

#define bufI2C Socket[0].outBuf

// --------------------------------------------------------------------------------------------------------------- 
//  Класс системный журнал пишет в консоль и в память ------------------------------------------------------------
//  Место размещения (озу ли флеш) определяется дефайном #define I2C_EEPROM_64KB
// --------------------------------------------------------------------------------------------------------------- 
// Инициализация
void Journal::Init()
{
	err = OK;
	SemaphoreCreate(Semaphore);
#ifndef I2C_EEPROM_64KB     // журнал в памяти
	bufferTail = 0;
	bufferHead = 0;
	full = 0;                   // Буфер не полный
	memset(_data, 0, JOURNAL_LEN);
	jprintf("\nSTART ---\nInit RAM journal, size %d . . .\n", JOURNAL_LEN);
	return;
#else                      // журнал во флеше

	uint8_t eepStatus = 0;
	uint16_t i;
	char *ptr;

	if((eepStatus = eepromI2C.begin(I2C_SPEED) != 0))  // Инициализация памяти
	{
#ifdef DEBUG
		SerialDbg.println("$ERROR - open I2C journal, check I2C chip!");   // ошибка открытия чипа
#endif
		err = ERR_OPEN_I2C_JOURNAL;
		return;
	}

	if(checkREADY() == false) // Проверка наличия журнал
	{
#ifdef DEBUG
		SerialDbg.print("I2C journal not found! ");
#endif
		Format();
		return;
	}

	bufferTail = bufferHead = -1;
	full = 0;
	for(i = 0; i < JOURNAL_LEN / W5200_MAX_LEN; i++)   // поиск журнала начала и конца, для ускорения читаем по W5200_MAX_LEN байт
	{
		WDT_Restart(WDT);
#ifdef DEBUG
		SerialDbg.print(".");
#endif
		if(readEEPROM_I2C(I2C_JOURNAL_START + i * W5200_MAX_LEN, (byte*) &bufI2C,W5200_MAX_LEN)) {
			err=ERR_READ_I2C_JOURNAL;
#ifdef DEBUG
			SerialDbg.print(errorReadI2C);
#endif
			break;
		}
		if((ptr = (char*) memchr(bufI2C, I2C_JOURNAL_HEAD, W5200_MAX_LEN)) != NULL) {
			bufferHead=i*W5200_MAX_LEN+(ptr-bufI2C);
		}
		if((ptr = (char*) memchr(bufI2C, I2C_JOURNAL_TAIL ,W5200_MAX_LEN)) != NULL) {
			bufferTail=i*W5200_MAX_LEN+(ptr-bufI2C);
		}
		if((bufferTail >= 0) && (bufferHead >= 0)) break;
	}
	if(bufferTail < bufferHead) full = 1;  // Буфер полный
	jprintf("\nSTART ---\nFound I2C journal: size %d bytes, head=%d, tail=%d\n", JOURNAL_LEN, bufferHead, bufferTail);
#endif //  #ifndef I2C_EEPROM_64KB     // журнал в памяти
}

  
#ifdef I2C_EEPROM_64KB  // функции долько для I2C журнала
// Записать признак "форматирования" журнала - журналом можно пользоваться
void Journal::writeREADY()
{  
    uint16_t  w=I2C_JOURNAL_READY; 
    if (writeEEPROM_I2C(I2C_JOURNAL_START-2, (byte*)&w,sizeof(w))) 
       { err=ERR_WRITE_I2C_JOURNAL; 
         #ifdef DEBUG
         SerialDbg.println(errorWriteI2C);
         #endif
        }
}
// Проверить наличие журнала
boolean Journal::checkREADY()
{  
    uint16_t  w=0x0; 
    if (readEEPROM_I2C(I2C_JOURNAL_START-2, (byte*)&w,sizeof(w))) 
       { err=ERR_READ_I2C_JOURNAL; 
         #ifdef DEBUG
         SerialDbg.print(errorReadI2C);
         #endif
        }
    if (w!=I2C_JOURNAL_READY) return false; else return true;
}

// Форматирование журнала (инициализация I2C памяти уже проведена), sizeof(buf)=W5200_MAX_LEN
void Journal::Format(void)
{
	err = OK;
	#ifdef DEBUG
	printf("Formating I2C journal ");
	#endif
//	memset(buf, I2C_JOURNAL_FORMAT, W5200_MAX_LEN);
//	for(uint32_t i = 0; i < JOURNAL_LEN / W5200_MAX_LEN; i++) {
//		#ifdef DEBUG
//		SerialDbg.print("*");
//		#endif
//		if(i == 0) {
//			buf[0] = I2C_JOURNAL_HEAD;
//			buf[1] = I2C_JOURNAL_TAIL;
//		} else {
//			buf[0] = I2C_JOURNAL_FORMAT;
//			buf[1] = I2C_JOURNAL_FORMAT;
//		}
//		if(writeEEPROM_I2C(I2C_JOURNAL_START + i * W5200_MAX_LEN, (byte*)&buf, W5200_MAX_LEN)) {
//			err = ERR_WRITE_I2C_JOURNAL;
//			#ifdef DEBUG
//			SerialDbg.println(errorWriteI2C);
//			#endif
//			break;
//		};
//		WDT_Restart(WDT);
//	}
	if(!SemaphoreTake(Semaphore, JOURNAL_TIME_WAIT)) {
#ifdef DEBUG
		if(Is_otg_vbus_high()) SerialDbg.print("JSem locked!\n");
#endif
	}
	uint8_t buf[] = { I2C_JOURNAL_HEAD, I2C_JOURNAL_TAIL };
	if(writeEEPROM_I2C(I2C_JOURNAL_START, (uint8_t*)buf, sizeof(buf))) {
		err = ERR_WRITE_I2C_JOURNAL;
		#ifdef DEBUG
		SerialDbg.println(errorWriteI2C);
		#endif
	}
	full = 0;                   // Буфер не полный
	bufferHead = 0;
	bufferTail = 1;
	if(err == OK) writeREADY();                 // было форматирование
	SemaphoreGive(Semaphore);
	if(err == OK) jprintf("\nFormat I2C journal (size %d bytes) - Ok\n", JOURNAL_LEN);
}
#endif
    
// Печать только в консоль
void Journal::printf(const char *format, ...)             
{
#ifdef DEBUG
	if(!Is_otg_vbus_high()) return;
	va_list ap;
	va_start(ap, format);
	m_vsnprintf(pbuf, PRINTF_BUF, format, ap);
	va_end(ap);
	SerialDbg.print(pbuf);
#endif
}

// Печать в консоль и журнал возвращает число записанных байт
void Journal::jprintf(const char *format, ...)
{
	if(!SemaphoreTake(Semaphore, JOURNAL_TIME_WAIT)) {
#ifdef DEBUG
		if(Is_otg_vbus_high()) SerialDbg.print("JSem locked!\n");
#endif
	}
	va_list ap;
	va_start(ap, format);
	m_vsnprintf(pbuf, PRINTF_BUF, format, ap);
	va_end(ap);
#ifdef DEBUG
	if(Is_otg_vbus_high()) SerialDbg.print(pbuf);
#endif
	// добавить строку в журнал
	_write(pbuf);
	SemaphoreGive(Semaphore);
}

// +Time, далее печать в консоль и журнал
void Journal::jprintf_time(const char *format, ...)
{
	if(!SemaphoreTake(Semaphore, JOURNAL_TIME_WAIT)) {
#ifdef DEBUG
		if(Is_otg_vbus_high()) SerialDbg.print("JSem locked!\n");
#endif
	}
	NowTimeToStr(pbuf);
	pbuf[8] = ' ';
	va_list ap;
	va_start(ap, format);
	m_vsnprintf(pbuf + 9, PRINTF_BUF - 9, format, ap);
	va_end(ap);
#ifdef DEBUG
	if(Is_otg_vbus_high()) SerialDbg.print(pbuf);
#endif
	_write(pbuf);   // добавить строку в журнал
	SemaphoreGive(Semaphore);
}   

// +DateTime, далее печать в консоль и журнал
void Journal::jprintf_date(const char *format, ...)
{
	if(!SemaphoreTake(Semaphore, JOURNAL_TIME_WAIT)) {
#ifdef DEBUG
		if(Is_otg_vbus_high()) SerialDbg.print("JSem locked!\n");
#endif
	}
	NowDateToStr(pbuf);
	pbuf[10] = ' ';
	NowTimeToStr(pbuf + 11);
	pbuf[19] = ' ';
	va_list ap;
	va_start(ap, format);
	m_vsnprintf(pbuf + 20, PRINTF_BUF - 20, format, ap);
	va_end(ap);
#ifdef DEBUG
	if(Is_otg_vbus_high()) SerialDbg.print(pbuf);
#endif
	_write(pbuf);   // добавить строку в журнал
	SemaphoreGive(Semaphore);
}

// Печать ТОЛЬКО в журнал возвращает число записанных байт для использования в критических секциях кода
void Journal::jprintf_only(const char *format, ...)
{
	if(!SemaphoreTake(Semaphore, JOURNAL_TIME_WAIT)) {
#ifdef DEBUG
		if(Is_otg_vbus_high()) SerialDbg.print("JSem locked!\n");
#endif
	}
	va_list ap;
	va_start(ap, format);
	m_vsnprintf(pbuf, PRINTF_BUF, format, ap);
	va_end(ap);
	_write(pbuf);
	SemaphoreGive(Semaphore);
}

// отдать журнал в сеть клиенту  Возвращает число записанных байт
int32_t Journal::send_Data(uint8_t thread)
{
	int32_t num, len, sum = 0;
#ifdef I2C_EEPROM_64KB // чтение еепром
	num = bufferHead + 1;                     // Начинаем с начала журнала, num позиция в буфере пропуская символ начала
	for(uint16_t i = 0; i < (JOURNAL_LEN / W5200_MAX_LEN + 1); i++) // Передаем пакетами по W5200_MAX_LEN байт, может быть два неполных пакета!!
	{
		__asm__ volatile ("" ::: "memory");
		if((num > bufferTail))                                        // Текущая позиция больше хвоста (начало передачи)
		{
			if(JOURNAL_LEN - num >= W5200_MAX_LEN) len = W5200_MAX_LEN;	else len = JOURNAL_LEN - num;   // Контроль достижения границы буфера
		} else {                                                        // Текущая позиция меньше хвоста (конец передачи)
			if(bufferTail - num >= W5200_MAX_LEN) len = W5200_MAX_LEN; else len = bufferTail - num;     // Контроль достижения хвоста журнала
		}
		if(readEEPROM_I2C(I2C_JOURNAL_START + num, (byte*) Socket[thread].outBuf, len))         // чтение из памяти
		{
			err = ERR_READ_I2C_JOURNAL;
#ifdef DEBUG
			SerialDbg.print(errorReadI2C);
#endif
			return 0;
		}
		if(sendPacketRTOS(thread, (byte*) Socket[thread].outBuf, len, 0) == 0) return 0;        // передать пакет, при ошибке выйти
		_delay(2);
		sum = sum + len;                                                                        // сколько байт передано
		if(sum >= available()) break;                                                           // Все передано уходим
		num = num + len;                                                                        // Указатель на переданные данные
		if(num >= JOURNAL_LEN) num = 0;                                                         // переходим на начало
	}  // for
#else
	num=bufferHead;                                                   // Начинаем с начала журнала, num позиция в буфере
	for(uint16_t i=0;i<(JOURNAL_LEN/W5200_MAX_LEN+1);i++)// Передаем пакетами по W5200_MAX_LEN байт, может быть два неполных пакета!!
	{
		if((num>bufferTail))                              // Текущая позиция больше хвоста (начало передачи)
		{
			if (JOURNAL_LEN-num>=W5200_MAX_LEN) len=W5200_MAX_LEN; else len=JOURNAL_LEN-num; // Контроль достижения границы буфера
		} else {                                                           // Текущая позиция меньше хвоста (конец передачи)
			if (bufferTail-num>=W5200_MAX_LEN) len=W5200_MAX_LEN; else len=bufferTail-num; // Контроль достижения хвоста журнала
		}
		if(sendPacketRTOS(thread,(byte*)_data+num,len,0)==0) return 0;          // передать пакет, при ошибке выйти
		_delay(2);
		sum=sum+len;// сколько байт передано
		if (sum>=available()) break;// Все передано уходим
		num=num+len;// Указатель на переданные данные
		if (num>=JOURNAL_LEN) num=0;// переходим на начало
	}  // for
#endif
	return sum;
}

// Возвращает размер журнала
int32_t Journal::available(void)
{ 
  #ifdef I2C_EEPROM_64KB
    if (full) return JOURNAL_LEN - 2; else return bufferTail-1;
  #else   
     if (full) return JOURNAL_LEN; else return bufferTail;
  #endif
}    
                 
// чтобы print рабоtал для это класса
size_t Journal::write (uint8_t c)
  {
  SerialDbg.print(char(c));
  return 1;   // one byte output
  }  // end of myOutputtingClass::write
         
// Записать строку в журнал
void Journal::_write(char *dataPtr)
{
	int32_t numBytes;
	if(dataPtr == NULL || (numBytes = strlen(dataPtr)) == 0) return;  // Записывать нечего
	if(numBytes > PRINTF_BUF) numBytes = PRINTF_BUF; // Ограничиваем размер
#ifdef I2C_EEPROM_64KB // запись в еепром
	// Запись в I2C память
	if(SemaphoreTake(xI2CSemaphore, JOURNAL_TIME_WAIT) == pdFALSE) {  // Если шедулер запущен то захватываем семафор
		journal.printf((char*) cErrorMutex, __FUNCTION__, MutexI2CBuzy);
		return;
	}
	__asm__ volatile ("" ::: "memory");
	dataPtr[numBytes] = I2C_JOURNAL_TAIL;
	if(full) dataPtr[numBytes + 1] = I2C_JOURNAL_HEAD;
	if(bufferTail + numBytes + 2 > JOURNAL_LEN) { //  Запись в два приема если число записываемых бит больше чем место от конца очереди до конца буфера ( помним про символ начала)
		int32_t n;
		if(eepromI2C.write(I2C_JOURNAL_START + bufferTail, (byte*) dataPtr, n = JOURNAL_LEN - bufferTail)) {
			#ifdef DEBUG
				if(err != ERR_WRITE_I2C_JOURNAL) SerialDbg.print(errorWriteI2C);
			#endif
			err = ERR_WRITE_I2C_JOURNAL;
		} else {
			dataPtr += n;
			numBytes -= n;
			full = 1;
			if(eepromI2C.write(I2C_JOURNAL_START, (byte*) dataPtr, numBytes + 2)) {
				err = ERR_WRITE_I2C_JOURNAL;
				#ifdef DEBUG
					SerialDbg.print(errorWriteI2C);
				#endif
			} else {
				bufferTail = numBytes >= 0 ? numBytes : JOURNAL_LEN - 1;
				bufferHead = bufferTail + 1;
				err = OK;
			}
		}
	} else {  // Запись в один прием
		if(eepromI2C.write(I2C_JOURNAL_START + bufferTail, (byte*) dataPtr, numBytes + 1 + full)) {
			#ifdef DEBUG
				if(err != ERR_WRITE_I2C_JOURNAL) SerialDbg.print(errorWriteI2C);
			#endif
			err = ERR_WRITE_I2C_JOURNAL;
		} else {
			bufferTail += numBytes;
			if(full) bufferHead = bufferTail + 1;
		}
	}
	SemaphoreGive(xI2CSemaphore);
#else   // Запись в память
	// SerialDbg.print(">"); SerialDbg.print(numBytes); SerialDbg.println("<");

	// Запись в журнал
	if(numBytes > JOURNAL_LEN - bufferTail)//  Запись в два приема если число записываемых бит больше чем место от конца очереди до конца буфера
	{
		int len = JOURNAL_LEN - bufferTail;             // сколько можно записать в конец
		memcpy(_data+bufferTail,dataPtr,len);// Пишем с конца очереди но до конца журнала
		memcpy(_data, dataPtr+len, numBytes-len);// Пишем в конец буфера с начала
		bufferTail = numBytes-len;// Хвост начинает рости с начала буфера
		bufferHead=bufferTail +1;// Буфер полный по этому начало стоит сразу за концом (затирание данных)
		full=1;// буфер полный
	} else   // Запись в один прием Буфер
	{
		memcpy(_data+bufferTail, dataPtr, numBytes);     // Пишем с конца очереди
		bufferTail = bufferTail + numBytes;// Хвост вырос
		if (full) bufferHead=bufferTail+1;// голова изменяется только при полном буфере (затирание данных)
		else bufferHead=0;
	}
#endif
}

    
// ---------------------------------------------------------------------------------
//  Класс Профиль ТН    ------------------------------------------------------------
// ---------------------------------------------------------------------------------
// Класс предназначен для работы с настройками ТН. в пямяти хранится текущий профиль, ав еепром можно хранить до 10 профилей, и загружать их по одному в пямять
// также есть функции по сохраннию и удалению из еепром
// номер текущего профиля хранится в структуре type_SaveON
// инициализация профиля
void Profile::initProfile()
{
  err=OK;
  strcpy(dataProfile.name,"unknow");
  strcpy(dataProfile.note,"default profile");
  dataProfile.flags=0x00;
  id=0;
  
  // Состояние ТН структура SaveON
  SaveON.magic=0x55;                   // признак данных, должно быть  0x55
  SaveON.flags = 0;                    // Все флаги - выкл
  SaveON.mode=pOFF;                    // выключено
  
  // Охлаждение
  Cool.Rule=pHYSTERESIS,               // алгоритм гистерезис, интервальный режим
  Cool.Temp1=2000;                     // Целевая температура дома
  Cool.Temp2=1200;                     // Целевая температура Обратки
  SETBIT0(Cool.flags,fTarget);         // Что является целью ПИД - значения true (температура в доме), false (температура обратки).
  SETBIT0(Cool.flags,fWeather);        // флаг Погодозависмости
  Cool.dTemp=200;                      // Гистерезис целевой температуры
  Cool.pid_time=90;                    // Постоянная интегрирования времени в секундах ПИД ТН
  Cool.pid.Kp=1;                      // Пропорциональная составляющая ПИД ТН
  Cool.pid.Ki=0;                       // Интегральная составляющая ПИД ТН
  Cool.pid.Kd=3;                       // Дифференциальная составляющая ПИД ТН
  Cool.tempPID=2200;                // Целевая температура ПИД
  Cool.delayOffPump = DEF_DELAY_OFF_PUMP;
 
 // Защиты
  Cool.tempInLim=1000;                    // Tемпература подачи (минимальная)
  Cool.tempOutLim=3500;                   // Tемпература обратки (макс)
  Cool.MaxDeltaTempOut=1500;                        // Максимальная разность температур конденсатора.
  
 // Cool.P1=0;
  
// Отопление
  Heat.Rule=pPID,              		 // алгоритм гистерезис, интервальный режим
  Heat.Temp1=2200;                     // Целевая температура дома
  Heat.Temp2=3500;                     // Целевая температура Обратки
  SETBIT1(Heat.flags,fTarget);         // Что является целью ПИД - значения true (температура в доме), false (температура обратки).
  SETBIT1(Heat.flags,fWeather);        // флаг Погодозависмости
  Heat.dTemp=050;                      // Гистерезис целевой температуры
  Heat.pid_time=60;                    // Постоянная интегрирования времени в секундах ПИД ТН
  Heat.pid.Kp=100;                      // Пропорциональная составляющая ПИД ТН
  Heat.pid.Ki=480;                       // Интегральная составляющая ПИД ТН
  Heat.pid.Kd=10;                       // Дифференциальная составляющая ПИД ТН
  Heat.tempPID=3200;                // Целевая температура ПИД
  Heat.add_delta_temp = 150;	 	   // Добавка температуры к установке бойлера, в градусах
  Heat.add_delta_hour = 5;		   	   // Начальный Час добавки температуры к установке бойлера
  Heat.add_delta_end_hour = 6;         // Конечный Час добавки температуры к установке
  Heat.tempInLim=4700;                    // Tемпература подачи (макс)
  Heat.tempOutLim=-5;                      // Tемпература обратки (минимальная)
  Heat.MaxDeltaTempOut=1500;                        // Максимальная разность температур конденсатора.
  Heat.kWeatherPID=0;                    // Коэффициент погодозависимости в СОТЫХ градуса на градус
  Heat.WeatherBase = 0;
  Heat.WeatherTargetRange = 0;
  Heat.delayOffPump = DEF_DELAY_OFF_PUMP;
  
 // Heat.P1=0;
 
 // Бойлер
  SETBIT1(Boiler.flags,fSchedule);      // !save! флаг Использование расписания выключено
  SETBIT0(Boiler.flags,fTurboBoiler);    // !save! флаг использование ТЭН для нагрева  выключено
  SETBIT0(Boiler.flags,fLegionella);    // !save! флаг легионелла раз внеделю греть бойлер  выключено
  SETBIT0(Boiler.flags,fCirculation);   // !save! флагУправления циркуляционным насосом ГВС  выключено
  SETBIT1(Boiler.flags,fAddHeating);    // флаг флаг догрева ГВС ТЭНом
  SETBIT1(Boiler.flags,fScheduleAddHeat);
  SETBIT0(Boiler.flags,fResetHeat);     // флаг Сброса лишнего тепла в СО
  Boiler.TempTarget=5000;               // !save! Целевая температура бойлера
  Boiler.dTemp=500;                     // !save! гистерезис целевой температуры
  Boiler.tempInLim=5400;                   // !save! Tемпература подачи максимальная
  for (uint8_t i=0;i<7; i++) Boiler.Schedule[i]=0;             // !save! Расписание бойлера
  Boiler.Circul_Work=60*3;              // Время  работы насоса ГВС секунды (fCirculation)
  Boiler.Circul_Pause=60*10;            // Пауза в работе насоса ГВС  секунды (fCirculation)
  Boiler.Reset_Time=30;                 // время сброса излишков тепла в секундах (fResetHeat)
  Boiler.pid_time=20;                   // Постоянная интегрирования времени в секундах ПИД ГВС
  Boiler.pid.Kp=1;                      // Пропорциональная составляющая ПИД ГВС
  Boiler.pid.Ki=0;                      // Интегральная составляющая ПИД ГВС
  Boiler.pid.Kd=3;                      // Дифференциальная составляющая ПИД ГВС
  Boiler.tempPID=3800;                  // Целевая температура ПИД ГВС
  Boiler.tempRBOILER=3500;              // Температура ГВС при котором включается бойлер и отключатся ТН
  Boiler.dAddHeat = HYSTERESIS_BoilerAddHeat;
  Boiler.add_delta_temp = 1800;		    // Добавка температуры к установке бойлера, в градусах
  Boiler.add_delta_hour = 6;		    // Начальный Час добавки температуры к установке бойлера
  Boiler.add_delta_end_hour = 6;        // Конечный Час добавки температуры к установке
  Boiler.DischargeDelta = 10;
  Boiler.delayOffPump = 60;

  DailySwitchStateT = 0;

}

// Охлаждение Установить параметры ТН из числа (float)
boolean Profile::set_paramCoolHP(char *var, float x)
{ 
 if(strcmp(var,hp_RULE)==0)  {  switch ((int)x)
				                   {
				                    case 0: Cool.Rule=pHYSTERESIS; break;
				                    case 1: Cool.Rule=pPID;        break;
				                    //case 2: Cool.Rule=pHYBRID;     break;
				                    default:Cool.Rule=pHYSTERESIS; break;
				                    }
 	 	 	 	 	 	 	 	 HP.resetPID(); return true; } else
 if(strcmp(var,hp_TEMP1)==0) {   if ((x>=0)&&(x<=30))  {Cool.Temp1=rd(x, 100); return true;} else return false;   }else   // целевая температура в доме
 if(strcmp(var,hp_TEMP2)==0) {   if ((x>=0)&&(x<=40))  {Cool.Temp2=rd(x, 100); return true;} else return false;  }else    // целевая температура обратки
 if(strcmp(var,hp_TARGET)==0) {  if (x==0) {SETBIT0(Cool.flags,fTarget); return true;} else if (x==1.0) {SETBIT1(Cool.flags,fTarget); return true;} else return false; }else // что является целью значения  0 (температура в доме), 1 (температура обратки).
 if(strcmp(var,hp_DTEMP)==0) {   if ((x>=0)&&(x<=30))  {Cool.dTemp=rd(x, 100); return true;} else return false;   }else   // гистерезис целевой температуры
 if(strcmp(var,hp_dTempGen)==0){ Cool.dTempGen = rd(x, 100); return true; }else
 if(strcmp(var,hp_HP_TIME)==0) { if ((x>=10)&&(x<=1000)) {UpdatePIDbyTime(x, Cool.pid_time, Cool.pid); Cool.pid_time=x; return true;} else return false;                                             }else             // Постоянная интегрирования времени в секундах ПИД ТН !
 if(strcmp(var,hp_HP_PRO)==0) {  if ((x>=0)&&(x<=32)) {Cool.pid.Kp=rd(x, 1000); return true;} else return false;    }else // Пропорциональная составляющая ПИД ТН
#ifdef PID_FORMULA2
 if(strcmp(var,hp_HP_IN)==0) {   if ((x>=0)&&(x<=32))  {x *= Cool.pid_time; if(x>32.7) x=32.7; Cool.pid.Ki=rd(x, 1000); return true;} else return false; }else             // Интегральная составляющая ПИД ТН
 if(strcmp(var,hp_HP_DIF)==0) {  if ((x>=0)&&(x<=32))  {Cool.pid.Kd=rd(x / Cool.pid_time, 1000); return true;} else return false; }else             // Дифференциальная составляющая ПИД ТН
#else
 if(strcmp(var,hp_HP_IN)==0) {   if ((x>=0)&&(x<=32))  {Cool.pid.Ki=rd(x, 1000); return true;} else return false;   }else // Интегральная составляющая ПИД ТН
 if(strcmp(var,hp_HP_DIF)==0) {  if ((x>=0)&&(x<=32))  {Cool.pid.Kd=rd(x, 1000); return true;} else return false;   }else // Дифференциальная составляющая ПИД ТН
#endif
 if(strcmp(var,hp_TEMP_IN)==0) { if ((x>=0)&&(x<=30))  {Cool.tempInLim=rd(x, 100); return true;} else return false;  }else// температура подачи (минимальная)
 if(strcmp(var,hp_TEMP_OUT)==0){ if ((x>=0)&&(x<=40))  {Cool.tempOutLim=rd(x, 100); return true;} else return false; }else// температура обратки (максимальная)
 if(strcmp(var,hp_D_TEMP)==0) {  if ((x>=0)&&(x<=50))  {Cool.MaxDeltaTempOut=rd(x, 100); return true;} else return false; }else // максимальная разность температур конденсатора.
 if(strcmp(var,hp_TEMP_PID)==0){ if ((x>=0)&&(x<=40))  {Cool.tempPID=rd(x, 100); return true;} else return false; }else   // Целевая темпеартура ПИД
 if(strcmp(var,hp_WEATHER)==0) { Cool.flags = (Cool.flags & ~(1<<fWeather)) | ((x!=0)<<fWeather); return true; }else      // Использование погодозависимости
 if(strcmp(var,hp_K_WEATHER)==0){ Cool.kWeatherPID=rd(x, 1000); return true; } else            // Коэффициент погодозависимости
 if(strcmp(var,hp_kWeatherTarget)==0){ Cool.kWeatherTarget=rd(x, 1000); return true; } else
 if(strcmp(var,hp_WeatherBase)==0){ Cool.WeatherBase = x; return true; } else
 if(strcmp(var,hp_WeatherTargetRange)==0){ Cool.WeatherTargetRange = rd(x, 10); return true; } else
 if(strcmp(var,hp_HEAT_FLOOR)==0) { Cool.flags = (Cool.flags & ~(1<<fHeatFloor)) | ((x!=0)<<fHeatFloor); return true; }else
 if(strcmp(var,hp_SUN)==0) { Cool.flags = (Cool.flags & ~(1<<fUseSun)) | ((x!=0)<<fUseSun); return true; }else
 if(strcmp(var,option_DELAY_OFF_PUMP)==0){ Cool.delayOffPump = x; return true; } else
 if(strcmp(var,hp_WorkPause)==0){ if(x >= 0) {Cool.WorkPause = x; return true; } else return false; } else
 if(strcmp(var,hp_fUseAdditionalTargets)==0) { Cool.flags = (Cool.flags & ~(1<<fUseAdditionalTargets)) | ((x!=0)<<fUseAdditionalTargets); return true; }else
 return false; 
}

//Охлаждение Получить параметр в виде строки  второй параметр - наличие частотника
char* Profile::get_paramCoolHP(char *var, char *ret)
{
   if(strcmp(var,hp_RULE)==0)     { return web_fill_tag_select(ret, "HYSTERESIS:0;PID:0;", Cool.Rule); } else
   if(strcmp(var,hp_TEMP1)==0)    {_dtoa(ret,Cool.Temp1/10,1); return ret;               } else             // целевая температура в доме
   if(strcmp(var,hp_TEMP2)==0)    {_dtoa(ret,Cool.Temp2/10,1); return ret;               } else             // целевая температура обратки
   if(strcmp(var,hp_TARGET)==0)   {if (!(GETBIT(Cool.flags,fTarget))) return strcat(ret,(char*)"Дом:1;Обратка:0;");
                                  else return strcat(ret,(char*)"Дом:0;Обратка:1;");           } else             // что является целью значения  0 (температура в доме), 1 (температура обратки).
   if(strcmp(var,hp_DTEMP)==0)    {_dtoa(ret,Cool.dTemp, 2); return ret;               } else             // гистерезис целевой температуры
   if(strcmp(var,hp_dTempGen)==0) { _dtoa(ret, Cool.dTempGen, 2); return ret; } else
   if(strcmp(var,hp_HP_TIME)==0)  {return  _itoa(Cool.pid_time,ret);                               } else             // Постоянная интегрирования времени в секундах ПИД ТН
   if(strcmp(var,hp_HP_PRO)==0)   {_dtoa(ret,Cool.pid.Kp,3); return ret;              } else             // Пропорциональная составляющая ПИД ТН
#ifdef PID_FORMULA2
   if(strcmp(var,hp_HP_IN)==0)    {_dtoa(ret,Cool.pid.Ki/Cool.pid_time,3); return ret;} else             // Интегральная составляющая ПИД ТН
   if(strcmp(var,hp_HP_DIF)==0)   {_dtoa(ret,Cool.pid.Kd*Cool.pid_time,3); return ret;} else             // Дифференциальная составляющая ПИД ТН
#else
   if(strcmp(var,hp_HP_IN)==0)    {_dtoa(ret,Cool.pid.Ki,3); return ret;              } else             // Интегральная составляющая ПИД ТН
   if(strcmp(var,hp_HP_DIF)==0)   {_dtoa(ret,Cool.pid.Kd,3); return ret;              } else             // Дифференциальная составляющая ПИД ТН
#endif
   if(strcmp(var,hp_TEMP_IN)==0)  {_dtoa(ret,Cool.tempInLim/10,1); return ret;              } else             // температура подачи (минимальная)
   if(strcmp(var,hp_TEMP_OUT)==0) {_dtoa(ret,Cool.tempOutLim/10,1); return ret;             } else             // температура обратки (максимальная)
   if(strcmp(var,hp_D_TEMP)==0)   {_dtoa(ret,Cool.MaxDeltaTempOut/10,1); return ret;                  } else             // максимальная разность температур конденсатора.
   if(strcmp(var,hp_TEMP_PID)==0) {_dtoa(ret,Cool.tempPID/10,1); return ret;          } else             // Целевая темпеартура ПИД
   if(strcmp(var,hp_WEATHER)==0)  { if(GETBIT(Cool.flags,fWeather)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else // Использование погодозависимости
   if(strcmp(var,hp_K_WEATHER)==0){ _dtoa(ret,Cool.kWeatherPID,3); return ret;            } else                 // Коэффициент погодозависимости
   if(strcmp(var,hp_kWeatherTarget)==0){ _dtoa(ret,Cool.kWeatherTarget,3); return ret;    } else
   if(strcmp(var,hp_WeatherBase)==0){ _dtoa(ret,Cool.WeatherBase,0); return ret;    } else
   if(strcmp(var,hp_WeatherTargetRange)==0){ _dtoa(ret,Cool.WeatherTargetRange,1); return ret;    } else
   if(strcmp(var,hp_HEAT_FLOOR)==0)  { if(GETBIT(Cool.flags,fHeatFloor)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
   if(strcmp(var,hp_SUN)==0)      { if(GETBIT(Cool.flags,fUseSun)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
   if(strcmp(var,hp_targetPID)==0){_dtoa(ret,HP.CalcTargetPID_Cool(),2); return ret;      } else
   if(strcmp(var,option_DELAY_OFF_PUMP)==0) { return _itoa(Cool.delayOffPump, ret); } else
   if(strcmp(var,hp_WorkPause)==0) { _itoa(Cool.WorkPause, ret); return ret; } else
   if(strcmp(var,hp_fUseAdditionalTargets)==0){ if(GETBIT(Cool.flags,fUseAdditionalTargets)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
 return  strcat(ret,(char*)cInvalid);   
}

// Отопление Установить параметры ТН из числа (float)
boolean Profile::set_paramHeatHP(char *var, float x)
{ 
	if(strcmp(var,hp_RULE)==0) {  switch ((int)x)
							  {
								 case 0: Heat.Rule=pHYSTERESIS; break;
								 case 1: Heat.Rule=pPID;        break;
//								 case 2: Heat.Rule=pHYBRID;     break; // Пока не поддерживается
								 default:Heat.Rule=pHYSTERESIS; break;
							  }
							  HP.resetPID(); return true; } else
#ifdef USE_HEATER
	if(strcmp(var,hp_fUseHeater)==0) { if(!HP.IsWorkingNow()) SaveON.flags = (SaveON.flags & ~(1<<fHeat_UseHeater)) | ((x!=0)<<fHeat_UseHeater); return true; }else
#else
	if(strcmp(var,hp_fUseHeater)==0) { SaveON.flags = (SaveON.flags & ~(1<<fHeat_UseHeater)); return true; }else
#endif
	if(strcmp(var,hp_TEMP1)==0) {   if((x>=0)&&(x<=40))   {Heat.Temp1=rd(x, 100); return true;} else return false;  }else           // целевая температура в доме
	if(strcmp(var,ADD_DELTA_TEMP)==0){ if((x>=-30)&&(x<=50))  {Heat.add_delta_temp=rd(x, 100); return true;}else return false; }else // Добавка к целевой температуры ВНИМАНИЕ здесь еденица измерения ГРАДУСЫ
	if(strcmp(var,ADD_DELTA_HOUR)==0){ if((x>=0)&&(x<=23))    {Heat.add_delta_hour=x; return true;} else return false; }else
	if(strcmp(var,ADD_DELTA_END_HOUR)==0){ if((x>=0)&&(x<=23)){Heat.add_delta_end_hour=x; return true;} else return false; }else
	if(strcmp(var,hp_TEMP2)==0) {   if((x>=5)&&(x<=90)){ Heat.Temp2=rd(x, 100); return true;} else return false;  }else           // целевая температура обратки
	if(strcmp(var,hp_TARGET)==0) {  if(x==0) {SETBIT0(Heat.flags,fTarget); return true;} else if (x==1) {SETBIT1(Heat.flags,fTarget); return true;} else return false; }else // что является целью значения  0 (температура в доме), 1 (температура обратки).
	if(strcmp(var,hp_DTEMP)==0) {   if((x>=0)&&(x<=30)) { Heat.dTemp=rd(x, 100); return true;} else return false;   }else          // гистерезис целевой температуры
	if(strcmp(var,hp_dTempGen)==0){ Heat.dTempGen = rd(x, 100); return true; }else
	if(strcmp(var,hp_MaxTargetRise)==0){ Heat.MaxTargetRise = rd(x, 10); return true; }else
	if(strcmp(var,hp_HP_TIME)==0) { if((x>=10)&&(x<=1000)) {UpdatePIDbyTime(x, Heat.pid_time, Heat.pid); Heat.pid_time=x; return true;} else return false; }else // Постоянная интегрирования времени в секундах ПИД ТН !
	if(strcmp(var,hp_HP_PRO)==0) {  if((x>=0)&&(x<=32)) {Heat.pid.Kp=rd(x, 1000); return true;} else return false;   }else         // Пропорциональная составляющая ПИД ТН
#ifdef PID_FORMULA2
	if(strcmp(var,hp_HP_IN)==0) {   if((x>=0)&&(x<=32))  {x *= Heat.pid_time; if(x>32.7) x=32.7; Heat.pid.Ki=rd(x, 1000); return true;} else return false; }else  // Интегральная составляющая ПИД ТН
	if(strcmp(var,hp_HP_DIF)==0) {  if((x>=0)&&(x<=32))  {Heat.pid.Kd=rd(x / Heat.pid_time, 1000); return true;} else return false; }else // Дифференциальная составляющая ПИД ТН
#else
	if(strcmp(var,hp_HP_IN)==0) {   if((x>=0)&&(x<=32))  {Heat.pid.Ki=rd(x, 1000); return true;} else return false;   }else        // Интегральная составляющая ПИД ТН
	if(strcmp(var,hp_HP_DIF)==0) {  if((x>=0)&&(x<=32))  {Heat.pid.Kd=rd(x, 1000); return true;} else return false;   }else        // Дифференциальная составляющая ПИД ТН
#endif
	if(strcmp(var,hp_TEMP_IN)==0) { if((x>=0)&&(x<=90))  {Heat.tempInLim=rd(x, 100); return true;} else return false;     }else    // температура подачи (минимальная)
	if(strcmp(var,hp_TEMP_OUT)==0){ if((x>=-10)&&(x<=90)){Heat.tempOutLim=rd(x, 100); return true;} else return false;    }else    // температура обратки (максимальная)
	if(strcmp(var,hp_D_TEMP)==0) {  if((x>=0)&&(x<=50))  {Heat.MaxDeltaTempOut=rd(x, 100); return true;} else return false;  }else // максимальная разность температур конденсатора.
	if(strcmp(var,hp_TEMP_PID)==0){ if((x>=5)&&(x<=90)) {Heat.tempPID=rd(x, 100); return true;} else return false;  }else          // Целевая темпеартура ПИД
	if(strcmp(var,hp_HEAT_FLOOR)==0) { Heat.flags = (Heat.flags & ~(1<<fHeatFloor)) | ((x!=0)<<fHeatFloor); return true; }else
	if(strcmp(var,hp_SUN)==0) { Heat.flags = (Heat.flags & ~(1<<fUseSun)) | ((x!=0)<<fUseSun); return true; }else
	if(strcmp(var,hp_fP_ContinueAfterBoiler)==0) { Heat.flags = (Heat.flags & ~(1<<fP_ContinueAfterBoiler)) | ((x!=0)<<fP_ContinueAfterBoiler); return true; }else
	if(strcmp(var,hp_fUseAdditionalTargets)==0) { Heat.flags = (Heat.flags & ~(1<<fUseAdditionalTargets)) | ((x!=0)<<fUseAdditionalTargets); return true; }else
	if(strcmp(var,hp_WEATHER)==0) { Heat.flags = (Heat.flags & ~(1<<fWeather)) | ((x!=0)<<fWeather); return true; }else            // Использование погодозависимости
	if(strcmp(var,hp_K_WEATHER)==0){ Heat.kWeatherPID=rd(x, 1000); return true; } else            // Коэффициент погодозависимости
	if(strcmp(var,hp_kWeatherTarget)==0){ Heat.kWeatherTarget=rd(x, 1000); return true; } else
	if(strcmp(var,hp_WeatherBase)==0){ Heat.WeatherBase = x; return true; } else
	if(strcmp(var,hp_WeatherTargetRange)==0){ Heat.WeatherTargetRange = rd(x, 10); return true; } else
	if(strcmp(var,hp_WorkPause)==0){ if(x >= 0) {Heat.WorkPause = x; return true; } else return false; } else
	if(strcmp(var,hp_FC_FreqLimit)==0){ Heat.FC_FreqLimit = rd(x, 100); if(Heat.FC_FreqLimit < HP.dFC.get_minFreq()) Heat.FC_FreqLimit = HP.dFC.get_minFreq(); return true; } else
	if(strcmp(var,option_ADD_HEAT)==0) { Heat.flags = (Heat.flags & ~((1<<fAddHeat1)|(1<<fAddHeat2))) | ((int(x)<<fAddHeat1)&((1<<fAddHeat1)|(1<<fAddHeat2))); return true; } else
	if(strcmp(var,hp_timeRHEAT)==0){ Heat.timeRHEAT = x; return true; } else
	if(strcmp(var,option_TEMP_RHEAT)==0){ Heat.tempRHEAT = rd(x, 100); return true; }else     // температура управления RHEAT (градусы)
	if(strcmp(var,option_PUMP_WORK)==0) { // работа насоса конденсатора при выключенном компрессоре
		Heat.workPump = x;
		if(x == 0 && HP.startPump == StartPump_Work_On) {
			HP.pump_in_pause_set(false);				// выключить насосы "в паузе"
			HP.startPump = StartPump_Stop;
		} else if(HP.startPump == StartPump_Stop && HP.get_State() == pWORK_HP && !HP.is_compressor_on()) {
			HP.startPump = StartPump_Start;
			HP.pump_in_pause_timer = 0;
		}
		return true;
	}else
	if(strcmp(var,option_PUMP_PAUSE)==0)    { Heat.pausePump=x; return true; }else               // пауза между работой насоса конденсатора при выключенном компрессоре МИНУТЫ
	if(strcmp(var,option_DELAY_OFF_PUMP)==0){ Heat.delayOffPump = x; return true; } else
	return false;
}

// Отопление Получить параметр в виде строки  второй параметр - наличие частотника
char* Profile::get_paramHeatHP(char *var,char *ret)
{
	if(strcmp(var,hp_RULE)==0)     { return web_fill_tag_select(ret, "HYSTERESIS:0;PID:0;", Heat.Rule); } else // было "HYSTERESIS:0;PID:0;HYBRID:0;"
	if(strcmp(var,hp_fUseHeater)==0){ if(GETBIT(SaveON.flags, fHeat_UseHeater)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
	if(strcmp(var,hp_TEMP1)==0)    { _dtoa(ret,Heat.Temp1/10,1); return ret;                } else             // целевая температура в доме
	if(strcmp(var,ADD_DELTA_TEMP)==0) 	{  _dtoa(ret,Heat.add_delta_temp, 2); return ret;}else
	if(strcmp(var,ADD_DELTA_HOUR)==0) 	{  _itoa(Heat.add_delta_hour, ret); return ret;         }else
	if(strcmp(var,ADD_DELTA_END_HOUR)==0){  _itoa(Heat.add_delta_end_hour, ret); return ret;    	}else
	if(strcmp(var,hp_TEMP2)==0)    { _dtoa(ret,Heat.Temp2/10,1); return ret;                } else            // целевая температура обратки
	if(strcmp(var,hp_TARGET)==0)   { return web_fill_tag_select(ret, "Дом:0;Обратка:0;", GETBIT(Heat.flags,fTarget)); } else
	if(strcmp(var,option_ADD_HEAT)==0){ return web_fill_tag_select(ret, "нет:0;по дому:0;по улице:0;интеллектуально:0;", (Heat.flags & ((1<<fAddHeat1)|(1<<fAddHeat2)))>>fAddHeat1); } else
	if(strcmp(var,hp_timeRHEAT)==0) { _itoa(Heat.timeRHEAT, ret); return ret; } else
	if(strcmp(var,option_TEMP_RHEAT)==0){_dtoa(ret, Heat.tempRHEAT, 2); return ret; }else           // температура управления RHEAT (градусы)
	if(strcmp(var,option_PUMP_WORK)==0) {return _itoa(Heat.workPump,ret);}else
	if(strcmp(var,option_PUMP_PAUSE)==0){return _itoa(Heat.pausePump,ret);}else
	if(strcmp(var,hp_DTEMP)==0)    { _dtoa(ret,Heat.dTemp,2); return ret;                } else             // гистерезис целевой температуры
	if(strcmp(var,hp_dTempGen)==0) { _dtoa(ret, Heat.dTempGen,2); return ret; } else
	if(strcmp(var,hp_MaxTargetRise)==0) { _dtoa(ret, Heat.MaxTargetRise, 1); return ret; } else
	if(strcmp(var,hp_HP_TIME)==0)  { return _itoa(Heat.pid_time,ret);                                } else             // Постоянная интегрирования времени в секундах ПИД ТН
	if(strcmp(var,hp_HP_PRO)==0)   { _dtoa(ret,Heat.pid.Kp,3); return ret;               } else             // Пропорциональная составляющая ПИД ТН
#ifdef PID_FORMULA2
	if(strcmp(var,hp_HP_IN)==0)    { _dtoa(ret,Heat.pid.Ki/Heat.pid_time,3); return ret;} else             // Интегральная составляющая ПИД ТН
	if(strcmp(var,hp_HP_DIF)==0)   { _dtoa(ret,Heat.pid.Kd*Heat.pid_time,3); return ret;} else             // Дифференциальная составляющая ПИД ТН
#else
	if(strcmp(var,hp_HP_IN)==0)    { _dtoa(ret,Heat.pid.Ki,3); return ret;               } else             // Интегральная составляющая ПИД ТН
	if(strcmp(var,hp_HP_DIF)==0)   { _dtoa(ret,Heat.pid.Kd,3); return ret;               } else             // Дифференциальная составляющая ПИД ТН
#endif
	if(strcmp(var,hp_TEMP_IN)==0)  { _dtoa(ret,Heat.tempInLim/10,1); return ret;               } else             // температура подачи (минимальная)
	if(strcmp(var,hp_TEMP_OUT)==0) { _dtoa(ret,Heat.tempOutLim/10,1); return ret;              } else             // температура обратки (максимальная)
	if(strcmp(var,hp_D_TEMP)==0)   { _dtoa(ret,Heat.MaxDeltaTempOut/10,1); return ret;                   } else             // максимальная разность температур конденсатора.
	if(strcmp(var,hp_TEMP_PID)==0) { _dtoa(ret,Heat.tempPID/10,1); return ret;              } else             // Целевая темпеартура ПИД
	if(strcmp(var,hp_HEAT_FLOOR)==0)  { if(GETBIT(Heat.flags,fHeatFloor)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
	if(strcmp(var,hp_SUN)==0)      { if(GETBIT(Heat.flags,fUseSun)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
	if(strcmp(var,hp_fP_ContinueAfterBoiler)==0){ if(GETBIT(Heat.flags,fP_ContinueAfterBoiler)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
	if(strcmp(var,hp_fUseAdditionalTargets)==0){ if(GETBIT(Heat.flags,fUseAdditionalTargets)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
	if(strcmp(var,hp_targetPID)==0){ _dtoa(ret,HP.CalcTargetPID_Heat(),2); return ret;    } else
	if(strcmp(var,hp_WEATHER)==0)  { if(GETBIT(Heat.flags,fWeather)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else // Использование погодозависимости
	if(strcmp(var,hp_K_WEATHER)==0){ _dtoa(ret,Heat.kWeatherPID,3); return ret;            } else                 // Коэффициент погодозависимости
	if(strcmp(var,hp_kWeatherTarget)==0){ _dtoa(ret,Heat.kWeatherTarget,3); return ret;    } else
	if(strcmp(var,hp_WeatherBase)==0){ _dtoa(ret,Heat.WeatherBase,0); return ret;    } else
	if(strcmp(var,hp_WeatherTargetRange)==0){ _dtoa(ret,Heat.WeatherTargetRange,1); return ret;    } else
	if(strcmp(var,hp_WorkPause)==0) { _itoa(Heat.WorkPause, ret); return ret; } else
	if(strcmp(var,hp_FC_FreqLimit)==0) { _dtoa(ret, Heat.FC_FreqLimit, 2); return ret; } else
	if(strcmp(var,option_DELAY_OFF_PUMP)==0) { return _itoa(Heat.delayOffPump, ret); } else
	if(strcmp(var,hp_FC_FreqLimitHour)==0) { strcat_time(ret, Heat.FC_FreqLimitHour * 10); return ret; } else
	if(strcmp(var, option_HeatTargetScheduler) == 0){
		ret += strlen(ret);
		for(uint8_t i = 0; i < 24; i++) *ret++ = (((Heat.HeatTargetSchedulerH<<16) | Heat.HeatTargetSchedulerL) & (1<<i)) ? '1' : '0';
		*ret = '\0';
		return ret;
	} else
	return  strcat(ret,(char*)cInvalid);
}

// Настройка бойлера --------------------------------------------------
//Установить параметр из строки
boolean Profile::set_boiler(char *var, char *c)
{ 
	if(strcmp(var,boil_SCHEDULER)==0)		{ return set_Schedule(c,Boiler.Schedule); }  // разбор строки расписания
	float x=my_atof(c);
	if(x == ATOF_ERROR) return false;   // Ошибка преобразования короме расписания - это не число
	if(strcmp(var,boil_BOILER_ON)==0)		{ if(x) SETBIT1(SaveON.flags,fBoilerON); else SETBIT0(SaveON.flags,fBoilerON); return true;} else
	if(strcmp(var,hp_fUseHeater)==0)		{ if(!HP.IsWorkingNow()) { if(x) SETBIT1(SaveON.flags,fBoiler_UseHeater); else SETBIT0(SaveON.flags,fBoiler_UseHeater); } return true;} else
	if(strcmp(var,boil_SCHEDULER_ON)==0)	{ if(x) SETBIT1(Boiler.flags,fSchedule); else { SETBIT0(Boiler.flags,fSchedule); SETBIT0(Boiler.flags,fScheduleAddHeat); } return true;} else
	if(strcmp(var,boil_SCHEDULER_ADDHEAT)==0){if(x) {SETBIT1(Boiler.flags,fScheduleAddHeat); SETBIT1(Boiler.flags,fSchedule); } else SETBIT0(Boiler.flags,fScheduleAddHeat); return true;} else
	if(strcmp(var,boil_TOGETHER_HEAT)==0)	{ if(x) SETBIT1(Boiler.flags,fBoilerTogetherHeat); else {
												SETBIT0(Boiler.flags,fBoilerTogetherHeat);
#ifdef RPUMPBH
												if((HP.get_modWork() & pHEAT)) HP.dRelay[RPUMPBH].set_OFF();   // насос ГВС - выключить
#endif
											} return true;} else
	if(strcmp(var,boil_fBoilerPID)==0)	    { if(x) SETBIT1(Boiler.flags,fBoilerPID); else SETBIT0(Boiler.flags,fBoilerPID); return true;} else
	if(strcmp(var,boil_TURBO_BOILER)==0)	{ if(x) SETBIT1(Boiler.flags,fTurboBoiler); else SETBIT0(Boiler.flags,fTurboBoiler); return true;} else
	if(strcmp(var,boil_LEGIONELLA)==0)		{ if(x) SETBIT1(Boiler.flags,fLegionella); else SETBIT0(Boiler.flags,fLegionella); return true;} else // Изменение максимальной температуры при включенном режиме легионелла
	if(strcmp(var,boil_CIRCULATION)==0)		{ if(x) SETBIT1(Boiler.flags,fCirculation); else SETBIT0(Boiler.flags,fCirculation); return true;} else
	if(strcmp(var,boil_fBoilerCircSchedule)==0) { if(x) SETBIT1(Boiler.flags,fBoilerCircSchedule); else SETBIT0(Boiler.flags,fBoilerCircSchedule); return true;} else
	if(strcmp(var,boil_TEMP_TARGET)==0)		{ if((x>=5)&&(x<=95)) {Boiler.TempTarget=rd(x, 100); return true;} else return false; } else  // Целевая температура бойлера
	if(strcmp(var,ADD_DELTA_TEMP)==0)		{ if((x>=-50)&&(x<=50)) {Boiler.add_delta_temp=rd(x, 100); return true;}else return false; } else  // Добавка к целевой температуры ВНИМАНИЕ здесь еденица измерения ГРАДУСЫ
	if(strcmp(var,ADD_DELTA_HOUR)==0)		{ if((x>=0)&&(x<=23)) {Boiler.add_delta_hour=x; return true;} else return false; } else      // Начальный Час добавки температуры к установке бойлера
	if(strcmp(var,ADD_DELTA_END_HOUR)==0)	{ if((x>=0)&&(x<=23)){Boiler.add_delta_end_hour=x; return true;} else return false; } else   // Конечный Час добавки температуры к установке
	if(strcmp(var,boil_DTARGET)==0)			{ if((x>=0)&&(x<=30)) {Boiler.dTemp=rd(x, 100); return true;} else return false; } else      // гистерезис целевой температуры
	if(strcmp(var,boil_TEMP_MAX)==0)		{ if((x>=5)&&(x<=70)) {Boiler.tempInLim=rd(x, 100); return true;} else return false; } else    // Tемпература подачи максимальная
	if(strcmp(var,boil_CIRCUL_WORK)==0) 	{ if((x>=0)&&(x<=60)){Boiler.Circul_Work=60*x; return true;} else return false;} else         // Время  работы насоса ГВС секунды (fCirculation)
	if(strcmp(var,boil_CIRCUL_PAUSE)==0)	{ if((x>=0)&&(x<=60)){Boiler.Circul_Pause=60*x; return true;} else return false;} else        // Пауза в работе насоса ГВС  секунды (fCirculation)
	if(strcmp(var,boil_RESET_HEAT)==0)		{ if(x) SETBIT1(Boiler.flags,fResetHeat); else SETBIT0(Boiler.flags,fResetHeat); return true;} else // флаг Сброса лишнего тепла в СО
	if(strcmp(var,boil_RESET_TIME)==0)		{ if((x>=1)&&(x<=10000)) {Boiler.Reset_Time=x; return true;} else return false; } else        // время сброса излишков тепла в секундах (fResetHeat)
	if(strcmp(var,boil_BOIL_TIME)==0)		{ if((x>=1)&&(x<=255)) {UpdatePIDbyTime(x, Boiler.pid_time, Boiler.pid); Boiler.pid_time=x; return true;} else return false; } else             // Постоянная интегрирования времени в секундах ПИД ГВС
	if(strcmp(var,boil_BOIL_PRO)==0)		{ if((x>=0)&&(x<=32)) {Boiler.pid.Kp=rd(x, 1000); return true;} else return false; } else  // Пропорциональная составляющая ПИД ГВС
#ifdef PID_FORMULA2
	if(strcmp(var,boil_BOIL_IN)==0)			{ if((x>=0)&&(x<=32)) {x *= Boiler.pid_time; Boiler.pid.Ki=rd(x, 1000); return true;} else return false; } else   // Интегральная составляющая ПИД ГВС
	if(strcmp(var,boil_BOIL_DIF)==0)		{ if((x>=0)&&(x<=32)) {Boiler.pid.Kd=rd(x / Boiler.pid_time, 1000); return true;} else return false; } else   // Дифференциальная составляющая ПИД ГВС
#else
	if(strcmp(var,boil_BOIL_IN)==0)			{ if((x>=0)&&(x<=32)) {Boiler.pid.Ki=rd(x, 1000); return true;} else return false; } else   // Интегральная составляющая ПИД ГВС
	if(strcmp(var,boil_BOIL_DIF)==0)		{ if((x>=0)&&(x<=32)) {Boiler.pid.Kd=rd(x, 1000); return true;} else return false; } else   // Дифференциальная составляющая ПИД ГВС
#endif
	if(strcmp(var,boil_BOIL_TEMP)==0)		{ if((x>=5)&&(x<=70)) {Boiler.tempPID=rd(x, 100); return true;} else return false; } else   // Целевая темпеартура ПИД ГВС
	if(strcmp(var,boil_ADD_HEATING)==0)		{ if(x) SETBIT1(Boiler.flags,fAddHeating); else SETBIT0(Boiler.flags,fAddHeating); return true;} else  // флаг использования тена для догрева ГВС
	if(strcmp(var,boil_fAddHeatingForce)==0){ if(x) SETBIT1(Boiler.flags,fAddHeatingForce); else SETBIT0(Boiler.flags,fAddHeatingForce); return true;} else
	if(strcmp(var,boil_HeatUrgently)==0)    { HP.set_HeatBoilerUrgently(x); return true;} else
	if(strcmp(var,hp_SUN)==0) 				{ Boiler.flags = (Boiler.flags & ~(1<<fBoilerUseSun)) | ((x!=0)<<fBoilerUseSun); return true; }else
	if(strcmp(var,boil_TEMP_RBOILER)==0)	{ if((x>=0)&&(x<=90))  {Boiler.tempRBOILER=rd(x, 100); return true;} else return false;} else   // температура включения догрева бойлера
	if(strcmp(var,boil_dAddHeat)==0)	    { Boiler.dAddHeat = rd(x, 100); return true;} else
	if(strcmp(var,boil_WF_MinTarget)==0)    { Boiler.WF_MinTarget = rd(x, 100); return true;} else
	if(strcmp(var,boil_WR_Target)==0)       { Boiler.WR_Target = rd(x, 100); return true;} else
	if(strcmp(var,boil_DischargeDelta)==0)	{ Boiler.DischargeDelta = rd(x, 10); return true;} else
	if(strcmp(var,boil_delayOffPump)==0)	{ Boiler.delayOffPump = x; return true;} else
	if(strcmp(var,hp_WorkPause)==0)         { if(x >= 0) { Boiler.WorkPause = x; return true; } else return false; } else
	if(strcmp(var,boil_fBoilerOnGenerator)==0){ if(x) SETBIT1(Boiler.flags, fBoilerOnGenerator); else SETBIT0(Boiler.flags, fBoilerOnGenerator); return true; } else
	if(strcmp(var,boil_fBoilerHeatElemSchPri)==0){ if(x) SETBIT1(Boiler.flags, fBoilerHeatElemSchPri); else SETBIT0(Boiler.flags, fBoilerHeatElemSchPri); return true; } else
	return false;
}

// Получить параметр из строки по имени var, результат ДОБАВЛЯЕТСЯ в строку ret
char* Profile::get_boiler(char *var, char *ret)
{
	if(strcmp(var,boil_CurrentTarget)==0){   _dtoa(ret, HP.sTemp[TBOILER].get_Temp(), 2); strcat(ret, "° "); if(GETBIT(SaveON.flags,fBoilerON)) goto xTargetTemp; else strcat(ret, "-/-"); return ret; } else
	if(strcmp(var,boil_BOILER_ON)==0){       if (GETBIT(SaveON.flags,fBoilerON))   return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,hp_fUseHeater)==0){       if (GETBIT(SaveON.flags,fBoiler_UseHeater))   return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_SCHEDULER_ON)==0){    if (GETBIT(Boiler.flags,fSchedule))   return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_SCHEDULER_ADDHEAT)==0){ if (GETBIT(Boiler.flags,fScheduleAddHeat)) return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_TOGETHER_HEAT)==0){ if (GETBIT(Boiler.flags,fBoilerTogetherHeat)) return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_fBoilerPID)==0){ if (GETBIT(Boiler.flags,fBoilerPID)) return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_TURBO_BOILER)==0){    if (GETBIT(Boiler.flags,fTurboBoiler))return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_LEGIONELLA)==0){      if (GETBIT(Boiler.flags,fLegionella)) return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_CIRCULATION)==0){     if (GETBIT(Boiler.flags,fCirculation))return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_fBoilerCircSchedule)==0){ if (GETBIT(Boiler.flags,fBoilerCircSchedule))return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_TEMP_TARGET)==0){     _dtoa(ret,Boiler.TempTarget/10,1); return ret;    }else
	if(strcmp(var,ADD_DELTA_TEMP)==0) 		{ _dtoa(ret,Boiler.add_delta_temp/10, 1); return ret; }else
	if(strcmp(var,ADD_DELTA_HOUR)==0) 		{ _itoa(Boiler.add_delta_hour, ret); return ret;           }else
	if(strcmp(var,ADD_DELTA_END_HOUR)==0) 	{ _itoa(Boiler.add_delta_end_hour, ret); return ret;    	}else
	if(strcmp(var,boil_DTARGET)==0){         _dtoa(ret,Boiler.dTemp/10,1); return ret;        }else
	if(strcmp(var,boil_TEMP_MAX)==0){        _dtoa(ret,Boiler.tempInLim/10,1); return ret;       }else
	if(strcmp(var,boil_SCHEDULER)==0){       return strcat(ret,get_Schedule(Boiler.Schedule));          }else
	if(strcmp(var,boil_CIRCUL_WORK)==0){     return _itoa(Boiler.Circul_Work/60,ret);                   }else                            // Время  работы насоса ГВС секунды (fCirculation)
	if(strcmp(var,boil_CIRCUL_PAUSE)==0){    return _itoa(Boiler.Circul_Pause/60,ret);                  }else                            // Пауза в работе насоса ГВС  секунды (fCirculation)
	if(strcmp(var,boil_RESET_HEAT)==0){      if (GETBIT(Boiler.flags,fResetHeat))   return  strcat(ret,(char*)cOne); else return  strcat(ret,(char*)cZero); }else       // флаг Сброса лишнего тепла в СО
	if(strcmp(var,boil_RESET_TIME)==0){      return  _itoa(Boiler.Reset_Time,ret);                      }else                            // время сброса излишков тепла в секундах (fResetHeat)
	if(strcmp(var,boil_BOIL_TIME)==0){       return  _itoa(Boiler.pid_time,ret);                        }else                            // Постоянная интегрирования времени в секундах ПИД ГВС
	if(strcmp(var,boil_BOIL_PRO)==0){        _dtoa(ret,Boiler.pid.Kp,3); return ret;        }else                            // Пропорциональная составляющая ПИД ГВС
#ifdef PID_FORMULA2
	if(strcmp(var,boil_BOIL_IN)==0){         _dtoa(ret,Boiler.pid.Ki/Boiler.pid_time,3); return ret;} else             // Интегральная составляющая ПИД ТН
	if(strcmp(var,boil_BOIL_DIF)==0){        _dtoa(ret,Boiler.pid.Kd*Boiler.pid_time,3); return ret;} else             // Дифференциальная составляющая ПИД ТН
#else
	if(strcmp(var,boil_BOIL_IN)==0){         _dtoa(ret,Boiler.pid.Ki,3); return ret;       }else                            // Интегральная составляющая ПИД ГВС
	if(strcmp(var,boil_BOIL_DIF)==0){        _dtoa(ret,Boiler.pid.Kd,3); return ret;       }else                            // Дифференциальная составляющая ПИД ГВС
#endif
	if(strcmp(var,boil_BOIL_TEMP)==0){       _dtoa(ret,Boiler.tempPID/10,1); return ret;       }else                            // Целевая темпеартура ПИД ГВС
	if(strcmp(var,boil_ADD_HEATING)==0){     if(GETBIT(Boiler.flags,fAddHeating)) return strcat(ret,(char*)cOne); else return strcat(ret,(char*)cZero); }else   // флаг использования тена для догрева ГВС
	if(strcmp(var,boil_fAddHeatingForce)==0){if(GETBIT(Boiler.flags,fAddHeatingForce)) return strcat(ret,(char*)cOne); else return strcat(ret,(char*)cZero); }else
	if(strcmp(var,hp_SUN)==0) { if(GETBIT(Boiler.flags,fBoilerUseSun)) return strcat(ret,(char*)cOne);else return strcat(ret,(char*)cZero);} else
	if(strcmp(var,boil_TEMP_RBOILER)==0){    _dtoa(ret,Boiler.tempRBOILER/10,1); return ret;    }else                            // температура включения догрева бойлера
	if(strcmp(var,boil_dAddHeat)==0){        _dtoa(ret,Boiler.dAddHeat/10,1); return ret;       }else
	if(strcmp(var,boil_WF_MinTarget)==0){    _dtoa(ret,Boiler.WF_MinTarget/10,1); return ret;    }else
	if(strcmp(var,boil_WR_Target)==0){       _dtoa(ret,Boiler.WR_Target/10,1); return ret;       }else
	if(strcmp(var,boil_DischargeDelta)==0){  _dtoa(ret, Boiler.DischargeDelta, 1); return ret;  }else
	if(strcmp(var,hp_WorkPause)==0) {        _itoa(Boiler.WorkPause, ret); return ret; } else
	if(strcmp(var,boil_HeatUrgently)==0){if(HP.HeatBoilerUrgently) return strcat(ret,(char*)cOne); else return strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_delayOffPump)==0){
#ifdef RPUMPBH	// насос бойлера
		return _itoa(Boiler.delayOffPump, ret);
#else
		return strcat(ret, "-");
#endif
	}else
	if(strcmp(var,boil_fBoilerOnGenerator)==0){ if(GETBIT(Boiler.flags, fBoilerOnGenerator)) return strcat(ret,(char*)cOne); else return strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_fBoilerHeatElemSchPri)==0){ if(GETBIT(Boiler.flags, fBoilerHeatElemSchPri)) return strcat(ret,(char*)cOne); else return strcat(ret,(char*)cZero); }else
	if(strcmp(var,boil_TargetTemp)==0) {
xTargetTemp:
	 if(GETBIT(HP.Prof.Boiler.flags, fAddHeating)) { // режим догрева
		 _dtoa(ret, (GETBIT(HP.Prof.Boiler.flags, fTurboBoiler) ? Boiler.tempRBOILER : HP.Boiler_Target_AddHeating()) / 10, 1);
		 strcat(ret, "/");
	 }
	 _dtoa(ret, HP.get_boilerTempTarget() / 10, 1);
	 return ret;
	} else
	return strcat(ret,(char*)cInvalid);
}

// static uint16_t crc= 0xFFFF;  // рабочее значение
uint16_t Profile::get_crc16_mem()  // Расчитать контрольную сумму
{
	uint16_t crc = 0xFFFF;
	crc = calc_crc16((uint8_t*)&dataProfile, sizeof(dataProfile), crc);
	crc = calc_crc16((uint8_t*)&SaveON, sizeof(SaveON), crc);
	crc = calc_crc16((uint8_t*)&Cool, sizeof(Cool), crc);
	crc = calc_crc16((uint8_t*)&Heat, sizeof(Heat), crc);
	crc = calc_crc16((uint8_t*)&Boiler, sizeof(Boiler), crc);
	crc = calc_crc16((uint8_t*)&DailySwitch, sizeof(DailySwitch), crc);
	return crc;
}

// Проверить контрольную сумму ПРОФИЛЯ в EEPROM для данных на выходе ошибка, длина определяется из заголовка
int8_t Profile::check_crc16_eeprom(int8_t num)
{
  uint16_t i, crc16tmp;
  byte x;
  uint16_t crc= 0xFFFF;
  int32_t adr = I2C_PROFILE_EEPROM + get_sizeProfile() * num;     // вычислить адрес начала данных
  
  if (readEEPROM_I2C(adr, (byte*)&x, sizeof(x))) { set_Error(ERR_LOAD_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(x);              // прочитать заголовок
  if (x != PROFILE_MAGIC) return OK;   // профиль пустой, или его вообще нет, контрольная сумма не актуальна
  if (readEEPROM_I2C(adr, (byte*)&crc16tmp, sizeof(crc16tmp))) { set_Error(ERR_LOAD_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(crc16tmp);        // прочитать crc16
  
  for (i=0;i<get_sizeProfile()-1-2;i++) {if (readEEPROM_I2C(adr+i, (byte*)&x, sizeof(x))) { set_Error(ERR_LOAD_PROFILE,(char*)nameHeatPump); return ERR_LOAD_PROFILE;}  crc=_crc16(crc,x);} // расчет - записанной в еепром сумму за вычетом заголовка
  if (crc==crc16tmp) return OK; 
  else            return err=ERR_CRC16_PROFILE;
}
// Проверить контрольную сумму в буфере для данных на выходе ошибка, длина определяется из заголовка
int8_t  Profile::check_crc16_buf(int32_t adr, byte* buf)   
{
  uint16_t i, readCRC, crc=0xFFFF;
  byte x;
  memcpy((byte*)&x,buf+adr,sizeof(x)); adr=adr+sizeof(x);                                              // заголовок
  if(x != PROFILE_MAGIC)   return OK;   // профиль пустой, или его вообще нет, контрольная сумма не актуальна
  memcpy((byte*)&readCRC,buf+adr,sizeof(readCRC)); adr=adr+sizeof(readCRC);                            // прочитать crc16
  // Расчет контрольной суммы
  for (i=0;i<get_sizeProfile()-1-2;i++) crc=_crc16(crc,buf[adr+i]);                                      // расчет -2 за вычетом CRC16 из длины и заголовка один байт
  if (crc==readCRC) return OK; 
  else              return err=ERR_CRC16_EEPROM;
}

int8_t  Profile::convert_to_new_version(void)
{
	/*
	  char checker(int);
	  char checkSizeOfInt1[sizeof(dataProfile)]={checker(&checkSizeOfInt1)};
	  char checkSizeOfInt2[sizeof(SaveON)]={checker(&checkSizeOfInt2)};
	  char checkSizeOfInt3[sizeof(Cool)]={checker(&checkSizeOfInt3)};
	  char checkSizeOfInt3[sizeof(Heat)]={checker(&checkSizeOfInt3)};
	  char checkSizeOfInt4[sizeof(Boiler)]={checker(&checkSizeOfInt4)};
	  char checkSizeOfInt5[sizeof(DailySwitch)]={checker(&checkSizeOfInt5)};
	//*/
	typeof(uint8_t) magic;
	typeof(uint16_t) crc16;
	uint16_t CNVPROF_SIZE_dataProfile, CNVPROF_SIZE_SaveON, CNVPROF_SIZE_Heat, CNVPROF_SIZE_Cool, CNVPROF_SIZE_Boiler, CNVPROF_SIZE_DailySwitch, CNVPROF_SIZE_ALL;
	if(HP.Option.ver < VER_SAVE) {
		if(HP.Option.ver <= 152) {
			CNVPROF_SIZE_dataProfile	=	120;
			CNVPROF_SIZE_SaveON			= 	12;
			CNVPROF_SIZE_Cool			=	38;
			CNVPROF_SIZE_Heat			=	38;
			CNVPROF_SIZE_Boiler			=	68;
#if I2C_SIZE_EEPROM >= 64
			CNVPROF_SIZE_DailySwitch	=	30;
#else
			CNVPROF_SIZE_DailySwitch	=	15;
#endif
			CNVPROF_SIZE_ALL = (sizeof(magic) + sizeof(crc16) + CNVPROF_SIZE_dataProfile + CNVPROF_SIZE_SaveON + CNVPROF_SIZE_Cool + CNVPROF_SIZE_Heat + CNVPROF_SIZE_Boiler + CNVPROF_SIZE_DailySwitch);
		} else if(HP.Option.ver <= 153) {
			CNVPROF_SIZE_dataProfile	=	120;
			CNVPROF_SIZE_SaveON			= 	12;
			CNVPROF_SIZE_Cool			=	42;
			CNVPROF_SIZE_Heat			=	42;
			CNVPROF_SIZE_Boiler			=	68;
#if I2C_SIZE_EEPROM >= 64
			CNVPROF_SIZE_DailySwitch	=	30;
#else
			CNVPROF_SIZE_DailySwitch	=	15;
#endif
			CNVPROF_SIZE_ALL = (sizeof(magic) + sizeof(crc16) + CNVPROF_SIZE_dataProfile + CNVPROF_SIZE_SaveON + CNVPROF_SIZE_Cool + CNVPROF_SIZE_Heat + CNVPROF_SIZE_Boiler + CNVPROF_SIZE_DailySwitch);
		} else if(HP.Option.ver <= 154) {
			CNVPROF_SIZE_dataProfile	=	120;
			CNVPROF_SIZE_SaveON			= 	12;
			CNVPROF_SIZE_Cool			=	48;
			CNVPROF_SIZE_Heat			=	48;
			CNVPROF_SIZE_Boiler			=	68;
#if I2C_SIZE_EEPROM >= 64
			CNVPROF_SIZE_DailySwitch	=	30;
#else
			CNVPROF_SIZE_DailySwitch	=	15;
#endif
			CNVPROF_SIZE_ALL = (sizeof(magic) + sizeof(crc16) + CNVPROF_SIZE_dataProfile + CNVPROF_SIZE_SaveON + CNVPROF_SIZE_Cool + CNVPROF_SIZE_Heat + CNVPROF_SIZE_Boiler + CNVPROF_SIZE_DailySwitch);
		} else if(HP.Option.ver <= 155) {
			CNVPROF_SIZE_dataProfile	=	120;
			CNVPROF_SIZE_SaveON			= 	12;
			CNVPROF_SIZE_Cool			=	50;
			CNVPROF_SIZE_Heat			=	50;
			CNVPROF_SIZE_Boiler			=	68;
#if I2C_SIZE_EEPROM >= 64
			CNVPROF_SIZE_DailySwitch	=	30;
#else
			CNVPROF_SIZE_DailySwitch	=	15;
#endif
			CNVPROF_SIZE_ALL = (sizeof(magic) + sizeof(crc16) + CNVPROF_SIZE_dataProfile + CNVPROF_SIZE_SaveON + CNVPROF_SIZE_Cool + CNVPROF_SIZE_Heat + CNVPROF_SIZE_Boiler + CNVPROF_SIZE_DailySwitch);
		} else if(HP.Option.ver <= 158) {
			CNVPROF_SIZE_dataProfile	=	120;
			CNVPROF_SIZE_SaveON			= 	12;
			CNVPROF_SIZE_Cool			=	54;
			CNVPROF_SIZE_Heat			=	54;
			CNVPROF_SIZE_Boiler			=	68;
#if I2C_SIZE_EEPROM >= 64
			CNVPROF_SIZE_DailySwitch	=	30;
#else
			CNVPROF_SIZE_DailySwitch	=	15;
#endif
			CNVPROF_SIZE_ALL = (sizeof(magic) + sizeof(crc16) + CNVPROF_SIZE_dataProfile + CNVPROF_SIZE_SaveON + CNVPROF_SIZE_Cool + CNVPROF_SIZE_Heat + CNVPROF_SIZE_Boiler + CNVPROF_SIZE_DailySwitch);

			HP.Option.SwitchHeaterHPTime = 15;
			HP.Option.Modbus_Attempts = 3;
		} else  { // last ver
			CNVPROF_SIZE_dataProfile	=	120;
			CNVPROF_SIZE_SaveON			= 	12;
			CNVPROF_SIZE_Cool			=	42;
			CNVPROF_SIZE_Heat			=	56;
			CNVPROF_SIZE_Boiler			=	72;
#if I2C_SIZE_EEPROM >= 64
			CNVPROF_SIZE_DailySwitch	=	30;
#else
			CNVPROF_SIZE_DailySwitch	=	15;
#endif
			CNVPROF_SIZE_ALL = (sizeof(magic) + sizeof(crc16) + CNVPROF_SIZE_dataProfile + CNVPROF_SIZE_SaveON + CNVPROF_SIZE_Cool + CNVPROF_SIZE_Heat + CNVPROF_SIZE_Boiler + CNVPROF_SIZE_DailySwitch);
		}
		journal.jprintf("Converting Profiles to new version...\n");
		if(readEEPROM_I2C(I2C_PROFILE_EEPROM, (byte*)&Socket[0].outBuf, CNVPROF_SIZE_ALL * I2C_PROFIL_NUM)) return ERR_LOAD_EEPROM;
		for(uint8_t i = 0; i < I2C_PROFIL_NUM; i++) {
			uint8_t *addr = (uint8_t*)&Socket[0].outBuf + CNVPROF_SIZE_ALL * i;
			//if(*addr != PROFILE_MAGIC) continue;
			addr += sizeof(magic) + sizeof(crc16);
			memcpy(&dataProfile, addr, CNVPROF_SIZE_dataProfile <= sizeof(dataProfile) ? CNVPROF_SIZE_dataProfile : sizeof(dataProfile));
			addr += CNVPROF_SIZE_dataProfile;
			memcpy(&SaveON, addr, CNVPROF_SIZE_SaveON <= sizeof(SaveON) ? CNVPROF_SIZE_SaveON : sizeof(SaveON));
			addr += CNVPROF_SIZE_SaveON;
			memcpy(&Cool, addr, CNVPROF_SIZE_Cool <= sizeof(Cool) ? CNVPROF_SIZE_Cool : sizeof(Cool));
			addr += CNVPROF_SIZE_Cool;
			memcpy(&Heat, addr, CNVPROF_SIZE_Heat <= sizeof(Heat) ? CNVPROF_SIZE_Heat : sizeof(Heat));
			addr += CNVPROF_SIZE_Heat;
			memcpy(&Boiler, addr, CNVPROF_SIZE_Boiler <= sizeof(Boiler) ? CNVPROF_SIZE_Boiler : sizeof(Boiler));
			addr += CNVPROF_SIZE_Boiler;
			memcpy(&DailySwitch, addr, CNVPROF_SIZE_DailySwitch <= sizeof(DailySwitch) ? CNVPROF_SIZE_DailySwitch : sizeof(DailySwitch));
			if(HP.Option.ver <= 158) { // flags 1b -> 2b
				//journal.printf("#%d - ", i);
				//for(uint8_t j = 0; j < sizeof(dataProfile); j++) journal.printf("%02X ", *((uint8_t *)&dataProfile + j));
				//journal.printf("\n");
				dataProfile.flags = dataProfile.ProfileNext;
				dataProfile.ProfileNext = 0;
				dataProfile.TimeStart = 0;
				dataProfile.TimeEnd = 0;
				memmove(&dataProfile.note, (void *)((uint32_t)&dataProfile.note - 1), sizeof(dataProfile.note) - 1);
				memmove((uint8_t *)&Cool + 2, &Cool, sizeof(Cool) - 2);
				Cool.flags = Cool.Rule;
				Cool.Rule = (RULE_HP)Cool._reserved_;
				memmove((uint8_t *)&Heat + 2, &Heat, sizeof(Heat) - 2);
				Heat.flags = Heat.Rule;
				Heat.Rule = (RULE_HP)Heat._reserved_;
				uint16_t _f = (uint16_t)Boiler.tempInLim;
				memmove((uint8_t *)&Boiler + 2, (uint8_t *)&Boiler, 10);
				SETBIT0(_f, fBoilerPID); _f |= (GETBIT(_f, fBoiler_reserved)<<fBoilerPID);
				SETBIT0(_f, fBoiler_reserved);
				Boiler.flags = _f;
				Boiler.WorkPause = Heat.WorkPause;
			}
			if(HP.Option.ver <= 156) {
				Boiler.pid_time = Boiler.delayOffPump > 255 ? 255 : Boiler.delayOffPump;
				Boiler.delayOffPump = HP.Option.delayOffPump;
			}
			if(save(i) < 0) return ERR_SAVE_EEPROM;
		}
		if(HP.save() < 0) return ERR_SAVE_EEPROM;
	}
	return OK;
}

// Записать профайл в еепром под номером num
// Возвращает число записанных байт или ошибку
int16_t  Profile::save(int8_t num)
{
  uint16_t crc16;
  dataProfile.saveTime=rtcSAM3X8.unixtime();    // запомнить время сохранения профиля
#ifdef WATTROUTER
  SaveON.WR_Loads = WR_Loads;
#endif

  int32_t adr = I2C_PROFILE_EEPROM + get_sizeProfile() * num;
  
  uint8_t magic = PROFILE_MAGIC;                         // заголовок
  if (writeEEPROM_I2C(adr, (byte*)&magic, sizeof(magic))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(magic);       // записать заголовок
  int32_t adrCRC16=adr;                                                                                                                                              // Запомнить адрес куда писать контрольную сумму
  adr=adr+sizeof(crc16);                                                                                                                                             // пропуск записи контрольной суммы
  if (writeEEPROM_I2C(adr, (byte*)&dataProfile, sizeof(dataProfile))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(dataProfile);// записать данные
  if (writeEEPROM_I2C(adr, (byte*)&SaveON, sizeof(SaveON))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(SaveON);    // записать состояние ТН
  if (writeEEPROM_I2C(adr, (byte*)&Cool, sizeof(Cool))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(Cool);          // записать настройки охлаждения
  if (writeEEPROM_I2C(adr, (byte*)&Heat, sizeof(Heat))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(Heat);          // записать настройи отопления
  if (writeEEPROM_I2C(adr, (byte*)&Boiler, sizeof(Boiler))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(Boiler);    // записать настройки ГВС
  if (writeEEPROM_I2C(adr, (byte*)&DailySwitch, sizeof(DailySwitch))) { set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return ERR_SAVE_PROFILE;}  adr=adr+sizeof(DailySwitch);    // записать настройки DailySwitch
  // запись контрольной суммы
  crc16=get_crc16_mem();
  if (writeEEPROM_I2C(adrCRC16, (byte*)&crc16, sizeof(crc16))) {set_Error(ERR_SAVE_PROFILE,(char*)nameHeatPump); return err=ERR_SAVE_PROFILE;} 

  if ((err=check_crc16_eeprom(num))!=OK) { journal.jprintf("- Verify Error!\n"); return (int16_t) err;}                            // ВЕРИФИКАЦИЯ Контрольные суммы не совпали
  journal.jprintf(" Save profile %d OK, wrote: %d bytes, crc: %04x\n", num + 1,get_sizeProfile(),crc16);                                                        // дошли до конца значит ошибок нет
  update_list(num);                                                                                                                                                  // обновить список
  return get_sizeProfile();
}

// загрузить профайл num из еепром память
int32_t Profile::load(int8_t num)
{
  byte magic;
  int32_t adr = I2C_PROFILE_EEPROM + get_sizeProfile() * num;     // вычислить адрес начала данных
   
  if (readEEPROM_I2C(adr, (byte*)&magic, sizeof(magic))) { set_Error(ERR_LOAD_PROFILE,(char*)nameHeatPump); return err=ERR_LOAD_PROFILE;}  adr=adr+sizeof(magic); // прочитать заголовок
 
  if (magic == PROFILE_EMPTY) {journal.jprintf(" Profile %d is empty\n", num + 1); return OK;}     // профиль пустой, загружать нечего, выходим
  if (magic != PROFILE_MAGIC)  {journal.jprintf(" Profile %d is bad format\n", num + 1); return OK; }    // профиль битый, читать нечего выходим

  #ifdef LOAD_VERIFICATION
    if ((err=check_crc16_eeprom(num))!=OK) { journal.jprintf(" Error load profile %d, CRC16 is wrong!\n", num + 1); return err;}  // проверка контрольной суммы перед чтением
  #endif

  uint16_t crc16;
  if (readEEPROM_I2C(adr, (byte*)&crc16, sizeof(crc16))) { set_Error(ERR_LOAD_PROFILE,(char*)nameHeatPump); return ERR_LOAD_PROFILE;}  adr=adr+sizeof(crc16); // прочитать crc16
  if (readEEPROM_I2C(adr, (byte*)&dataProfile, sizeof(dataProfile))) { set_Error(ERR_LOAD_PROFILE,(char*)nameHeatPump); return ERR_LOAD_PROFILE;}  adr=adr+sizeof(dataProfile); // прочитать данные
  id = num;
  
//  #ifdef LOAD_VERIFICATION
//  if (dataProfile.len!=get_sizeProfile())  { set_Error(ERR_BAD_LEN_PROFILE,(char*)nameHeatPump); return err=ERR_BAD_LEN_PROFILE;}   // длины не совпали
//  #endif
  
  // Выключение дневных реле, кроме HTTP!
  if(DailySwitch_on & ~DailySwitch_on_MASK_OFF) {
	  for(uint8_t i = 0; i < DAILY_SWITCH_MAX; i++) {
		  if(DailySwitch[i].Device == 0) break;
		  if(!GETBIT(DailySwitch_on, i)) continue;
		  if(DailySwitch[i].Device >= RNUMBER) {
			  DailySwitch_on |= 1<<(DailySwitch[i].Device - RNUMBER + 24);
			  continue;
		  } else HP.dRelay[DailySwitch[i].Device].set_Relay(-fR_StatusDaily); // OFF
	  }
	  DailySwitch_on &= DailySwitch_on_MASK_OFF;
  }

  magic = TaskSuspendAll(); // Запрет других задач
  // читаем основные данные
  if(readEEPROM_I2C(adr, (byte*) &SaveON, sizeof(SaveON))) err = ERR_LOAD_PROFILE; // прочитать состояние ТН
  else if(readEEPROM_I2C(adr += sizeof(SaveON), (byte*) &Cool, sizeof(Cool))) err = ERR_LOAD_PROFILE; // прочитать настройки охлаждения
  else if(readEEPROM_I2C(adr += sizeof(Cool), (byte*) &Heat, sizeof(Heat))) err = ERR_LOAD_PROFILE; // прочитать настройки отопления
  else if(readEEPROM_I2C(adr += sizeof(Heat), (byte*) &Boiler, sizeof(Boiler))) err = ERR_LOAD_PROFILE; // прочитать настройки ГВС
  else if(readEEPROM_I2C(adr += sizeof(Boiler), (byte*) &DailySwitch, sizeof(DailySwitch))) err = ERR_LOAD_PROFILE; // прочитать настройки DailySwitch
  adr += sizeof(DailySwitch);

  if(magic) xTaskResumeAll(); // Разрешение других задач
  if(err) {
	  set_Error(err, (char*) nameHeatPump);
	  return err;
  }
 
// ВСЕ ОК
   #ifdef LOAD_VERIFICATION
  // проверка контрольной суммы
  if(crc16!=get_crc16_mem()) { set_Error(ERR_CRC16_PROFILE,(char*)nameHeatPump); return err=ERR_CRC16_PROFILE;}                                                           // прочитать crc16
  if (get_sizeProfile() != adr-(I2C_PROFILE_EEPROM+get_sizeProfile()*num))  {err=ERR_BAD_LEN_EEPROM;set_Error(ERR_BAD_LEN_EEPROM,(char*)nameHeatPump); return err;} // Проверка длины
    journal.jprintf(" Load profile %d OK, read: %d bytes, crc: %04x\n", num + 1,adr-(I2C_PROFILE_EEPROM+get_sizeProfile()*num),crc16);
  #else
    journal.jprintf(" Load profile %d OK, read: %d bytes VERIFICATION OFF!\n", num + 1,adr-(I2C_PROFILE_EEPROM+get_sizeProfile()*num));
  #endif
  update_list(num); 
   
//  #ifdef TBOILER // Изменение максимальной температуры при включенном режиме легионелла
//  if (GETBIT(HP.Prof.Boiler.flags,fLegionella)) {HP.sTemp[TBOILER].set_maxTemp(LEGIONELLA_TEMP+300);journal.jprintf(" Set boiler max t=%.2d for legionella\n", HP.sTemp[TBOILER].get_maxTemp());}
//  else HP.sTemp[TBOILER].set_maxTemp(MAXTEMP[TBOILER]);
//  #endif

#ifdef WATTROUTER
  WR_Loads = SaveON.WR_Loads;
  if(GETBIT(WR.Flags, WR_fActive)) WR_Refresh = WR_Loads;
#endif

  if(SaveON.bTIN == 0) { // Первоначальное заполнение
	  for(uint8_t i = 0; i < TNUMBER; i++) {
		  if(HP.sTemp[i].get_setup_flags() & ((1<<fTEMP_as_TIN_average) | (1<<fTEMP_as_TIN_min))) SaveON.bTIN |= (1<<i);
	  }
  }

  return adr;
 }

/*
// Считать настройки из буфера на входе адрес с какого, на выходе код ошибки (меньше нуля)
int8_t Profile::loadFromBuf(uint32_t adr,byte *buf)
{
  uint16_t i;
  byte magic;
  uint32_t aStart=adr;
   
  // Прочитать заголовок
  memcpy((byte*)&magic,buf+adr,sizeof(magic)); adr=adr+sizeof(magic);                      // заголовок
  if(magic == PROFILE_EMPTY) { journal.jprintf(" Profile of memory is empty\n"); return OK; } // профиль пустой, загружать нечего, выходим
  if(magic != PROFILE_MAGIC) { journal.jprintf(" Profile of memory is bad format\n"); return OK; }  // профиль битый, читать нечего выходим

  // проверка контрольной суммы
  #ifdef LOAD_VERIFICATION 
  if((err = check_crc16_buf(aStart,buf)) != OK) { journal.jprintf(" Error load profile from file, crc is wrong!\n"); return err; }
  #endif 
  
  memcpy((byte*)&i, buf + adr, sizeof(i)); adr = adr + sizeof(i);                                                     // прочитать crc16
  
  // Прочитать переменные ТН
   memcpy((byte*)&dataProfile,buf+adr,sizeof(dataProfile)); adr=adr+sizeof(dataProfile);                              // прочитать структуры  dataProfile
   memcpy((byte*)&SaveON,buf+adr,sizeof(SaveON)); adr=adr+sizeof(SaveON);                                             // прочитать SaveON
   memcpy((byte*)&Cool,buf+adr,sizeof(Cool)); adr=adr+sizeof(Cool);                                                   // прочитать параметры охлаждения
   memcpy((byte*)&Heat,buf+adr,sizeof(Heat)); adr=adr+sizeof(Heat);                                                   // прочитать параметры отопления
   memcpy((byte*)&Boiler,buf+adr,sizeof(Boiler)); adr=adr+sizeof(Boiler);                                             // прочитать параметры бойлера
   memcpy((byte*)&DailySwitch,buf+adr,sizeof(DailySwitch)); adr=adr+sizeof(DailySwitch);                              // прочитать параметры DailySwitch

   #ifdef LOAD_VERIFICATION
    if (get_sizeProfile() != adr - aStart)  {err=ERR_BAD_LEN_EEPROM;set_Error(ERR_BAD_LEN_EEPROM,(char*)nameHeatPump); return err;}    // Проверка длины
    journal.jprintf(" Load profile from file OK, read: %d bytes, crc: %04x\n",adr-aStart,i);                                                                    // ВСЕ ОК
  #else
    journal.jprintf(" Load setting from file OK, read: %d bytes VERIFICATION OFF!\n",adr-aStart);
  #endif
  return OK;       
}
*/

// Профиль Установить параметры ТН из числа (float)
boolean Profile::set_paramProfile(char *var, char *c)
{
	uint32_t x = atoi(c);
	if(strcmp(var, prof_NAME_PROFILE) == 0) {
		urldecode(dataProfile.name, c, sizeof(dataProfile.name));
		return true;
	} else if(strcmp(var, prof_ENABLE_PROFILE) == 0) {
		if(strcmp(c, cZero) == 0) {
			SETBIT0(dataProfile.flags, fEnabled);
			return true;
		} else if(strcmp(c, cOne) == 0) {
			SETBIT1(dataProfile.flags, fEnabled);
			return true;
		}
	} else if(strcmp(var, prof_ID_PROFILE) == 0) {
		if(x == 0 || x > I2C_PROFIL_NUM) return false; // не верный номер профиля
		id = x - 1;
		return true;
	} else if(strcmp(var, prof_NOTE_PROFILE) == 0) {
		urldecode(dataProfile.note, c, sizeof(dataProfile.note));
		return true;
	} else if(strcmp(var, prof_DATE_PROFILE) == 0) { return true;
	} else if(strcmp(var, prof_fAutoSwitchProf_mode) == 0) { if(x == 1) SETBIT1(SaveON.flags, fAutoSwitchProf_mode); else SETBIT0(SaveON.flags, fAutoSwitchProf_mode); return true;
	} else if(strncmp(var, prof_DailySwitch, sizeof(prof_DailySwitch)-1) == 0) {
		var += sizeof(prof_DailySwitch)-1;
		uint32_t i = *(var + 1) - '0';
		if(i >= DAILY_SWITCH_MAX) return false;
		if(*var == prof_DailySwitchDevice) { // set_DSDn
			DailySwitch[i].Device = x;
		} else if(*var == prof_DailySwitchOn && x == 0 && *c != '0') { // [N][>,<]TOUT
			x = DS_TimeOn_Extended;
			if(*c == 'N') {// +ночью
				x += 2;
				c++;
			}
			if(strchr(c, '<')) ;
			else if(strchr(c, '>')) x += 1;
			else return false;
			if(strncmp(c, nameTemp[TOUT], m_strlen(nameTemp[TOUT])) == 0) {
				DailySwitch[i].TimeOn = x;
			} else return false;
		} else if(*var == prof_DailySwitchOff && DailySwitch[i].TimeOn >= DS_TimeOn_Extended) {
			DailySwitch[i].TimeOff = x; // градусы
		} else { // по времени
			uint32_t h = x / 10;
			if(h > 23) h = 23;
			uint32_t m = x % 10;
			if(m > 5) m = 5;
			x = h * 10 + m;
			if(*var == prof_DailySwitchOn) { // set_DSSn
				DailySwitch[i].TimeOn = x;
			} else if(*var == prof_DailySwitchOff) { // set_DSEn
				DailySwitch[i].TimeOff = x;
			}
		}
		return true;
	} else if(strcmp(var, prof_ProfileNext) == 0) {
		if(x > I2C_PROFIL_NUM || (int8_t)x == id + 1) return false; // не верный номер профиля
		dataProfile.ProfileNext = x;
		return true;
	} else if(strcmp(var, prof_TimeStart) == 0) {
		uint32_t h = x / 10;
		if(h > 23) h = 23;
		uint32_t m = x % 10;
		if(m > 5) m = 5;
		x = h * 10 + m;
		dataProfile.TimeStart = x;
		return true;
	} else if(strcmp(var, prof_TimeEnd) == 0) {
		uint32_t h = x / 10;
		if(h > 23) h = 23;
		uint32_t m = x % 10;
		if(m > 5) m = 5;
		x = h * 10 + m;
		dataProfile.TimeEnd = x;
		return true;
	} else if(strcmp(var, prof_fSwitchProfileNext_OnError) == 0) { if(x == 1) SETBIT1(dataProfile.flags, fSwitchProfileNext_OnError); else SETBIT0(dataProfile.flags, fSwitchProfileNext_OnError); return true;
	} else if(strcmp(var, prof_fSwitchProfileNext_ByTime) == 0) { if(x == 1) SETBIT1(dataProfile.flags, fSwitchProfileNext_ByTime); else SETBIT0(dataProfile.flags, fSwitchProfileNext_ByTime); return true;
	// параметры только чтение
	} else if(strcmp(var, prof_NUM_PROFILE) == 0) {
		return true;
	} 
	return false;
}
 // профиль Получить параметры по имени var, результат ДОБАВЛЯЕТСЯ в строку ret
char*   Profile::get_paramProfile(char *var, char *ret)
{
	if(strcmp(var,prof_NAME_PROFILE)==0)   { return strcat(ret,dataProfile.name);                             }else
	if(strcmp(var,prof_ENABLE_PROFILE)==0) { if (GETBIT(dataProfile.flags,fEnabled)) return  strcat(ret,(char*)cOne);
											 else                                    return  strcat(ret,(char*)cZero);}else
	if(strcmp(var,prof_ID_PROFILE)==0)     { return _itoa(id + 1,ret);                            }else
	if(strcmp(var,prof_NOTE_PROFILE)==0)   { return strcat(ret,dataProfile.note);                             }else
	if(strcmp(var,prof_DATE_PROFILE)==0)   { return DecodeTimeDate(dataProfile.saveTime,ret);                 }else// параметры только чтение
	if(strcmp(var,prof_NUM_PROFILE)==0)    { return _itoa(I2C_PROFIL_NUM,ret);                                }else
	if(strcmp(var, prof_fAutoSwitchProf_mode)==0) { return _itoa(GETBIT(SaveON.flags, fAutoSwitchProf_mode), ret); }else
	if(strcmp(var, prof_fSwitchProfileNext_OnError)==0) { return _itoa(GETBIT(dataProfile.flags, fSwitchProfileNext_OnError), ret); }else
	if(strcmp(var, prof_fSwitchProfileNext_ByTime)==0) { return _itoa(GETBIT(dataProfile.flags, fSwitchProfileNext_ByTime), ret); }else
	if(strcmp(var, prof_TimeStart)==0) { m_snprintf(ret + m_strlen(ret), 32, "%02d:%d0", dataProfile.TimeStart / 10, dataProfile.TimeStart % 10); return ret; }else
	if(strcmp(var, prof_TimeEnd)==0) { m_snprintf(ret + m_strlen(ret), 32, "%02d:%d0", dataProfile.TimeEnd / 10, dataProfile.TimeEnd % 10); return ret; }else
	if(strcmp(var, prof_ProfileNext)==0) { if(dataProfile.ProfileNext) _itoa(dataProfile.ProfileNext, ret); else strcat(ret, "-"); return ret; }else
	if(strncmp(var, prof_DailySwitch, sizeof(prof_DailySwitch)-1) == 0) { // Дубль в WebServer.ino -> Функция get_tblPDS
		var += sizeof(prof_DailySwitch)-1;
		uint8_t i = *(var + 1) - '0';
		if(i >= DAILY_SWITCH_MAX) return ret;
		if(*var == prof_DailySwitchDevice) {
			_itoa(DailySwitch[i].Device, ret);
		} else if(*var == prof_DailySwitchState) { // get_Prof(DSO)
			if(GETBIT(DailySwitch_on, i)) strcat(ret, "*");
		} else if(*var == prof_DailySwitchOn) {
			uint8_t on = DailySwitch[i].TimeOn;
			if(on >= DS_TimeOn_Extended) {
				if(on & 2) strcat(ret, "N");
				strcat(ret, nameTemp[TOUT]);
				strcat(ret, (on & 1) ? ">" : "<");
			} else m_snprintf(ret + m_strlen(ret), 32, "%02d:%d0", on / 10, on % 10);
		} else if(*var == prof_DailySwitchOff) {
			if(DailySwitch[i].TimeOn >= DS_TimeOn_Extended) _itoa((int8_t)DailySwitch[i].TimeOff, ret);
			else m_snprintf(ret + m_strlen(ret), 32, "%02d:%d0", DailySwitch[i].TimeOff / 10, DailySwitch[i].TimeOff % 10);
		}
		return ret;
	}
	return  strcat(ret,(char*)cInvalid);
}

// Возврат: 0 - выкл, 1 - вкл, -1 - в гистерезисе
int8_t Profile::check_DailySwitch(uint8_t idx, uint32_t hhmm)
{
#ifdef WATTROUTER
	int8_t wr = -1; // >=0 index WR relay, -1 - not WR relay, -2 - no turn off
	int8_t pin = DailySwitch[idx].Device < RNUMBER ? HP.dRelay[DailySwitch[idx].Device].get_pinD() : -(DailySwitch[idx].Device - RNUMBER + 1);
	for(int8_t i = 0; i < WR_NumLoads; i++) {
		if(pin != WR_Load_pins[i] /*|| !GETBIT(WR.Loads, i)*/) continue;
		if(WR_LoadRun[i] == WR.LoadPower[i]) return -1;
		wr = WR_LoadRun[i] ? -2 : i;
		break;
	}
#endif
	uint32_t st = DailySwitch[idx].TimeOn, end;
	int8_t ret = 0;
	if(st >= DS_TimeOn_Extended) {
		if(st & 2) { // ночь
			if(DailySwitch[idx].Device < RNUMBER) {
				st = TARIF_NIGHT_START * 100 + 1;
				end = (TARIF_NIGHT_END-1) * 100 + 59;
			} else { // HTTP relay
				st = TARIF_NIGHT_START * 100 + 10;
				end = (TARIF_NIGHT_END-1) * 100 + 50;
			}
		} else goto xCheckTemp;
	} else {
		st *= 10;
		end = DailySwitch[idx].TimeOff * 10;
	}
	ret = (end >= st && hhmm >= st && hhmm < end) || (end < st && (hhmm >= st || hhmm < end));
	if(ret && (st = DailySwitch[idx].TimeOn) >= DS_TimeOn_Extended) { // Ночью температура, было - ночью всегда, температура в другое время : if(!ret && (st = DailySwitch[idx].TimeOn) >= DS_TimeOn_Extended) {
		//ret = 0;
xCheckTemp:
		int16_t t = HP.sTemp[TOUT].get_Temp();
		int16_t trg = ((int8_t)DailySwitch[idx].TimeOff) * 100;
		if(st & 1) { // T>
			if(t > trg) {
				ret = 1;
				DailySwitchStateT = (DailySwitchStateT & (1<<idx));
			} else if(!GETBIT(DailySwitchStateT, idx) || trg - HP.Option.DailySwitchHysteresis * 10 > t) {
				ret = 0;
				DailySwitchStateT = (DailySwitchStateT & ~(1<<idx));
			} else ret = -1;
		} else { // T<
			if(t < trg) {
				ret = 1;
				DailySwitchStateT = (DailySwitchStateT & (1<<idx));
			} else if(!GETBIT(DailySwitchStateT, idx) || trg + HP.Option.DailySwitchHysteresis * 10 < t) {
				ret = 0;
				DailySwitchStateT = (DailySwitchStateT & ~(1<<idx));
			} else ret = -1;
		}
	}
#ifdef WATTROUTER
	if(wr != -1) {
		if(ret == 0) {
			if(wr == -2) ret = -1;
			else if(GETBIT(WR.Loads, wr)) SETBIT1(WR_Loads, wr);
		} else if(ret == 1) SETBIT0(WR_Loads, wr);
	}
#endif
	return ret;
}

// ДОБАВЛЯЕТ к строке с - описание профиля num
char* Profile::get_info(char *c, int8_t num)
{
	byte b;
	uint16_t crc16temp;
	char *out;
	out = c + strlen(c);
	int32_t adr = I2C_PROFILE_EEPROM + get_sizeProfile() * num;                          // вычислить адрес начала профиля
	if(readEEPROM_I2C(adr, (byte*) &b, sizeof(b))) {
		strcpy(out, "ERROR READ PROFILE");
		return c;
	}
	if(b == PROFILE_EMPTY) {
		strcpy(out, "EMPTY PROFILE"); // Данных нет
		return c;
	}
	if(b != PROFILE_MAGIC) {
		strcpy(out, "BAD FORMAT PROFILE"); // Заголовок не верен, данных нет
		return c;
	}
	adr = adr + sizeof(b);
	uint16_t crc16;
	if(readEEPROM_I2C(adr, (byte*) &crc16temp, sizeof(crc16))) {
		strcpy(out, "ERROR READ PROFILE");
		return c;
	}
	adr += sizeof(crc16);                                            // вычислить адрес начала данных
	adr += (uint8_t *)&dataProfile.flags - (uint8_t *)&dataProfile;
	if(readEEPROM_I2C(adr, &b, sizeof(b))) { strcpy(out, "ERROR READ PROFILE"); return c; }
	// формируем строку с описанием
	if(GETBIT(b, fEnabled)) strcat(out, "* ");                         // flags - отметка об использовании в списке
	uint32_t u32;
	adr += (uint8_t *)&dataProfile.saveTime - (uint8_t *)&dataProfile.flags;
	if(readEEPROM_I2C(adr, (byte*) &u32, sizeof(u32))) { strcpy(out, "ERROR READ PROFILE"); return c; }
	adr += (uint8_t *)&dataProfile.name - (uint8_t *)&dataProfile.saveTime;
	if(readEEPROM_I2C(adr, (byte*) out, sizeof(dataProfile.name))) { strcpy(out, "ERROR READ PROFILE"); return c; }
	strcat(out, ": ");
	out += strlen(out);
	adr += (uint8_t *)&dataProfile.note - (uint8_t *)&dataProfile.name;
	if(readEEPROM_I2C(adr, (byte*) out, sizeof(dataProfile.note))) { strcpy(out, "ERROR READ PROFILE"); return c; }
	strcat(out, " [");
	DecodeTimeDate(u32, out);
//    strcat(out," ");
//    strcat(out, uint16ToHex(crc16temp));
	strcat(out, "]");
	return c;
}

// стереть профайл num из еепром  (ставится признак пусто, данные не стираются)
int32_t Profile::erase(int8_t num)
{
	int32_t adr = I2C_PROFILE_EEPROM + get_sizeProfile() * num; // вычислить адрес начала профиля
	uint8_t xx;
	if(readEEPROM_I2C(adr, &xx, sizeof(xx))) { set_Error(ERR_LOAD_PROFILE, (char*) __FUNCTION__); return ERR_LOAD_PROFILE; }
	xx = xx == PROFILE_EMPTY ? PROFILE_MAGIC : PROFILE_EMPTY;
	if(writeEEPROM_I2C(adr, &xx, sizeof(xx))) { set_Error(ERR_SAVE_PROFILE, (char*) __FUNCTION__); return ERR_SAVE_PROFILE; }  // записать заголовок
	return get_sizeProfile(); // вернуть длину профиля
}

// ДОБАВЛЯЕТ к строке с - список возможных конфигураций num - текущий профиль,
char *Profile::get_list(char *c/*,int8_t num*/)
{
  return strcat(c,list);
}

// Устанавливает текущий профиль из номера списка, новый профиль;
int8_t Profile::set_list(int8_t num)
{
	if(num != id) { // new
		HP.SwitchToProfile(num);
	}
	return num;
}

// обновить список имен профилей, запоминается в строке list
// Возможно что будет отсутвовать выбранный элемент - это нормально
// такое будет догда когда текущйий профиль не отмечен что учасвует в списке
int8_t Profile::update_list(int8_t num)
{
	byte b;
	uint32_t adr;
	char *p = list;
	list[0] = '\0';														// стереть список
	for(uint8_t i = 0; i < I2C_PROFIL_NUM; i++) {								// перебор по всем профилям
		adr = I2C_PROFILE_EEPROM + get_sizeProfile() * i;				// вычислить адрес начала профиля
		if(readEEPROM_I2C(adr, (byte*) &b, sizeof(b))) continue;
		if(b != PROFILE_MAGIC) continue;								// заголовок не верен, данных нет, пропускаем чтение профиля это не ошибка
		adr += 1 + 2;													// + magic + CRC16
		adr += (uint8_t *)&dataProfile.flags - (uint8_t *)&dataProfile;
		if(readEEPROM_I2C(adr, &b, sizeof(b))) continue;				// прочитать флаги
		if((GETBIT(b, fEnabled)) || (i == num))	{						// если разрешено использовать или ТЕКУЩИЙ профиль
			p += strlen(p);
			_itoa(i + 1, p);
			strcat(p, ". ");
			p += strlen(p);
			adr += (uint8_t *)&dataProfile.name - (uint8_t *)&dataProfile.flags;
			readEEPROM_I2C(adr, (byte*) p, LEN_PROFILE_NAME);
			if(i == num) strcat(p, ":1;"); else strcat(p, ":0;");
		}
	}
	return OK;
}

// проверка нужно ли переключиться на ProfileNext, возвращает номер профиля+1 или 0, если нет
uint8_t Profile::check_switch_to_ProfileNext_byTime(type_dataProfile *dp) // только поля: flags, ProfileNext, TimeStart, TimeEnd
{
	if(GETBIT(HP.work_flags, fHP_ProfileSetByError)) return 0;	// Профиль установлен по переключению из-за ошибки, для дальнейшей автосмены нужно ручное вмешательство
	uint32_t hhmm = rtcSAM3X8.get_hours() * 100 + rtcSAM3X8.get_minutes();
	uint32_t st = dp->TimeStart * 10;
	uint32_t end = dp->TimeEnd * 10;
	return GETBIT(dp->flags, fSwitchProfileNext_ByTime) && dp->ProfileNext
			&& !((end >= st && hhmm >= st && hhmm < end) || (end < st && (hhmm >= st || hhmm < end))) ? dp->ProfileNext : 0; 	// время профиля вышло
}
