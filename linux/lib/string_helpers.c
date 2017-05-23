/*
 * Helpers for formatting and printing strings
 *
 * Copyright 31 August 2008 James Bottomley
 * Copyright (C) 2013, Intel Corporation
 */
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/export.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/string_helpers.h>

/**
 * string_get_size - get the size in the specified units
 * @size:	The size to be converted
 * @units:	units to use (powers of 1000 or 1024)
 * @buf:	buffer to format to
 * @len:	length of buffer
 *
 * This function returns a string formatted to 3 significant figures
 * giving the size in the required units.  @buf should have room for
 * at least 9 bytes and will always be zero terminated.
 *
 */
void string_get_size(u64 size, const enum string_size_units units,
		     char *buf, int len)
{
	static const char *const units_10[] = {
		"B", "kB", "MB", "GB", "TB", "PB", "EB"
	};
	static const char *const units_2[] = {
		"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"
	};
	static const char *const *const units_str[] = {
		[STRING_UNITS_10] = units_10,
		[STRING_UNITS_2] = units_2,
	};
	static const unsigned int divisor[] = {
		[STRING_UNITS_10] = 1000,
		[STRING_UNITS_2] = 1024,
	};
	int i, j;
	u32 remainder = 0, sf_cap;
	char tmp[8];

	tmp[0] = '\0';
	i = 0;
	if (size >= divisor[units]) {
		while (size >= divisor[units]) {
			remainder = do_div(size, divisor[units]);
			i++;
		}

		sf_cap = size;
		for (j = 0; sf_cap*10 < 1000; j++)
			sf_cap *= 10;

		if (j) {
			remainder *= 1000;
			remainder /= divisor[units];
			snprintf(tmp, sizeof(tmp), ".%03u", remainder);
			tmp[j+1] = '\0';
		}
	}

	snprintf(buf, len, "%u%s %s", (u32)size,
		 tmp, units_str[units][i]);
}
EXPORT_SYMBOL(string_get_size);

static bool unescape_space(char **src, char **dst)
{
	char *p = *dst, *q = *src;

	switch (*q) {
	case 'n':
		*p = '\n';
		break;
	case 'r':
		*p = '\r';
		break;
	case 't':
		*p = '\t';
		break;
	case 'v':
		*p = '\v';
		break;
	case 'f':
		*p = '\f';
		break;
	default:
		return false;
	}
	*dst += 1;
	*src += 1;
	return true;
}

static bool unescape_octal(char **src, char **dst)
{
	char *p = *dst, *q = *src;
	u8 num;

	if (isodigit(*q) == 0)
		return false;

	num = (*q++) & 7;
	while (num < 32 && isodigit(*q) && (q - *src < 3)) {
		num <<= 3;
		num += (*q++) & 7;
	}
	*p = num;
	*dst += 1;
	*src = q;
	return true;
}

static bool unescape_hex(char **src, char **dst)
{
	char *p = *dst, *q = *src;
	int digit;
	u8 num;

	if (*q++ != 'x')
		return false;

	num = digit = hex_to_bin(*q++);
	if (digit < 0)
		return false;

	digit = hex_to_bin(*q);
	if (digit >= 0) {
		q++;
		num = (num << 4) | digit;
	}
	*p = num;
	*dst += 1;
	*src = q;
	return true;
}

static bool unescape_special(char **src, char **dst)
{
	char *p = *dst, *q = *src;

	switch (*q) {
	case '\"':
		*p = '\"';
		break;
	case '\\':
		*p = '\\';
		break;
	case 'a':
		*p = '\a';
		break;
	case 'e':
		*p = '\e';
		break;
	default:
		return false;
	}
	*dst += 1;
	*src += 1;
	return true;
}

/**
 * string_unescape - unquote characters in the given string
 * @src:	source buffer (escaped)
 * @dst:	destination buffer (unescaped)
 * @size:	size of the destination buffer (0 to unlimit)
 * @flags:	combination of the flags (bitwise OR):
 *	%UNESCAPE_SPACE:
 *		'\f' - form feed
 *		'\n' - new line
 *		'\r' - carriage return
 *		'\t' - horizontal tab
 *		'\v' - vertical tab
 *	%UNESCAPE_OCTAL:
 *		'\NNN' - byte with octal value NNN (1 to 3 digits)
 *	%UNESCAPE_HEX:
 *		'\xHH' - byte with hexadecimal value HH (1 to 2 digits)
 *	%UNESCAPE_SPECIAL:
 *		'\"' - double quote
 *		'\\' - backslash
 *		'\a' - alert (BEL)
 *		'\e' - escape
 *	%UNESCAPE_ANY:
 *		all previous together
 *
 * Description:
 * The function unquotes characters in the given string.
 *
 * Because the size of the output will be the same as or less than the size of
 * the input, the transformation may be performed in place.
 *
 * Caller must provide valid source and destination pointers. Be aware that
 * destination buffer will always be NULL-terminated. Source string must be
 * NULL-terminated as well.
 *
 * Return:
 * The amount of the characters processed to the destination buffer excluding
 * trailing '\0' is returned.
 */
int string_unescape(char *src, char *dst, size_t size, unsigned int flags)
{
	char *out = dst;

	while (*src && --size) {
		if (src[0] == '\\' && src[1] != '\0' && size > 1) {
			src++;
			size--;

			if (flags & UNESCAPE_SPACE &&
					unescape_space(&src, &out))
				continue;

			if (flags & UNESCAPE_OCTAL &&
					unescape_octal(&src, &out))
				continue;

			if (flags & UNESCAPE_HEX &&
					unescape_hex(&src, &out))
				continue;

			if (flags & UNESCAPE_SPECIAL &&
					unescape_special(&src, &out))
				continue;

			*out++ = '\\';
		}
		*out++ = *src++;
	}
	*out = '\0';

	return out - dst;
}
EXPORT_SYMBOL(string_unescape);

static int escape_passthrough(unsigned char c, char **dst, size_t *osz)
{
	char *out = *dst;

	if (*osz < 1)
		return -ENOMEM;

	*out++ = c;

	*dst = out;
	*osz -= 1;

	return 1;
}

static int escape_space(unsigned char c, char **dst, size_t *osz)
{
	char *out = *dst;
	unsigned char to;

	if (*osz < 2)
		return -ENOMEM;

	switch (c) {
	case '\n':
		to = 'n';
		break;
	case '\r':
		to = 'r';
		break;
	case '\t':
		to = 't';
		break;
	case '\v':
		to = 'v';
		break;
	case '\f':
		to = 'f';
		break;
	default:
		return 0;
	}

	*out++ = '\\';
	*out++ = to;

	*dst = out;
	*osz -= 2;

	return 1;
}

static int escape_special(unsigned char c, char **dst, size_t *osz)
{
	char *out = *dst;
	unsigned char to;

	if (*osz < 2)
		return -ENOMEM;

	switch (c) {
	case '\\':
		to = '\\';
		break;
	case '\a':
		to = 'a';
		break;
	case '\e':
		to = 'e';
		break;
	default:
		return 0;
	}

	*out++ = '\\';
	*out++ = to;

	*dst = out;
	*osz -= 2;

	return 1;
}

static int escape_null(unsigned char c, char **dst, size_t *osz)
{
	char *out = *dst;

	if (*osz < 2)
		return -ENOMEM;

	if (c)
		return 0;

	*out++ = '\\';
	*out++ = '0';

	*dst = out;
	*osz -= 2;

	return 1;
}

static int escape_octal(unsigned char c, char **dst, size_t *osz)
{
	char *out = *dst;

	if (*osz < 4)
		return -ENOMEM;

	*out++ = '\\';
	*out++ = ((c >> 6) & 0x07) + '0';
	*out++ = ((c >> 3) & 0x07) + '0';
	*out++ = ((c >> 0) & 0x07) + '0';

	*dst = out;
	*osz -= 4;

	return 1;
}

static int escape_hex(unsigned char c, char **dst, size_t *osz)
{
	char *out = *dst;

	if (*osz < 4)
		return -ENOMEM;

	*out++ = '\\';
	*out++ = 'x';
	*out++ = hex_asc_hi(c);
	*out++ = hex_asc_lo(c);

	*dst = out;
	*osz -= 4;

	return 1;
}

/**
 * string_escape_mem - quote characters in the given memory buffer
 * @src:	source buffer (unescaped)
 * @isz:	source buffer size
 * @dst:	destination buffer (escaped)
 * @osz:	destination buffer size
 * @flags:	combination of the flags (bitwise OR):
 *	%ESCAPE_SPACE:
 *		'\f' - form feed
 *		'\n' - new line
 *		'\r' - carriage return
 *		'\t' - horizontal tab
 *		'\v' - vertical tab
 *	%ESCAPE_SPECIAL:
 *		'\\' - backslash
 *		'\a' - alert (BEL)
 *		'\e' - escape
 *	%ESCAPE_NULL:
 *		'\0' - null
 *	%ESCAPE_OCTAL:
 *		'\NNN' - byte with octal value NNN (3 digits)
 *	%ESCAPE_ANY:
 *		all previous together
 *	%ESCAPE_NP:
 *		escape only non-printable characters (checked by isprint)
 *	%ESCAPE_ANY_NP:
 *		all previous together
 *	%ESCAPE_HEX:
 *		'\xHH' - byte with hexadecimal value HH (2 digits)
 * @esc:	NULL-terminated string of characters any of which, if found in
 *		the source, has to be escaped
 *
 * Description:
 * The process of escaping byte buffer includes several parts. They are applied
 * in the following sequence.
 *	1. The character is matched to the printable class, if asked, and in
 *	   case of match it passes through to the output.
 *	2. The character is not matched to the one from @esc string and thus
 *	   must go as is to the output.
 *	3. The character is checked if it falls into the class given by @flags.
 *	   %ESCAPE_OCTAL and %ESCAPE_HEX are going last since they cover any
 *	   character. Note that they actually can't go together, otherwise
 *	   %ESCAPE_HEX will be ignored.
 *
 * Caller must provide valid source and destination pointers. Be aware that
 * destination buffer will not be NULL-terminated, thus caller have to append
 * it if needs.
 *
 * Return:
 * The amount of the characters processed to the destination buffer, or
 * %-ENOMEM if the size of buffer is not enough to put an escaped character is
 * returned.
 *
 * Even in the case of error @dst pointer will be updated to point to the byte
 * after the last processed character.
 */
int string_escape_mem(const char *src, size_t isz, char **dst, size_t osz,
		      unsigned int flags, const char *esc)
{
	char *out = *dst, *p = out;
	bool is_dict = esc && *esc;
	int ret = 0;

	while (isz--) {
		unsigned char c = *src++;

		/*
		 * Apply rules in the following sequence:
		 *	- the character is printable, when @flags has
		 *	  %ESCAPE_NP bit set
		 *	- the @esc string is supplied and does not contain a
		 *	  character under question
		 *	- the character doesn't fall into a class of symbols
		 *	  defined by given @flags
		 * In these cases we just pass through a character to the
		 * output buffer.
		 */
		if ((flags & ESCAPE_NP && isprint(c)) ||
		    (is_dict && !strchr(esc, c))) {
			/* do nothing */
		} else {
			if (flags & ESCAPE_SPACE) {
				ret = escape_space(c, &p, &osz);
				if (ret < 0)
					break;
				if (ret > 0)
					continue;
			}

			if (flags & ESCAPE_SPECIAL) {
				ret = escape_special(c, &p, &osz);
				if (ret < 0)
					break;
				if (ret > 0)
					continue;
			}

			if (flags & ESCAPE_NULL) {
				ret = escape_null(c, &p, &osz);
				if (ret < 0)
					break;
				if (ret > 0)
					continue;
			}

			/* ESCAPE_OCTAL and ESCAPE_HEX always go last */
			if (flags & ESCAPE_OCTAL) {
				ret = escape_octal(c, &p, &osz);
				if (ret < 0)
					break;
				continue;
			}
			if (flags & ESCAPE_HEX) {
				ret = escape_hex(c, &p, &osz);
				if (ret < 0)
					break;
				continue;
			}
		}

		ret = escape_passthrough(c, &p, &osz);
		if (ret < 0)
			break;
	}

	*dst = p;

	if (ret < 0)
		return ret;

	return p - out;
}
EXPORT_SYMBOL(string_escape_mem);
