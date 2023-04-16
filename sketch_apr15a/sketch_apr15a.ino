#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>

#include <Time.h>
#include <MD_MAX72xx.h>
#include <Talkie.h>

#define GAME_HEIGHT     8
#define GAME_WIDTH      24

#define SPRITE_HEIGHT   4
#define SPRITE_WIDTH    4

#define MAX_NUM_ENEMIES 4
#define NUM_ETYPES      6

#define NUM_LEVELS		1

#define E_PROJ_INTV     300.f    // (ms)
#define P_PROJ_INTV     200.f    // (ms)

#define P_ATK_CD        100.f   // (ms)
#define P_SUPER_CD      30000.f // (ms)

#define UD              0
#define LR              1

#define PATH0LR         "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"
#define PATH0UD         "UUuuUUDDUUDDUUDDUUDDUUDDUUDDUUD"

#define PATH1LR         "LLLLLLRRRRRRLLLLLLRRRRRRLLLLLL"
#define PATH1UD         "DDDUUUUUUDDDDDDUUUUUUDDDDDDUUU"

#define U32LROT(X)      (0x80000000 & X ? ((X << 1) | 1) : (X << 1))
#define U32RROT(X)      (0x00000001 & X ? ((X >> 1) | 0x80000000) : (X >> 1))

// game states
#define GAME_OVER       0x01
#define PLAYING         0x02
#define INIT            0x04

// game flags
#define GAME_WIN        0x01
#define GAME_LOSE       0x02
#define GAME_CONTINUE   0x04


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
// #define S_BTN       (uint8_t)0x20  // digital pin 11 (Start/Select)

typedef struct level_t {
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
    game_timer_t     super_timer;
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
    uint8_t     g_flags;

    uint32_t    frame[GAME_HEIGHT];
} game_data;

MD_MAX72XX disp = MD_MAX72XX(HWTYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DISPS);

// Registers for Digital Pins 50 - 53, 10 - 13 (Controls)
volatile unsigned char *PORT_B  = (unsigned char *) 0x25;
volatile unsigned char *DDR_B   = (unsigned char *) 0x24;
volatile unsigned char *PIN_B   = (unsigned char *) 0x23;

// ENEMY SPRITE PREFABS
static const uint16_t e_sprites[NUM_ETYPES] = { 0x136D, 0x0969, 0x05E5, 0x23C4, 0x25A4, 0xFFFF };
static const uint16_t e_emitters[NUM_ETYPES] = { 0x0240, 0x0808, 0x0080, 0x0400, 0x2480, 0x8888 };

// static level_t LEVEL0 = { 2, {1, 1, 0, 0}, {1, 1, 0, 0}, {1500.f, 2750.f, 0, 0}, {1000.f, 1000.f, 0.f, 0.f}, {1000.f, 1000.f, 0.f, 0.f}, \
// 						{"DDDUUUDDDUUUDDDUUUDDDUUUDDDUUUDU", "UUUDDDUUUDDDUUUDDDUUUDDDUUUDDDUD", "", ""},	 \
// 						{"LLLRRRLLLRRRLLLRRRLLLRRRLLLRRRLR", "RRRLLLRRRLLLRRRLLLRRRLLLRRRLLLRL", "", ""}};

static level_t LEVEL0 = { 2, {1, 1, 0, 0}, {1, 1, 0, 0}, {3000.f, 3000.f, 0, 0}, {2700.f, 1500.f, 0.f, 0.f}, {1000.f, 1000.f, 0.f, 0.f}, \
						{PATH0UD, PATH0UD, "", ""},	 \
						{PATH1LR, PATH1LR, "", ""}};

level_t *levels[NUM_LEVELS] = { &LEVEL0 };

static player_t player;
static enemy_t enemies[MAX_NUM_ENEMIES];

void init_game();
void init_io();

void spawn_enemies(uint8_t level);

void create_character();

void update_game();

void update_input();
void update_player();
void update_enemies();
void update_projectiles();
void update_conditions();

void update_frame();
void push_frame();

void player_attack(uint8_t super);

void process_movement();
uint32_t set_movement(const char *mv_str);

uint32_t get_sprite_slice(uint8_t depth, uint16_t sprite);
uint16_t clear_sprite_slice(uint8_t depth, uint16_t slice_mask, uint16_t sprite);

void p_bits(uint32_t x, int width);
void print_frame();

void game_over_animation(uint8_t type);

void setup() {
  // put your setup code here, to run once:
  init_io();

  init_game();

  disp.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (game_data.g_state & GAME_OVER) {
    // play animation?
    // game over screen?? (continue/play again options?)
  } else if (game_data.g_state & INIT) {
    // character creation and serial data sync
    create_character();
    game_data.g_state = PLAYING;
  } else if (game_data.g_state & PLAYING) {
    update_game();
    push_frame();
  }
}

void init_io() {
  // input pins
  *DDR_B &= ~(U_BTN | D_BTN | R_BTN | L_BTN | A_BTN | B_BTN);
}

void p_bits(uint32_t x, int width) {
    for (int i = width - 1; i >= 0; i -= 1) {
        Serial.print(((1<<i) & x) ? '1' : '0');
    }
    Serial.println();
}

void print_frame() {
    Serial.print("========= BEGIN FRAME =========\n");
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
        p_bits(game_data.frame[i], 32);
    }
    Serial.print("=========  END  FRAME =========\n");
}

