/*
 * Bitops Module
 *
 * Copyright (C) 2010 Corentin Chary <corentin.chary@gmail.com>
 *
 * Mostly inspired by (stolen from) linux/bitmap.h and linux/bitops.h
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef BITOPS_H
#define BITOPS_H

/**
 * extract32:
 * @value: the value to extract the bit field from
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 *
 * Extract from the 32 bit input @value the bit field specified by the
 * @start and @length parameters, and return it. The bit field must
 * lie entirely within the 32 bit word. It is valid to request that
 * all 32 bits are returned (ie @length 32 and @start 0).
 *
 * Returns: the value of the bit field extracted from the input value.
 */
static inline uint32_t extract32(uint32_t value, int start, int length)
{
    assert(start >= 0 && length > 0 && length <= 32 - start);
    return (value >> start) & (~0U >> (32 - length));
}

/**
 * extract16:
 * @value: the value to extract the bit field from
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 *
 * Extract from the 16 bit input @value the bit field specified by the
 * @start and @length parameters, and return it. The bit field must
 * lie entirely within the 16 bit word. It is valid to request that
 * all 16 bits are returned (ie @length 16 and @start 0).
 *
 * Returns: the value of the bit field extracted from the input value.
 */
static inline uint16_t extract16(uint16_t value, int start, int length)
{
    assert(start >= 0 && length > 0 && length <= 16 - start);
    return extract32(value, start, length);
}


/**
 * deposit32:
 * @value: initial value to insert bit field into
 * @start: the lowest bit in the bit field (numbered from 0)
 * @length: the length of the bit field
 * @fieldval: the value to insert into the bit field
 *
 * Deposit @fieldval into the 32 bit @value at the bit field specified
 * by the @start and @length parameters, and return the modified
 * @value. Bits of @value outside the bit field are not modified.
 * Bits of @fieldval above the least significant @length bits are
 * ignored. The bit field must lie entirely within the 32 bit word.
 * It is valid to request that all 32 bits are modified (ie @length
 * 32 and @start 0).
 *
 * Returns: the modified @value.
 */
static inline uint32_t deposit32(uint32_t value, int start, int length,
                                 uint32_t fieldval)
{
    uint32_t mask;
    assert(start >= 0 && length > 0 && length <= 32 - start);
    mask = (~0U >> (32 - length)) << start;
    return (value & ~mask) | ((fieldval << start) & mask);
}

#endif
