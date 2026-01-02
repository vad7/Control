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
#include "StepMotor.h"

void StepMotor::initStepMotor(uint16_t number_of_steps, uint8_t motor_pin_1, uint8_t motor_pin_2, uint8_t motor_pin_3, uint8_t motor_pin_4)
{
	this->number_of_steps = number_of_steps; // total number of steps for this motor
	this->pin_count = 4;
	// Arduino pins for the motor control connection:
	this->motor_pin_1 = motor_pin_1;
	this->motor_pin_2 = motor_pin_2;
	this->motor_pin_3 = motor_pin_3;
	this->motor_pin_4 = motor_pin_4;
	// setup the pins on the microcontroller:
	pinMode(this->motor_pin_1, OUTPUT);
	pinMode(this->motor_pin_2, OUTPUT);
	pinMode(this->motor_pin_3, OUTPUT);
	pinMode(this->motor_pin_4, OUTPUT);
	motor_off();        // Снять напряжение
	suspend();
}

bool StepMotor::check_suspend(void)
{
	if(task == STEPMOTOR_TASK_STOP) return true;
	if(suspend_work && --suspend_work) return true; // *1 ms
	return false;
}

// Движение до pos_steps
// На входе АБСОЛЮТНАЯ координата, в очередь уходит АБСОЛЮТНАЯ координата
void StepMotor::move_to_newpos(int16_t pos_steps)
{
	new_pos = pos_steps;
	resume();
}

// выставить один пин
__attribute__((always_inline)) inline void StepMotor::setPinMotor(uint8_t pin, boolean val)
{
#ifdef DRV_EEV_L9333                        // использование драйвера L9333 нужно инвертирование!!!
	digitalWriteDirect(pin, !val);
#else
	digitalWriteDirect(pin, val);
#endif  
}

/*
 * Движение на один шаг  в зависимости от выбранной последовательности
 */
void StepMotor::stepOne(uint8_t thisStep)
{
#if EEV_PHASE == PHASE_8s    // движение 8 фаз полушаг (pav2000)
 //  SerialDbg.print("PHASE_8s ");
    switch (thisStep) {
      case 0:  // 0001
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, HIGH);
      break;
      case 1:  //1001
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, HIGH);       
      break;
      case 2:  //1000
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 3:  //1100
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 4:  //0100
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 5:  //0110
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 6:  //0010
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 7:  // 0011
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, HIGH);
      break;
    }
#elif  EEV_PHASE == PHASE_8   // движение 8 фаз  
 //  SerialDbg.print("PHASE_8 ");
    switch (thisStep) {
      case 0:  // 1000
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);
      break;
      case 1:  //1100
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);       
      break;
      case 2:  //0100
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 3:  //0110
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 4:  //0010
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 5:  //0011
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, HIGH);         
      break;
      case 6:  //0001
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, HIGH);         
      break;
      case 7:  // 1001
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, HIGH);
      break;
    }   
#elif EEV_PHASE == PHASE_4   // движение 4 фаз
//   SerialDbg.print("PHASE_4 ");
   switch (thisStep) {
      case 0:  // 1001
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, HIGH);
      break;
      case 1:  //0011
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, LOW);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, HIGH);       
      break;
      case 2:  //0110
        setPinMotor(motor_pin_1, LOW);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, HIGH);
        setPinMotor(motor_pin_4, LOW);         
      break;
      case 3:  //1100
        setPinMotor(motor_pin_1, HIGH);
        setPinMotor(motor_pin_2, HIGH);
        setPinMotor(motor_pin_3, LOW);
        setPinMotor(motor_pin_4, LOW);         
      break;
   }
#else 
      #error "Wrong EVI_PHASE constant!"
#endif    
 // SerialDbg.print("thisStep ");   SerialDbg.println(thisStep);    
}

// Снять напряжения с шаговика
void StepMotor::motor_off()
{
  #ifdef DRV_EEV_L9333                        // использование драйвера L9333 нужно инвертирование!!!
  digitalWriteDirect(motor_pin_1, HIGH);
  digitalWriteDirect(motor_pin_2, HIGH);
  digitalWriteDirect(motor_pin_3, HIGH);
  digitalWriteDirect(motor_pin_4, HIGH); 
 #else
  digitalWriteDirect(motor_pin_1, LOW);
  digitalWriteDirect(motor_pin_2, LOW);
  digitalWriteDirect(motor_pin_3, LOW);
  digitalWriteDirect(motor_pin_4, LOW); 
#endif 
}
