void init_game() {
  game_data.g_state = PLAYING;

  player.sprite = 0x0660;
  player.emitter = 0x0200;
  player.r_pos = 0;
  player.c_pos = 0;
  player.atk_timer.interval = P_ATK_CD;
  player.super_timer.interval = P_ATK_CD;

  player.atk_timer.i_time = millis();
  player.super_timer.i_time = millis();

	spawn_enemies(0);

  game_data.p_proj_timer.interval = P_PROJ_INTV;
  game_data.e_proj_timer.interval = E_PROJ_INTV;
}

void spawn_enemies(uint8_t level) {
	if (level >= 0 && level < NUM_LEVELS) {
		game_data.num_enemies = levels[level]->enemy_ct;

		for (int i = 0; i < game_data.num_enemies; i += 1) {
			enemies[i].r_pos = (i * SPRITE_HEIGHT) % GAME_HEIGHT;
			enemies[i].c_pos = GAME_WIDTH - (i * SPRITE_WIDTH);

			enemies[i].sprite = e_sprites[(int)random(0, NUM_ETYPES)];
			enemies[i].emitter = e_emitters[(int)random(0, NUM_ETYPES)];

			enemies[i].atk_timer.interval = levels[level]->atk_intervals[i];

			enemies[i].mvmt_mask[UD] = levels[level]->ud_masks[i];
			enemies[i].mvmt_mask[LR] = levels[level]->lr_masks[i];

			enemies[i].mvmt_timer[UD].interval = levels[level]->ud_intervals[i];
			enemies[i].mvmt_timer[LR].interval = levels[level]->lr_intervals[i];

			enemies[i].ud_mvmt = set_movement(levels[level]->ud_pathing[i]);
			enemies[i].lr_mvmt = set_movement(levels[level]->lr_pathing[i]);

			enemies[i].mvmt_timer[UD].i_time = millis();
			enemies[i].mvmt_timer[LR].i_time = millis();
		}
	}
}

void update_game() {
  update_input();

  update_player();
  update_enemies();
  update_projectiles();
  update_conditions();

  update_frame();
}

void update_input() {
  static uint8_t prev_input = 0;
  static uint16_t eq_count = 0;

  // can potentially debounce PIN_B before storing
  game_data.input = *PIN_B;

  if (prev_input != game_data.input || (eq_count > 10 && eq_count < 25)) {
    if (prev_input == game_data.input) {
      delay(30);
      eq_count = (eq_count == 25) ? 0 : eq_count + 1;
    } else {
      eq_count = 0;
    }

    if (game_data.input & A_BTN) {
      // atk cooldown check
      if (player.atk_timer.c_time >= player.atk_timer.interval) {
        // base attack
        player_attack(0);
        // reset cd
        player.atk_timer.i_time = millis();
      }
    } if (game_data.input & B_BTN) {
      // super cooldown check
      if (player.super_timer.c_time >= player.super_timer.interval) {
        // super attack (needs robust game states for stop-time effect)
        player_attack(1);   // needs other testing before full implementaiton 
      }      
    } if (game_data.input & U_BTN) {
      player.r_pos = (player.r_pos > 0 ? player.r_pos - 1 : 0);
    } if (game_data.input & D_BTN) {
      player.r_pos = (player.r_pos + player.s_height < GAME_HEIGHT ? player.r_pos + 1 : GAME_HEIGHT - player.s_height);
    } if (game_data.input & R_BTN) {
      player.c_pos = (player.c_pos < GAME_WIDTH - 1 ? player.c_pos + 1 : GAME_WIDTH);
    } if (game_data.input & L_BTN) {
      player.c_pos = (player.c_pos > 0 ? player.c_pos - 1 : 0);
    }
  } else {
    eq_count = (eq_count == 25) ? 0 : eq_count + 1;
  }

  prev_input = game_data.input;
}

void update_player() {
  static uint32_t sprite_cache = 0;
  uint32_t p_slice = 0, e_slice = 0;

  player.atk_timer.c_time = millis() - player.atk_timer.i_time;
  player.super_timer.c_time = millis() - player.super_timer.i_time;

  // recalculates sprite height if sprite deformation is detected
  if (sprite_cache != player.sprite) {
    player.s_height = 0;
    for (int i = 0; i < SPRITE_HEIGHT; i += 1) {
      if (player.sprite & (0x000F << (i * SPRITE_WIDTH))) player.s_height += 1;
    }

    if (!(player.sprite & 0xF000)) {
      player.sprite <<= SPRITE_WIDTH;
    }
  }
  
  for (int i = 0; i < GAME_HEIGHT; i += 1) {
    uint8_t p_depth = i - player.r_pos;
    if (p_depth >= 0 && p_depth < player.s_height) { /* if "scanline" hits player */
      p_slice = get_sprite_slice(p_depth, player.sprite);
      p_slice <<= GAME_WIDTH - player.c_pos - SPRITE_WIDTH;
      p_slice &= game_data.e_proj[i];

      if (p_slice) {
        // consume projectile
        game_data.e_proj[i] &= ~p_slice;

        p_slice >>= GAME_WIDTH - player.c_pos - SPRITE_WIDTH;

        // deform player sprite
        player.sprite = clear_sprite_slice(p_depth, p_slice, player.sprite);
      }

      for (int k = 0; k < game_data.num_enemies; k += 1) {
        uint8_t e_depth = i - enemies[k].r_pos;
        if (e_depth == p_depth) {
          e_slice = get_sprite_slice(e_depth, enemies[k].sprite);
          p_slice = get_sprite_slice(p_depth, player.sprite);
          e_slice <<= GAME_WIDTH - enemies[k].c_pos - SPRITE_WIDTH;
          p_slice <<= GAME_WIDTH - player.c_pos - SPRITE_WIDTH;
          p_slice &= e_slice;

          if (p_slice) {
            game_data.g_state |= GAME_OVER;
          }
        }
      }
    }
  }

  sprite_cache = player.sprite;
}

void update_enemies() {
  uint16_t e_occ = 0;
  uint32_t e_slice = 0, e_proj_mask = 0;

  for (int k = 0; k < game_data.num_enemies; k += 1) {
    process_movement();

    enemies[k].atk_timer.c_time = millis() - enemies[k].atk_timer.i_time;
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
      uint8_t e_depth = i - enemies[k].r_pos;
      if (e_depth >= 0 && e_depth < SPRITE_HEIGHT) {  /* if "scanline" hits enemy */
        if (enemies[k].atk_timer.c_time >= enemies[k].atk_timer.interval) { /* fire projectile */
          uint32_t em_slice = get_sprite_slice(e_depth, enemies[k].emitter);
          em_slice <<= GAME_WIDTH - enemies[k].c_pos - SPRITE_WIDTH;

          // fire projectile
          game_data.e_proj[i] |= em_slice;

          // reset atk timer
          enemies[k].atk_timer.i_time = millis();
        }

        e_slice = get_sprite_slice(e_depth, enemies[k].sprite);
        e_slice <<= GAME_WIDTH - enemies[k].c_pos - SPRITE_WIDTH;
        e_slice &= game_data.p_proj[i];

        if (e_slice) {  /* p_proj and enemy collision */
          // consume projectile
          game_data.p_proj[i] &= ~e_slice;

          e_slice >>= GAME_WIDTH - enemies[k].c_pos - SPRITE_WIDTH;

          // deform enemy sprite
          enemies[k].sprite = clear_sprite_slice(e_depth, e_slice, enemies[k].sprite);
        }
      }
    }        
  }
}

void update_projectiles() {
  game_data.p_proj_timer.c_time = millis() - game_data.p_proj_timer.i_time;
  game_data.e_proj_timer.c_time = millis() - game_data.e_proj_timer.i_time;

  for (int i = 0; i < GAME_HEIGHT; i += 1) {
    if (game_data.p_proj[i]) {
      if (game_data.p_proj_timer.c_time >= game_data.p_proj_timer.interval) { /* advance  player projectiles */
        game_data.p_proj[i] >>= 1;

        game_data.p_proj_timer.i_time = millis();
      }
    }
    
    if (game_data.e_proj[i]) {
      if (game_data.e_proj_timer.c_time >= game_data.e_proj_timer.interval) { /* advance enemy projectiles */
        game_data.e_proj[i] <<= 1;

        game_data.e_proj_timer.i_time = millis();
      }
    }

    uint32_t proj_cmp = game_data.e_proj[i] & game_data.p_proj[i];
    if (proj_cmp) {
      game_data.e_proj[i] &= ~proj_cmp;
      game_data.p_proj[i] &= ~proj_cmp;
    }
  }
}

void update_conditions() {
    if (player.sprite == 0) {
        game_data.g_state |= GAME_OVER;
    }

    uint8_t alive_ct = game_data.num_enemies;
    for (int i = 0; i < game_data.num_enemies; i += 1) {
        if (enemies[i].sprite == 0) {
            alive_ct -= 1;
        }
    }

    if (alive_ct == 0) {
        // next wave
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
    if (p_depth >= 0 && p_depth <= player.s_height) {
      uint32_t p_slice = get_sprite_slice(p_depth, player.sprite);
      // printf("\tp_slice       : "); p_bits(p_slice, 32);
      p_slice <<= GAME_WIDTH - player.c_pos - SPRITE_WIDTH;
      game_data.frame[i] |= p_slice;
    }

    // store enemy sprites
    for (int k = 0; k < game_data.num_enemies; k += 1) {
      e_depth = i - enemies[k].r_pos;
      if (e_depth >= 0 && e_depth < SPRITE_HEIGHT) {
        uint32_t e_slice = get_sprite_slice(e_depth, enemies[k].sprite);
        // printf("\te_slice[%d]       : ", k); p_bits(e_slice, 32);
        e_slice <<= GAME_WIDTH - enemies[k].c_pos - SPRITE_WIDTH;
        game_data.frame[i] |= e_slice;
      }
    }
  }
}

void player_attack(uint8_t super) {
  if (super) {
    // leave for now
  } else {
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
      uint8_t p_depth = i - player.r_pos;
      if (p_depth >= 0 && p_depth < player.s_height) {
        uint32_t em_slice = get_sprite_slice(p_depth, player.emitter);
        em_slice <<= GAME_WIDTH - player.c_pos - SPRITE_WIDTH;
        game_data.p_proj[i] |= em_slice;
      }
    }
  } 
}

void push_frame() {
  disp.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  disp.clear();
  // print_frame();
  for (int i = 0; i < GAME_HEIGHT; i += 1) {
    disp.setRow(2, i, (uint8_t)((game_data.frame[i] & 0x07FF0000) >> 16));
    disp.setRow(1, i, (uint8_t)((game_data.frame[i] & 0x0000FF00) >> 8));
    disp.setRow(0, i, (uint8_t)(game_data.frame[i] &  0x000000FF));
  }
  disp.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void process_movement() {
    for (int i = 0; i < game_data.num_enemies; i += 1) {
        enemies[i].mvmt_timer[UD].c_time = millis() - enemies[i].mvmt_timer[UD].i_time;
        enemies[i].mvmt_timer[LR].c_time = millis() - enemies[i].mvmt_timer[LR].i_time;

        if (enemies[i].mvmt_timer[UD].c_time >= enemies[i].mvmt_timer[UD].interval) {
            if (enemies[i].mvmt_mask[UD]) { /* if mvmt enabled */
                if (enemies[i].mvmt_mask[UD] & enemies[i].ud_mvmt) { /* move up */
                    enemies[i].r_pos = (enemies[i].r_pos > 0 ? enemies[i].r_pos - 1 : 0);
                } else { /* move down */
                    enemies[i].r_pos = (enemies[i].r_pos + 2 < GAME_HEIGHT - 1 ? enemies[i].r_pos + 1 : GAME_HEIGHT - 2);
                }

                // rotate mask
                enemies[i].mvmt_mask[UD] = U32LROT(enemies[i].mvmt_mask[UD]);
            }

            enemies[i].mvmt_timer[UD].i_time = millis();
        }

        if (enemies[i].mvmt_timer[LR].c_time >= enemies[i].mvmt_timer[LR].interval) {
            if (enemies[i].mvmt_mask[LR]) { /* if mvmt enabled */
                if (enemies[i].mvmt_mask[LR] & enemies[i].lr_mvmt) { /* move left */
                    enemies[i].c_pos = (enemies[i].c_pos > 0 ? enemies[i].c_pos - 1 : 0);
                } else { /* move right */
                    enemies[i].c_pos = (enemies[i].c_pos < GAME_WIDTH ? enemies[i].c_pos + 1 : GAME_WIDTH);
                }

                // rotate mask
                enemies[i].mvmt_mask[LR] = U32LROT(enemies[i].mvmt_mask[LR]);
            }           

            enemies[i].mvmt_timer[LR].i_time = millis();
        }
    }

    // printf("\tmvmt_mask     : "); p_bits(enemies[0].mvmt_mask[UD], 32);
}

uint32_t set_movement(const char *mv_str) {
    uint32_t m = 0;
    uint8_t len = strlen(mv_str);
    if (mv_str && len < 32) {
        for (int i = 0; i < len; i += 1) {
            switch (mv_str[i]) {
                case 'l':
                case 'L':
                case 'u':
                case 'U':
                    m |= 1 << i;
                    break;
                    break;
                default:
                    break;
            }
        }
    }

    return m;
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