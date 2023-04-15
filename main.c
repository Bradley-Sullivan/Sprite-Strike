#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>

#define GAME_HEIGHT     8
#define GAME_WIDTH      24

#define SPRITE_HEIGHT   4
#define SPRITE_WIDTH    4

#define MAX_NUM_ENEMIES 4
#define NUM_ETYPES      6

#define E_PROJ_INTV     750.f    // (ms)
#define P_PROJ_INTV     500.f    // (ms)

typedef struct player_t {
    uint16_t    sprite;
    uint8_t     emitter;        // shift value
    uint8_t     c_pos;
    uint8_t     r_pos;
    uint8_t     super_cd;
    uint8_t     atk_cd;
} player_t;

typedef struct enemy_t {
    uint16_t    sprite;
    uint16_t    emitter;
    uint8_t     c_pos;
    uint8_t     r_pos;

    float       atk_interval;   // (ms)
    float       super_interval; // (ms)

    float       atk_itime;      // (ms)
    float       atk_ctime;      // (ms)
    float       super_ctime;    // (ms)
} enemy_t;

struct game_data {
    float       i_time;         // (ms)
    float       g_time;         // (ms)

    /* time since last proj. position update */
    float       p_proj_itime;    // (ms)
    float       e_proj_itime;    // (ms)
    float       p_proj_ctime;    // (ms)
    float       e_proj_ctime;    // (ms)

    uint8_t     num_enemies;

    uint32_t    e_proj[GAME_HEIGHT];
    uint32_t    p_proj[GAME_HEIGHT];

    uint8_t     g_state;
    uint8_t     g_flags;

    uint32_t    frame[GAME_HEIGHT];
} game_data;

// ENEMY SPRITES
static const uint16_t e_sprites[NUM_ETYPES] = { 0x96F9, 0x966F, 0x136D, 0x6317, 0x3EE3, 0x06F0 };

static player_t player;
static enemy_t enemies[MAX_NUM_ENEMIES];

void init_game();

void update_game();

void update_input();
void update_player();
void update_enemies();
void update_projectiles();

void update_frame();

uint32_t get_sprite_slice(uint8_t depth, uint16_t sprite);
uint16_t clear_sprite_slice(uint8_t depth, uint16_t slice_mask, uint16_t sprite);

void p_bits(uint32_t x, int width);
void print_frame();

int main(void) {
    init_game();

    game_data.i_time = (clock() * 1000 / CLOCKS_PER_SEC);
    while (1) { /*  game loop   */
        usleep(50000);
        game_data.g_time = clock() - game_data.i_time;


        update_game();
        print_frame();
    }

    return 0;
}

void init_game() {
    player.sprite = 0x0660;
    player.r_pos = 0;
    player.c_pos = 0;

    game_data.num_enemies = 2;

    enemies[0].sprite = e_sprites[0];
    enemies[1].sprite = e_sprites[1];

    enemies[0].r_pos = 0;
    enemies[0].c_pos = GAME_WIDTH;

    enemies[1].r_pos = GAME_HEIGHT - SPRITE_HEIGHT;
    enemies[1].c_pos = GAME_WIDTH;
    enemies[1].emitter = 0x8008;
    enemies[1].atk_interval = 5000;

    // game_data.p_proj[2] = (1 << 20);
    // game_data.p_proj_itime = clock();

    // game_data.e_proj[2] = (1 << 7);
    // game_data.e_proj_itime = clock();
}

void update_game() {
    // update_input();

    update_player();
    update_enemies();
    update_projectiles();

    update_frame();
}

void update_player() {
    uint32_t p_slice = 0;

    for (int i = 0; i < GAME_HEIGHT; i += 1) {
        uint8_t p_depth = i - player.r_pos;
        if (p_depth >= 0 && p_depth < SPRITE_HEIGHT) {
            p_slice = get_sprite_slice(p_depth, player.sprite);
            p_slice <<= GAME_WIDTH - player.c_pos;
            p_slice &= game_data.e_proj[i];
            if (p_slice) {
                game_data.e_proj[i] &= ~p_slice;
                p_slice >>= GAME_WIDTH - player.c_pos;
                player.sprite = clear_sprite_slice(p_depth, p_slice, player.sprite);
            }
        }
    }

    // for (int i = 0; i < game_data.num_enemies; i += 1) {
    //     // want bit-resolution on enemy-player collisions!!
    //     if (enemies[i].c_pos <= player.c_pos + (SPRITE_WIDTH - 1)) {
    //         if (enemies[i].r_pos <= player.r_pos + (SPRITE_HEIGHT - 1) &&
    //             enemies[i].r_pos >= player.r_pos) {
    //             // game over
    //         }
    //     }
    // }
}

void update_enemies() {
    uint16_t e_occ = 0;
    uint32_t e_slice = 0, e_proj_mask = 0;

    // enemy -> player proj. collision update
    for (int k = 0; k < game_data.num_enemies; k += 1) {
        // process position updates
            // do testing before this

        enemies[k].atk_ctime = clock() - enemies[k].atk_itime;
        for (int i = 0; i < GAME_HEIGHT; i += 1) {
            uint8_t e_depth = i - enemies[k].r_pos;
            if (e_depth >= 0 && e_depth < SPRITE_HEIGHT) {
                if (enemies[k].atk_ctime >= enemies[k].atk_interval) { /* fire projectile */
                    uint32_t em_slice = get_sprite_slice(e_depth, enemies[k].emitter);
                    em_slice <<= GAME_WIDTH - enemies[k].c_pos;
                    game_data.e_proj[i] |= em_slice;
                    enemies[k].atk_itime = clock();
                }
                e_slice = get_sprite_slice(e_depth, enemies[k].sprite);
                e_slice <<= GAME_WIDTH - enemies[k].c_pos;
                e_slice &= game_data.p_proj[i];
                if (e_slice) {
                    game_data.p_proj[i] &= ~e_slice;
                    e_slice >>= GAME_WIDTH - enemies[k].c_pos;
                    enemies[k].sprite = clear_sprite_slice(e_depth, e_slice, enemies[k].sprite);
                }
            }
        }        
    }
    

}

void update_projectiles() {
    game_data.p_proj_ctime = clock() - game_data.p_proj_itime;
    game_data.e_proj_ctime = clock() - game_data.e_proj_itime;

    for (int i = 0; i < GAME_HEIGHT; i += 1) {
        if (game_data.p_proj[i]) { /* p_proj update */
            if (game_data.p_proj_ctime >= P_PROJ_INTV) {
                // advance player projectiles
                game_data.p_proj[i] >>= 1;

                game_data.p_proj_itime = clock();
            }
        }
        
        if (game_data.e_proj[i]) { /* e_proj update */
            if (game_data.e_proj_ctime >= E_PROJ_INTV) {
                // advance enemy projectiles
                game_data.e_proj[i] <<= 1;

                game_data.e_proj_itime = clock();
            }
        }
    }
}

void update_frame() {
    uint8_t p_depth, e_depth;
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
        game_data.frame[i] = 0;

        // store projectiles
        game_data.frame[i] = (game_data.p_proj[i] | game_data.e_proj[i]);

        // store player sprite
        p_depth = i - player.r_pos;
        if (p_depth >= 0 && p_depth < SPRITE_HEIGHT) {
            uint32_t p_slice = get_sprite_slice(p_depth, player.sprite);
            // printf("\tp_slice       : "); p_bits(p_slice, 32);
            p_slice <<= GAME_WIDTH - player.c_pos;
            game_data.frame[i] |= p_slice;
        }

        // store enemy sprites
        for (int k = 0; k < game_data.num_enemies; k += 1) {
            e_depth = i - enemies[k].r_pos;
            if (e_depth >= 0 && e_depth < SPRITE_HEIGHT) {
                uint32_t e_slice = get_sprite_slice(e_depth, enemies[k].sprite);
                // printf("\te_slice[%d]       : ", k); p_bits(e_slice, 32);
                e_slice <<= GAME_WIDTH - enemies[k].c_pos;
                game_data.frame[i] |= e_slice;
            }
        }
    }
}

uint32_t get_sprite_slice(uint8_t depth, uint16_t sprite) {
    uint32_t slice = 0;
    uint8_t slice_shift = (SPRITE_HEIGHT - depth - 1) * SPRITE_WIDTH;

    if (depth >= 0 && depth < SPRITE_HEIGHT) {
        slice = sprite & (0x000F << (slice_shift));
        slice >>= slice_shift;
    } else {
        return 0;
    }
    
    return slice;
}

uint16_t clear_sprite_slice(uint8_t depth, uint16_t slice_mask, uint16_t sprite) {
    if (depth >= 0 && depth < SPRITE_HEIGHT) {
        slice_mask <<= ((SPRITE_HEIGHT - depth - 1) * SPRITE_WIDTH);
        sprite &= ~slice_mask;
    }

    return sprite;
}

void p_bits(uint32_t x, int width) {
    for (int i = width - 1; i >= 0; i -= 1) {
        printf(" %c ", ((1<<i) & x) ? '1' : '0');
    }
    printf("\n");
}

void print_frame() {
    printf("========= BEGIN FRAME =========\n");
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
        p_bits(game_data.frame[i], 32);
    }
    printf("=========  END  FRAME =========\n");
}
