/*
 * Copyright (c) 2016-2021 by Vadim Kulakov vad7@yahoo.com, vad711
 * &                       by Pavel Panfilov <firstlast2007@gmail.com> pav2000
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
#ifndef __NEXTION_H__
#define __NEXTION_H__

#include <Arduino.h>
#include "Constant.h"

#ifdef NEXTION_PROTECTED
// Защищённый обмен - только с эмулятором Nextion на ESP32 (github.com/vad7/Nextion_0.53_Compatible_Emulator), с настоящим Nextion
// не работает. SS - номер кадра (2 hex), CCCC - CRC16 calc_crc16() от байта номера и байтов команды/сообщения (4 hex).
// Контроллер -> экран: '#' SS CCCC команда FF FF FF, экран отвечает F0 SS FF FF FF, иначе - повтор.
// Экран -> контроллер: сообщение SS CCCC FF FF FF, контроллер отвечает '!' SS FF FF FF, иначе экран повторит.
// Повтор с тем же SS и CCCC не выполняется второй раз (потерялось подтверждение).
#define NXP_CMD_START	'#'						// начало защищённой команды контроллера
#define NXP_HOST_ACK	'!'						// подтверждение контроллера
#define NXP_ACK			0xF0					// подтверждение экрана (первый байт кадра)
#define NXP_HDR			7						// заголовок команды '#' SS CCCC, байт
#define NXP_TRAILER		6						// SS CCCC в конце сообщения экрана, байт
#define NXP_RETRIES		3						// попыток передачи команды
#define NXP_BOOT_TRIES	4						// попыток связи при старте (экран может ещё загружаться)
#define NXP_ACK_MARGIN	40						// запас ожидания подтверждения сверх времени передачи, мс
#define NXP_NUM			5						// число индекса или значения строкой ("-128" + 0), байт
#endif

class Nextion {
public:
  boolean init();
  void    init_display();
  void    Update();
  void	  set_need_refresh() { fUpdate = 2; };	// обновить дисплей
  boolean check_incoming(void);
  void    readCommand();
  boolean sendCommand(const char* cmd);
  boolean setComponentText(const char* component, char* txt);
  boolean setComponentIdxText(const char* component, uint8_t idx, char* txt);
  boolean setComponentValIdx(const char* component, uint8_t idx, int8_t val);
  boolean setComponentPic(const char* component, char* txt);
  boolean sendCommandToComponentIdx(const char* component, const char* cmd, uint8_t idx, int8_t val);
  void    set_dim(uint8_t dim);					// Установить яркость экрана
  const char*Encode_UTF8_to_ISO8859_5(char* outstr, const char* instr, uint16_t outstrsize);
private:
  int8_t   PageID;								// Текущая страница
  uint8_t  fUpdate;								// Признак обновления: 0 - нет, 1 - штатно, 2 - обновить срочно
  uint8_t  DataAvaliable;
  uint16_t StatusCrc;
  uint16_t Page1crc;
  uint8_t  Page1flags;
  uint8_t  flags;
  uint8_t  input_delay;							// Delay for * NEXTION_READ ms
  uint8_t  sleep_cnt;							// вызовов Update() во сне после последнего опроса get sleep
  uint8_t  time_cnt;							// вызовов Update() после последней отправки времени
  void    getTargetTemp(char *rstr);			// Получить целевую температуру
  void    StatusLine();
#ifdef NEXTION_PROTECTED
  uint8_t  tx_seq;								// номер следующей защищённой команды
  boolean  link_down;							// команда не подтверждена - не передавать до следующего Update()
  uint16_t rx_last_seq;							// номер последнего принятого сообщения экрана, 0xFFFF - не было
  uint16_t rx_last_crc;							// его CRC16 (вместе с номером - отсев повторов)
  char     tx_num[2][NXP_NUM];					// индекс и значение строкой - части команды в set*Idx()
  boolean  send_parts(const char *p0, const char *p1 = NULL, const char *p2 = NULL, const char *p3 = NULL, const char *p4 = NULL);
  boolean  wait_ack(uint8_t seq, uint32_t timeout);
  uint8_t  rx_check(uint8_t len);
#endif
};
#endif
