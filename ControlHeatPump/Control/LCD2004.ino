/*
 * Copyright (c) 2016-2020 by Vadim Kulakov vad7@yahoo.com, vad711
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
#include "Constant.h"

#ifdef LCD2004

#include "LCD2004.h"

// LCD ---------- rs, en, d4, d5, d6, d7
LiquidCrystal lcd(23, 24, 25, 26, 27, 28);

//////////////////////////////////////////////////////////////////////////
uint32_t LCD_setup = 0; // 0x8000MMII: 8 - Setup active, MМ - Menu item (0..<max LCD_SetupMenuItems-1>) , II - Selecting item (0...)

// Задача Пользовательский интерфейс (HP.xHandleKeysLCD) "KeysLCD"
void vKeysLCD( void * )
{
	pinMode(PIN_KEY_UP, INPUT_PULLUP);
	pinMode(PIN_KEY_DOWN, INPUT_PULLUP);
	pinMode(PIN_KEY_OK, INPUT_PULLUP);
	lcd.begin(LCD_COLS, LCD_ROWS); // Setup: cols, rows
	lcd.print((char*)"HeatPump v");
	lcd.print((char*)VERSION);
	lcd.setCursor(0, 3);
	lcd.print((char*)"vad7@yahoo.com");
	vTaskDelay(3000);
	static uint32_t DisplayTick = xTaskGetTickCount();
	static char buffer[ALIGN(LCD_COLS * 2)];
	static uint32_t setup_timeout = 0;
	static uint32_t _FlowPulseCounterRest;
	for(;;)
	{
		if(LCD_setup) if(--setup_timeout == 0) goto xSetupExit;
		if(!digitalReadDirect(PIN_KEY_OK)) {
			vTaskDelay(KEY_DEBOUNCE_TIME);
			if(LCD_setup) {
				{
					uint32_t t = 2000 / KEY_DEBOUNCE_TIME;
					while(!digitalReadDirect(PIN_KEY_OK)) {
						if(--t == 0) {
							lcd.clear();
							while(!digitalReadDirect(PIN_KEY_OK)) vTaskDelay(KEY_DEBOUNCE_TIME);
							break;
						}
						vTaskDelay(KEY_DEBOUNCE_TIME);
					}
					if(t == 0) goto xSetupExit;
				}
				if((LCD_setup & 0xFFFF) == 0) { // menu item 1 selected - Exit
xSetupExit:
					lcd.command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
					LCD_setup = 0;
					setup_timeout = 0;
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Relays) { // inside menu item selected - Relay
					HP.dRelay[LCD_setup & 0xFF].set_Relay(HP.dRelay[LCD_setup & 0xFF].get_Relay() ? fR_StatusAllOff : fR_StatusManual);
//					if((LCD_setup & 0xFF) == RFILL) FillingTankLastLevel = 0;
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Options) { // inside menu item selected - Options
//					HP.fNetworkReset = 1;
					goto xSetupExit;
				} else if((LCD_setup & 0xFF00) == 0) {	// select menu item
					LCD_setup = (LCD_setup << 8) | LCD_SetupFlag;
					if((LCD_setup & 0xFF00) == LCD_SetupMenu_FlowCheck) { // Flow check
//						FlowPulseCounter = 0;
//						FlowPulseCounterRest = _FlowPulseCounterRest = HP.sFrequency[FLOW].PassedRest;
						lcd.command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
					} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Sensors) {
						lcd.command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
					}
				} else { // inside menu item selected
					goto xSetupExit;
				}
				DisplayTick = ~DisplayTick;
				setup_timeout = DISPLAY_SETUP_TIMEOUT / KEY_CHECK_PERIOD;
			} else if(HP.get_errcode() && !Error_Beep_confirmed) Error_Beep_confirmed = true; // Supress beeping
			else { // Enter Setup
				LCD_setup = LCD_SetupFlag;
				lcd.command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON);
				DisplayTick = ~DisplayTick;
				setup_timeout = DISPLAY_SETUP_TIMEOUT / KEY_CHECK_PERIOD;
			}
			while(!digitalReadDirect(PIN_KEY_OK)) vTaskDelay(KEY_DEBOUNCE_TIME);
			//journal.jprintfopt("[OK]\n");
		} else if(!digitalReadDirect(PIN_KEY_UP)) {
			vTaskDelay(KEY_DEBOUNCE_TIME);
			while(!digitalReadDirect(PIN_KEY_UP)) vTaskDelay(KEY_DEBOUNCE_TIME);
			if(LCD_setup) {
				if((LCD_setup & 0xFF00) == 0) { // select menu item
					if((LCD_setup & 0xFF) < LCD_SetupMenuItems-1) {
						LCD_setup++;
						DisplayTick = ~DisplayTick;
					}
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Relays) {
					LCD_setup++;
					if((LCD_setup & 0xFF) >= (RNUMBER > LCD_SetupMenu_Relays_Max ? LCD_SetupMenu_Relays_Max : RNUMBER)) {
						LCD_setup &= ~0xFF;
					}
					DisplayTick = ~DisplayTick;
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Options) { // Options
					goto xSetupExit;
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Sensors) DisplayTick = ~DisplayTick;
				setup_timeout = DISPLAY_SETUP_TIMEOUT / KEY_CHECK_PERIOD;
			} else {
xErrorsProcessing:
				if(HP.get_errcode()) {
					if(!Error_Beep_confirmed) Error_Beep_confirmed = true;
					else {
						lcd.setCursor(0, 2);
						lcd.print(" OK - CLEAR ERRORS? ");
						setup_timeout = 10000 / KEY_CHECK_PERIOD;
						while(--setup_timeout) {
							vTaskDelay(KEY_CHECK_PERIOD);
							if(!digitalReadDirect(PIN_KEY_UP) || !digitalReadDirect(PIN_KEY_DOWN)) break;
							if(!digitalReadDirect(PIN_KEY_OK)) {
//								HP.clear_all_errors();
								break;
							}
						}
						do { vTaskDelay(KEY_DEBOUNCE_TIME); } while(!digitalReadDirect(PIN_KEY_UP) || !digitalReadDirect(PIN_KEY_DOWN) || !digitalReadDirect(PIN_KEY_OK));
						DisplayTick = ~DisplayTick;
					}
				}
			}
			//journal.jprintfopt("[UP]\n");
		} else if(!digitalReadDirect(PIN_KEY_DOWN)) {
			vTaskDelay(KEY_DEBOUNCE_TIME);
			while(!digitalReadDirect(PIN_KEY_DOWN)) vTaskDelay(KEY_DEBOUNCE_TIME);
			if(LCD_setup) {
				if((LCD_setup & 0xFF00) == LCD_SetupMenu_Temp) { // Temperature

				} else if((LCD_setup & 0xFF00) == 0) {
					if(LCD_setup & 0xFF) {
						LCD_setup--;
						DisplayTick = ~DisplayTick;
					}
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Relays) {
					if((LCD_setup & 0xFF) != 0) LCD_setup--; else LCD_setup |= RNUMBER > LCD_SetupMenu_Relays_Max ? LCD_SetupMenu_Relays_Max-1 : RNUMBER-1;
					DisplayTick = ~DisplayTick;
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Options) { // Options
					goto xSetupExit;
				}
				setup_timeout = DISPLAY_SETUP_TIMEOUT / KEY_CHECK_PERIOD;
			} else goto xErrorsProcessing;
			//journal.jprintfopt("[DWN]\n");
		}
		if(xTaskGetTickCount() - DisplayTick > DISPLAY_UPDATE) { // Update display
			// Display:
			// 12345678901234567890
			// [Hp2]: Heating
			// Target: 21.2→24.0°
			// Boiler: 54.2→60.0°
			// Freq: 50.2
			char *buf = buffer;
			if(LCD_setup) {
				if((LCD_setup & 0xFF00) == LCD_SetupMenu_Relays) { // Relays
					lcd.clear();
					for(uint8_t i = 0; i < (RNUMBER > LCD_SetupMenu_Relays_Max ? LCD_SetupMenu_Relays_Max : RNUMBER) ; i++) {
						lcd.setCursor(10 * (i % 2), i / 2);
						lcd.print(HP.dRelay[i].get_Relay() ? '*' : ' ');
						lcd.print(HP.dRelay[i].get_name());
					}
					lcd.setCursor(10 * ((LCD_setup & 0xFF) % 2), (LCD_setup & 0xFF) / 2);
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Sensors) { // Sensors
					lcd.clear();
					for(uint8_t i = 0; i < (INUMBER > 8 ? 8 : INUMBER) ; i++) {
						lcd.setCursor(10 * (i % 2), i / 2);
						lcd.print(HP.sInput[i].get_Input() ? '*' : ' ');
						lcd.print(HP.sInput[i].get_name());
					}
					lcd.setCursor(10 * ((LCD_setup & 0xFF) % 2), (LCD_setup & 0xFF) / 2);
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Temp) {
					// 12345678901234567890
					// Edges: 41234
					// Liters: 112.1234
					// Flow: 2332
					// Sp: 124.123  154.123
					lcd.setCursor(0, 0);
					strcpy(buf, "Flow: "); buf += 6;
//					buf = dptoa(buf, HP.sFrequency[FLOW].get_Value(), 3);
//					buffer_space_padding(buf, LCD_COLS - (buf - buffer));
					lcd.print(buffer);

					lcd.setCursor(0, 1);
					strcpy(buf = buffer, "Edges: "); buf += 7;
//					uint32_t tmp = (FlowPulseCounter * HP.sFrequency[FLOW].get_kfValue() + FlowPulseCounterRest - _FlowPulseCounterRest) / 100;
//					buf += i10toa(tmp, buf, 0);
//					buffer_space_padding(buf, LCD_COLS - (buf - buffer));
					lcd.print(buffer);

					lcd.setCursor(0, 2);
					strcpy(buf = buffer, "Liters: "); buf += 8;
//					tmp *= 100;
//					buf += i10toa(tmp / HP.sFrequency[FLOW].get_kfValue(), buf, 0);
					*buf++ = '.';
//					buf += i10toa((uint32_t)(tmp % HP.sFrequency[FLOW].get_kfValue()) * 10000 / HP.sFrequency[FLOW].get_kfValue(), buf, 4);
//					buffer_space_padding(buf, LCD_COLS - (buf - buffer));
					lcd.print(buffer);

					lcd.setCursor(0, 3);
					strcpy(buf = buffer, "Sp: "); buf += 4;
//					buf = dptoa(buf, HP.CalcFilteringSpeed(HP.FilterTankSquare), 3);
//					*buf++ = ' '; *buf++ = ' ';
//					buf = dptoa(buf, HP.CalcFilteringSpeed(HP.FilterTankSoftenerSquare), 3);
//					buffer_space_padding(buf, LCD_COLS - (buf - buffer));
					lcd.print(buffer);

					DisplayTick = xTaskGetTickCount() - (DISPLAY_UPDATE - 1000);
					vTaskDelay(KEY_CHECK_PERIOD);
					continue;
				} else if((LCD_setup & 0xFF00) == LCD_SetupMenu_Options) { // Options
					lcd.clear();

					lcd.print("Ok - Network reset");
				} else {
					lcd.clear();
					lcd.print(LCD_SetupMenu[LCD_setup & 0xFF]);
					lcd.setCursor(0, 1);
					lcd.print("Long press OK - Exit");
					lcd.setCursor(0, 3);
					NowDateToStr(buffer);
					buffer[10] = ' ';
					NowTimeToStr(buffer + 11);
					lcd.print(buffer);
					lcd.setCursor(0, 0);
				}
			} else {
				lcd.setCursor(0, 0);
				int32_t tmp = 0; //HP.sFrequency[FLOW].get_Value();
				buf = dptoa(buf, tmp, 3);
				strcpy(buf, " m3h "); buf += 5;
//				if(HP.sInput[REG_ACTIVE].get_Input() || HP.sInput[REG_BACKWASH_ACTIVE].get_Input() || HP.sInput[REG2_ACTIVE].get_Input()) {
//					*buf++ = 'R';
//					tmp = HP.RTC_store.UsedRegen;
//				} else {
//					*buf++ = HP.NO_Power ? 'x' : LowConsumeMode ? 'G' : '\x7E'; // '->'
//					tmp = HP.WorkStats.UsedSinceLastRegen + HP.RTC_store.UsedToday;
//				}
				if(tmp < 10000) *buf++ = ' ';
				buf = dptoa(buf, tmp, 3);
//				buffer_space_padding(buf, LCD_COLS - (buf - buffer));
				lcd.print(buffer);

				lcd.setCursor(0, 1);
				strcpy(buf = buffer, "P: "); buf += 3;
//				buf = dptoa(buf, HP.sADC[PWATER].get_Value(), 2);
//				strcpy(buf, " Tank: "); buf += 7;
//				buf = dptoa(buf, HP.sADC[LTANK].get_Value() / 100, 0);
//				*buf++ = '%';
//				buffer_space_padding(buf, LCD_COLS - (buf - buffer));
				lcd.print(buffer);

				lcd.setCursor(0, 2);
				strcpy(buf = buffer, "Day:"); buf += 4;
//				tmp = HP.RTC_store.UsedToday;
				if(tmp < 10000) *buf++ = ' ';
				buf = dptoa(buf, tmp, 3);
				strcpy(buf, " Yd:"); buf += 4;
//				tmp = HP.WorkStats.UsedYesterday;
				if(tmp < 10000) *buf++ = ' ';
				buf = dptoa(buf, tmp, 3);
//				buffer_space_padding(buf, LCD_COLS - (buf - buffer));
				lcd.print(buffer);

				lcd.setCursor(0, 3);
				buf = buffer; *buf = '\0';
				// 12345678901234567890
				// FLOOD! EMPTY! ERR-21
//				if(CriticalErrors & ERRC_Flooding) {
//					if(HP.sInput[FLOODING].get_Input()) {
//						strcpy(buf, "FLOOD! "); buf += 7;
//					} else {
//						strcpy(buf, "LEAK! "); buf += 6;
//					}
//				}
//				if(CriticalErrors & ERRC_TankEmpty) {
//					strcpy(buf, "EMPTY! "); buf += 7;
//				}
//				if(CriticalErrors & ERRC_SepticAlarm) {
//					strcpy(buf, "SEPTIC! "); buf += 8;
//				}
//				if(CriticalErrors & ERRC_WeightEmpty) {
//					strcpy(buf, "BRINE! "); buf += 7;
//					goto xShowWeight;
//				}
				if(HP.get_errcode()) {
					strcpy(buf, "ERR"); buf += 3;
					buf = dptoa(buf, HP.get_errcode(), 0);
//					if(HP.get_errcode() == ERR_WEIGHT_LOW || HP.get_errcode() == ERR_WEIGHT_EMPTY) {
//						strcpy(buf, " W:"); buf += 3;
//						goto xShowWeight;
//					}
				}
//				if(buf == buffer) {
//					if((tmp = HP.dPWM.get_Power()) != 0) {
//						strcpy(buf, "Power,W: "); buf += 9;
//						buf = dptoa(buf, tmp, 0);
//					} else {
//						if((setup_timeout = !setup_timeout)) {
//							strcpy(buf, "Weight:"); buf += 7;
//							if(Weight_Percent < 10000) *buf++ = ' ';
//xShowWeight:
//							buf = dptoa(buf, Weight_value / 10, 0);
//							*buf++ = '\x7E'; // '->'
//							buf = dptoa(buf, Weight_Percent / 10, 1);
//							*buf++ = '%';
//						} else {
//							strcpy(buf, "Days,Fe:"); buf += 8;
//							tmp = HP.WorkStats.DaysFromLastRegen;
//							if(tmp < 100) *buf++ = ' ';
//							buf = dptoa(buf, tmp, 0);
//							strcpy(buf, " Soft:"); buf += 6;
//							tmp = HP.WorkStats.DaysFromLastRegenSoftening;
//							if(tmp < 100) *buf++ = ' ';
//							buf = dptoa(buf, tmp, 0);
//						}
//					}
//				}
//				tmp = LCD_COLS - (buf - buffer);
//				if(tmp > 0)	buffer_space_padding(buf, tmp); else buffer[LCD_COLS] = '\0';
				lcd.print(buffer);
			}

			DisplayTick = xTaskGetTickCount();
		}
		vTaskDelay(KEY_CHECK_PERIOD);
	}
	vTaskSuspend(NULL);
}



#endif
