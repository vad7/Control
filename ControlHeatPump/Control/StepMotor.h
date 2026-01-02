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
#ifndef StepMotor_h
#define StepMotor_h

#include "Arduino.h"
//#include "FreeRTOS_ARM.h"

// flags:
#define STEPMOTOR_TASK_WORK		0
#define STEPMOTOR_TASK_MOVING	1
#define STEPMOTOR_TASK_STOP		2

// library interface description
class StepMotor {
  public:
   void initStepMotor(uint16_t number_of_steps, uint8_t motor_pin_1, uint8_t motor_pin_2, uint8_t motor_pin_3, uint8_t motor_pin_4); // Иницилизация
   void set_speed(uint8_t whatSpeed) { step_delay = 1000 / whatSpeed; };// Установка скорости шагов в секунду
   void move_to_newpos(int16_t pos_steps);                             // Движение на новое положение
   inline void wait_moving(void) { suspend_work = step_delay; task = STEPMOTOR_TASK_MOVING; }// ожидать для следующего импульса
   inline void suspend(void) { task = STEPMOTOR_TASK_STOP; }           // остановить задачу EEV
   inline void resume(void) { task = STEPMOTOR_TASK_WORK; }            // начать задачу EEV
   bool check_suspend(void);                                           // ожидать?
   inline bool is_buzy(void) { return task != STEPMOTOR_TASK_STOP; }   // True если мотор шагает
   void motor_off(void);                                               // выключить двигатель
  private:
    void stepOne(uint8_t this_step);
    inline void setPinMotor(uint8_t pin, boolean val);
    int16_t new_pos;
    uint8_t flags;
    uint8_t task;
    uint8_t suspend_work;                                               // ожидание ms
    uint8_t step_delay;                                                 // delay between steps, in ms, based on speed
    int16_t number_of_steps;                                            // total number of steps this motor can take
    uint8_t pin_count;                                                  // how many pins are in use.
    // motor pin numbers:
    uint8_t motor_pin_1;
    uint8_t motor_pin_2;
    uint8_t motor_pin_3;
    uint8_t motor_pin_4;
    friend void vUpdateStepperEEV(void *);
};
#endif
