#ifndef SPRITE_STRIKE_H
#define SPRITE_STRIKE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>

#include <MD_MAX72xx.h>
#include <Talkie.h>
#include <Vocab_US_Large.h>
#include <Vocab_US_TI99.h>
#include <Vocab_US_Acorn.h>
#include <Vocab_Soundbites.h>

#define GAME_HEIGHT     8
#define GAME_WIDTH      24

#define SPRITE_HEIGHT   4
#define SPRITE_WIDTH    4

#define MAX_NUM_ENEMIES 4
#define NUM_ETYPES      6

#define NUM_LEVELS		  3

#define E_PROJ_INTV     300.f    // (ms)
#define P_PROJ_INTV     200.f    // (ms)

#define P_ATK_CD        175.f   // (ms)

#define UD              0
#define LR              1

#define PATH0LR         "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
#define PATH0UD         "UUUUDDDDUUDDUUDDUUDDUUDDUUDDUUD"

#define PATH1LR         "LLLLLLRRRRRRLLLLLLRRRRRRLLLLLL"
#define PATH1UD         "DDDUUUUUUDDDDDDUUUUUUDDDDDDUUU"

#define PATH2LR         "LRLLLLLLLLLRRRLLLLLLRRRLLLLLLR"
#define PATH2UD         "UDUDUDUDUDUDUDUDUDUDUDUDUDUDUD"

#define PATH3LR         "LLLLLLLLLLLLLLRRRRLLLLLLLLLRRR"
#define PATH3UD         "UUUUDDDDDDUUUUDDDDDDUUUUDDDDUU"

#define U32LROT(X)      (0x80000000 & X ? ((X << 1) | 1) : (X << 1))
#define U32RROT(X)      (0x00000001 & X ? ((X >> 1) | 0x80000000) : (X >> 1))

// game states
#define GAME_OVER       0x01
#define PLAYING         0x02
#define INIT            0x04

// DISPLAY
#define HWTYPE      MD_MAX72XX::GENERIC_HW
#define MAX_DISPS   3
#define CLK_PIN     2
#define CS_PIN      3
#define DATA_PIN    4

// INPUT
#define U_BTN       0x40  // digital pin 12 (Up)
#define D_BTN       0x02  // digital pin 52 (Down)
#define R_BTN       0x04  // digital pin 51 (Right)
#define L_BTN       0x08  // digital pin 50 (Left)
#define A_BTN       0x01  // digital pin 53 (A)
#define B_BTN       0x10  // digital pin 10 (B)

typedef struct level_t {
  uint8_t   num_waves;
  uint8_t   eq_firing;
  uint8_t		enemy_ct;

  uint32_t	ud_masks[MAX_NUM_ENEMIES];
  uint32_t	lr_masks[MAX_NUM_ENEMIES];

  float 		atk_intervals[MAX_NUM_ENEMIES];

  float		  ud_intervals[MAX_NUM_ENEMIES];
  float		  lr_intervals[MAX_NUM_ENEMIES];

  char		  ud_pathing[MAX_NUM_ENEMIES][32];
  char		  lr_pathing[MAX_NUM_ENEMIES][32];
} level_t;

typedef struct game_timer_t {
  float interval;
  float i_time;
  float c_time;
} game_timer_t;

typedef struct player_t {
  uint16_t    sprite;
  uint32_t    emitter;
  uint8_t     c_pos;
  uint8_t     r_pos;
  uint8_t     s_height;   // local height that reflects you guessed it sprite deformation

  game_timer_t     atk_timer;
} player_t;

typedef struct enemy_t {
  uint16_t    sprite;
  uint16_t    emitter;
  uint8_t     c_pos;
  uint8_t     r_pos;

  uint32_t    mvmt_mask[2];      // get's rotated
  uint32_t    ud_mvmt;           // 1's -> UP : 0's -> DOWN
  uint32_t    lr_mvmt;           // 1's -> LEFT : 0's -> RIGHT

  game_timer_t     atk_timer;
  game_timer_t     super_timer;
  game_timer_t     mvmt_timer[2];
} enemy_t;

struct game_data {
  /* time since last proj. position update */
  game_timer_t     p_proj_timer;
  game_timer_t     e_proj_timer;

  uint8_t     num_enemies;
  uint8_t     input;

  uint32_t    e_proj[GAME_HEIGHT];
  uint32_t    p_proj[GAME_HEIGHT];

  uint8_t     g_state;
  uint8_t     cur_level;
  uint8_t     cur_stage;

  uint32_t    frame[GAME_HEIGHT];
} game_data;

MD_MAX72XX disp = MD_MAX72XX(HWTYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DISPS);

Talkie quip;

// Registers for Digital Pins 50 - 53, 10 - 13 (Controls)
volatile unsigned char *PORT_B  = (unsigned char *) 0x25;
volatile unsigned char *DDR_B   = (unsigned char *) 0x24;
volatile unsigned char *PIN_B   = (unsigned char *) 0x23;

// ENEMY SPRITE PREFABS
static const uint16_t e_sprites[NUM_ETYPES] = { 0x136D, 0x0969, 0x05E5, 0x23C4, 0x25A4, 0xFFFF };
static const uint16_t e_emitters[NUM_ETYPES] = { 0x0240, 0x0808, 0x0080, 0x0400, 0x2480, 0x8080 };

static level_t LEVEL0 = { 1, 0, 1, {1, 1, 0, 0}, {1, 1, 0, 0}, {2250.f, 2250.f, 0, 0}, {2700.f, 1500.f, 0.f, 0.f}, {1000.f, 1000.f, 0.f, 0.f}, \
						              {PATH0UD, PATH0UD, "", ""}, {PATH1LR, PATH1LR, "", ""}};

static level_t LEVEL1 = { 2, 1, 1, {1, 1, 0, 0}, {1, 1, 0, 0}, {1750.f, 0, 0, 0}, {2000.f, 0, 0, 0}, {1000.f, 0, 0, 0},    \
                          {PATH3UD, "", "", ""}, {PATH3LR, "", "", ""}};

static level_t LEVEL2 = { 3, 1, 2, {1, 1, 0, 0}, {1, 1, 0, 0}, {5000.f, 2000.f, 0, 0}, {1200.f, 4000.f, 0, 0}, {1000.f, 1000.f, 0, 0}, \
                          {PATH2UD, PATH1UD, "", ""}, {PATH2LR, PATH2LR}};

level_t *levels[NUM_LEVELS] = { &LEVEL0, &LEVEL1, &LEVEL2 };

static player_t player;
static enemy_t enemies[MAX_NUM_ENEMIES];

// CUSTOM LPC HEX
const unsigned char create[] PROGMEM = {0x00,0xE8,0x9F,0x4A,0x3D,0x53,0x63,0x23,0xA6,0x32,0xAB,0x4E,0x8D,0x8C,0xFC,0xEA,0x23,0xCC,0xC3,0x31,0x79,0x6B,0x8D,0x32,0x77,0x39,0x0A,0xAE,0x23,0x3B,0x22,0x62,0x3B,0xA8,0xF5,0x6E,0x0F,0x8B,0xE3,0xC8,0xD6,0xAA,0x2B,0xD5,0xB6,0xD3,0x52,0xE6,0xF0,0x8A,0x26,0x41,0xCB,0x05,0x9B,0x2F,0xD3,0x16,0xA6,0x45,0x6C,0xFB,0x68,0x9C,0x94,0xEE,0xA1,0xF3,0x23,0x74,0x92,0x9B,0xC2,0xED,0x55,0x29,0x41,0xAE,0x82,0x76,0x4A,0xA4,0x26,0xA6,0x08,0xDE,0x6D,0xB3,0xDC,0x90,0x2A,0x69,0xBA,0xB5,0x69,0x33,0xBB,0xC6,0xAE,0xD2,0x38,0xED,0x18,0x1E,0xBB,0x5A,0xAA,0x34,0x7B,0x38,0xAC,0x1A,0x89,0x53,0xEC,0xAE,0x69,0xAA,0x25,0x4E,0x71,0xBB,0xA2,0x99,0xD1,0x28,0x05,0x6A,0x92,0x76,0xCE,0xEA,0x24,0xB8,0x2B,0x9C,0x5E,0xAB,0x9B,0xE2,0xAE,0xB1,0x6B,0x24,0x4E,0x48,0x86,0xC2,0xE9,0xE5,0x38,0x26,0xA9,0x9C,0xB6,0x9B,0x63,0x8B,0xA4,0x70,0x9A,0x6E,0xB2,0x8D,0xB2,0x2A,0xA8,0x23,0x3D,0x11,0x42,0x93,0xF1,0xF0,0x4C,0xAB,0xCC,0x9A,0xD1,0xC3,0x43,0xE3,0x28,0x60,0xE5,0x0A,0x06,0xEC,0x9C,0x09,0x00,0x00,0x00,0xF0,0x80,0x2E,0xC3};
const unsigned char pacman_death[] PROGMEM = {0x05,0xC8,0x2E,0xAB,0xF4,0xD9,0x47,0xE3,0x5F,0xC9,0xD4,0x67,0xEF,0x03,0x77,0xB3,0x4A,0x9F,0x7D,0x34,0xFE,0x4D,0x2A,0x43,0xF6,0x59,0xF4,0x17,0xA9,0x0C,0xC5,0x55,0xF2,0x9F,0xC5,0x36,0x14,0x57,0x21,0xDF,0xA5,0x4B,0x9F,0x7D,0x34,0xFE,0xCD,0x4E,0x43,0x0E,0x3E,0x70,0xB7,0x38,0x71,0x39,0xD8,0xC0,0xEE,0x9A,0xC4,0xA5,0x68,0x0B,0xBD,0x5F,0x12,0x9B,0x83,0x0D,0xEC,0xAE,0x4E,0x43,0x0E,0x3E,0x70,0xB7,0x28,0xF5,0xD9,0x47,0xE3,0xDF,0xEC,0x32,0x64,0x9F,0x45,0x3F,0x8B,0x12,0x9B,0x83,0x0F,0xDC,0x2E,0x4E,0x5C,0x0A,0x36,0x30,0xFB,0x26,0x81,0x29,0xEA,0x42,0xCD,0x96,0x04,0xA6,0xA8,0x0B,0xD9,0x7B,0x13,0x98,0xA2,0x2E,0xF4,0x6C,0x49,0x5C,0x0E,0x36,0xB0,0xBB,0x3A,0x79,0x39,0xF8,0xC0,0xDD,0xE2,0x82,0xE5,0xE0,0x8D,0xDB,0xAB,0x12,0x98,0xA2,0x2D,0xF4,0x6E,0x49,0x60,0x8A,0xBA,0x50,0xBD,0x37,0x41,0x29,0xC9,0xA2,0xD7,0xBE,0x00,0xC7,0x2C,0x8B,0x9E,0xFB,0x12,0x98,0x92,0x2C,0x64,0xEF,0x4D,0x60,0x8A,0xBA,0x50,0xB3,0x27,0xA9,0x29,0xD8,0xC0,0xEC,0x9B,0x02,0xA6,0x60,0x83,0x53,0x5B,0x12,0x98,0x92,0x2C,0x64,0xEF,0x4D,0x70,0x4C,0xB2,0x68,0xB5,0x2F,0x21,0x31,0xF3,0x2A,0x76,0xBE,0x84,0xC4,0xC2,0xD3,0x50,0xD9,0x13,0x1C,0x33,0x2F,0x6A,0xF5,0x4B,0x50,0x4A,0xB2,0x10,0xB5,0x2F,0x81,0x29,0xEA,0x42,0xCD,0xD6,0x02,0xA5,0xA8,0x8B,0x99,0xDD,0x02,0x1C,0x33,0x2F,0x49,0xF7,0x4B,0x68,0x2C,0x3C,0x09,0x95,0x3D,0x61,0xA1,0x70,0x2F,0x4A,0xF3,0x84,0x85,0xCA,0xBD,0x44,0xCD,0x93,0x19,0x32,0x77,0x83,0xA7,0x73,0x0E,0x79,0xE1,0xA9,0x98,0xD8,0x39,0xE6,0xAB,0x78,0xA6,0x2B,0x47,0x86,0xCF,0xDE,0x55,0x8A,0xDC,0x9C,0xBE,0xD4,0x85,0xDA,0x71,0x73,0xF9,0x32,0x43,0xC2,0xC0,0xCC,0xED,0xDB,0x2A,0xA8,0x59,0xE7,0xF6,0x07,0x0E,0x36,0xB2,0x1C,0xA6,0x15,0x6C,0x86,0xDB,0x36,0x0E,0x79,0x25,0x66,0x94,0xC4,0x39,0xE6,0x8B,0x46,0xA6,0x23,0x47,0xC6,0xCF,0xDE,0x9D,0x8A,0xDC,0x90,0xBF,0xD4,0x85,0xD8,0x71,0x73,0xF9,0xB2,0xCD,0x2C,0xC3,0xCC,0xED,0xDB,0x0A,0x98,0xD9,0x10,0xF4,0x03,0x0E,0x32,0x11,0x4D,0x90,0x34,0x58,0xA6,0xCB,0x42,0x37,0x00,0x2D,0xA1,0xEE,0x18,0x1E,0xD0,0x65};
const unsigned char DARE[] PROGMEM = {0xA0,0x8A,0x9E,0xD3,0xCD,0x33,0x4C,0xE8,0x7A,0x75,0x63,0xC9,0xA9,0xA5,0xEF,0xD5,0x4C,0x34,0xE7,0x96,0xBE,0x4F,0x57,0x31,0xBD,0x9B,0x86,0x3E,0x55,0x25,0xE2,0x4E,0x42,0xDB,0x16,0xD5,0xB4,0x3B,0x09,0x69,0x57,0xC2,0x5C,0xE6,0x06,0xA4,0x5E,0x4E,0x73,0x9D,0x6B,0xD6,0xB2,0xA8,0x3C,0xEC,0x8E,0x80,0xCB,0xC2,0xD6,0x8C,0xDB,0x04,0xAE,0x8F,0xD2,0x53,0x16,0x22,0xB9,0x3E,0x49,0x0F,0x5D,0x89,0xE8,0x3A,0xD4,0x22,0x2D,0x11,0x00,0xC0,0x03,0xBA,0x0C};

void init_game(uint8_t lvl);
void init_io();

void spawn_enemies(uint8_t level);

void create_character();
void game_over_animation();

void update_game();

void update_input();
void update_player();
void update_enemies();
void update_projectiles();
void update_conditions();

void update_frame();
void push_frame();
void tx_frame_data();

void player_attack();
void enemy_attack();

void game_over();

void process_movement();
uint32_t set_movement(const char *mv_str);

uint32_t sprite_proj_collision(uint32_t sprite, uint32_t proj, uint8_t depth, uint8_t col);
uint32_t sprite_collision();

uint32_t get_sprite_slice(uint8_t depth, uint16_t sprite);
uint16_t clear_sprite_slice(uint8_t depth, uint16_t slice_mask, uint16_t sprite);

void p_bits(uint32_t x, int width);
void print_frame();

void game_over_animation(uint8_t type);

#endif // SPRITE_STRIKE_H