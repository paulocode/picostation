#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "main.pio.h"

#include "f_util.h"
#include "ff.h"
#include "rtc.h"
#include "hw_config.h"
#include "utils.h"
#include "values.h"
#include "subq.h"
#include "cmd.h"
#include "i2s.h"

extern volatile int sector;
extern volatile uint latched;
extern volatile int num_logical_tracks;
extern volatile int current_logical_track;
extern volatile int sector_sending;
extern volatile bool SENS_data[16];
extern volatile bool soct;
extern volatile bool hasData;
extern int *logical_track_to_sector;
extern bool *is_data_track;
extern mutex_t mechacon_mutex;


char SCExData[][44] = {
    {1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,1,0,1,1,0,1,0,0},
    {1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,1,1,1,1,0,1,0,0},
    {1,0,0,1,1,0,1,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,1,0,1,1,0,1,0,0},
};

void i2s_data_thread() {
    // TODO: separate PSNEE, cue parse, and i2s functions
    int sector_t = -1;
    int bytesRead;
    uint32_t *pio_samples[2];
    uint64_t psneeTimer = time_us_64();
    uint64_t sector_change_timer = 0;
    int buffer_for_dma = 1;
    int buffer_for_sd_read = 0;
    int psnee_hysteresis = 0;
    int cachedSectors[SECTOR_CACHE] = { -1 };
    int sector_loaded[2] = { -1 };
    int roundRobinCacheIndex = 0;
    FRESULT fr;
    FIL fil;
    sd_card_t *pSD;
    int bytes;
    char buf[128];
    ushort *cd_samples[SECTOR_CACHE];
    uint16_t CD_scrambling_key[1176] = { 0 };
    int key = 1;
    int logical_track = 0;

    pio_samples[0] = malloc(CD_SAMPLES_BYTES*2);
    pio_samples[1] = malloc(CD_SAMPLES_BYTES*2);
    memset(pio_samples[0], 0, CD_SAMPLES_BYTES*2);
    memset(pio_samples[1], 0, CD_SAMPLES_BYTES*2);

    for (int i=0; i<SECTOR_CACHE; i++) {
        cd_samples[i] = malloc(CD_SAMPLES_BYTES);
        if (cd_samples[i] == NULL) {
            while(true) {
                printf("not enough memory for cache!\n");
            }
        }
    }

    for (int i=6; i<1176; i++) {
        char upper = key & 0xFF;
        for(int j=0; j<8; j++) {
            int bit = ((key & 1)^((key & 2)>>1)) << 15;
            key = (bit | key) >> 1;
        }

        char lower = key & 0xFF;

        CD_scrambling_key[i] = (lower << 8) | upper;

        for(int j=0; j<8; j++) {
            int bit = ((key & 1)^((key & 2)>>1)) << 15;
            key = (bit | key) >> 1;
        }
    }

    pSD = sd_get_by_num(0);
    fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    fr = f_open(&fil, "UNIROM.cue", FA_READ);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: (%d)\n", FRESULT_str(fr), fr);

    f_gets(buf, 128, &fil);

    while (1) {
        f_gets(buf, 128, &fil);
        char * token = strtok(buf, " ");
        if (strcmp("TRACK", token) == 0) {
            num_logical_tracks++;
        }
        if (f_eof(&fil)) {
            break;
        }
    }

    f_rewind(&fil);
    printf("num_logical_tracks: %d\n",num_logical_tracks);

    logical_track_to_sector = malloc(sizeof(int)*(num_logical_tracks+2));
    is_data_track = malloc(sizeof(bool)*(num_logical_tracks+2));
    logical_track_to_sector[0] = 0;
    logical_track_to_sector[1] = 4500;

    f_gets(buf, 128, &fil);
    while (1) {
        f_gets(buf, 128, &fil);

        if (f_eof(&fil)) {
            break;
        }
        char * token;
        token = strtok(buf, " ");
        if (strcmp("TRACK", token) == 0) {
            token = strtok(NULL, " ");
            logical_track = atoi(token);
        }
        token = strtok(NULL, " ");
        token[strcspn(token, "\r\n")] = 0;
        is_data_track[logical_track] = strcmp("AUDIO", token);
        if (is_data_track[logical_track]) {
            hasData = 1;
        }
        f_gets(buf, 128, &fil);
        token = strtok(buf, " ");
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");

        int mm = atoi(strtok(token, ":"));
        int ss = atoi(strtok(NULL, ":"));
        int ff = atoi(strtok(NULL, ":"));
        if (logical_track != 1) {
            logical_track_to_sector[logical_track] = mm*60*75 + ss*75 + ff + 4650;
        }
        printf("cue: %d %d %d %d\n", logical_track, mm, ss, ff);
    }

    f_close(&fil);
    fr = f_open(&fil, "UNIROM.bin", FA_READ);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: (%d)\n", FRESULT_str(fr), fr);

    logical_track_to_sector[num_logical_tracks+1] = f_size(&fil)/2352 + 4650;
    is_data_track[num_logical_tracks+1] = 0;
    is_data_track[0] = 0;

    for(int i=0; i<num_logical_tracks+2; i++) {
        printf("sector_t: %d data: %d\n",logical_track_to_sector[i], is_data_track[i]);
    }


    int channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(channel);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);
    dma_channel_configure(channel, &c, &pio0->txf[I2S_DATA_SM], pio_samples[0], CD_SAMPLES*2, false);



    while (true) {
        if (mutex_try_enter(&mechacon_mutex,0)) {
            while(!pio_sm_is_rx_fifo_empty(pio1, MECHACON_SM)) {
                uint c = reverseBits(pio_sm_get_blocking(pio1, MECHACON_SM),8);
                latched >>= 8;
                latched |= c << 16;
            }
            gpio_put(SENS, SENS_data[latched >> 20]);
            mutex_exit(&mechacon_mutex);
        }

        if (sector_t > 0 && sector_t < PSNEE_SECTOR_LIMIT &&
                SENS_data[SENS_GFS] && !soct && hasData &&
                ((time_us_64() - psneeTimer) > 13333)) {
            psnee_hysteresis++;
            psneeTimer = time_us_64();
        }

        if (psnee_hysteresis > 100) {
            psnee_hysteresis = 0;
            printf("+SCEX\n");
            gpio_put(SCEX_DATA, 0);
            psneeTimer = time_us_64();
            while ((time_us_64() - psneeTimer) < 90000) {
                if (sector >= PSNEE_SECTOR_LIMIT || soct) {
                    goto abort_psnee;
                }
            }
            for (int i=0; i<6; i++) {
                for (int j=0; j<44; j++) {
                    gpio_put(SCEX_DATA, SCExData[i%3][j]);
                    psneeTimer = time_us_64();
                    while ((time_us_64() - psneeTimer) < 4000) {
                        if (sector >= PSNEE_SECTOR_LIMIT || soct) {
                            goto abort_psnee;
                        }
                    }
                }
                gpio_put(SCEX_DATA, 0);
                psneeTimer = time_us_64();
                while ((time_us_64() - psneeTimer) < 90000) {
                    if (sector >= PSNEE_SECTOR_LIMIT || soct) {
                        goto abort_psnee;
                    }
                }
            }

            goto normal_end;
abort_psnee:
normal_end:
            gpio_put(SCEX_DATA, 1);
            psneeTimer = time_us_64();
            printf("-SCEX\n");
        }

        if (buffer_for_dma != buffer_for_sd_read) {
            sector_change_timer = time_us_64();
            while((time_us_64() - sector_change_timer) < 100) {
                if (sector_t != sector) {
                    sector_t = sector;
                    sector_change_timer = time_us_64();
                }
            }
            int cacheHit = -1;
            int sector_to_search = sector_t < 4650 ? (sector_t % SECTOR_CACHE) + 4650 : sector_t;
            for (int i=0; i<SECTOR_CACHE; i++) {
                if (cachedSectors[i] == sector_to_search) {
                    cacheHit = i;
                    break;
                }
            }

            if (cacheHit == -1) {
                uint64_t seek_bytes = (sector_to_search-4650)*2352LL;
                if (seek_bytes >= 0) {
                    fr = f_lseek(&fil, seek_bytes);
                    if (FR_OK != fr) {
                        f_rewind(&fil);
                    }
                }

                fr = f_read(&fil, cd_samples[roundRobinCacheIndex], CD_SAMPLES_BYTES, &bytesRead);
                if (FR_OK != fr)
                    panic("f_read(%s) error: (%d)\n", FRESULT_str(fr), fr);

                cachedSectors[roundRobinCacheIndex] = sector_to_search;
                cacheHit = roundRobinCacheIndex;
                roundRobinCacheIndex = (roundRobinCacheIndex + 1) % SECTOR_CACHE;
            }

            if (sector_t >= 4650) {
                for (int i=0; i<CD_SAMPLES*2; i++) {
                    uint32_t i2s_data;
                    if (is_data_track[current_logical_track]) {
                        i2s_data = reverseBits(cd_samples[cacheHit][i]^CD_scrambling_key[i], 16) << 8;
                    } else {
                        i2s_data = reverseBits(cd_samples[cacheHit][i], 16) << 8;
                    }

                    if (i2s_data & 0x100) {
                        i2s_data |= 0xFF;
                    }

                    pio_samples[buffer_for_sd_read][i] = i2s_data;
                }
            } else {
                memset(pio_samples[buffer_for_sd_read],0,CD_SAMPLES_BYTES*2);
            }

            sector_loaded[buffer_for_sd_read] = sector_t;
            buffer_for_sd_read = (buffer_for_sd_read + 1) % 2;
        }

        if (!dma_channel_is_busy(channel)) {
            buffer_for_dma = (buffer_for_dma + 1) % 2;
            sector_sending = sector_loaded[buffer_for_dma];

            dma_hw->ch[channel].read_addr = pio_samples[buffer_for_dma];
            dma_channel_start(channel);
        }
    }
}
