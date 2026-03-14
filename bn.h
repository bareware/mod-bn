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

uint32_t bn_clz(const bn_value_t* val);
int32_t bn_cmp(const bn_value_t* a, const bn_value_t* b);
void bn_shl(bn_value_t* val, uint32_t bits);
void bn_shr(bn_value_t* val, uint32_t bits);
void bn_and(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
void bn_or(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
void bn_xor(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
bn_word_t bn_add(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
bn_word_t bn_sub(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
int32_t bn_mul(bn_value_t* res, const bn_value_t* a, const bn_value_t* b);
void bn_div(bn_value_t* res, bn_value_t* rem, const bn_value_t* a, const bn_value_t* b);

#ifdef __cplusplus
}
#endif

#endif


#ifdef BN_IMPLEMENTATION

#define BN_WORD_BITS        (sizeof(bn_word_t) * 8)

uint32_t bn_clz(const bn_value_t* val) {
    uint32_t cnt = 0;
    int32_t i;
    for (i=BN_WORDS-1; i>=0 && val->words[i]==0; i--) {
        cnt += BN_WORD_BITS;
    }
    if (i >= 0) {
        bn_word_t tmp = val->words[i];
        if (tmp <= 0x0000FFFF) { cnt += 16; tmp <<= 16; }
        if (tmp <= 0x00FFFFFF) { cnt += 8;  tmp <<= 8;  }
        if (tmp <= 0x0FFFFFFF) { cnt += 4;  tmp <<= 4;  }
        if (tmp <= 0x3FFFFFFF) { cnt += 2;  tmp <<= 2;  }
        if (tmp <= 0x7FFFFFFF) { cnt += 1; }
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

int32_t bn_mul(bn_value_t* res, const bn_value_t* a, const bn_value_t* b) {
    for (uint32_t i=0; i<BN_WORDS; i++) {
        res->words[i] = 0;
    }

    for (uint32_t i=0; i<BN_WORDS; i++) {
        uint32_t av = a->words[i];
        if (av == 0) { continue; }

        uint32_t al = av & 0x0000FFFF;
        uint32_t ah = av >> 16;
        uint32_t carry = 0;

        for (uint32_t j=0; j<BN_WORDS-i; j++) {
            uint32_t bv = b->words[j];
            uint32_t bl = bv & 0x0000FFFF;
            uint32_t bh = bv >> 16;

            uint32_t p0 = al * bl;
            uint32_t p1 = al * bh;
            uint32_t p2 = ah * bl;
            uint32_t p3 = ah * bh;

            uint32_t mi = p1 + (p0 >> 16) + (p2 & 0x0000FFFF);
            uint32_t lo =      (mi << 16) | (p0 & 0x0000FFFF);
            uint32_t hi = p3 + (mi >> 16) + (p2 >> 16);

            uint32_t word = res->words[i + j];

            uint32_t s1 = word + lo;
            uint32_t c1 = (s1 < word);

            uint32_t s2 = s1 + carry;
            uint32_t c2 = (s2 < s1);

            res->words[i + j] = s2;

            carry = hi + c1 + c2;
        }
        if (carry) { return 1; }
        for (uint32_t j=BN_WORDS-i; j<BN_WORDS; j++) {
            if (b->words[j] != 0) { return 1; }
        }
    }
    return 0;
}

void bn_div(bn_value_t* res, bn_value_t* rem, const bn_value_t* a, const bn_value_t* b) {
    for (uint32_t i=0; i<BN_WORDS; i++) {
        res->words[i] = 0;
        rem->words[i] = a->words[i];
    }
    if (bn_cmp(a, b) < 0) { return; }

    uint32_t num_lz = bn_clz(a);
    uint32_t den_lz = bn_clz(b);
    uint32_t bits = den_lz - num_lz;

    bn_value_t den = *b;
    if (bits > 0) {
        bn_shl(&den, bits);
    }

    for (uint32_t i=0; i<=bits; i++) {
        bn_shl(res, 1);
        if (bn_cmp(rem, &den) >= 0) {
            bn_sub(rem, rem, &den); // bn_sub hould be safe to use in place
            res->words[0] |= 1;
        }
        bn_shr(&den, 1);
    }
}

#endif
