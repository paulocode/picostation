#include <math.h>
#include "pico/stdlib.h"

#include "utils.h"


extern inline int tobcd(int in);
extern inline uint32_t reverseBits(uint32_t num, int bits);
extern inline int track_to_sector(int track);
extern inline int sectors_per_track(int track);
