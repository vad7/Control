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

#define PIN_KEY_OK			PIN_KEY1	// KEYS.2
#define PIN_KEY_DOWN		67			// KEYS.4
#define PIN_KEY_UP			68			// KEYS.3

#include "LiquidCrystal.h"

void vKeysLCD( void * ) __attribute__((naked));

#endif
