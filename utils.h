#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

extern inline int tobcd(int in);
extern inline uint32_t reverseBits(uint32_t num, int bits);
extern inline int track_to_sector(int track);
extern inline int sectors_per_track(int track);

#endif