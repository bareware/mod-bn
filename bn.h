/* SPDX-License-Identifier: MIT-0 | Copyright (c) 2026 barewaredev */

#ifndef BN_H
#define BN_H

#ifndef BN_VALUE_BITS
#define BN_VALUE_BITS       512
#endif

#define BN_WORDS            (BN_VALUE_BITS / (sizeof(bn_word_t) * 8))

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint32_t bn_word_t;
typedef uint64_t bn_dword_t;

typedef struct {
    bn_word_t words[BN_WORDS];
} bn_value_t;

typedef struct {
    bn_word_t words[BN_WORDS * 2];
} bn_dvalue_t;

uint32_t bn_clz(const bn_value_t* val);
int32_t bn_cmp(const bn_value_t* a, const bn_value_t* b);
void bn_shl(bn_value_t* val, uint32_t bits);
void bn_shr(bn_value_t* val, uint32_t bits);
void bn_and(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
void bn_or(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
void bn_xor(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
bn_word_t bn_add(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
bn_word_t bn_sub(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
void bn_mul(bn_dvalue_t* res, const bn_value_t* a, const bn_value_t* b);
int bn_div(bn_value_t* res, bn_value_t* rem, const bn_dvalue_t* a, const bn_value_t* b);

#ifdef __cplusplus
}
#endif

#endif


#ifdef BN_IMPLEMENTATION

#define BN_WORD_BITS        (sizeof(bn_word_t) * 8)

static uint32_t _bn_clz_word(bn_word_t val) {
    if (val == 0) return 32;
    uint32_t cnt = 0;
    while (!(val & 0x80000000)) {
        val <<= 1;
        cnt++;
    }
    return cnt;
}

uint32_t bn_clz(const bn_value_t* val) {
    uint32_t cnt = 0;
    int32_t i;
    for (i=BN_WORDS-1; i>=0 && val->words[i]==0; i--) {
        cnt += BN_WORD_BITS;
    }
    if (i >= 0) {
        cnt += _bn_clz_word(val->words[i]);
    }
    return cnt;
}

int32_t bn_cmp(const bn_value_t* a, const bn_value_t* b) {
    for (int32_t i=BN_WORDS-1; i>=0; i--) {
        if (a->words[i] < b->words[i]) { return -1; }
        if (a->words[i] > b->words[i]) { return 1; }
    }
    return 0;
}

void bn_shl(bn_value_t* val, uint32_t bits) {
    const uint32_t nw = bits / BN_WORD_BITS;
    const uint32_t nb = bits % BN_WORD_BITS;

    if (nw) {
        for (int32_t i=BN_WORDS-1; i>=nw; i--) {
            val->words[i] = val->words[i-nw];
        }
        for (uint32_t i=0; i<nw; i++) {
            val->words[i] = 0;
        }
    }

    if (nb) {
        for (uint32_t i=BN_WORDS-1; i>0; i--) {
            val->words[i] = (val->words[i] << nb) | (val->words[i-1] >> (BN_WORD_BITS - nb));
        }
        val->words[0] <<= nb;
    }
}

void bn_shr(bn_value_t* val, uint32_t bits) {
    const uint32_t nw = bits / BN_WORD_BITS;
    const uint32_t nb = bits % BN_WORD_BITS;

    if (nw) {
        for (uint32_t i=0; i<BN_WORDS-nw; i++) {
            val->words[i] = val->words[i+nw];
        }
        for (uint32_t i=BN_WORDS-nw; i<BN_WORDS; i++) {
            val->words[i] = 0;
        }
    }

    if (nb) {
        for (uint32_t i=0; i<BN_WORDS-1; i++) {
            val->words[i] = (val->words[i] >> nb) | (val->words[i+1] << (BN_WORD_BITS - nb));
        }
        val->words[BN_WORDS-1] >>= nb;
    }
}

void bn_and(bn_value_t* res, const bn_value_t* a, const bn_value_t* b) {
    for (uint32_t i=0; i<BN_WORDS; i++)  {
        res->words[i] = a->words[i] & b->words[i];
    }
}

void bn_or(bn_value_t* res, const bn_value_t* a, const bn_value_t* b) {
    for (uint32_t i=0; i<BN_WORDS; i++) {
        res->words[i] = a->words[i] | b->words[i];
    }
}

void bn_xor(bn_value_t* res, const bn_value_t* a, const bn_value_t* b) {
    for (uint32_t i=0; i<BN_WORDS; i++) {
        res->words[i] = a->words[i] ^ b->words[i];
    }
}

bn_word_t bn_add(bn_value_t* res, const bn_value_t* a, const bn_value_t* b) {
    bn_dword_t carry = 0;
    for (uint32_t i=0; i<BN_WORDS; i++) {
        carry += (bn_dword_t)a->words[i] + b->words[i];
        res->words[i] = (bn_word_t)carry;
        carry >>= BN_WORD_BITS;
    }
    return (bn_word_t)carry;
}

bn_word_t bn_sub(bn_value_t* res, const bn_value_t* a, const bn_value_t* b) {
    bn_word_t borrow = 0;
    for (uint32_t i=0; i<BN_WORDS; i++) {
        bn_dword_t tmp = (bn_dword_t)a->words[i] - b->words[i] - borrow;
        res->words[i] = (bn_word_t)tmp;
        borrow = (tmp >> (BN_WORD_BITS - 1)) & 1;
    }
    return borrow;
}

void bn_mul(bn_dvalue_t* res, const bn_value_t* a, const bn_value_t* b) {
    for (uint32_t i=0; i<BN_WORDS*2; i++) {
        res->words[i] = 0;
    }

    for (uint32_t i=0; i<BN_WORDS; i++) {
        bn_dword_t acc = 0;
        for (uint32_t j=0; j<BN_WORDS; j++) {
            acc += (bn_dword_t)a->words[i] * b->words[j] + res->words[i+j];
            res->words[i+j] = (bn_word_t)acc;
            acc >>= BN_WORD_BITS;
        }
        res->words[i+BN_WORDS] = (bn_word_t)acc;
    }
}

int bn_div(bn_value_t* res, bn_value_t* rem, const bn_dvalue_t* a, const bn_value_t* b) {
    // TODO
    return 0;
}

#endif

