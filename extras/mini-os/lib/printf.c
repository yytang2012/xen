/*
 ****************************************************************************
 * (C) 2003 - Rolf Neugebauer - Intel Research Cambridge
 ****************************************************************************
 *
 *        File: printf.c
 *      Author: Rolf Neugebauer (neugebar@dcs.gla.ac.uk)
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk) 
 *
 *        Date: Aug 2003, Aug 2005
 *
 * Environment: Xen Minimal OS
 * Description: Library functions for printing
 *              (Linux port, mainly lib/vsprintf.c)
 *
 ****************************************************************************
 */

/*
 * Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

/*
 * Fri Jul 13 2001 Crutcher Dunnavant <crutcher+kernel@datastacks.com>
 * - changed to provide snprintf and vsnprintf functions
 * So Feb  1 16:51:32 CET 2004 Juergen Quade <quade@hsnr.de>
 * - scnprintf and vscnprintf
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Copyright (c) 2011 The FreeBSD Foundation
 * All rights reserved.
 * Portions of this software were developed by David Chisnall
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if !defined HAVE_LIBC

#include <mini-os/os.h>
#include <mini-os/types.h>
#include <mini-os/hypervisor.h>
#include <mini-os/lib.h>
#include <mini-os/mm.h>
#include <mini-os/ctype.h>
#include <mini-os/posix/limits.h>

/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
    unsigned long result = 0,value;

    if (!base) {
        base = 10;
        if (*cp == '0') {
            base = 8;
            cp++;
            if ((*cp == 'x') && isxdigit(cp[1])) {
                cp++;
                base = 16;
            }
        }
    }
    while (isxdigit(*cp) &&
           (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

/**
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long simple_strtol(const char *cp,char **endp,unsigned int base)
{
    if(*cp=='-')
        return -simple_strtoul(cp+1,endp,base);
    return simple_strtoul(cp,endp,base);
}

/**
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long long simple_strtoull(const char *cp,char **endp,unsigned int base)
{
    unsigned long long result = 0,value;

    if (!base) {
        base = 10;
        if (*cp == '0') {
            base = 8;
            cp++;
            if ((*cp == 'x') && isxdigit(cp[1])) {
                cp++;
                base = 16;
            }
        }
    }
    while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
                                                               ? toupper(*cp) : *cp)-'A'+10) < base) {
        result = result*base + value;
        cp++;
    }
    if (endp)
        *endp = (char *)cp;
    return result;
}

/**
 * simple_strtoll - convert a string to a signed long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long long simple_strtoll(const char *cp,char **endp,unsigned int base)
{
    if(*cp=='-')
        return -simple_strtoull(cp+1,endp,base);
    return simple_strtoull(cp,endp,base);
}

static int skip_atoi(const char **s)
{
    int i=0;

    while (isdigit(**s))
        i = i*10 + *((*s)++) - '0';
    return i;
}

#define ZEROPAD 1               /* pad with zero */
#define SIGN    2               /* unsigned/signed long */
#define PLUS    4               /* show plus */
#define SPACE   8               /* space if plus */
#define LEFT    16              /* left justified */
#define SPECIAL 32              /* 0x */
#define LARGE   64              /* use 'ABCDEF' instead of 'abcdef' */

static char * number(char * buf, char * end, long long num, int base, int size, int precision, int type)
{
    char c,sign,tmp[66];
    const char *digits;
    const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;

    digits = (type & LARGE) ? large_digits : small_digits;
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return buf;
    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }
    if (type & SPECIAL) {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }
    i = 0;
    if (num == 0)
        tmp[i++]='0';
    else 
    {
        /* XXX KAF: force unsigned mod and div. */
        unsigned long long num2=(unsigned long long)num;
        unsigned int base2=(unsigned int)base;
        while (num2 != 0) { tmp[i++] = digits[num2%base2]; num2 /= base2; }
    }
    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type&(ZEROPAD+LEFT))) {
        while(size-->0) {
            if (buf <= end)
                *buf = ' ';
            ++buf;
        }
    }
    if (sign) {
        if (buf <= end)
            *buf = sign;
        ++buf;
    }
    if (type & SPECIAL) {
        if (base==8) {
            if (buf <= end)
                *buf = '0';
            ++buf;
        } else if (base==16) {
            if (buf <= end)
                *buf = '0';
            ++buf;
            if (buf <= end)
                *buf = digits[33];
            ++buf;
        }
    }
    if (!(type & LEFT)) {
        while (size-- > 0) {
            if (buf <= end)
                *buf = c;
            ++buf;
        }
    }
    while (i < precision--) {
        if (buf <= end)
            *buf = '0';
        ++buf;
    }
    while (i-- > 0) {
        if (buf <= end)
            *buf = tmp[i];
        ++buf;
    }
    while (size-- > 0) {
        if (buf <= end)
            *buf = ' ';
        ++buf;
    }
    return buf;
}

typedef union {                     /* floating point arguments %[aAeEfFgG] */
    double dbl;
    long double ldbl;
} fparg;

/*
 * MAXEXPDIG is the maximum number of decimal digits needed to store a
 * floating point exponent in the largest supported format.  It should
 * be ceil(log10(LDBL_MAX_10_EXP)) or, if hexadecimal floating point
 * conversions are supported, ceil(log10(LDBL_MAX_EXP)).  But since it
 * is presently never greater than 5 in practice, we fudge it.
 */
#define MAXEXPDIG 6
#if LDBL_MAX_EXP > 999999
#error "floating point buffers too small"
#endif

#define DEFPREC 6


static char *buf_pad(char *buf, char *end, int howmany, char with)
{
    while (howmany > 0) {
        if (buf <= end)
            *buf = with;
        ++buf;
        --howmany;
    }
    return buf;
}

static char *buf_print(char *buf, char *end, char *src, int len)
{
    while (len > 0) {
        if (buf <= end)
            *buf = *src;
        ++buf;
        ++src;
        --len;
    }
    return buf;
}

static char *buf_print_and_pad(char *buf, char *end, char *src, char *endsrc, int howmany, char with)
{
    while (src < endsrc) {
        if (buf <= end)
            *buf = *src;
        ++buf;
        ++src;
        --howmany;
    }
    return buf_pad(buf, end, howmany, with);
}

static char *__hldtoa(long double val, char *xdigs, int prec, int *expt, int *signflag, char **dtoaend)
{
}

/** sprintf_format_float - format a double into a buffer (used by vsnprintf)
 * @buf: Buffer for result
 * @end: The last available char (at the end of buf)
 * @num: The number to format
 * @ch: The printf specifier [eEfFgGaA]
 * @width: The field width (i.e. the minimum width; add padding as necessary)
 * @precision: meaning depends on fmt
 * @type: Flags (ZEROPAD, etc)
 *
 * Based on FreeBSD 10's vfprintf.c.
 */
static char *format_float(char *buf, char *end, fparg fparg, char qualifier, char ch, int width, int prec, int flags)
{
    /*
     * We can decompose the printed representation of floating
     * point numbers into several parts, some of which may be empty:
     *
     * [+|-| ] [0x|0X] MMM . NNN [e|E|p|P] [+|-] ZZ
     *    A       B     ---C---      D       E   F
     *
     * A:     'sign' holds this value if present; '\0' otherwise
     * B:     ox[1] holds the 'x' or 'X'; '\0' if not hexadecimal
     * C:     cp points to the string MMMNNN.  Leading and trailing
     *        zeros are not in the string and must be added.
     * D:     expchar holds this character; '\0' if no exponent, e.g. %f
     * F:     at least two digits for decimal, at least one digit for hex
     */
    char *cp;                   /* handy char pointer (short term usage) */
    char *decimal_point = ".";  /* locale specific decimal point */
    int decpt_len = 1;          /* length of decimal_point */
    int signflag;               /* true if float is negative */
    int expt;                   /* integer value of exponent */
    char expchar;               /* exponent character: [eEpP\0] */
    char *dtoaend;              /* pointer to end of converted digits */
    int expsize;                /* character count for expstr */
    int ndig;                   /* actual number of digits returned by dtoa */
    char expstr[MAXEXPDIG+2];   /* buffer for exponent string: e+ZZZ */
    char *dtoaresult;           /* buffer allocated by dtoa */
    const char *xdigs;          /* digits for %[xX] conversion */
    static const char xdigs_lower[16] = "0123456789abcdef";
    static const char xdigs_upper[16] = "0123456789ABCDEF";
    char ox[2] = {"0\0"};     /* space for 0x; ox[1] is either x, X, or \0 */

#define PAD(howmany, with) (buf = buf_pad(buf, end, (howmany), (with)))
#define PRINT(ptr, len) (buf = buf_print(buf, end, (ptr), (len)))
#define PRINTANDPAD(p, ep, len, with) (buf = buf_print_and_pad(buf, end, (p), (ep), (len), (with)))

    switch (ch) {
        case 'a':
        case 'A':
            if (ch == 'a') {
                ox[1] = 'x';
                xdigs = xdigs_lower;
                expchar = 'p';
            } else {
                ox[1] = 'X';
                xdigs = xdigs_upper;
                expchar = 'P';
            }
            if (prec >= 0)
                prec++;
            if (qualifier == 'L') {
                dtoaresult = cp =
                    __hldtoa(fparg.ldbl, xdigs, prec,
                            &expt, &signflag, &dtoaend);
            } else {
                dtoaresult = cp =
                    __hdtoa(fparg.dbl, xdigs, prec,
                            &expt, &signflag, &dtoaend);
            }
            if (prec < 0)
                prec = dtoaend - cp;
            if (expt == INT_MAX)
                ox[1] = '\0';
            goto fp_common;
        case 'e':
        case 'E':
            expchar = ch;
            if (prec < 0)       /* account for digit before decpt */
                prec = DEFPREC + 1;
            else
                prec++;
            goto fp_begin;
        case 'f':
        case 'F':
            expchar = '\0';
            goto fp_begin;
        case 'g':
        case 'G':
            expchar = ch - ('g' - 'e');
            if (prec == 0)
                prec = 1;
fp_begin:
            if (prec < 0)
                prec = DEFPREC;

            if (type == 'L') {
                dtoaresult = cp =
                    __ldtoa(&fparg.ldbl, expchar ? 2 : 3, prec,
                            &expt, &signflag, &dtoaend);
            } else {
                dtoaresult = cp =
                    dtoa(fparg.dbl, expchar ? 2 : 3, prec,
                            &expt, &signflag, &dtoaend);
                if (expt == 9999)
                    expt = INT_MAX;
            }
fp_common:
            if (signflag)
                sign = '-';
            if (expt == INT_MAX) {          /* inf or nan */
                if (*cp == 'N') {
                    cp = (ch >= 'a') ? "nan" : "NAN";
                    sign = '\0';
                } else
                    cp = (ch >= 'a') ? "inf" : "INF";
                size = 3;
                flags &= ~ZEROPAD;
                break;
            }
            //flags |= FPT;
            ndig = dtoaend - cp;
            if (ch == 'g' || ch == 'G') {
                if (expt > -4 && expt <= prec) {
                    /* Make %[gG] smell like %[fF] */
                    expchar = '\0';
                    if (flags & SPECIAL)
                        prec -= expt;
                    else
                        prec = ndig - expt;
                    if (prec < 0)
                        prec = 0;
                } else {
                    /*
                     * Make %[gG] smell like %[eE], but
                     * trim trailing zeroes if no # flag.
                     */
                    if (!(flags & SPECIAL))
                        prec = ndig;
                }
            }
            if (expchar) {
                expsize = exponent(expstr, expt - 1, expchar);
                size = expsize + prec;
                if (prec > 1 || flags & SPECIAL)
                    size += decpt_len;
            } else {
                /* space for digits before decimal point */
                if (expt > 0)
                    size = expt;
                else            /* "0" */
                    size = 1;
                /* space for decimal pt and following digits */
                if (prec || flags & SPECIAL)
                    size += prec + decpt_len;
                if ((flags & GROUPING) && expt > 0)
                    size += grouping_init(&gs, expt, locale);
            }
    }

    /*
     * All reasonable formats wind up here.  At this point, `cp'
     * points to a string which (if not flags&LEFT) should be
     * padded out to `width' places.  If flags&ZEROPAD, it should
     * first be prefixed by any sign or other prefix; otherwise,
     * it should be blank padded before the prefix is emitted.
     * After any left-hand padding and prefixing, emit zeroes
     * required by a decimal [diouxX] precision, then print the
     * string proper, then emit zeroes required by any leftover
     * floating precision; finally, if LEFT, pad with blanks.
     *
     * Compute actual size, so we know how much to pad.
     * size excludes decimal prec; realsz includes it.
     */
    realsz = dprec > size ? dprec : size;
    if (sign)
        realsz++;
    if (ox[1])
        realsz += 2;

    prsize = width > realsz ? width : realsz;
#if 0
    if ((unsigned)ret + prsize > INT_MAX) {
        ret = EOF;
        errno = EOVERFLOW;
        goto error;
    }
#endif

    /* right-adjusting blank padding */
    if ((flags & (LEFT|ZEROPAD)) == 0) {
        PAD(width - realsz, ' ');
    }

    /* prefix */
    if (sign)
        PRINT(&sign, 1);

    if (ox[1]) {            /* ox[1] is either x, X, or \0 */
        ox[0] = '0';
        PRINT(ox, 2);
    }

    /* right-adjusting zero padding */
    if ((flags & (LEFT|ZEROPAD)) == ZEROPAD)
        PAD(width - realsz, '0');

    /* glue together f_p fragments */
    if (!expchar) {     /* %[fF] or sufficiently short %[gG] */
        if (expt <= 0) {
            PRINT(zeroes, 1);
            if (prec || flags & SPECIAL)
                PRINT(decimal_point,decpt_len);
            PAD(-expt, '0');
            /* already handled initial 0's */
            prec += expt;
        } else {
            PRINTANDPAD(cp, dtoaend,
                    expt, zeroes);
            cp += expt;
            if (prec || flags & SPECIAL)
                PRINT(decimal_point,decpt_len);
        }
        PRINTANDPAD(cp, dtoaend, prec, zeroes);
    } else {    /* %[eE] or sufficiently long %[gG] */
        if (prec > 1 || flags & SPECIAL) {
            PRINT(cp++, 1);
            PRINT(decimal_point, decpt_len);
            PRINT(cp, ndig-1);
            PAD(prec - ndig, '0');
        } else  /* XeYYY */
            PRINT(cp, 1);
        PRINT(expstr, expsize);
    }
#endif
    /* left-adjusting padding (always blank) */
    if (flags & LEFT)
        PAD(width - realsz, ' ');

    return buf;
}

/**
* vsnprintf - Format a string and place it in a buffer
* @buf: The buffer to place the result into
* @size: The size of the buffer, including the trailing null space
* @fmt: The format string to use
* @args: Arguments for the format string
*
* Call this function if you are already dealing with a va_list.
* You probably want snprintf instead.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    int len;
    unsigned long long num;
    int i, base;
    char *str, *end, c;
    const char *s;
    long double d;

    int flags;          /* flags to number() */

    int field_width;    /* width of output field */
    int precision;              /* min. # of digits for integers; max
                                   number of chars for from string */
    int qualifier;              /* 'h', 'l', or 'L' for integer fields */
                                /* 'z' support added 23/7/1999 S.H.    */
                                /* 'z' changed to 'Z' --davidm 1/25/99 */

    str = buf;
    end = buf + size - 1;

    if (end < buf - 1) {
        end = ((void *) -1);
        size = end - buf + 1;
    }

    for (; *fmt ; ++fmt) {
        if (*fmt != '%') {
            if (str <= end)
                *str = *fmt;
            ++str;
            continue;
        }

        /* process flags */
        flags = 0;
    repeat:
        ++fmt;          /* this also skips first '%' */
        switch (*fmt) {
        case '-': flags |= LEFT; goto repeat;
        case '+': flags |= PLUS; goto repeat;
        case ' ': flags |= SPACE; goto repeat;
        case '#': flags |= SPECIAL; goto repeat;
        case '0': flags |= ZEROPAD; goto repeat;
        }

        /* get field width */
        field_width = -1;
        if (isdigit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*') {
            ++fmt;
            /* it's the next argument */
            field_width = va_arg(args, int);
            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        /* get the precision */
        precision = -1;
        if (*fmt == '.') {
            ++fmt;
            if (isdigit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*') {
                ++fmt;
                          /* it's the next argument */
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt =='Z') {
            qualifier = *fmt;
            ++fmt;
            if (qualifier == 'l' && *fmt == 'l') {
                qualifier = 'L';
                ++fmt;
            }
        }
        if (*fmt == 'q') {
            qualifier = 'L';
            ++fmt;
        }

        /* default base */
        base = 10;

        switch (*fmt) {
        case 'c':
            if (!(flags & LEFT)) {
                while (--field_width > 0) {
                    if (str <= end)
                        *str = ' ';
                    ++str;
                }
            }
            c = (unsigned char) va_arg(args, int);
            if (str <= end)
                *str = c;
            ++str;
            while (--field_width > 0) {
                if (str <= end)
                    *str = ' ';
                ++str;
            }
            continue;

        case 's':
            s = va_arg(args, char *);
            if (!s)
                s = "<NULL>";

            len = strnlen(s, precision);

            if (!(flags & LEFT)) {
                while (len < field_width--) {
                    if (str <= end)
                        *str = ' ';
                    ++str;
                }
            }
            for (i = 0; i < len; ++i) {
                if (str <= end)
                    *str = *s;
                ++str; ++s;
            }
            while (len < field_width--) {
                if (str <= end)
                    *str = ' ';
                ++str;
            }
            continue;

        case 'p':
            if (field_width == -1) {
                field_width = 2*sizeof(void *);
                flags |= ZEROPAD;
            }
            str = number(str, end,
                         (unsigned long) va_arg(args, void *),
                         16, field_width, precision, flags);
            continue;


        case 'n':
            if (qualifier == 'l') {
                long * ip = va_arg(args, long *);
                *ip = (str - buf);
            } else if (qualifier == 'Z') {
                size_t * ip = va_arg(args, size_t *);
                *ip = (str - buf);
            } else {
                int * ip = va_arg(args, int *);
                *ip = (str - buf);
            }
            continue;

        case '%':
            if (str <= end)
                *str = '%';
            ++str;
            continue;

            /* integer number formats - set up the flags and "break" */
        case 'o':
            base = 8;
            break;

        case 'X':
            flags |= LARGE;
        case 'x':
            base = 16;
            break;

        case 'd':
        case 'i':
            flags |= SIGN;
        case 'u':
            break;
    
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
            BUG_ON(qualifier == 'L');   /* Long doubles not supported */
            d = va_arg(args, double);
            str = format_float(str, end, d, *fmt, qualifier, field_width, precision, flags);
            continue;

        default:
            if (str <= end)
                *str = '%';
            ++str;
            if (*fmt) {
                if (str <= end)
                    *str = *fmt;
                ++str;
            } else {
                --fmt;
            }
            continue;
        }
        if (qualifier == 'L')
            num = va_arg(args, long long);
        else if (qualifier == 'l') {
            num = va_arg(args, unsigned long);
            if (flags & SIGN)
                num = (signed long) num;
        } else if (qualifier == 'Z') {
            num = va_arg(args, size_t);
        } else if (qualifier == 'h') {
            num = (unsigned short) va_arg(args, int);
            if (flags & SIGN)
                num = (signed short) num;
        } else {
            num = va_arg(args, unsigned int);
            if (flags & SIGN)
                num = (signed int) num;
        }

        str = number(str, end, num, base,
                     field_width, precision, flags);
    }
    if (str <= end)
        *str = '\0';
    else if (size > 0)
        /* don't write out a null byte if the buf size is zero */
        *end = '\0';
    /* the trailing null byte doesn't count towards the total
     * ++str;
     */
    return str-buf;
}

/**
 * snprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer, including the trailing null space
 * @fmt: The format string to use
 * @...: Arguments for the format string
 */
int snprintf(char * buf, size_t size, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i=vsnprintf(buf,size,fmt,args);
    va_end(args);
    return i;
}

/**
 * vsprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @args: Arguments for the format string
 *
 * Call this function if you are already dealing with a va_list.
 * You probably want sprintf instead.
 */
int vsprintf(char *buf, const char *fmt, va_list args)
{
    return vsnprintf(buf, 0xFFFFFFFFUL, fmt, args);
}


/**
 * sprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @fmt: The format string to use
 * @...: Arguments for the format string
 */
int sprintf(char * buf, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i=vsprintf(buf,fmt,args);
    va_end(args);
    return i;
}

/**
 * vsscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	format of buffer
 * @args:	arguments
 */
int vsscanf(const char * buf, const char * fmt, va_list args)
{
	const char *str = buf;
	char *next;
	char digit;
	int num = 0;
	int qualifier;
	int base;
	int field_width;
	int is_sign = 0;

	while(*fmt && *str) {
		/* skip any white space in format */
		/* white space in format matchs any amount of
		 * white space, including none, in the input.
		 */
		if (isspace(*fmt)) {
			while (isspace(*fmt))
				++fmt;
			while (isspace(*str))
				++str;
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;
		
		/* skip this conversion.
		 * advance both strings to next white space
		 */
		if (*fmt == '*') {
			while (!isspace(*fmt) && *fmt)
				fmt++;
			while (!isspace(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
		    *fmt == 'Z' || *fmt == 'z') {
			qualifier = *fmt++;
			if (unlikely(qualifier == *fmt)) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}
		base = 10;
		is_sign = 0;

		if (!*fmt || !*str)
			break;

		switch(*fmt++) {
		case 'c':
		{
			char *s = (char *) va_arg(args,char*);
			if (field_width == -1)
				field_width = 1;
			do {
				*s++ = *str++;
			} while (--field_width > 0 && *str);
			num++;
		}
		continue;
		case 's':
		{
			char *s = (char *) va_arg(args, char *);
			if(field_width == -1)
				field_width = INT_MAX;
			/* first, skip leading white space in buffer */
			while (isspace(*str))
				str++;

			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width--) {
				*s++ = *str++;
			}
			*s = '\0';
			num++;
		}
		continue;
		case 'n':
			/* return number of characters read so far */
		{
			int *i = (int *)va_arg(args,int*);
			*i = str - buf;
		}
		continue;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		case 'i':
                        base = 0;
		case 'd':
			is_sign = 1;
		case 'u':
			break;
		case '%':
			/* looking for '%' in str */
			if (*str++ != '%') 
				return num;
			continue;
		default:
			/* invalid format; stop here */
			return num;
		}

		/* have some sort of integer conversion.
		 * first, skip white space in buffer.
		 */
		while (isspace(*str))
			str++;

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
                    || (base == 16 && !isxdigit(digit))
                    || (base == 10 && !isdigit(digit))
                    || (base == 8 && (!isdigit(digit) || digit > '7'))
                    || (base == 0 && !isdigit(digit)))
				break;

		switch(qualifier) {
		case 'H':	/* that's 'hh' in format */
			if (is_sign) {
				signed char *s = (signed char *) va_arg(args,signed char *);
				*s = (signed char) simple_strtol(str,&next,base);
			} else {
				unsigned char *s = (unsigned char *) va_arg(args, unsigned char *);
				*s = (unsigned char) simple_strtoul(str, &next, base);
			}
			break;
		case 'h':
			if (is_sign) {
				short *s = (short *) va_arg(args,short *);
				*s = (short) simple_strtol(str,&next,base);
			} else {
				unsigned short *s = (unsigned short *) va_arg(args, unsigned short *);
				*s = (unsigned short) simple_strtoul(str, &next, base);
			}
			break;
		case 'l':
			if (is_sign) {
				long *l = (long *) va_arg(args,long *);
				*l = simple_strtol(str,&next,base);
			} else {
				unsigned long *l = (unsigned long*) va_arg(args,unsigned long*);
				*l = simple_strtoul(str,&next,base);
			}
			break;
		case 'L':
			if (is_sign) {
				long long *l = (long long*) va_arg(args,long long *);
				*l = simple_strtoll(str,&next,base);
			} else {
				unsigned long long *l = (unsigned long long*) va_arg(args,unsigned long long*);
				*l = simple_strtoull(str,&next,base);
			}
			break;
		case 'Z':
		case 'z':
		{
			size_t *s = (size_t*) va_arg(args,size_t*);
			*s = (size_t) simple_strtoul(str,&next,base);
		}
		break;
		default:
			if (is_sign) {
				int *i = (int *) va_arg(args, int*);
				*i = (int) simple_strtol(str,&next,base);
			} else {
				unsigned int *i = (unsigned int*) va_arg(args, unsigned int*);
				*i = (unsigned int) simple_strtoul(str,&next,base);
			}
			break;
		}
		num++;

		if (!next)
			break;
		str = next;
	}
	return num;
}

/**
 * sscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	formatting of buffer
 * @...:	resulting arguments
 */
int sscanf(const char * buf, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args,fmt);
	i = vsscanf(buf,fmt,args);
	va_end(args);
	return i;
}

#endif
