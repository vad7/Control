/*
 * Ваттроутер - нагрев бойлера и других потребителей от солнечного инвертора, 
 *   работающего в режиме подкачки в сеть
 * 
 * Copyright (c) 2016-2026 by Vadim Kulakov vad7@yahoo.com, vad711
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
#include "Wattrouter.h"
#include "HeatPump.h"

void WR_Process(void)
{
			if((GETBIT(WR.Flags, WR_fActive) || WR.PWM_FullPowerTime || WR_Refresh) /*&& HP.get_State() == pWORK_HP*/) {
				while(1) {
					static uint8_t skip_next_small_increase = 0;
					if(skip_next_small_increase) skip_next_small_increase--;
#ifdef PWM_CALC_POWER_ARRAY
					if(GETBIT(PWM_CalcFlags, PWM_fCalcNow)) break;
#endif
					uint8_t nopwr = GETBIT(WR.Flags, WR_fActive) && (GETBIT(HP.Option.flags, fBackupPower) || HP.NO_Power
#ifdef WR_PowerMeter_Modbus
									|| WR_Error_Read_PowerMeter > WR_Error_Read_PowerMeter_Max
#endif
									 );
#ifdef WR_NEXTION_FULL_SUN
					int8_t mppt = -1;
					if(GETBIT(WR_WorkFlags, WR_fWF_Read_MPPT) || WR_LastSunPowerOutCnt == 0 || nopwr) {
						mppt = WR_Check_MPPT();				// Чтение солнечного контроллера
						SETBIT0(WR_WorkFlags, WR_fWF_Read_MPPT);
					}
#endif
					// Выключить все
					if(nopwr) {
						WR_Refresh |= WR_Loads;
						// При отсутствии питания мониторим напряжение на АКБ (не должно быть ниже буферного - дельта)
						int16_t Vd;
#if defined(HTTP_MAP_Read_MAP) && defined(WR_NOPWR_READ_MAP_INSTEAD_OF_MPPT)
						Vd = WR_Read_MAP();
#else
						if(WR_MAP_Ubat) Vd = WR_MAP_Ubuf - WR_MAP_Ubat; else Vd = -32768;
#endif //HTTP_MAP_Read_MAP
						if(Vd != -32768) {
							if(Vd <= WR.DeltaUbatmin) { // Можно нагружать
#ifdef WR_Load_pins_Boiler_INDEX
								if(HP.sTemp[TBOILER].get_Temp() <= HP.Prof.Boiler.WR_Target && GETBIT(WR_Loads, WR_Load_pins_Boiler_INDEX)) { // Нужно греть бойлер
									if(Vd <= 0) { // Можно увеличивать нагрузку
										if(WR_LoadRun[WR_Load_pins_Boiler_INDEX] > 0 || HP.sTemp[TBOILER].get_Temp() <= HP.Prof.Boiler.WR_Target - WR_Boiler_Hysteresis) {
											if(GETBIT(WR.PWM_Loads, WR_Load_pins_Boiler_INDEX)) WR_Change_Load_PWM(WR_Load_pins_Boiler_INDEX, WR.LoadAdd);
											else WR_Switch_Load(WR_Load_pins_Boiler_INDEX, 1);
											Vd = WR_NO_POWER_WORK_DELTA_Uacc - 1; // Выключить остальные нагрузки
										}
										goto xNOPWR_OtherLoad;
									} else if(Vd > WR_NO_POWER_WORK_DELTA_Uacc) { // Уменьшаем или выключаем
										if(GETBIT(WR.PWM_Loads, WR_Load_pins_Boiler_INDEX)) WR_Change_Load_PWM(WR_Load_pins_Boiler_INDEX, -WR.LoadAdd);
									}
								} else { // Другая нагрузка
									if(WR_LoadRun[WR_Load_pins_Boiler_INDEX] > 0) {
										if(GETBIT(WR.PWM_Loads, WR_Load_pins_Boiler_INDEX)) WR_Change_Load_PWM(WR_Load_pins_Boiler_INDEX, -32768);
										else WR_Switch_Load(WR_Load_pins_Boiler_INDEX, 0);
									}
#else
								{
#endif
xNOPWR_OtherLoad:					uint32_t t = rtcSAM3X8.unixtime();
									for(uint8_t i = 0; i < WR_NumLoads; i++) { // Управляем еще одной нагрузкой
										if(i == WR_Load_pins_Boiler_INDEX || !GETBIT(WR_Loads, i)) continue;
										if(Vd <= 0) { // Можно увеличивать нагрузку
											if(WR_LoadRun[i] < WR.LoadPower[i]) {
												if(GETBIT(WR.PWM_Loads, i)) {
													WR_Change_Load_PWM(i, WR.LoadAdd);
												} else if(Vd < 0) { // +0.1V на АКБ для реле
													if(WR_SwitchTime[i] && t - WR_SwitchTime[i] <= WR.TurnOnPause) continue;
													WR_Switch_Load(i, 1);
												}
											}
										} else if(Vd > WR_NO_POWER_WORK_DELTA_Uacc) { // Уменьшаем или выключаем
											if(WR_LoadRun[i] > 0) {
												if(GETBIT(WR.PWM_Loads, i)) WR_Change_Load_PWM(i, -WR.LoadAdd); else WR_Switch_Load(i, 0);
											}
										}
										break;
									}
								}
								break; // больше ни чего не делаем
							} // отключаем нагрузку
						}
					}
					if(WR_Refresh || WR.PWM_FullPowerTime) {
						for(uint8_t i = 0; i < WR_NumLoads; i++) {
							//if(!GETBIT(WR_Loads, i)) continue;
							if(GETBIT(WR.PWM_Loads, i)) {
								if(nopwr) {
									if(WR_LoadRun[i] == 0) continue;
									WR_Change_Load_PWM(i, -32768);
								} else if(GETBIT(WR_Refresh, i) || (WR.PWM_FullPowerTime && WR_LoadRun[i] && rtcSAM3X8.unixtime() - WR_SwitchTime[i] > WR.PWM_FullPowerTime * 60)) {
									WR_Change_Load_PWM(i, 0);
								}
							} else if(GETBIT(WR_Refresh, i)) {
								if(nopwr) {
									if(WR_LoadRun[i] == 0) continue;
									WR_Switch_Load(i, 0);
								} else WR_Switch_Load(i, WR_LoadRun[i] ? true : false);
//								if(WR_Load_pins[i] < 0) {
//									WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
//								}
							}
						}
						WR_Refresh = 0;
					}
#ifdef WR_Load_pins_Boiler_INDEX
					if(GETBIT(WR_Loads, WR_Load_pins_Boiler_INDEX) && !HP.dRelay[RBOILER].get_Relay()) {
#ifdef WR_Boiler_Substitution_INDEX
						if(digitalReadDirect(PIN_WR_Boiler_Substitution)) {
							if(WR_LoadRun[WR_Boiler_Substitution_INDEX] == 0 && HP.sTemp[TBOILER].get_Temp() <= HP.Prof.Boiler.WR_Target - WR_Boiler_Hysteresis) {
								digitalWriteDirect(PIN_WR_Boiler_Substitution, 0); // Переключаемся на бойлер
								if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf_time("WR: SW->Boiler\n");
							}
						} else
#endif
						{
							int16_t curr = WR_LoadRun[WR_Load_pins_Boiler_INDEX];
							if(curr > 0) {
								if(WR_TestLoadStatus) {
									if(HP.sTemp[TBOILER].get_Temp() > WR_BOILER_MAX_TEMP) { // Перегрели
										if(GETBIT(WR.PWM_Loads, WR_Load_pins_Boiler_INDEX)) WR_Change_Load_PWM(WR_Load_pins_Boiler_INDEX, -32768);
										else WR_Switch_Load(WR_Load_pins_Boiler_INDEX, 0);
									}
								} else if(HP.sTemp[TBOILER].get_Temp() > HP.Prof.Boiler.WR_Target) { // Нагрели
									if(GETBIT(WR.PWM_Loads, WR_Load_pins_Boiler_INDEX)) WR_Change_Load_PWM(WR_Load_pins_Boiler_INDEX, -32768);
									else WR_Switch_Load(WR_Load_pins_Boiler_INDEX, 0);
									if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf_time("WR: Boiler OK\n");
//									for(uint8_t i = 0; i < 5; i++) { // >1/100 sec
//										WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
//									}
									// Компенсируем
									for(uint8_t i = 0; i < WR_NumLoads; i++) {
										if(i == WR_Load_pins_Boiler_INDEX || !GETBIT(WR_Loads, i) || WR_LoadRun[i] == WR.LoadPower[i]) continue;
										if(GETBIT(WR.PWM_Loads, i)) {
											int16_t chg = WR.LoadPower[i] - WR_LoadRun[i];
											if(chg > curr) chg = curr;
											WR_Change_Load_PWM(i, chg);
											if(curr == chg) break;
											curr -= chg;
										} else {
											if(WR.LoadPower[i] > curr || (WR_SwitchTime[i] && rtcSAM3X8.unixtime() - WR_SwitchTime[i] <= WR.TurnOnPause)) continue;
											WR_Switch_Load(i, 1);
											curr -= WR.LoadPower[i];
										}
									}
									break;
								}
							}
						}
					}
#endif

#ifdef WR_CurrentSensor_4_20mA
					HP.sADC[IWR].Read();
					int32_t pnet = HP.sADC[IWR].get_Value() * HP.dSDM.get_voltage();
#elif WR_PowerMeter_Modbus
					int32_t pnet = WR_PowerMeter_Power; //round_div_int32(WR_PowerMeter_Power, 10);
					if(pnet == -1) break; // Ошибка
#else
					// HTTP power meter
					int err = Send_HTTP_Request(HTTP_MAP_Server, WebSec_Microart.hash, HTTP_MAP_Read_MAP, 1);
					if(err) {
						if(GETBIT(WR.Flags, WR_fLog) && !GETBIT(Logflags, fLog_HTTP_RelayError)) {
							SETBIT1(Logflags, fLog_HTTP_RelayError);
							journal.jprintf("WR: HTTP request Error %d\n", err);
						}
						break;
					} else SETBIT0(Logflags, fLog_HTTP_RelayError);
					// todo: check "_MODE" >= 3
					char *fld = strstr(Socket[MAIN_WEB_TASK].outBuf, HTTP_MAP_JSON_PNET_calc);
					if(!fld) {
						if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf("WR: HTTP json wrong!\n");
						break;
					}
					char *fld2 = strchr(fld += sizeof(HTTP_MAP_JSON_PNET_calc) + 1, '"');
					if(!fld2) break;
					*(fld2 - 2) = '\0' ; // integer part "0.0"
					int pnet = atoi(fld);
#endif
					//
					if(!GETBIT(WR.Flags, WR_fActive) || nopwr) break;

					int16_t _MinNetLoad = WR.MinNetLoad;
					if(WR.MinNetLoadSunDivider) _MinNetLoad += WR_LastSunPowerOut / WR.MinNetLoadSunDivider;
#ifdef WR_TestAvailablePowerForRelayLoads
					if(WR_TestLoadStatus) { // Тестирование нагрузки
						if(++WR_TestLoadStatus > WR_TestAvailablePowerTime) {
							WR_TestLoadStatus = 0;
#ifdef WR_Boiler_Substitution_INDEX
								uint8_t idx = digitalReadDirect(PIN_WR_Boiler_Substitution) ? WR_Boiler_Substitution_INDEX : WR_TestAvailablePowerForRelayLoads;
#else
								uint8_t idx = WR_TestAvailablePowerForRelayLoads;
#endif
							WR_Change_Load_PWM(idx, -WR.LoadPower[WR_TestLoadIndex]);
							if(pnet <= _MinNetLoad) {
//								for(uint8_t i = 0; i < 5; i++) { // >1/100 sec
//									WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
//								}
								WR_Switch_Load(WR_TestLoadIndex, 1);
								if(GETBIT(WR.Flags, WR_fLogFull)) journal.jprintf("WRT: OK\n");
							} else {
								WR_LastSwitchTime = rtcSAM3X8.unixtime();
								if(GETBIT(WR.Flags, WR_fLogFull)) journal.jprintf("WRT: < %d\n", WR.LoadPower[WR_TestLoadIndex]);
							}
						}
						break;
					} else
#endif
					{
						// Если есть нагрузка, то фильтруем
//						bool need_average;
//						if(pnet < 0) need_average = false;
//						else {
//							need_average = true;
//							if(pnet > _MinNetLoad) {
//								for(int8_t i = 0; i < WR_NumLoads; i++) {
//									if(!GETBIT(WR.PWM_Loads, i) || WR_LoadRun[i] == 0 || !GETBIT(WR_Loads, i)) continue;
//									need_average = false;
//									break;
//								}
//							}
//						}
#ifdef WR_SKIP_EXTREMUM
//						if(need_average) {
							if(WR_Pnet != -32768 && /*abs*/(pnet - WR_Pnet) > WR_SKIP_EXTREMUM) {
								WR_Pnet = -32768;
								if(GETBIT(WR.Flags, WR_fLogFull)) journal.jprintf_time("WR: Skip %d\n", pnet);
								break;
							}
//						}
#endif

	#ifdef WR_PNET_MEDIAN
						if(GETBIT(WR.Flags, WR_fMedianFilter)) {
							// Медианный фильтр на увеличение
							static int median1, median2;
							if(WR_Pnet_avg_init) median1 = median2 = pnet;
							int median3 = pnet;
							if(median3 > median2) { // предыдущее меньше
								if(median1 <= median2 && median1 <= median3) {
									pnet = median2 <= median3 ? median2 : median3;
								} else if(median2 <= median1 && median2 <= median3) {
									pnet = median1 <= median3 ? median1 : median3;
								} else {
									pnet = median1 <= median2 ? median1 : median2;
								}
							}
							median1 = median2;
							median2 = median3;
						}
	#endif
#ifdef WR_PNET_AVERAGE
						if(GETBIT(WR.Flags, WR_fAverage)) {
	#if WR_PNET_AVERAGE == 1
							if(!WR_Pnet_avg_init) pnet = (WR_Pnet_avg + pnet) / 2;
							WR_Pnet_avg = pnet;
	#else
							if(WR_Pnet_avg_init) { // first time
								for(uint8_t i = 0; i < sizeof(WR_Pnet_avg) / sizeof(WR_Pnet_avg[0]); i++) WR_Pnet_avg[i] = pnet;
								WR_Pnet_avg_sum = pnet * int32_t(sizeof(WR_Pnet_avg) / sizeof(WR_Pnet_avg[0]));
							} else {
								WR_Pnet_avg_sum = WR_Pnet_avg_sum - WR_Pnet_avg[WR_Pnet_avg_idx] + pnet;
								WR_Pnet_avg[WR_Pnet_avg_idx] = pnet;
								if(WR_Pnet_avg_idx < sizeof(WR_Pnet_avg) / sizeof(WR_Pnet_avg[0]) - 1) WR_Pnet_avg_idx++; else WR_Pnet_avg_idx = 0;
							}
//							if(need_average)
								pnet = WR_Pnet_avg_sum / int32_t(sizeof(WR_Pnet_avg) / sizeof(WR_Pnet_avg[0]));
//							else WR_Pnet = pnet;
	#endif
						}
#endif
						WR_Pnet_avg_init = false;
					}
					WR_Pnet = pnet; //need_average ? pnet : median3;
#ifndef WR_NEXTION_FULL_SUN
					int8_t mppt = -1;
#endif
					if((WR.Flags & ((1<<WR_fLogFull)|(1<<WR_fLog))) == ((1<<WR_fLogFull)|(1<<WR_fLog))) {
						journal.jprintf("WR: P=%d%c", WR_Pnet, mppt == 0 ? '!' : mppt == 1 ? '+' : mppt == 2 ? '*' : mppt == 3 ? '-': '?');
						if(WR_Pnet != WR_PowerMeter_Power) journal.jprintf("(%d)", WR_PowerMeter_Power);
						journal.jprintf(",%.1d\n", WR_MAP_Ubat);
					}
					// проверка перегрузки
					if(WR_Pnet > _MinNetLoad) { // Потребление из сети больше - уменьшаем нагрузку
						pnet = WR_Pnet - _MinNetLoad + WR.MinNetLoadHyst / 2; // / 2;
						if(pnet <= 0) break;
						for(int8_t i = WR_NumLoads-1; i >= 0; i--) { // PWM only
							if(!GETBIT(WR.PWM_Loads, i) || WR_LoadRun[i] == 0 || !GETBIT(WR_Loads, i)) continue;
#ifdef WR_Load_pins_Boiler_INDEX
							if(i == WR_Load_pins_Boiler_INDEX && HP.dRelay[RBOILER].get_Relay()) continue;
#endif
#ifdef HTTP_MAP_Read_MPPT
							if(mppt == -1) {
								mppt = WR_Check_MPPT();					// Проверка наличия свободного солнца
							}
							if(mppt > 1) break;
#endif
							int chg = WR_LoadRun[i];
							if(chg > pnet && chg - pnet > WR_PWM_POWER_MIN) chg = pnet;
							WR_Change_Load_PWM(i, WR_Adjust_PWM_delta(i, -chg));
							pnet -= chg;
							if(pnet <= 0) break;
						}
						if(pnet > 0 && mppt <= 1) { // not PWM
							uint8_t reserv = 255;
							uint32_t t = rtcSAM3X8.unixtime();
							for(int8_t i = WR_NumLoads-1; i >= 0; i--) {  // Relay only
								if(GETBIT(WR.PWM_Loads, i) || WR_LoadRun[i] == 0 || !GETBIT(WR_Loads, i)) continue;
#ifdef WR_Load_pins_Boiler_INDEX
								if(i == WR_Load_pins_Boiler_INDEX && HP.dRelay[RBOILER].get_Relay()) continue;
#endif
								if(WR_LastSwitchTime && t - WR_LastSwitchTime <= WR.NextSwitchPause) continue;
								if(WR_SwitchTime[i] && t - WR_SwitchTime[i] <= WR.TurnOnMinTime) continue;
#ifdef HTTP_MAP_Read_MPPT
								if(mppt == -1) {
									if((mppt = WR_Check_MPPT()) > 1) break;				// Проверка наличия свободного солнца
								}
#endif
								if(pnet - WR.LoadHist >= WR_LoadRun[i]) {
//									if(WR_Load_pins[i] < 0) WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
									WR_Switch_Load(i, 0);
									break;
								} else if(reserv == 255) reserv = i;
							}
							if(reserv != 255 && pnet > WR.LoadHist) { // еще не все
//								if(WR_Load_pins[reserv] < 0) WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
								WR_Switch_Load(reserv, 0);
							}
						}
					} else if(WR_Pnet <= _MinNetLoad - WR.MinNetLoadHyst || mppt == 3) { // Увеличиваем нагрузку
#ifdef WR_Load_pins_Boiler_INDEX
						bool need_heat_boiler =	WR.LoadPower[WR_Load_pins_Boiler_INDEX] - WR_LoadRun[WR_Load_pins_Boiler_INDEX] > 0
												&& (HP.sTemp[TBOILER].get_Temp() <= HP.Prof.Boiler.WR_Target - WR_Boiler_Hysteresis) && !HP.dRelay[RBOILER].get_Relay();
						if(need_heat_boiler) {
							// Переключаемся на нагрев бойлера
							for(int8_t i = 0; i < WR_NumLoads; i++) {
								int16_t lr;
								if(i == WR_Load_pins_Boiler_INDEX || WR_LoadRun[i] == 0 || !GETBIT(WR_Loads, i)) continue;
								if(GETBIT(WR.PWM_Loads, i)) {
									lr = WR_LoadRun[i];
									WR_Change_Load_PWM(i, -(WR.LoadPower[WR_Load_pins_Boiler_INDEX] - WR_LoadRun[WR_Load_pins_Boiler_INDEX]));
									need_heat_boiler = false;
								} else {
									if(WR_SwitchTime[i] && rtcSAM3X8.unixtime() - WR_SwitchTime[i] <= WR.TurnOnMinTime) continue;
//									if(WR_Load_pins[i] < 0) WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
									lr = WR_LoadRun[i];
									WR_Switch_Load(i, 0);
									need_heat_boiler = false;
								}
								if(!need_heat_boiler) {
//									WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
									if(GETBIT(WR.PWM_Loads, WR_Load_pins_Boiler_INDEX)) WR_Change_Load_PWM(WR_Load_pins_Boiler_INDEX, lr);
									else WR_Switch_Load(WR_Load_pins_Boiler_INDEX, lr);

								}
							}
							if(!need_heat_boiler) break;
						}
#endif
						for(int8_t i = 0; i < WR_NumLoads; i++) {
							if(WR_LoadRun[i] == WR.LoadPower[i] || !GETBIT(WR_Loads, i)) continue;
#ifdef WR_Load_pins_Boiler_INDEX
							if(i == WR_Load_pins_Boiler_INDEX) {
								int16_t d = HP.Prof.Boiler.WR_Target - HP.sTemp[TBOILER].get_Temp();
								if(d < 0 || HP.dRelay[RBOILER].get_Relay()) continue;
#ifdef WR_Boiler_Substitution_INDEX
								if(digitalReadDirect(PIN_WR_Boiler_Substitution) && d <= WR_Boiler_Hysteresis) continue; // Если работает замена бойлера и t меньше гистерезиса - пропускаем бойлер
#endif

							}
#endif
							int8_t availidx = 0;
							if(!GETBIT(WR.PWM_Loads, i)) {
								uint32_t t = rtcSAM3X8.unixtime();
								if(WR_LastSwitchTime && t - WR_LastSwitchTime <= WR.NextSwitchPause) continue;
								if(WR_SwitchTime[i] && t - WR_SwitchTime[i] <= WR.TurnOnPause) continue;
								// Расчет мощности, которую можно взять от менее приритетных нагрузок
								for(int8_t ii = i + 1; ii < WR_NumLoads; ii++) {
									if(WR_LoadRun[ii] >= WR.LoadPower[i]) {
										availidx = ii;
										break;
									}
								}
							}
#ifdef HTTP_MAP_Read_MPPT
							if(mppt == -1 && pnet > 0) {
								mppt = WR_Check_MPPT();
							}
							if(mppt == 2 || (mppt == 0 && pnet == 0)) break;	// Проверка наличия свободного солнца
							// Отключено, на КЭС можно рассчитывать только в плане избытка
							//if(mppt == 1 && availidx == 0) break;
#endif
							if(GETBIT(WR.PWM_Loads, i)) {
#ifdef WR_Boiler_Substitution_INDEX
								if(i == WR_Boiler_Substitution_INDEX && WR_LoadRun[WR_Load_pins_Boiler_INDEX] != 0) continue;
#endif
								int16_t chg;
								if(mppt == 0 || mppt == 2) { // Добавляем помаленьку, когда ошибка или пауза у MPPT
									if(skip_next_small_increase) break;
									if(pnet > 0) {
										chg = _MinNetLoad - pnet;
										if(chg < WR_PWM_POWER_MIN) break;
									} else chg = pnet + WR_MIN_LOAD_POWER;
									skip_next_small_increase = 2;
								} else chg = WR.LoadAdd;
								WR_Change_Load_PWM(i, WR_Adjust_PWM_delta(i, chg));
								break;
							} else {
								if(/*mppt <= 1 &&*/ availidx) { // на КЭС можно рассчитывать только в плане избытка
									if(GETBIT(WR.PWM_Loads, availidx)) {
										WR_Change_Load_PWM(availidx, -WR.LoadPower[i]);
									} else {
										if(WR_SwitchTime[i] && rtcSAM3X8.unixtime() - WR_SwitchTime[i] <= WR.TurnOnPause) continue;
										WR_Switch_Load(i, 0);
									}
								// на КЭС можно рассчитывать только в плане избытка
								//} else {
								//	if(mppt < 3 && pnet > 0) continue;
								//	availidx = 0;
								}
#ifdef WR_TestAvailablePowerForRelayLoads
	#if defined(WR_Load_pins_Boiler_INDEX) && WR_TestAvailablePowerForRelayLoads == WR_Load_pins_Boiler_INDEX
		#ifdef WR_Boiler_Substitution_INDEX
								uint8_t idx = digitalReadDirect(PIN_WR_Boiler_Substitution) ? WR_Boiler_Substitution_INDEX : WR_Load_pins_Boiler_INDEX;
								if(availidx == 0 && GETBIT(WR_Loads, idx) && (idx != WR_Load_pins_Boiler_INDEX || (HP.sTemp[TBOILER].get_Temp() < WR_BOILER_MAX_TEMP && !HP.dRelay[RBOILER].get_Relay())) && WR_LoadRun[idx] < WR.LoadPower[idx]) {
		#else
								if(availidx == 0 && GETBIT(WR_Loads, WR_Load_pins_Boiler_INDEX) && (HP.sTemp[TBOILER].get_Temp() < WR_BOILER_MAX_TEMP && !HP.dRelay[RBOILER].get_Relay())) && WR_LoadRun[WR_Load_pins_Boiler_INDEX] < WR.LoadPower[WR_Load_pins_Boiler_INDEX]) {
		#endif
	#else
								uint8_t idx = WR_TestAvailablePowerForRelayLoads;
								if(availidx == 0 && GETBIT(WR_Loads, WR_TestAvailablePowerForRelayLoads) && WR_LoadRun[WR_TestAvailablePowerForRelayLoads] < WR.LoadPower[WR_TestAvailablePowerForRelayLoads]) {
	#endif
									WR_TestLoadIndex = i;
									WR_TestLoadStatus = 1;
									WR_Change_Load_PWM(idx, WR.LoadPower[i]);
									break;
								}
#endif
//								if(WR_Load_pins[i] < 0) WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
								WR_Switch_Load(i, 1);
								break;
							}
						}
					}
					break;
				}
			} else { // NOT (GETBIT(WR.Flags, WR_fActive) || WR.PWM_FullPowerTime || WR_Refresh))
				WR_Pnet = WR_PowerMeter_Power;
				if(GETBIT(WR_WorkFlags, WR_fWF_Read_MPPT)) {
					WR_Check_MPPT();				// Чтение солнечного контроллера
					SETBIT0(WR_WorkFlags, WR_fWF_Read_MPPT);
//					WEB_SERVER_MAIN_TASK();	/////////////////////////////////////// Выполнить задачу веб сервера
				} else WR_LastSunSign = -1;
				if((WR.Flags & ((1<<WR_fLogFull)|(1<<WR_fLog))) == ((1<<WR_fLogFull)|(1<<WR_fLog))) {
					journal.jprintf("WR: P=%d%c", WR_Pnet, WR_LastSunSign == 0 ? '!' : WR_LastSunSign == 1 ? '+' : WR_LastSunSign == 2 ? '*' : WR_LastSunSign == 3 ? '-': '?');
					if(WR_Pnet != WR_PowerMeter_Power) journal.jprintf("(%d)", WR_PowerMeter_Power);
					journal.jprintf(",%.1d(%.1d)\n", WR_MAP_Ubat, WR_MAP_Ubuf);
				}
			}
	#ifdef RSOLINV
			if(HP.dRelay[RSOLINV].get_Relay()) { 	// Relay is ON
#ifdef WR_INVERTOR2_SUN_OFF_WHEN_NO_WORK
				if(HP.is_compressor_on()) WR_Invertor2_off_cnt = 0;
#endif
				if(WR_Invertor2_off_cnt > WR_INVERTOR2_SUN_OFF_TIMER) { // || (GETBIT(WR_WorkFlags, WR_fWF_Charging_BAT) && )) {
					HP.dRelay[RSOLINV].set_Relay(fR_StatusAllOff);
					WR_Invertor2_off_cnt = 0;
				} else if(GETBIT(WR_WorkFlags, WR_fWF_Charging_BAT) && WR_LastSunPowerOut < WR_INVERTOR2_SUN_PWR_ON) WR_Invertor2_off_cnt += 10;
				else if(WR_LastSunPowerOut <= WR_INVERTOR2_SUN_PWR_OFF || rtcSAM3X8.get_hours() >= WR_INVERTOR2_SUN_OFF_HOUR) WR_Invertor2_off_cnt++;
				else if(WR_Invertor2_off_cnt > 0) WR_Invertor2_off_cnt--;
			} else { 								// Relay is OFF
				if(WR_LastSunPowerOut > WR_INVERTOR2_SUN_PWR_ON && WR_Pnet > 0 && WR_LastSunPowerOut > WR_Pnet && !GETBIT(WR_WorkFlags, WR_fWF_Charging_BAT)) {
					if(--WR_Invertor2_off_cnt < -WR_INVERTOR2_SUN_ON_TIMER) {
						WR_Invertor2_off_cnt = 0;
						if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf("WR: Sun=%d, Net=%d\n", WR_LastSunPowerOut, WR_Pnet);
						HP.dRelay[RSOLINV].set_Relay(fR_StatusSun);
						WR_Invertor2_off_cnt = 0;
					}
				}
			}
	#endif
			//

}


void WR_Init(void)
{
	memset(WR_LoadRun, 0, sizeof(WR_LoadRun));
	memset(WR_SwitchTime, 0, sizeof(WR_SwitchTime));
	for(uint8_t i = 0; i < WR_NumLoads; i++) {
		if(WR_Load_pins[i] > 0) pinMode(WR_Load_pins[i], OUTPUT);
	}
 #ifdef PIN_WR_Boiler_Substitution
	pinMode(PIN_WR_Boiler_Substitution, OUTPUT);
 #endif
	journal.jprintf("WattRouter ");
	if(GETBIT(WR.Flags, WR_fActive)) {
		journal.jprintf(" started\n");
#ifdef WR_CHECK_Vbat_INSTEAD_OF_MPPT_SIGN
		if(WR_Read_MAP() == -32768) journal.jprintf("WR: Error read Ubuf\n");
		journal.jprintf(" Ubuf=%.1d\n", WR_MAP_Ubuf);
#endif
	} else journal.jprintf(" not active\n");
}

void WR_Switch_Load(uint8_t idx, boolean On)
{
	int8_t pin = WR_Load_pins[idx];
	if(pin < 0) { // HTTP
		strcpy(Socket[MAIN_WEB_TASK].outBuf, HTTP_MAP_RELAY_SW_1);
		_itoa(abs(pin), Socket[MAIN_WEB_TASK].outBuf + sizeof(HTTP_MAP_RELAY_SW_1)-1);
		strcat(Socket[MAIN_WEB_TASK].outBuf + sizeof(HTTP_MAP_RELAY_SW_1)-1, HTTP_MAP_RELAY_SW_2);
		_itoa(On, Socket[MAIN_WEB_TASK].outBuf + sizeof(HTTP_MAP_RELAY_SW_1)-1 + sizeof(HTTP_MAP_RELAY_SW_2)-1);
		if(Send_HTTP_Request(HTTP_MAP_Server, WebSec_Microart.hash, Socket[MAIN_WEB_TASK].outBuf, 3) == 1) { // Ok?
			SETBIT0(Logflags, fLog_HTTP_RelayError);
			goto xSwitched;
		} else {
			if(GETBIT(WR.Flags, WR_fLog) && !GETBIT(Logflags, fLog_HTTP_RelayError)) {
				SETBIT1(Logflags, fLog_HTTP_RelayError);
				journal.jprintf("WR: Error set R%d\n", idx + 1);
			}
		}
	} else {
		digitalWriteDirect(pin, On ? WR_RELAY_LEVEL_ON : !WR_RELAY_LEVEL_ON);
xSwitched:
		if((WR_LoadRun[idx] > 0) != On) WR_SwitchTime[idx] = WR_LastSwitchTime = rtcSAM3X8.unixtime();
		WR_LoadRun[idx] = On ? WR.LoadPower[idx] : 0;
		if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf_time("WR: R%d=>%d\n", idx + 1, On);
	}
}

void WR_Change_Load_PWM(uint8_t idx, int16_t delta)
{
	#define MP WR.LoadPower[idx]
	int n = WR_LoadRun[idx] + delta;
	if(n <= 0) n = 0;
	else if(n >= MP) n = MP;
#ifdef PWM_ACCURATE_POWER
	else {
		n = n * (220*220L) / (HP.dSDM.get_voltage()*HP.dSDM.get_voltage());
		if(n > MP) n = MP;
	}
#endif
	uint32_t t = rtcSAM3X8.unixtime();
	if(WR.PWM_FullPowerTime) {
		if(n > 0) {
			int max = MP * WR.PWM_FullPowerLimit / 100;
			if(n > max) {
				if(WR_LoadRun[idx] <= max) {
					if(t - WR_SwitchTime[idx] <= WR.PWM_FullPowerTime * 60) n = max; // Включаемся, но еще не остыли
					else WR_SwitchTime[idx] = t;
				} else if(WR_SwitchTime[idx] && t - WR_SwitchTime[idx] > WR.PWM_FullPowerTime * 60) n = max; // Перегрелись
			} else if(n < max && WR_LoadRun[idx] > max) WR_SwitchTime[idx] = t;
		} else if(WR_LoadRun[idx]) WR_SwitchTime[idx] = t;
	}
	if(n != WR_LoadRun[idx] || GETBIT(WR_Refresh, idx)) {
		if(n != WR_LoadRun[idx]) {
#ifdef WR_Boiler_Substitution_INDEX
			if(n > 0) {
				if(idx == WR_Boiler_Substitution_INDEX && !digitalReadDirect(PIN_WR_Boiler_Substitution)) {
					if(WR_LoadRun[WR_Load_pins_Boiler_INDEX] > 0) {
						PWM_Write(WR_Load_pins[WR_Load_pins_Boiler_INDEX], ((1<<PWM_WRITE_OUT_RESOLUTION)-1));
						WR_SwitchTime[WR_Load_pins_Boiler_INDEX] = t;
						WR_LoadRun[WR_Load_pins_Boiler_INDEX] = 0;
						_delay(10); // 1/100 Hz
					}
					digitalWriteDirect(PIN_WR_Boiler_Substitution, 1);	// to Substitution
					_delay(WR_Boiler_Substitution_swtime);
				} else if(idx == WR_Load_pins_Boiler_INDEX && digitalReadDirect(PIN_WR_Boiler_Substitution)) {
					if(WR_LoadRun[WR_Boiler_Substitution_INDEX] > 0) {
						PWM_Write(WR_Load_pins[WR_Boiler_Substitution_INDEX], ((1<<PWM_WRITE_OUT_RESOLUTION)-1));
						WR_SwitchTime[WR_Boiler_Substitution_INDEX] = t;
						WR_LoadRun[WR_Boiler_Substitution_INDEX] = 0;
						_delay(10); // 1/100 Hz
					}
					digitalWriteDirect(PIN_WR_Boiler_Substitution, 0); // to Boiler
					_delay(WR_Boiler_Substitution_swtime);
				}
			}
#endif
			if((WR_LoadRun[idx] == 0 || n == 0) && WR_TestLoadStatus == 0) WR_SwitchTime[idx] = t;
		}
		WR_LoadRun[idx] = n;
		if(GETBIT(WR.Flags, WR_fLogFull)) journal.jprintf_time("WR: P%d=%d\n", idx + 1, n);
#ifdef PWM_ACCURATE_POWER
		n = n * (sizeof(PWM_POWER_ARRAY)/sizeof(PWM_POWER_ARRAY[0])-1)*10 / MP; // 0..100
		int r = n % 10;
		int val = PWM_POWER_ARRAY[n /= 10];
		if(n < (int)(sizeof(PWM_POWER_ARRAY)/sizeof(PWM_POWER_ARRAY[0]))-1) val += (PWM_POWER_ARRAY[n + 1] - val) * r / 10;
		PWM_Write(WR_Load_pins[idx], val);
#else
		PWM_Write(WR_Load_pins[idx], ((1<<PWM_WRITE_OUT_RESOLUTION)-1) - n * ((1<<PWM_WRITE_OUT_RESOLUTION)-1) / MP);
#endif
	}
}

inline int16_t WR_Adjust_PWM_delta(uint8_t idx, int16_t delta)
{
	if(delta != 0) {
		int16_t m = WR.LoadPower[idx] >> PWM_WRITE_OUT_RESOLUTION;
		if(delta < 0) {
			m = -m;
			if(m < delta) return m;
		} else if(m > delta) return m;
	}
	return delta;
}

#ifdef HTTP_MAP_Read_MPPT
// Проверка наличия свободного солнца
// 0 - Oшибка, 1 - Нет свободной энергии, 2 - Нужна пауза, 3 - Есть свободная энергия
int8_t WR_Check_MPPT(void)
{
	int err = Send_HTTP_Request(HTTP_MAP_Server, WebSec_Microart.hash, HTTP_MAP_Read_MPPT, 1);
	if(err) {
		if(WR_LastSunSign == 0) {
			WR_MAP_Ubat = 0;
			WR_LastSunPowerOut = 0;
			WR_LastSunPowerOutCnt = 0;
		}
		if(testMode != NORMAL) {
			return WR_LastSunSign = 3;
		}
		if(GETBIT(WR.Flags, WR_fLog) && !GETBIT(Logflags, fLog_HTTP_RelayError)) {
			SETBIT1(Logflags, fLog_HTTP_RelayError);
			journal.jprintf("WR: MPPT request Error %d\n", err);
		}
		return WR_LastSunSign = 0;
	} else SETBIT0(Logflags, fLog_HTTP_RelayError);
	char *fld = strstr(Socket[MAIN_WEB_TASK].outBuf, HTTP_MAP_JSON_Vbat);
	if(fld) {
		char *fld2 = strchr(fld += sizeof(HTTP_MAP_JSON_Vbat) + 1, '"');
		if(fld2) {
			WR_MAP_Ubat = atoi(fld) * 10;
			WR_MAP_Ubat += *(fld2 - 1) - '0';
		}
	} else {
		if(WR_LastSunSign == 0) {
			WR_MAP_Ubat = 0;
			WR_LastSunPowerOut = 0;
			WR_LastSunPowerOutCnt = 0;
		}
		return WR_LastSunSign = 0;
	}
	fld = strstr(fld, HTTP_MAP_JSON_P_Out);
	if(!fld) return WR_LastSunSign = 0;
	WR_LastSunPowerOut = strtol(fld + sizeof(HTTP_MAP_JSON_P_Out) + 1, NULL, 0);
	fld = strstr(fld, HTTP_MAP_JSON_Mode);
	if(!fld) return WR_LastSunSign = 0;
	char _mode = *(fld + sizeof(HTTP_MAP_JSON_Mode) + 1);
	if(_mode == 'i' || _mode == 'v') SETBIT1(WR_WorkFlags, WR_fWF_Charging_BAT); else SETBIT0(WR_WorkFlags, WR_fWF_Charging_BAT);
	if(WR_LastSunPowerOut == 0 || WR_LastSunPowerOut < WR_Pnet) {
		if(++WR_LastSunPowerOutCnt > 10) return WR_LastSunSign = 1;
	} else WR_LastSunPowerOutCnt = 0;
	if(_mode == 'S') return WR_LastSunSign = 2;
#ifdef WR_CHECK_Vbat_INSTEAD_OF_MPPT_SIGN
	return WR_LastSunSign = WR_MAP_Ubuf - WR_MAP_Ubat <= WR.DeltaUbatmin ? 3 : 1;
#else
	fld = strstr(fld, HTTP_MAP_JSON_Sign);
	if(fld && *(fld + sizeof(HTTP_MAP_JSON_Sign) + 1) == '-') return WR_LastSunSign = 3;
	return WR_LastSunSign = 1;
#endif
}
#endif

#ifdef HTTP_MAP_Read_MAP
// Прочитать данные напряжения АКБ MAP, возврат дельты Ubuf - Ubat, ошибка: -32768
int16_t WR_Read_MAP(void)
{
	// Read MAP
	int16_t retval = -32768;
	int err = Send_HTTP_Request(HTTP_MAP_Server, WebSec_Microart.hash, HTTP_MAP_Read_MAP, 1);
	if(err) {
		if(GETBIT(WR.Flags, WR_fLog) && !GETBIT(Logflags, fLog_HTTP_RelayError)) {
			SETBIT1(Logflags, fLog_HTTP_RelayError);
			journal.jprintf("WR: HTTP request Error %d\n", err);
		}
	} else {
		SETBIT0(Logflags, fLog_HTTP_RelayError);
		char *fld = strstr(Socket[MAIN_WEB_TASK].outBuf, HTTP_MAP_JSON_Uacc);
		if(!fld) {
			if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf("WR: HTTP json wrong!\n");
		} else {
			char *fld2 = strchr(fld += sizeof(HTTP_MAP_JSON_Uacc) + 1, '"');
			if(fld2) {
				int16_t Vd = atoi(fld) * 10;
				Vd += *(fld2 - 1) - '0';
				char *fld = strstr(fld2, HTTP_MAP_JSON_Ubuf);
				if(!fld) {
					if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf("WR: HTTP json wrong!\n");
				} else {
					char *fld2 = strchr(fld += sizeof(HTTP_MAP_JSON_Ubuf) + 1, '"');
					if(fld2) {
						WR_MAP_Ubuf = atoi(fld) * 10;
						WR_MAP_Ubuf += *(fld2 - 1) - '0';
						retval = WR_MAP_Ubuf - Vd;
					}
				}
			}
		}
	}
	return retval;
}
#endif

#ifdef PWM_CALC_POWER_ARRAY
// Вычисление массива точного расчета мощности

// power - 0.1W
void WR_Calc_Power_Array_NewMeter(int32_t power)
{
	if(GETBIT(PWM_CalcFlags, PWM_fCalcNow)) {
#ifdef WR_CurrentSensor_4_20mA
		HP.sADC[IWR].Read();
#ifndef TEST_BOARD
		if(PWM_AverageCnt++) PWM_AverageSum += HP.sADC[IWR].get_Value() * HP.dSDM.get_voltage();
#else
		if(PWM_AverageCnt++) {
			if(HP.dSDM.get_Voltage() != 0) PWM_AverageSum += HP.sADC[IWR].get_Value() * HP.dSDM.get_voltage();
			else {
				PWM_AverageSum += 10 + (GETBIT(PWM_CalcFlags, PWM_fCalcRelax) ? 0 : PWM_CalcIdx * 9) + (rand() & 0x3);
			}
		}
#endif
#else
		if(++PWM_AverageCnt > PWM_CALC_POWER_SW_SKIP) PWM_AverageSum += round_div_int32(power, 10); // skip n
#endif
#if PWM_CALC_POWER_ARRAY == 1
		if(GETBIT(PWM_CalcFlags, PWM_fCalcRelax)) {
			if(PWM_AverageCnt >= PWM_CALC_POWER_SW_SKIP + 2) { // Relax
				PWM_AverageSum = PWM_AverageSum / (PWM_AverageCnt - PWM_CALC_POWER_SW_SKIP);
				if(GETBIT(WR.Flags, WR_fLogFull)) journal.jprintf("Relax: %d\n", PWM_AverageSum);
				PWM_AverageSum -= PWM_CalcArray[0];
				if(abs(PWM_AverageSum) > 5) journal.jprintf("Non stable Relax power: %s%d\n", PWM_AverageSum > 0 ? "+" : "", PWM_AverageSum);
				PWM_Write(WR_Load_pins[PWM_CalcLoadIdx], ((1<<PWM_WRITE_OUT_RESOLUTION)-1) - PWM_CalcIdx);
				PWM_CalcFlags &= ~(1<<PWM_fCalcRelax);
				PWM_AverageSum = PWM_AverageCnt = 0;
			}
		} else
#endif
		if(PWM_AverageCnt >= PWM_CALC_POWER_SW_SKIP + 2) { // Ok
			PWM_AverageSum /= (PWM_AverageCnt - PWM_CALC_POWER_SW_SKIP);
			if(PWM_CalcIdx) PWM_AverageSum -= PWM_CalcArray[0];
			PWM_CalcArray[PWM_CalcIdx] = PWM_AverageSum;
			if(GETBIT(WR.Flags, WR_fLogFull)) journal.jprintf("Power[%d]: %d\n", PWM_CalcIdx, PWM_AverageSum);
#if PWM_CALC_POWER_ARRAY == 1
			PWM_Write(WR_Load_pins[PWM_CalcLoadIdx], ((1<<PWM_WRITE_OUT_RESOLUTION)-1));
#endif
			if(PWM_CalcIdx++ == ((1<<PWM_WRITE_OUT_RESOLUTION)-1)) {
#if PWM_CALC_POWER_ARRAY != 1
				PWM_Write(WR_Load_pins[PWM_CalcLoadIdx], ((1<<PWM_WRITE_OUT_RESOLUTION)-1));
#endif
				WR.LoadPower[PWM_CalcLoadIdx] = PWM_AverageSum;
				WR_LoadRun[PWM_CalcLoadIdx] = 0;
				TaskSuspendAll();
				journal.jprintf_time("PWM Calc Ok\nPower[%d] = ", (1<<PWM_WRITE_OUT_RESOLUTION));
				for(uint16_t i = 0; i < (1<<PWM_WRITE_OUT_RESOLUTION); i++) {
					journal.jprintf("%d,", PWM_CalcArray[i]);
				}
				journal.jprintf("\nPWM[%d] = %d,", sizeof(PWM_POWER_ARRAY)/sizeof(PWM_POWER_ARRAY[0]), ((1<<PWM_WRITE_OUT_RESOLUTION)-1));
				uint16_t last_idx = 1;
				for(uint16_t i = 1; i < sizeof(PWM_POWER_ARRAY)/sizeof(PWM_POWER_ARRAY[0]) - 2; i++) {
					int n = PWM_AverageSum * i / (sizeof(PWM_POWER_ARRAY)/sizeof(PWM_POWER_ARRAY[0])-1);
					for(; last_idx < (1<<PWM_WRITE_OUT_RESOLUTION) - 1; last_idx++) {
						if(n >= PWM_CalcArray[last_idx] && n <= PWM_CalcArray[last_idx + 1]) {
							journal.jprintf("%d,", ((1<<PWM_WRITE_OUT_RESOLUTION)-1) - (abs(n - PWM_CalcArray[last_idx]) <= abs(n - PWM_CalcArray[last_idx + 1]) ? last_idx : ++last_idx));
							break;
						}
					}
				}
				journal.jprintf("0, 0\n\n");
				xTaskResumeAll();
				free(PWM_CalcArray);
				PWM_CalcFlags = 0;
			} else {
#if PWM_CALC_POWER_ARRAY == 1
				PWM_CalcFlags |= (1<<PWM_fCalcRelax);
#else
				PWM_Write(WR_Load_pins[PWM_CalcLoadIdx], ((1<<PWM_WRITE_OUT_RESOLUTION)-1) - PWM_CalcIdx);
#endif
				WR_LoadRun[PWM_CalcLoadIdx] = PWM_CalcIdx;
				PWM_AverageSum = PWM_AverageCnt = 0;
			}
		}
	}
}

void WR_Calc_Power_Array_Start(int8_t load_idx)
{
#if !defined(WR_CurrentSensor_4_20mA) && !defined(WR_PowerMeter_Modbus)
	journal.jprintf("PWM Calc not supported!\n");
#else
	journal.jprintf_time("PWM Calc begin:\n");
	PWM_CalcLoadIdx = load_idx;
	PWM_AverageSum = PWM_AverageCnt = PWM_StandbyPower = PWM_CalcIdx = 0;
	PWM_CalcArray = (int16_t*) malloc(sizeof(int16_t) * (1<<PWM_WRITE_OUT_RESOLUTION));
	if(PWM_CalcArray == NULL) {
		journal.jprintf("Memory low!\n");
		return;
	}
	PWM_Write(load_idx, ((1<<PWM_WRITE_OUT_RESOLUTION)-1));
	PWM_CalcFlags |= (1<<PWM_fCalcNow);
#endif
}
#endif //PWM_CALC_POWER_ARRAY

void WR_ReadPowerMeter(void)
{
	if(GETBIT(HP.Option.flags, fBackupPower)) {
		WR_PowerMeter_Power = -1;
		WR_PowerMeter_New = true;
	} else {
	#ifdef WR_PowerMeter_DDS238
		int8_t i = Modbus.readInputRegisters16(WR_PowerMeter_Modbus, WR_PowerMeter_ModbusReg, (uint16_t*)&WR_PowerMeter_Power);
	#else
		int8_t i = Modbus.readInputRegisters32(WR_PowerMeter_Modbus, WR_PowerMeter_ModbusReg, (uint32_t*)&WR_PowerMeter_Power);
	#endif
		if(i == OK) {
			WR_PowerMeter_Power /= 10;
			WR_Error_Read_PowerMeter = 0;
	#ifdef PWM_CALC_POWER_ARRAY
			WR_Calc_Power_Array_NewMeter(WR_PowerMeter_Power);
	#endif
		} else {
			if(WR_Error_Read_PowerMeter < 255) WR_Error_Read_PowerMeter++;
			if(WR_Error_Read_PowerMeter == WR_Error_Read_PowerMeter_Max) {
				WR_PowerMeter_Power = -1;
				if(GETBIT(WR.Flags, WR_fLog)) journal.jprintf("WR: Modbus read err %d\n", i);
			}
		}
		WR_PowerMeter_New = true;
	}
}
