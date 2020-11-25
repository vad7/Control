/*
 * The Minimal snprintf() implementation
 *
 * Copyright (c) 2013,2014 Michal Ludvig <michal@logix.cz>
 * All rights reserved.
 *
 * Modified by vad7@yahoo.com - minimum stack usage, added float to string: %.xf, x - precision
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the auhor nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ----
 *
 * This is a minimal snprintf() implementation optimised
 * for embedded systems with a very limited program memory.
 * mini_snprintf() doesn't support _all_ the formatting
 * the glibc does but on the other hand is a lot smaller.
 * Here are some numbers from my STM32 project (.bin file size):
 *      no snprintf():      10768 bytes
 *      mini snprintf():    11420 bytes     (+  652 bytes)
 *      glibc snprintf():   34860 bytes     (+24092 bytes)
 * Wasting nearly 24kB of memory just for snprintf() on
 * a chip with 32kB flash is crazy. Use mini_snprintf() instead.
 *
 */

#include "mini-printf.h"

unsigned long strlen(const char *s);
// Warning - strlen() on ARM works faster on length > 6
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
unsigned int m_itoa(unsigned long value, char *buffer, unsigned char radix, unsigned char flags)
{
	char	*pbuffer = buffer;
	unsigned char	negative = 0;
	unsigned int	i, len;

	/* No support for unusual radixes. */
	if (radix > 36 || radix <= 1) return 0;

	if (!(flags & f_unsigned) && (long)value < 0) {
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
			unsigned char zero_pad = 0;
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
					if(b.buffer_len - (b.pbuffer - b.buffer) < (unsigned int)(zero_pad ? zero_pad+1 : 12)) break;
					b.pbuffer += m_itoa(va_arg(va, int), b.pbuffer, 10, (ch=='u' ? f_unsigned : 0) + zero_pad);
//					len = mini_itoa(va_arg(va, unsigned int), 10, 0, (ch=='u'), bf, zero_pad);
//					_puts(bf, len, &b);
					break;

				case 'x':
				case 'X':
					if(b.buffer_len - (b.pbuffer - b.buffer) < (unsigned int)(zero_pad ? zero_pad+1 : 9)) break;
					b.pbuffer += m_itoa(va_arg(va, unsigned int), b.pbuffer, 16, ((ch=='X') ? f_uppercase : 0) + f_unsigned + zero_pad);
//					len = mini_itoa(va_arg(va, unsigned int), 16, (ch=='X'), 1, bf, zero_pad);
//					_puts(bf, len, &b);
					break;

				case 'c' :
					_putc((char)(va_arg(va, int)), &b);
					break;

				case 's' :
					ptr = va_arg(va, char*);
					_puts(ptr, strlen(ptr), &b);
					break;

#ifdef MINI_PRINTF_USE_FLOAT
				case '.' :  // %.xf - float with decimal point, x - precision; %.xd - int32 with decimal point, divided by 10^x
			    	if(b.buffer_len - (b.pbuffer - b.buffer) < 16) break;
				    ch = *fmt++;
				    if(*fmt++ == 'f') {
				    	ftoa(b.pbuffer, (float)va_arg(va, double), ch - '0');
				    	b.pbuffer += m_strlen(b.pbuffer);
				    } else {
				    	b.pbuffer = dptoa(b.pbuffer, va_arg(va, int), ch - '0');
				    }
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

#ifdef MINI_PRINTF_USE_FLOAT
// float to string, low stack usage
char *ftoa(char *outstr, float val, unsigned char precision)
{
	char *ret = outstr;
	// compute the rounding factor and fractional multiplier
	float roundingFactor = 0.5f;
	unsigned long mult = 1;
	unsigned char padding = precision;
	while(precision--) {
		roundingFactor /= 10.0f;
		mult *= 10;
	}
	if(val < 0.0f){
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

// Decimal (fractional cast to integer) in string, precision: 1..10000
char *dptoa(char *outstr, int val, unsigned int precision)
{
    //while(*outstr) outstr++;
	if(val < 0) {
		*outstr++ = '-';
		val = -val;
	}
    int div;
    switch(precision) {
    	case 1: div = 10; break;
    	case 2: div = 100; break;
    	case 3: div = 1000; break;
    	case 4: div = 10000; break;
    	default: div = 1;
    }
    outstr += i10toa(val / div, outstr, 0);
    if(precision > 0) {
		*(outstr++) = '.';
		outstr += i10toa(val % div, outstr, precision);
    }
    return outstr;
}

//int в *char в строку ДОБАВЛЯЕТСЯ значение экономим место и скорость и стек radix=10
unsigned int i10toa(int value, char *string, unsigned int zero_pad)
{
	char *pbuffer = string;
	unsigned char negative;
	if(value < 0) {
		negative = 1;
		value = -value;
	} else negative = 0;
	do {
		*(pbuffer++) = '0' + value % 10;
		value /= 10;
	} while (value > 0);
	for(unsigned int i = (pbuffer - string); i < zero_pad; i++)	*(pbuffer++) = '0';
	if(negative) *(pbuffer++) = '-';
	*(pbuffer) = '\0';
	unsigned int len = (pbuffer - string);
	for(unsigned int i = 0; i < len / 2; i++) {
		char j = string[i];
		string[i] = string[len-i-1];
		string[len-i-1] = j;
	}
	return len;
}

#endif
