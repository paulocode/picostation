#include <stdio.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "main.pio.h"

#include "utils.h"
#include "values.h"
#include "cmd.h"

extern volatile uint latched;
extern volatile uint count_track;
extern volatile uint soct_offset;
extern volatile uint subq_offset;
extern volatile bool soct;
extern volatile uint sled_move_direction;
extern volatile uint track;
extern volatile uint original_track;
extern volatile uint sector;
extern volatile uint sector_for_track_update;
extern volatile int subq_start;
extern volatile int mode;
extern bool SENS_data[16];

volatile uint jump_track = 0;

void autosequence() {
    int subcommand = (latched & 0x0F0000) >> 16;

    SENS_data[SENS_AUTOSEQ] = (subcommand != 0);

    switch (subcommand) {
    case 0xC:
        track = track + 2*jump_track;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0xD:
        track = track - 2*jump_track;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0x8:
        track = track + 1;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0x9:
        track = track - 1;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0xA:
        track = track + 10;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0xB:
        track = track - 10;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0xE:
        track = track + jump_track;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0xF:
        track = track - jump_track;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0x4:
        track = track + jump_track;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0x5:
        track = track - jump_track;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    }

}

void sled_move() {
    int subcommand_move = (latched & 0x030000) >> 16;
    int subcommand_track = (latched & 0x0C0000) >> 16;
    switch (subcommand_move) {
    case 2:
        sled_move_direction = SLED_MOVE_FORWARD;
        original_track = track;
        break;
    case 3:
        sled_move_direction = SLED_MOVE_REVERSE;
        original_track = track;
        break;
    default:
        if (sled_move_direction != SLED_MOVE_STOP) {
            sector = track_to_sector(track);
            sector_for_track_update = sector;
        }
        sled_move_direction = SLED_MOVE_STOP;
        break;
    }

    switch (subcommand_track) {
    case 8:
        track++;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    case 0xC:
        track--;
        sector = track_to_sector(track);
        sector_for_track_update = sector;
        break;
    }
}

void spindle() {
    int subcommand = (latched & 0x0F0000) >> 16;

    SENS_data[SENS_GFS] = (subcommand == 6);
}

void interrupt_xlat(uint gpio, uint32_t events) {
    int command = (latched & 0xF00000) >> 20;

    switch (command) {
    case CMD_AUTOSEQ:
        autosequence();
        break;
    case CMD_SOCT:
        soct = 1;
        pio_sm_set_enabled(pio1, SUBQ_SM, false);
        soct_program_init(pio1, SOCT_SM, soct_offset, SQSO, SQCK);
        pio_sm_set_enabled(pio1, SOCT_SM, true);
        pio_sm_put_blocking(pio1, SOCT_SM, 0xFFFFFFF);
        break;
    case CMD_JUMP_TRACK:
        jump_track = (latched & 0xFFFF0) >> 4;
        //printf("%d\n", jump_track);
        break;
    case CMD_COUNT_TRACK:
        count_track = (latched & 0xFFFF0) >> 4;
        //printf("%d\n", count_track);
        break;
    case CMD_SPINDLE:
        spindle();
        break;
    case CMD_SLED:
        sled_move();
        break;
    case CMD_SPEED:
        if ((latched & 0xF40000) == 0x940000 && mode == 1) {
            mode = 2;
        } else if ((latched & 0xF40000) == 0x900000 && mode == 2) {
            mode = 1;
        }
        break;
    }

    /*
    if (command == CMD_SLED || command == CMD_AUTOSEQ ||
    	command == CMD_JUMP_TRACK || command == CMD_SPEED ||
    	command == CMD_COUNT_TRACK || command == CMD_SPINDLE
    ) {
    	printf("%06X\n", latched);
    }
    */

    latched = 0;
}