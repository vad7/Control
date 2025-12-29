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

// library interface description
class StepMotor {
  public:
   void initStepMotor(int number_of_steps, int motor_pin_1, int motor_pin_2,int motor_pin_3, int motor_pin_4); // Иницилизация
   void setSpeed(uint8_t whatSpeed) { step_delay = 1000 / whatSpeed; };// Установка скорости шагов в секунду
   void step(int number_of_steps);                                     // Движение
   void set_pulse_waiting(void) { suspend_work = step_delay; }         // ожидать для следующего импульса
   void suspend(void) { suspend_work = 255; }                          // остановить работу EEV
   void resume(void) { suspend_work = 0; }                             // начать работу EEV
   inline boolean isBuzy()  {return buzy;}                             // True если мотор шагает
   inline void offBuzy()    {buzy=false;}                              // Установка признака - мотор остановлен
   void off();                                                         // выключить двигатель
   TaskHandle_t xHandleStepperEEV;                                     // Заголовок задачи шаговый двигатель двигается
   QueueHandle_t xCommandQueue;                                        // очередь комманд на управление двигателем
  private:
    void stepOne(int this_step);
    inline void setPinMotor(int pin, boolean val);
    boolean buzy;
    uint8_t suspend_work;                                               // ожидание, если 255 - останов, 2*ms
    uint8_t step_delay;                                                 // delay between steps, in ms, based on speed
    int number_of_steps;                                                // total number of steps this motor can take
    int pin_count;                                                      // how many pins are in use.
    // motor pin numbers:
    int motor_pin_1;
    int motor_pin_2;
    int motor_pin_3;
    int motor_pin_4;
    friend void vUpdateStepperEEV(void *);
};
#endif
