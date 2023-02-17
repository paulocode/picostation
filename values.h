#ifndef VALUES_H_INCLUDED
#define VALUES_H_INCLUDED

// GPIO pinouts
#define XLAT 0
#define CLK 21
#define SCEX_DATA 4
#define DOOR 6
#define SENS 14 
#define DA15 15 // next pin is DA16
#define LRCK 17
#define SCOR 18
#define SQSO 19
#define SQCK 1 
#define LMTSW 2
#define CMD_DATA 26
#define CMD_CK 27
#define RESET 7

// C2PO, WFCK is always GND

// PIO0
#define I2S_DATA_SM 0
#define LRCK_DATA_SM 1
#define CPU_CLK_SM 3

// PIO1
#define SCOR_SM 0
#define MECHACON_SM 1
#define SOCT_SM 2
#define SUBQ_SM 3


// Commands
#define CMD_SLED 0x2
#define CMD_AUTOSEQ 0x4
#define CMD_JUMP_TRACK 0x7
#define CMD_SOCT 0x8
#define CMD_SPEED 0x9
#define CMD_COUNT_TRACK 0xB
#define CMD_SPINDLE 0xE

// SENS 
#define SENS_AUTOSEQ 0x4
#define SENS_FOCUS 0x5
#define SENS_GFS 0xA
#define SENS_COUT 0xC

// SLED
#define SLED_MOVE_STOP 0
#define SLED_MOVE_REVERSE 0x11
#define SLED_MOVE_FORWARD 0x22

// 
#define CD_SAMPLES 588
#define CD_SAMPLES_BYTES CD_SAMPLES*2*2

// 
#define TRACK_MOVE_TIME_US 15

//
#define PSNEE_SECTOR_LIMIT 4500
#define SECTOR_CACHE 50

#endif
