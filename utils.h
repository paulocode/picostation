#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <math.h>
#include "stdlib.h"

inline int tobcd(int in) {
    if (in > 99) {
        return 0x99;
    } else {
        return (in / 10) << 4 | (in % 10);
    }
}

inline uint32_t reverseBits(uint32_t num, int bits) {
    uint32_t NO_OF_BITS = bits;
    uint32_t reverse_num = 0;
    int i;
    for (i = 0; i < NO_OF_BITS; i++) {
        if ((num & (1 << i)))
            reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
    }
    return reverse_num;
}

inline int track_to_sector(int track) {
    return pow(track, 2) * 0.00031499 + track * 9.357516535;
}

inline int sectors_per_track(int track) {
    return round(track * 0.000616397 + 9);
}
#endif