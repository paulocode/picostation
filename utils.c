#include <math.h>
#include "pico/stdlib.h"

#include "utils.h"

extern inline int tobcd(int in) {
    if (in > 99) {
        return 0x99;
    } else {
        return (in / 10) << 4 | (in % 10);
    }
}

extern inline uint32_t reverseBits(uint32_t num, int bits) {
    uint32_t NO_OF_BITS = bits;
    uint32_t reverse_num = 0;
    int i;
    for (i = 0; i < NO_OF_BITS; i++) {
        if ((num & (1 << i)))
            reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
    }
    return reverse_num;
}

extern inline int track_to_sector(int track) {
    return pow(track, 2) * 0.00031499 + track * 9.357516535;
}

extern inline int sectors_per_track(int track) {
    return round(track * 0.000616397 + 9);
}