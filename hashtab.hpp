/*
 * A simple hash-table.
 * Just include this header file, there is only one file.
 *
 * Copyright (C) 2021 Simpidbit Isaiah.
 *     Author: Simpidbit Isaiah <8155530@gmail.com>
 *
 * Use secondary probe and rehash algorithm mainly, but the times 
 * of rehashing must be less then 5, or will use chaining instead.
 */

#ifndef _SP_HASHTAB_H
#define _SP_HASHTAB_H

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <set>
#include <string>

// 1024 * 1024 buckets
#define _SP_HTAB_BKT_DEFAULT_LEN (1024 * 1024)
#define _SP_HTAB_MAX_LOAD_FACTOR (0.75)

typedef unsigned long _SP_ulong;

template <typename VTYPE = void *>
class _SP_Bucket
{
public:
    char *key;
    VTYPE value;
    _SP_Bucket<VTYPE> *next;
};

typedef struct _SP_Index {
    _SP_ulong bkt_index;
    _SP_ulong chain_index;
} _SP_Index;


namespace SP {

template <typename VTYPE = void *>
class Hashtab
{
public:
    std::set<std::string> keyset;

    Hashtab()
    {
        this->bkt_size = _SP_HTAB_BKT_DEFAULT_LEN;
        this->buckets = new _SP_Bucket<VTYPE>[this->bkt_size];

        this->max_load_factor = _SP_HTAB_MAX_LOAD_FACTOR;
    }

    Hashtab(_SP_ulong length, double max_load_fac = _SP_HTAB_MAX_LOAD_FACTOR)
            :bkt_size(length), max_load_factor(max_load_fac)
    {
        this->buckets = new _SP_Bucket<VTYPE>[this->bkt_size];
    }

    ~Hashtab()
    {
        delete[] this->buckets;
    }

     // Compute average number of characters, compute the offsets of each
     // characters by average number. If offset is positive, then set the
     // character into 0b1. If offset is negative or 0, then set the character
     // into 0b0. At last there will be a binary number list, fill the number
     // list with 0b0 until the length of list is multiple of 8, transform
     // the binary list into an unsigned int, times (average number / 10),
     // mod by length of buckets, so it is the index.
     //
     // If there is a bucket clashing with the index, use secondary probe and
     // rehash algorithm (less than 4 times). Afert 4 times of rehashing, if
     // there is still a bucket clashing, so use the chaining algorithm.
     //
     // Maybe it doesn't work well, because this is the first time I design
     // the algorithm.
    _SP_Index hash(char *key)
    {
        // Compute average number
        _SP_ulong sum = 0;
        _SP_ulong keylen = 0;
        _SP_ulong averg = 0;
        for (int i = 0; key[i] != '\0'; i++) {
            sum += key[i];
            keylen++;
        }
        averg = sum / keylen;

        // Compute offsets
        unsigned char *binlist = new unsigned char[keylen];
        for (int i = 0; i < keylen; i++) {
            int offset = key[i] - averg;
            if (offset >= 0) {
                binlist[i] = 1;
            } else {
                binlist[i] = 0;
            }
        }

        // Fill list.
        _SP_ulong newlen = 0;
        if (char mod = (keylen % 8))
            newlen = keylen + mod;
        else
            newlen = keylen;
        unsigned char *newlist = new unsigned char[newlen];
        memcpy(newlist, binlist, sizeof(unsigned char) * keylen);

        delete[] binlist;

        _SP_ulong inum = (this->bin_to__SP_ulong(newlist, newlen) * averg + sum);

        return this->check_and_rehash(inum % this->bkt_size, key);
    }

       
    void insert(char *key, VTYPE value)
    {
        this->keyset.insert(std::string(key));

        _SP_Index ind = this->hash(key);
        if (ind.chain_index == 0) {
            if (this->buckets[ind.bkt_index].key == 0)
                this->ele_num++;            // Add to the counter

            this->buckets[ind.bkt_index].key = key;
            this->buckets[ind.bkt_index].value = value;
            goto out;
        } else {
            _SP_Bucket<VTYPE> *cur = &this->buckets[ind.bkt_index];
            assert(cur->next != 0);
            for (;;) {
                cur = cur->next;
                if (cur->next == 0) {
                    if (cur->key == 0)
                        this->ele_num++;    // Add to the counter

                    cur->key = key;
                    cur->value = value;
                    goto out;
                }
            }
        }
    out:;
        return;
    }

    VTYPE get(char *key)
    {
        VTYPE ret;
        _SP_Index ind = this->hash(key);
        if (ind.chain_index == 0) {
            ret = this->buckets[ind.bkt_index].value;
            return ret;
        } else {
            _SP_Bucket<VTYPE> *cur = &this->buckets[ind.bkt_index];
            assert(cur->next != 0);
            for (;;) {
                cur = cur->next;
                if (cur->next == 0) {
                    ret = cur->value;
                    return ret;
                }
            }
        }
    }

    Hashtab &operator[](char *key)
    {
        this->temp_key = key;
        return (*this);
    }

    VTYPE operator()(char *key)
    {
        return this->get(key);
    }

    void operator=(VTYPE value)
    {
        this->insert(this->temp_key, value);
        return;
    }

private:
    // little-endian
    static _SP_ulong bin_to__SP_ulong(unsigned char *binlist, _SP_ulong length)
    {
        _SP_ulong num = 0;
        for (_SP_ulong i = 0; i < length; i++) {
            if (binlist[i] == 1) {
                num += pow((double)2.0, (double)i);
            }
        }
        return num;
    }

     // Secondary probe and rehash.
     // After 5 times rehash, use chaining algorithm.
    _SP_Index check_and_rehash(_SP_ulong tent, char *key)
    {
        _SP_Index ret;
        ret.bkt_index = tent;
        ret.chain_index = 0;

        if (this->buckets[ret.bkt_index].key == 0
            || !strcmp(this->buckets[ret.bkt_index].key, key)) {
            ;
        } else {
            for (int i = 1; i <= 5; i++) {
            int flag = 1;
                for (int j = 0; j < 2; j++) {
                    ret.bkt_index = ret.bkt_index + pow((double)i, 2.0) * flag;
                    flag = -flag;

                    if (ret.bkt_index < 0 
                        || ret.bkt_index >= this->bkt_size)
                        continue;
                    if (this->buckets[ret.bkt_index].key == 0
                        || !strcmp(this->buckets[ret.bkt_index].key, key))
                        goto out;
                }
            }
            // Chaining
            for (int i = 1; ; i++) {
                if (this->buckets[ret.bkt_index].next == 0) {
                    this->buckets[ret.bkt_index].next = new _SP_Bucket<VTYPE>;
                    ret.chain_index = i;
                    goto out;
                }
            }
        }
    out:;
        return ret;
    }

    _SP_Bucket<VTYPE> *buckets;
    _SP_ulong bkt_size;
    _SP_ulong ele_num;
    double max_load_factor;

    char *temp_key;             // For index insert (operator)
};

};      // namespace
#endif
