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
// --------------------------------------------------------------------------------
// Описание: Класс работы с инвертором Omron MX2
// Паблик методы класса уинифицированы для всех инверторов
// --------------------------------------------------------------------------------
#ifndef OmronFC_h
#define OmronFC_h
#include "Constant.h"                       // Вся конфигурация и константы проекта Должен быть первым !!!!

#ifndef FC_VACON

// КОНКРЕТНЫЙ ИНВЕРТОР OMRON MX2 --------------------------------------------------------------------------------------------------------------
// Управление идет частотой в герцах. Внутри частота хранится в сотых герца  максимально возможная частота 650 Гц!!
// Мощность хранится в 0.1 кВт
// Напряжение в 0.1 В
const char *nameOmron    = {"Omron MX2"}; //  Имя, марка инвертора
// Регистры Omron MX2
#define MX2_STATE         0x0003       // (2 байта) Состояние ПЧ
#define MX2_TARGET_FR     0x0001       // (4 байта) Источник (задание) задания частоты (0,01 [Гц])
#define MX2_ACCEL_TIME    0x1103       // (4 байта) Время разгона (см компрессор) в 0.01 сек
#define MX2_DEACCEL_TIME  0x1105       // (4 байта) Время торможения (см компрессор) в 0.01 сек

#define MX2_CURRENT_FR    0x1001       // (4 байта) Контроль выходной частоты (0,01 [Гц])
#define MX2_AMPERAGE      0x1003       // (2 байта) Контроль выходного тока (0,01 [A])
#define MX2_VOLTAGE       0x1011       // (2 байта) Контроль выходного  напряжения 0,1 [В]
#define MX2_POWER         0x1012       // (2 байта) Контроль мощности 0,1 [кВт]
#define MX2_POWER_HOUR    0x1013       // (4 байта) Контроль ватт-часов 0,1 [кВт/ч]
#define MX2_HOUR          0x1015       // (4 байта) Контроль времени наработки в режиме "Ход" 1 [ч]
#define MX2_HOUR1         0x1017       // (4 байта) Контроль времени наработки при включенном питании 1 [ч]
#define MX2_TEMP          0x1019       // (2 байта) Контроль температуры радиатора (0.1 градус) -200...1500
#define MX2_VOLTAGE_DC    0x1026       // (2 байта) Контроль напряжения  постоянного тока (P-N) 0,1 [В]
#define MX2_NUM_ERR       0x0011       // (2 байта) Счетчик аварийных отключений 0...65530
#define MX2_ERROR1        0x0012       // (20 байт) Описалово 1 отключения  остальные 5 лежат последовательно за первой ошибкой адреса вычисляются MX2_ERROR1+i*0x0a
#define MX2_INIT_DEF      0x1357       // (2 байта) Задать режим инициализации 0 (ничего),1 (очистка истории отключений),2 (инициализация данных), 3 (очистка истории отключений и инициализация данных), 4 (очистка истории отключений, инициализация данных ипрограммы EzSQ)
#define MX2_INIT_RUN      0x13b7       // (2 байта) Запуск инициализации 0 (выключено), 1 (включено)

#define MX2_SOURCE_FR     0x1201       // (2 байта) Источник задания частоты
#define MX2_SOURCE_CMD    0x1202       // (2 байта) Источник команды
#define MX2_BASE_FR       0x1203       // (2 байта) Основная частота 300...«максимальная частота»  0.1 Гц
#define MX2_MAX_FR        0x1204       // (2 байта) Максимальная частота 300...4000 (10000) 0.1 Гц
#define MX2_DC_BRAKING    0x1245       // (2 байта) Разрешение торможения постоянным током
#define MX2_STOP_MODE     0x134e       // (2 байта) Выбор способа остановки  B091=01
#define MX2_MODE          0x13ae       // (2 байта) Выбор режима ПЧ b171=03

// Настройка инвертора под конкретный компрессор Регистры Hxxx Двигатель с постоянными магнитами (PM-двигатель)
#define MX2_b171          0x13ae       // b171 Выбор режима ПЧ b171 чт./зап. 0 (выключено), 1 (режим IM), 2 (режим высокой частоты), 3 (режим PM) =03
#define MX2_b180          0x13b7       // b180 Initialization trigger =01
#define MX2_H102          0x1571       // H102 Установка кода PM-двигателя  00 (стандартные данные Omron) 01 (данные автонастройки) = 1
#define MX2_H103          0x1572       // H103 Мощность PM-двигателя (0,1/0,2/0,4/0,55/0,75/1,1/1,5/2,2/3,0/3,7/4,0/5,5/7,5/11,0/15,0/18,5) = 7
#define MX2_H104          0x1573       // H104 Установка числа полюсов PM двигателя = 4
#define MX2_H105          0x1574       // H105 Номинальный ток PM-двигателя = 1000 (это 11А)
#define MX2_H106          0x1575       // H106 Константа R PM-двигателя От 0,001 до 65,535 Ом =0.55
#define MX2_H107          0x1576       // H107 Константа Ld PM-двигателя От 0,01 до 655,35 мГн =2.31
#define MX2_H108          0x1577       // H108 Константа Lq PM-двигателя От 0,01 до 655,35 мГн =2.7
#define MX2_H109          0x1578       // H109 Константа Ke PM-двигателя 0,0001...6,5535 Вмакс./(рад/с) =750 надо подбирать влияет на потребление и шум
#define MX2_H110          0x1579       // (4 байта) H110 Константа J PM-двигателя От 0,001 до 9999,000 кг/мІ =0.01
#define MX2_H111          0x157B       // H111 Константа R автонастройки От 0,001 до 65,535 Ом
#define MX2_H112          0x157C       // H112 Константа Ld автонастройки От 0,01 до 655,35 мГн
#define MX2_H113          0x157D       // H113 Константа Lq автонастройки От 0,01 до 655,35 мГн 
#define MX2_H116          0x1581       // H116 Отклик PM-двигателя по скорости 1...1000 =100 (default)
#define MX2_H117          0x1582       // H117 Пусковой ток PM-двигателя От 20,00 до 100,00% =70 (default)
#define MX2_H118          0x1583       // H118 Пусковое время PM-двигателя 0,01 ... 60,00 с =1 (default)
#define MX2_H119          0x1584       // H119 Постоянная стабилизации PM двигателя От 0 до 120% с =100
#define MX2_H121          0x1586       // H121 Минимальная частота PM двигателя От 0,0 до 25,5%  =0
#define MX2_H122          0x1587       // H122 Ток холостого хода PM двигателя От 0,00 до 100,00%   =50 (default)
#define MX2_H123          0x1588       // H123 Выбор способа запуска PM двигателя 00 (выключено) 01 (включено) =0 (default)
#define MX2_H131          0x158A       // H131 Оценка начального положения ротора PM-двигателя: время ожидания 0 В 0...255 =10 (default)
#define MX2_H132          0x158B       // H132 Оценка начального положения ротора PM-двигателя: время ожидания определения 0...255  =10 (default)
#define MX2_H133          0x158C       // H133 Оценка начального положения ротора PM-двигателя: время определения 0...255  =30 (default)
#define MX2_H134          0x158D       // H134 Оценка начального положения ротора PM-двигателя: коэффициент усиления напряжения 0...200   =100 (default)
#define MX2_C001          0x1401       // C001 Функция входа [1] 0 (FW: ход вперед) =0
#define MX2_C004          0x1404       // C004 Функция входа [4] 18 (RS: сброс) =18
#define MX2_C005          0x1405       // C005 Функция входа [5] [также вход «PTC»]   = 19 PTC Термистор с положительным ТКС для тепловой защиты (только C005)
#define MX2_C026          0x1404       // C026  Функция релейного выхода 5 (AL: сигнал ошибки) =05
#define MX2_b091          0x135E       // b091 Выбор способа остановки 0 (торможение до полной остановки),1 (остановка выбегом)=1
#define MX2_b021          0x1316       // b021 Режим работы при ограничении перегрузки 0 (выключено), 1 (включено при разгоне и вращении с постоянной скоростью), 
                                       //      2 (включено при вращении с постоянной скоростью), 3 (включено при разгоне и вращении с постоянной скоростью [повышение 
                                       //      скорости в генераторном режиме]) =1
#define MX2_b022          0x1317       // b022 Уровень ограничения перегрузки  200...2000 (0.1%) =                                      
#define MX2_b023          0x1318       // b023  Время торможения при ограничении перегрузки (0.1 сек)=10
#define MX2_F002          0x1103       // (4 байта) F002  Время разгона (1) Стандартное, принимаемое по умолчанию ускорение, диапазон от 0,001 до 3600 с (0.01 сек) =20 *100
#define MX2_F003          0x1105       // (4 байта) F003  Время торможения (1) Стандартное, принимаемое по умолчанию ускорение, диапазон от 0,001 до 3600 с (0.01 сек) =20 *100
#define MX2_A001          0x1201       // A001 Источник задания частоты  00 ...Потенц. на внешн. панели 01 ...Клеммы управления 02 ...Настройка параметра F001
                                       //      03 ...Ввод по сети ModBus 04 ...Доп. карта 06 ...Вход имп. последов. 07 ...через EzSQ 10 ...Результат арифметической операции =03
#define MX2_A002          0x1202       // A002 Источник команды «Ход» 01 ..Клеммы управления 02 ...Клавиша «Run» на клавишной или цифровой панели 03 ...Ввод по сети ModBus 04 ...Доп. карта =01
#define MX2_A003          0x1203       // A003 Основная частота Может быть установлено в диапазоне от 30 Гц до максимальной частоты (A004)( 0.1 Гц) =120*10
#define MX2_A004          0x1204       // A004 Максимальная частота Может быть установлена в диапазоне от основной частоты до 400 Гц (0.1 Гц) =120*10

// Биты Omron MX2
#define MX2_START         0x0001       // (бит) Команда «Ход» 1: Ход, 0: Стоп (действительно при A002 = 03)
#define MX2_SET_DIR       0x0002       // (бит) Команда направления вращения 1: Обратное вращение, 0: Вращение в  прямом направлении (действительно при  A002 = 03)
#define MX2_RESET         0x0004       // (бит) Сброс аварийного отключения (RS) 1: Сброс
#define MX2_READY         0x0011       // (бит) Готовность ПЧ 1: Готов, 0: Не готов
#define MX2_DIRECTION     0x0010       // (бит) Направление вращения 1: Обратное вращение, 0: Вращение в  прямом направлении (взаимоблокировка с  "d003")

#define TEST_NUMBER       1234         // Проверочный код для функции 0x08

// Значения регистров по умолчанию для НК (нужно для программирования инвертора (метод progFC), требуется для конкретного инвертора и компрессора)
#define valH102 0x01   // H102 Установка кода PM-двигателя  00 (стандартные данные Omron) 01 (данные автонастройки) = 1
#define valH103 7      // H103 Мощность PM-двигателя (0,1/0,2/0,4/0,55/0,75/1,1/1,5/2,2/3,0/3,7/4,0/5,5/7,5/11,0/15,0/18,5) = 7
#define valH104 0x04   // H104 Установка числа полюсов PM двигателя = 4
#define valH105 1000   // H105 Номинальный ток PM-двигателя = 1000 (это 11А)
#define valH106 550    // H106 Константа R PM-двигателя От 0,001 до 65,535 Ом =0.55 *1000
#define valH107 231    // H107 Константа Ld PM-двигателя От 0,01 до 655,35 мГн =2.31*100
#define valH108 270    // H108 Константа Lq PM-двигателя От 0,01 до 655,35 мГн =2.7*100
#define valH109 810    // H109 Константа Ke PM-двигателя 0,0001...6,5535 Вмакс./(рад/с) =750 надо подбирать влияет на потребление и шум
#define valH110 10     // H110 Константа J PM-двигателя От 0,001 до 9999,000 кг/мІ =0.01
#define valH119 100    // H119 Постоянная стабилизации PM двигателя От 0 до 120% с =100 
#define valH121 10     // H121 Минимальная частота PM двигателя От 0,0 до 25,5%  =10 (default) (менять не надо)
#define valH122 10     // H122 Ток холостого хода PM двигателя От 0,00 до 100,00%   =50 (default) (менять не надо)
#define valC001 0      // C001 Функция входа [1] 0 (FW: ход вперед) =0
#define valC004 18     // C004 Функция входа [4] 18 (RS: сброс) =18
#define valC005 19     // C005 Функция входа [5] [также вход «PTC»]   = 19 PTC Термистор с положительным ТКС для тепловой защиты (только C005) (менять не надо)
#define valC026 5      // C026  Функция релейного выхода 5 (AL: сигнал ошибки) =05
#define valb091 1      // b091 Выбор способа остановки 0 (торможение до полной остановки),1 (остановка выбегом)=1 (менять не надо)
#define valb021 1      // b021 Режим работы при ограничении перегрузки =1 (менять не надо)
#define valb022 1000   // b022 Уровень ограничения перегрузки  200...2000 (0.1%) =                                      
#define valb023 10     // b023 Время торможения при ограничении перегрузки (0.1 сек)=10
#define valA001 3      // A001 Источник задания частоты =03 (менять не надо)
#define valA002 1      // A002 Источник команды «Ход» =1 (менять не надо)

struct type_errorMX2       // структура ошибки
{
  uint16_t code;           // причина
  uint16_t status;         // Состояние ПЧ при отключении
  uint16_t noUse;          // Не используется
  uint16_t fr;             // Частота ПЧ при отключении
  uint16_t cur;            // Ток ПЧ при отключении
  uint16_t vol;            // Напряжение ПЧ при отключении
  uint32_t time1;          // Общее время наработки в режиме ХОД при отключении
  uint32_t time2;          // Общее время работы ПЧ при включенном питании в момент отключения
};

union union_errorFC        // Перевод ошибки Омрона
{
  type_errorMX2 MX2;
  uint16_t  inputBuf[10]; 
};

#define ERR_LINK_FC 0xFF  	    // Состояние инертора - нет связи.

class devOmronMX2   // Класс частотный преобразователь Omron MX2
{
public:
  int8_t initFC();                                 // Инициализация Частотника
  int8_t progFC();                                 // Программирование инвертора под конкретный компрессор
  void check_link(void);
  
  __attribute__((always_inline)) inline boolean get_present(){return GETBIT(flags,fFC);} // Наличие ПЧ в текущей конфигурации
  int8_t get_err(){return err;}                    // Получить последню ошибку частотника
  uint16_t get_numErr(){return numErr;}            // Получить число ошибок чтения
  void get_paramFC(char *var, char *ret);         // Получить параметр инвертора в виде строки
  boolean set_paramFC(char *var, float x);         // Установить параметр инвертора из строки

  // Получение отдельных значений 
  uint16_t get_Uptime(){return _data.Uptime;}				     // Время обновления алгоритма пид регулятора (сек) Основной цикл управления
  uint16_t get_PidFreqStep(){return _data.PidFreqStep;}          // Максимальный шаг (на увеличение) изменения частоты при ПИД регулировании в 0.01 Гц Необходимо что бы ЭРВ успевал
  uint16_t get_PidStop(){return _data.PidStop;}				     // Проценты от уровня защит (мощность, ток, давление, темпеартура) при которой происходит блокировка роста частоты пидом
  uint16_t get_dtCompTemp(){return _data.dtCompTemp;}    		 // Защита по температуре компрессора - сколько градусов не доходит до максимальной (TCOMP) и при этом происходит уменьшение частоты
  uint16_t get_startFreq(){return _data.startFreq;}              // Стартовая частота инвертора (см компрессор) в 0.01 
  uint16_t get_startFreqBoiler(){return _data.startFreqBoiler;}  // Стартовая частота инвертора (см компрессор) в 0.01  ГВС
  uint16_t get_minFreq(){return _data.minFreq;}                  // Минимальная  частота инвертора (см компрессор) в 0.01 
  uint16_t get_minFreqCool(){return _data.minFreqCool;}          // Минимальная  частота инвертора при охлаждении в 0.01 
  uint16_t get_minFreqBoiler(){return _data.minFreqBoiler;}      // Минимальная  частота инвертора при нагреве ГВС в 0.01
  uint16_t get_minFreqUser(){return _data.minFreqUser;}          // Минимальная  частота инвертора РУЧНОЙ РЕЖИМ (см компрессор) в 0.01 
  uint16_t get_maxFreq(){return _data.maxFreq;}                  // Максимальная частота инвертора (см компрессор) в 0.01 
  uint16_t get_maxFreqCool(){return _data.maxFreqCool;}          // Максимальная частота инвертора в режиме охлаждения  в 0.01 
  uint16_t get_maxFreqBoiler(){return _data.maxFreqBoiler;}      // Максимальная частота инвертора в режиме ГВС в 0.01 Гц поглощение бойлера обычно меньше чем СО
  uint16_t get_maxFreqUser(){return _data.maxFreqUser;}          // Максимальная частота инвертора РУЧНОЙ РЕЖИМ (см компрессор) в 0.01 
  uint16_t get_stepFreq(){return _data.stepFreq;}                // Шаг уменьшения инвертора при достижении максимальной температуры, мощности и тока (см компрессор) в 0.01 
  uint16_t get_stepFreqBoiler(){return _data.stepFreqBoiler;}    // Шаг уменьшения инвертора при достижении максимальной температуры, мощности и тока ГВС в 0.01 
  uint16_t get_dtTemp(){return _data.dtTemp;}                    // Привышение температуры от уставок (подача) при которой срабатыват защита (уменьшается частота) в сотых градуса
  uint16_t get_dtTempBoiler(){return _data.dtTempBoiler;}        // Привышение температуры от уставок (подача) при которой срабатыват защита ГВС в сотых градуса
  uint16_t get_maxFreqGen(){return _data.maxFreqGen;}            // Максимальная частота инвертора при работе от генератора в 0.01
  uint16_t get_PidMaxStep(){return _data.PidMaxStep;}
  uint16_t get_MaxPower() { return FC_MAX_POWER; }
  uint16_t get_MaxPowerBoiler() { return FC_MAX_POWER_BOILER; }

  // Управление по модбас Общее для всех частотников
  int16_t get_target() {return FC;}            // Получить целевую частоту в 0.01 герцах
  int8_t  set_target(int16_t x,boolean show, int16_t _min, int16_t _max);// Установить целевую частоту в 0.01 герцах show - выводить сообщение или нет + границы
  __attribute__((always_inline)) inline uint16_t get_power(){return power * 100;}              // Получить текущую мощность. В ваттах еденица измерения
  __attribute__((always_inline)) inline uint16_t get_current(){return current;}          // Получить текущий ток в 0.01А
  char * get_infoFC(char *bufn);                   // Получить информацию о частотнике
  boolean reset_errorFC();                         // Сброс ошибок инвертора
  boolean reset_FC();                              // Сброс инвертора через модбас
  int16_t read_stateFC();                          // Текущее состояние инвертора
  __attribute__((always_inline)) inline int16_t get_state(void) { return state; };
  int16_t read_tempFC();                           // Tемпература радиатора
   
  int16_t get_frequency() { return freqFC; }       // Получить текущую частоту в 0.01 Гц
  int8_t get_readState();                          // Прочитать (внутренние переменные обновляются) состояние Инвертора, возвращает или ОК или ошибку
  int8_t start_FC();                               // Команда ход на инвертор (целевая частота выставляется)
  int8_t stop_FC();                                // Команда стоп на инвертор
  boolean isfOnOff(){return GETBIT(flags,fOnOff);} // получить состояние инвертора вкл или выкл
  inline bool isRetOilWork(){ return GETBIT(flags, fFC_RetOilSt); }

  void check_blockFC();                            // Установить запрет на использование инвертора
  boolean get_blockFC(){return GETBIT(flags,fErrFC);}// Получить флаг блокировки инвертора
  union_errorFC get_errorFC(){return error;}       // Получить структуру с ошибкой
  
  // Аналоговое управление
  int16_t get_DAC(){return dac;};                  // Получить установленное значеие ЦАП
  int16_t get_level0(){return level0;}             // Получить Отсчеты ЦАП соответсвующие 0   частоте
  int16_t get_level100(){return level100;}         // Получить Отсчеты ЦАП соответсвующие максимальной частоте
  int16_t get_levelOff(){return levelOff;}         // Получить Минимальная частота при котором частотник отключается (ограничение минимальной мощности)
  int8_t set_level0(int16_t x);                    // Установить Отсчеты ЦАП соответсвующие 0   мощности
  int8_t set_level100(int16_t x);                  // Установить Отсчеты ЦАП соответсвующие 100 мощности
  int8_t set_levelOff(int16_t x);                  // Установить Минимальная мощность при котором частотник отключается (ограничение минимальной частоты)
  uint8_t get_pinA(){return  pin;}                 // Ножка куда прицеплено FC
   
  // Сервис
  char*   get_note(){return  note;}                // Получить описание
  char*   get_name(){return  name;}                // Получить имя
  uint8_t *get_save_addr(void) { return (uint8_t *)&_data; } // Адрес структуры сохранения
  uint16_t get_save_size(void) { return sizeof(_data); } // Размер структуры сохранения

private:
  int8_t  err;                                     // ошибка частотника (работа) при ошибке останов ТН
  uint16_t numErr;                                 // число ошибок чтение по модбасу
  uint8_t number_err;                              // Число ошибок связи при превышении FC_NUM_READ блокировка инвертора
   // Управление по 485
  uint16_t FC;                                     // Целевая частота инвертора в 0.01 герцах
  uint16_t freqFC;                                 // Чтение: текущая частота инвертора в 0.01 герцах
  uint16_t power;                                  // Чтение: Текущая мощность инвертора  в 100 ватных единицах (3 это 300 вт)
  uint16_t current;                                // Чтение: Текущий ток инвертора  в 0.01 Ампера единицах
  
  int16_t state;                                   // Чтение: Состояние ПЧ регистр MX2_STATE
  union_errorFC error;                             // Структура для дешефрации ошибки инвертора
        
  // Аналоговое управление
  int16_t dac;                                     // Текущее значение ЦАП
  int16_t level0;                                  // Отсчеты ЦАП соответсвующие 0   частота
  int16_t level100;                                // Отсчеты ЦАП соответсвующие максимальной частотет
  int16_t levelOff;                                // Минимальная мощность при котором частотник отключается (ограничение минимальной мощности)
  uint8_t pin;                                     // Ножка куда прицеплено FC
  
  char *note;                                      // Описание
  char *name;                                      // Имя инвертора

// Структура для сохранения настроек, Uptime всегда первый
  struct {
	  uint16_t Uptime;				  // Время обновления алгоритма пид регулятора (сек) Основной цикл управления
	  uint16_t PidFreqStep;           // Максимальный шаг (на увеличение) изменения частоты при ПИД регулировании в 0.01 Гц Необходимо что бы ЭРВ успевал
	  uint16_t PidStop;				  // Проценты от уровня защит (мощность, ток, давление, темпеартура) при которой происходит блокировка роста частоты пидом
	  uint16_t dtCompTemp;    		  // Защита по температуре компрессора - сколько градусов не доходит до максимальной (TCOMP) и при этом происходит уменьшение частоты
	  int16_t startFreq;              // Стартовая частота инвертора (см компрессор) в 0.01 
	  int16_t startFreqBoiler;        // Стартовая частота инвертора (см компрессор) в 0.01  ГВС
	  int16_t minFreq;                // Минимальная  частота инвертора (см компрессор) в 0.01 
	  int16_t minFreqCool;            // Минимальная  частота инвертора при охлаждении в 0.01 
	  int16_t minFreqBoiler;          // Минимальная  частота инвертора при нагреве ГВС в 0.01
	  int16_t minFreqUser;            // Минимальная  частота инвертора РУЧНОЙ РЕЖИМ (см компрессор) в 0.01 
	  int16_t maxFreq;                // Максимальная частота инвертора (см компрессор) в 0.01 
	  int16_t maxFreqCool;            // Максимальная частота инвертора в режиме охлаждения  в 0.01 
	  int16_t maxFreqBoiler;          // Максимальная частота инвертора в режиме ГВС в 0.01 Гц поглощение бойлера обычно меньше чем СО
	  int16_t maxFreqUser;            // Максимальная частота инвертора РУЧНОЙ РЕЖИМ (см компрессор) в 0.01 
	  int16_t stepFreq;               // Шаг уменьшения инвертора при достижении максимальной температуры, мощности и тока (см компрессор) в 0.01 
	  int16_t stepFreqBoiler;         // Шаг уменьшения инвертора при достижении максимальной температуры, мощности и тока ГВС в 0.01 
	  uint16_t dtTemp;                // Привышение температуры от уставок (подача) при которой срабатыват защита (уменьшается частота) в сотых градуса
	  uint16_t dtTempBoiler;          // Привышение температуры от уставок (подача) при которой срабатыват защита ГВС в сотых градуса
	#ifdef FC_ANALOG_CONTROL
	  int16_t  level0;                // Отсчеты ЦАП соответсвующие 0   частота
	  int16_t  level100;              // Отсчеты ЦАП соответсвующие максимальной частота
	  int16_t  levelOff;              // Минимальная мощность при котором частотник отключается (ограничение минимальной мощности)
	#endif
	  uint8_t  setup_flags;           // флаги настройки
	  int16_t maxFreqGen;				// Максимальная скорость инвертора при работе от генератора в 0.01
	  uint16_t PidMaxStep;				// Максимальный шаг изменения частоты инвертора у PID регулятора, сотые
   } _data;  // Структура для сохранения настроек, setup_flags всегда последний!
	  uint8_t  flags;                 // флаги настройки
     
  // Функции работы с OMRON MX2  Чтение регистров
  #ifndef FC_ANALOG_CONTROL    // НЕ АНАЛОГОВОЕ УПРАВЛЕНИЕ
  int8_t   write_0x10_32(uint16_t cmd, uint32_t data);// Запись данных (4 байта) в регистр cmd возвращает код ошибки
  int16_t  read_0x03_16(uint16_t cmd);                // Функция Modbus 0х03 прочитать 2 байта
  uint32_t read_0x03_32(uint16_t cmd);                // Функция Modbus 0х03 прочитать 4 байта
  int16_t  read_0x03_error(uint16_t cmd);             // Функция Modbus 0х03 описание ошибки num
  int8_t   write_0x05_bit(uint16_t cmd, boolean f);   // Запись отдельного бита в регистр cmd возвращает код ошибки
  boolean  read_0x01_bit(uint16_t cmd);               // Чтение отдельного бита в регистр cmd возвращает код ошибки
  int8_t   write_0x06_16(uint16_t cmd, uint16_t data);// Запись данных (2 байта) в регистр cmd возвращает код ошибки
  int8_t   progReg16(uint16_t adrReg, char* nameReg, uint16_t valReg); // Программирование отдельного регистра инвертора c записью в журнал результатов
  int8_t   progReg32(uint16_t adrReg, char* nameReg, uint32_t valReg); // Программирование двух регистров инвертора c записью в журнал результатов
  #endif
 };

#endif

#endif
