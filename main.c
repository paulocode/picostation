#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "main.pio.h"
#include "pico/multicore.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"

#include "utils.h"
#include "values.h"
#include "subq.h"
#include "cmd.h"
#include "i2s.h"

// globals
mutex_t mechacon_mutex;
volatile uint latched = 0;
volatile uint mechachon_sm_offset;
volatile uint soct_offset;
volatile uint subq_offset;
volatile bool soct = 0;
volatile bool hasData = 0;
volatile uint sled_move_direction = SLED_MOVE_STOP;
volatile uint count_track = 0;
volatile uint track = 0;
volatile uint original_track = 0;
volatile uint sector = 0;
volatile uint sector_sending = -1;
volatile uint sector_for_track_update = 0;
volatile uint64_t subq_start_time = 0;
volatile uint64_t sled_timer = 0;
volatile int num_logical_tracks = 0;
int *logical_track_to_sector;
bool *is_data_track;
volatile int current_logical_track = 0;
volatile int mode = 1;

bool SENS_data[16] = {
    0,0,0,0,0,
    1, // FOK
    0,0,0,0,
    0, // GFS
    0, // COMP
    0, // COUT
    0,0,0
};


void initialize() {
    set_sys_clock_pll(948000000, 7, 1); // 135428571 Hz, 67714286 Hz PS1 clock
    srand(time(NULL));
    mutex_init(&mechacon_mutex);

    gpio_init(SCEX_DATA);
    gpio_init(SENS);
    gpio_init(LMTSW);
    gpio_init(XLAT);
    gpio_init(DOOR);
    gpio_init(RESET);
    gpio_init(SQCK);
    gpio_init(SQSO);
    gpio_init(CMD_CK);
    gpio_init(LRCK);

    gpio_set_dir(SCEX_DATA, GPIO_OUT);
    gpio_put(SCEX_DATA, 0);
    gpio_set_dir(SENS, GPIO_OUT);
    gpio_set_dir(LMTSW, GPIO_OUT);
    gpio_set_dir(XLAT, GPIO_IN);
    gpio_set_dir(DOOR, GPIO_IN);
    gpio_set_dir(RESET, GPIO_IN);
    gpio_set_dir(SQCK, GPIO_IN);
    gpio_set_dir(SQSO, GPIO_OUT);
    gpio_set_dir(CMD_CK, GPIO_IN);
    gpio_put(SQSO, 0);

    gpio_set_input_hysteresis_enabled(RESET,true);
    gpio_set_input_hysteresis_enabled(SQCK,true);
    gpio_set_input_hysteresis_enabled(XLAT,true);
    gpio_set_input_hysteresis_enabled(CMD_CK,true);
    gpio_set_drive_strength(LRCK, GPIO_DRIVE_STRENGTH_12MA);
    uint i2s_pio_offset = pio_add_program(pio0, &i2s_data_program);
    i2s_data_program_init(pio0, I2S_DATA_SM, i2s_pio_offset, DA15);

    uint offset2 = pio_add_program(pio0, &i2s_lrck_program);
    i2s_lrck_program_init(pio0, LRCK_DATA_SM, offset2, DA15+2);

    mechachon_sm_offset = pio_add_program(pio1, &mechacon_program);
    mechacon_program_init(pio1, MECHACON_SM, mechachon_sm_offset, CMD_DATA);

    uint offset3 = pio_add_program(pio0, &cpu_clk_program);
    cpu_clk_program_init(pio0, CPU_CLK_SM, offset3, CLK);

    uint offset5 = pio_add_program(pio1, &scor_program);
    scor_program_init(pio1, SCOR_SM, offset5, SCOR);

    soct_offset = pio_add_program(pio1, &soct_program);

    subq_offset = pio_add_program(pio1, &subq_program);

    uint64_t startTime = time_us_64();

    pio_enable_sm_mask_in_sync(pio0, (1u << CPU_CLK_SM) | (1u << I2S_DATA_SM) | (1u << LRCK_DATA_SM));

    gpio_set_dir(RESET, GPIO_OUT);
    gpio_put(RESET,0);
    sleep_ms(300);
    gpio_set_dir(RESET, GPIO_IN);

    while((time_us_64() - startTime) < 30000) {
        if (gpio_get(RESET) == 0) {
            startTime = time_us_64();
        }
    }

    while((time_us_64() - startTime) < 30000) {
        if (gpio_get(CMD_CK) == 0) {
            startTime = time_us_64();
        }
    }

    printf("ON!\n");
    multicore_launch_core1(i2s_data_thread);
    gpio_set_irq_enabled_with_callback(XLAT, GPIO_IRQ_EDGE_RISE, true, &interrupt_xlat);
    pio_enable_sm_mask_in_sync(pio1, (1u << SCOR_SM) | (1u << MECHACON_SM));
}

int main() {
    stdio_init_all();

    initialize();
    int prevMode = 1;
    int sectors_per_track_i = sectors_per_track(0);
    bool subq_delay = 0;
    uint64_t subq_delay_time = 0;

    while (true) {
        gpio_put(LMTSW, sector > 3000);
        if (mutex_try_enter(&mechacon_mutex,0)) {
            while(!pio_sm_is_rx_fifo_empty(pio1, MECHACON_SM)) {
                uint c = reverseBits(pio_sm_get_blocking(pio1, MECHACON_SM),8);
                latched >>= 8;
                latched |= c << 16;
            }
            gpio_put(SENS, SENS_data[latched >> 20]);
            mutex_exit(&mechacon_mutex);
        }

        if (prevMode == 1 && mode == 2) {
            pio_sm_set_clkdiv(pio0, I2S_DATA_SM, 1);
            pio_sm_set_clkdiv(pio1, SCOR_SM, 1);
            prevMode = 2;
            printf("x2\n");
        } else if (prevMode == 2 && mode == 1) {
            pio_sm_set_clkdiv(pio0, I2S_DATA_SM, 2);
            pio_sm_set_clkdiv(pio1, SCOR_SM, 2);
            prevMode = 1;
            printf("x1\n");
        }

        if (track < 0 || sector < 0) {
            track = 0;
            sector = 0;
            sector_for_track_update = 0;
        }

        if (track > 24000 || sector > 440000) {
            track = 24000;
            sector = track_to_sector(track);
            sector_for_track_update = sector;
        }

        if (gpio_get(RESET) == 0) {
            printf("RESET!\n");
            pio_sm_set_enabled(pio1, SUBQ_SM, false);
            pio_sm_set_enabled(pio1, SOCT_SM, false);
            mechacon_program_init(pio1, MECHACON_SM, mechachon_sm_offset, CMD_DATA);
            subq_delay = 0;
            soct = 0;

            gpio_init(SQSO);
            gpio_set_dir(SQSO, GPIO_OUT);
            gpio_put(SQSO, 0);

            uint64_t startTime = time_us_64();

            while ((time_us_64() - startTime) < 30000) {
                if (gpio_get(RESET) == 0) {
                    startTime = time_us_64();
                }
            }

            while ((time_us_64() - startTime) < 30000) {
                if (gpio_get(CMD_CK) == 0) {
                    startTime = time_us_64();
                }
            }

            pio_sm_set_enabled(pio1, MECHACON_SM, true);
        }


        if (soct) {
            uint interrupts = save_and_disable_interrupts();
            // waiting for RX FIFO entry does not work.
            sleep_us(300);
            soct = 0;
            pio_sm_set_enabled(pio1, SOCT_SM, false);
            subq_start_time = time_us_64();
            restore_interrupts(interrupts);
        } else if (sled_move_direction == SLED_MOVE_FORWARD) {
            if ((time_us_64() - sled_timer) > TRACK_MOVE_TIME_US) {
                sled_timer = time_us_64();
                track++;
                sector = track_to_sector(track);
                sector_for_track_update = sector;

                if ((track - original_track) >= count_track) {
                    original_track = track;
                    SENS_data[SENS_COUT] = !SENS_data[SENS_COUT];
                }
            }
        } else if (sled_move_direction == SLED_MOVE_REVERSE) {
            if ((time_us_64() - sled_timer) > TRACK_MOVE_TIME_US) {
                sled_timer = time_us_64();
                track--;
                sector = track_to_sector(track);
                sector_for_track_update = sector;
                if ((original_track - track) >= count_track) {
                    original_track = track;
                    SENS_data[SENS_COUT] = !SENS_data[SENS_COUT];
                }
            }
        } else if (SENS_data[SENS_GFS]) {
            if (sector < 4650 && (time_us_64() - subq_start_time) > 13333) {
                subq_start_time = time_us_64();
                start_subq();
                sector++;
                if ((sector - sector_for_track_update) >= sectors_per_track_i) {
                    sector_for_track_update = sector;
                    track++;
                    sectors_per_track_i = sectors_per_track(track);
                }
            } else {
                if (sector_sending == sector) {
                    if (!subq_delay) {
                        sector++;
                        if ((sector - sector_for_track_update) >= sectors_per_track_i) {
                            sector_for_track_update = sector;
                            track++;
                            sectors_per_track_i = sectors_per_track(track);
                        }
                        subq_delay = 1;
                        subq_delay_time = time_us_64();
                    }
                }

                if (subq_delay && (sector >= 4650 && (time_us_64() - subq_delay_time) > 3333)) {
                    subq_delay = 0;
                    start_subq();
                }
            }
        } else {
            subq_delay = 0;
        }
    }

}
