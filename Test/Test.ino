// Test WireSam, extEEPROM, DS3232RTC library
// https://github.com/vad7/Arduino-DUE-WireSam


#include "Arduino.h"
#include <rtc_clock.h>                      // работа со встроенными часами  Это основные часы!!!
RTC_clock rtcSAM3X8(1);                                               // Внутренние часы, используется часовой кварц

//#include <WireSam.h>
//#include <extEEPROM.h>
//#include <DS3232.h>

//#include "mini-printf.h"

unsigned int m_strlen(const char *s)
{
	unsigned int len = 0;
	while (s[len] != '\0') len++;
	return len;
}

struct mini_buff {
	char *buffer, *pbuffer;
	unsigned int buffer_len;
};

#define f_uppercase 0x10
#define f_unsigned  0x20
#define m_zero_pad	0x0F

// flags: f_uppercase + f_unsigned + (zero_pad & m_zero_pad)
unsigned int m_itoa(long value, char *buffer, unsigned char radix, unsigned char flags)
{
	char	*pbuffer = buffer;
	unsigned char	negative = 0;
	unsigned int	i, len;

	/* No support for unusual radixes. */
	if (radix > 36 || radix <= 1) return 0;

	if (value < 0 && !(flags & f_unsigned)) {
		negative = 1;
		value = -value;
	}

	/* This builds the string back to front ... */
	do {
		unsigned char digit = value % radix;
		*(pbuffer++) = (digit < 10 ? '0' + digit : ((flags & f_uppercase) ? 'A' : 'a') + digit - 10);
		value /= radix;
	} while (value > 0);

	for (i = (pbuffer - buffer); i < (flags & m_zero_pad); i++)	*(pbuffer++) = '0';

	if (negative) *(pbuffer++) = '-';

	*(pbuffer) = '\0';

	/* ... now we reverse it (could do it recursively but will
	 * conserve the stack space) */
	len = (pbuffer - buffer);
	for (i = 0; i < len / 2; i++) {
		char j = buffer[i];
		buffer[i] = buffer[len-i-1];
		buffer[len-i-1] = j;
	}

	return len;
}

static int _putc(int ch, struct mini_buff *b)
{
	if ((unsigned int)((b->pbuffer - b->buffer) + 1) >= b->buffer_len)
		return 0;
	*(b->pbuffer++) = ch;
	*(b->pbuffer) = '\0';
	return 1;
}

static int _puts(char *s, unsigned int len, struct mini_buff *b)
{
	unsigned int i;

	if (b->buffer_len - (b->pbuffer - b->buffer) - 1 < len)
		len = b->buffer_len - (b->pbuffer - b->buffer) - 1;

	/* Copy to buffer */
	for (i = 0; i < len; i++)
		*(b->pbuffer++) = s[i];
	*(b->pbuffer) = '\0';

	return len;
}

unsigned int m_vsnprintf(char *buffer, unsigned int buffer_len, const char *fmt, va_list va)
{
	struct mini_buff b;
//	char bf[24];
	char ch;

	b.buffer = buffer;
	b.pbuffer = buffer;
	b.buffer_len = buffer_len;

	while ((ch=*(fmt++))) {
		if ((unsigned int)((b.pbuffer - b.buffer) + 1) >= b.buffer_len)
			break;
		if (ch!='%')
			_putc(ch, &b);
		else {
			char zero_pad = 0;
			char *ptr;
//			unsigned int len;

			ch=*(fmt++);

			/* Zero padding requested */
			if (ch=='0') {
				ch=*(fmt++);
				if (ch == '\0')
					goto end;
				if (ch >= '0' && ch <= '9')
					zero_pad = ch - '0';
				ch=*(fmt++);
			}

			switch (ch) {
				case 0:
					goto end;

				case 'u':
				case 'd':
					if(b.buffer_len - (b.pbuffer - b.buffer) < 12) break;
					b.pbuffer += m_itoa(va_arg(va, unsigned int), b.pbuffer, 10, (ch=='u' ? f_unsigned : 0) + zero_pad);
//					len = mini_itoa(va_arg(va, unsigned int), 10, 0, (ch=='u'), bf, zero_pad);
//					_puts(bf, len, &b);
					break;

				case 'x':
				case 'X':
					if(b.buffer_len - (b.pbuffer - b.buffer) < 9) break;
					b.pbuffer += m_itoa(va_arg(va, unsigned int), b.pbuffer, 16, ((ch=='X') ? f_uppercase : 0) + f_unsigned + zero_pad);
//					len = mini_itoa(va_arg(va, unsigned int), 16, (ch=='X'), 1, bf, zero_pad);
//					_puts(bf, len, &b);
					break;

				case 'c' :
					_putc((char)(va_arg(va, int)), &b);
					break;

				case 's' :
					ptr = va_arg(va, char*);
					_puts(ptr, m_strlen(ptr), &b);
					break;

#ifdef MINI_PRINTF_USE_FLOAT
				case '.' :  // %.xf - float with decimal point, x - precision
				    ch = *fmt;
				    fmt += 2;
					if(b.buffer_len - (b.pbuffer - b.buffer) < 16) break;
					ftoa(b.pbuffer, (float)va_arg(va, double), ch - '0');
					b.pbuffer += m_strlen(b.pbuffer);
					break;
#endif
				default:
					_putc(ch, &b);
					break;
			}
		}
	}
end:
	*b.pbuffer = '\0';
	return b.pbuffer - b.buffer;
}

unsigned int m_snprintf(char* buffer, unsigned int buffer_len, const char *fmt, ...)
{
	int ret;
	va_list va;
	va_start(va, fmt);
	ret = m_vsnprintf(buffer, buffer_len, fmt, va);
	va_end(va);

	return ret;
}

#define MINI_PRINTF_USE_FLOAT
#ifdef MINI_PRINTF_USE_FLOAT
// float to string, low stack usage
char *ftoa(char *outstr, float val, unsigned char precision)
{
	char *ret = outstr;
	// compute the rounding factor and fractional multiplier
	float roundingFactor = 0.5;
	unsigned long mult = 1;
	unsigned char padding = precision;
	while(precision--) {
		roundingFactor /= 10.0;
		mult *= 10;
	}
	if(val < 0.0){
		*outstr++ = '-';
		val = -val;
	}
	val += roundingFactor;
	outstr += m_itoa((long)val, outstr, 10, 0);
	if(padding > 0) {
		*(outstr++) = '.';
		m_itoa((val - (long)val) * mult, outstr, 10, padding);
	}
	return ret;
}
#endif



#define UART_SPEED        250000 //115200      // Скорость отладочного порта
#define I2C_SPEED         twiClock100kHz // Частота работы шины I2C
#define I2C_NUM_INIT      3           // Число попыток инициализации шины
#define I2C_TIME_WAIT     2000        // Время ожидания захвата мютекса шины I2C мсек
#define I2C_ADR_RTC       0x68        // Адрес чипа rtc на шине I2C
#define I2C_ADR_DS2482    0x18        // Адрес чипа OneWire на шине I2C

#define PIN_SPI_CS_W5XXX   10       // ++ ETH-CS   сигнал CS управление сетевым чипом
#define PIN_SPI_CS_SD      4        // ++ SD-CS    сигнал CS управление SD картой

#define I2C_ADR_EEPROM    0x50         // Адрес чипа eeprom на шине I2C
#define I2C_SIZE_EEPROM   kbits_256    // Объем чипа eeprom в килобитах
#define I2C_PAGE_EEPROM   32           // Размер страницы для чтения eeprom байты
#define I2C_FRAM_MEMORY   0		  // 1 - FRAM память


static char _buf[16+1];
// Получить причину последнего сброса контроллера
char* ResetCause( void )
{
  const uint32_t resetCause = rstc_get_reset_cause(RSTC);
 // strcpy(_buf,"");
  switch ( resetCause )
  {
  case RSTC_GENERAL_RESET:  strcpy(_buf, "General" );  break;
  case RSTC_BACKUP_RESET:   strcpy(_buf, "Backup" );   break;
  case RSTC_WATCHDOG_RESET: strcpy(_buf, "Watchdog" ); break;
  case RSTC_SOFTWARE_RESET: strcpy(_buf, "Software" ); break;
  case RSTC_USER_RESET:     strcpy(_buf, "User" );     break;
  default:                  strcpy(_buf, "Unknown" );  break;
  }
  return _buf;
}

#define PRINTF_BUF 256
char pbuf[PRINTF_BUF+1];

// Печать только в консоль
void pprintf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	m_vsnprintf(pbuf, sizeof(pbuf), format, ap);
	va_end(ap);
	Serial.print(pbuf);
}

#define WDT_TIME 0 //10
void watchdogSetup(void)
{
#if WDT_TIME == 0
	watchdogDisable();
#else
	watchdogEnable(WDT_TIME * 1000);
#endif
}

void setup()
{

    Serial.begin(UART_SPEED);
    Serial.print("Reset: ");
    Serial.println(ResetCause());
    pprintf("Before reset: %02d:%02d:%02d\n", GPBR->SYS_GPBR[1], GPBR->SYS_GPBR[2], GPBR->SYS_GPBR[3]);

/*
    char ssss[100];
    ssss[0] = 0;
    ssss[1] = 0;
    journal.printf("Start test\n");
    uint32_t ll = 0;
    uint32_t tt = micros();
    for(uint32_t i = 0; i < 20000; i++) {
		__asm__ volatile ("" ::: "memory");
    	ll += m_strlen(ssss);
    }
    journal.printf(" m_strlen(%d) = %u ms\n", ll, micros() - tt);
    ll = 0;
    tt = micros();
    for(uint32_t i = 0; i < 20000; i++) {
		__asm__ volatile ("" ::: "memory");
    	ll += strlen(ssss);
    }
    journal.printf(" strlen(%d) = %u ms\n", ll, micros() - tt);

    strcpy(ssss, "345235");
    journal.printf("Start test 2\n");
    ll = 0;
    tt = micros();
    for(uint32_t i = 0; i < 20000; i++) {
		__asm__ volatile ("" ::: "memory");
    	ll += m_strlen(ssss);
    }
    journal.printf(" m_strlen(%d) = %u ms\n", ll, micros() - tt);
    ll = 0;
    tt = micros();
    for(uint32_t i = 0; i < 20000; i++) {
		__asm__ volatile ("" ::: "memory");
    	ll += strlen(ssss);
    }
    journal.printf(" strlen(%d) = %u ms\n", ll, micros() - tt);

    strcpy(ssss, "dfgrewf");
    journal.printf("Start test 3\n");
    ll = 0;
    tt = micros();
    for(uint32_t i = 0; i < 20000; i++) {
		__asm__ volatile ("" ::: "memory");
    	ll += m_strlen(ssss);
    }
    journal.printf(" m_strlen(%d) = %u ms\n", ll, micros() - tt);
    ll = 0;
    tt = micros();
    for(uint32_t i = 0; i < 20000; i++) {
		__asm__ volatile ("" ::: "memory");
    	ll += strlen(ssss);
    }
    journal.printf(" strlen(%d) = %u ms\n", ll, micros() - tt);

*/


    SerialUSB.begin(UART_SPEED);
    SerialUSB.print("Native port!");


    unsigned long time;
    rtcSAM3X8.init();                             // Запуск внутренних часов
    pprintf("Init clock\n");
    rtcSAM3X8.set_time(10, 25, 15);                // Установить внутренние часы по i2c
    while(1) {
    	delay(1000);
    	pprintf("%02d:%02d:%02d\n", GPBR->SYS_GPBR[1] = rtcSAM3X8.get_hours(), GPBR->SYS_GPBR[2] = rtcSAM3X8.get_minutes(), GPBR->SYS_GPBR[3] = rtcSAM3X8.get_seconds());
        SerialUSB.print(rtcSAM3X8.get_minutes()); SerialUSB.print(':'); SerialUSB.println(rtcSAM3X8.get_seconds());
    }

	pprintf("Test: %d, %u, %x, %X, %c, %.2f\n", -235353, 1111111, 65535, 353554, '$', 123.45678);



  while(1) ;
}

// The loop function is called in an endless loop
void loop()
{

}

