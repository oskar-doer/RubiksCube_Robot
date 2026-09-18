/*
 * QiYi smart-cube protocol implementation for this project.
 *
 * Based on protocol documentation by Simon Schwartz:
 * https://codeberg.org/Flying-Toast/qiyi_smartcube_protocol
 * Upstream revision: 0c1d02eaac7097d246709fb211da58478b351863
 * The upstream documentation is MIT-licensed; see LICENSE-qiyi.txt.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "cube_model.h"

namespace qiyi {
static const uint8_t key[16] = {0x57,0xb1,0xf9,0xab,0xcd,0x5a,0xe8,0xa7,
                              0x9c,0xb9,0x8c,0xe7,0x57,0x8c,0x51,0x08};
inline uint16_t crc(const uint8_t* p, size_t n) {
    uint16_t c = 0xffff;
    while (n--) {
        c ^= *p++;
        for (int i=0;i<8;i++) c = (c >> 1) ^ ((c & 1) ? 0xa001 : 0);
    }
    return c;
}
inline size_t frame(uint8_t* out, const uint8_t* payload, size_t n) {
    if (n > 240) return 0;
    size_t length = n+4, padded = (length+15)&~size_t(15);
    memset(out,0,padded);
    out[0]=0xfe; out[1]=length;
    memcpy(out+2,payload,n);
    uint16_t c=crc(out,length-2);
    out[length-2]=c; out[length-1]=c>>8;
    return padded;
}
inline bool valid(const uint8_t* p,size_t n) {
    return n>=16 && n<=256 && n%16==0 && p[0]==0xfe && p[1]>=9 &&
           p[1]<=n && crc(p,p[1])==0;
}
inline bool state(const uint8_t* p,size_t n,cube_t& c) {
    // 2=Hello, 3=Bewegung, 4=State-Sync, 5=frischen Zustand anfordern.
    if (!valid(p,n) || p[2]<2 || p[2]>5 || p[1]<38) return false;
    const uint8_t mapping[6]={CF_L,CF_R,CF_D,CF_U,CF_F,CF_B};
    for(int i=0;i<54;i++) {
        uint8_t color=(p[7+i/2]>>((i%2)*4))&15;
        if(color>=6) return false;
        c.f[i]=mapping[color];
    }
    return true;
}
}
