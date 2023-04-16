void init_game(uint8_t lvl) {
  game_data.g_state = PLAYING;

  player.sprite = 0;
  player.emitter = 0;
  player.r_pos = 0;
  player.c_pos = 0;

  player.atk_timer.interval = P_ATK_CD;
  player.atk_timer.i_time = millis();

	spawn_enemies(lvl);

  game_data.p_proj_timer.interval = P_PROJ_INTV;
  game_data.e_proj_timer.interval = E_PROJ_INTV;

  for (int i = 0; i < GAME_HEIGHT; i += 1) {
    game_data.e_proj[i] = 0;
    game_data.p_proj[i] = 0;
  }

  game_data.g_state = INIT;
}

void spawn_enemies(uint8_t level) {
	if (level >= 0 && level < NUM_LEVELS) {
		game_data.num_enemies = levels[level]->enemy_ct;

		for (int i = 0; i < game_data.num_enemies; i += 1) {
			enemies[i].r_pos = (i * SPRITE_HEIGHT) % GAME_HEIGHT;
			enemies[i].c_pos = GAME_WIDTH - (i * SPRITE_WIDTH);

      randomSeed(analogRead(0));
			enemies[i].sprite = e_sprites[(int)random(NUM_ETYPES)];
      randomSeed(analogRead(0));
			enemies[i].emitter = e_emitters[(int)random(NUM_ETYPES)];

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

void create_character() {
  uint8_t done = 0;
  uint8_t create_frame[8];
  uint8_t curs_row = 2, curs_col = 2, s_bits_left = 8, e_bits_left = 2;
  uint8_t prev_input = 0;
  uint16_t eq_count = 0;

  static uint16_t sprite = 0, emitter = 0;
  if (game_data.cur_level == 0) {
    for (int i = 0; i < 8; i += 1) {
      create_frame[i] = 0;
      if (i == 1 || i == 6) create_frame[i] = 0x7E;
      else if (i != 0 && i != 7) create_frame[i] = 0x42;
    }

    quip.say(create);
    quip.say(spa_YOUR);
    quip.say(spa_CHARACTER);

    while (!done) {
      // can potentially debounce PIN_B before storing
      game_data.input = *PIN_B;

      if (prev_input != game_data.input || (eq_count > 100 && eq_count < 500)) {
        if (prev_input == game_data.input) {
          delay(30);
          eq_count = (eq_count == 500) ? 0 : eq_count + 1;
        } else {
          eq_count = 0;
        }

        if (game_data.input & A_BTN) {
          if (s_bits_left) {
            create_frame[curs_row] |= (1 << curs_col);
            sprite |= (1 << curs_col - 2) << ((SPRITE_HEIGHT - (curs_row - 2) - 1) * SPRITE_WIDTH);
            s_bits_left -= 1;
          } else if (e_bits_left) {
            emitter |= (1 << curs_col - 2) << ((SPRITE_HEIGHT - (curs_row - 2) - 1) * SPRITE_WIDTH);
            e_bits_left -= 1;
          } else {
            done = 1;
          }
        } if (game_data.input & B_BTN) {
          if (e_bits_left < 2) done = 1;
        } if (game_data.input & U_BTN) {
          if (curs_row > 2) curs_row -= 1;
        } if (game_data.input & D_BTN) {
          if (curs_row < 5) curs_row += 1;
        } if (game_data.input & R_BTN) {
          if (curs_col > 2) curs_col -= 1;
        } if (game_data.input & L_BTN) {
          if (curs_col < 5) curs_col += 1;
        }
      } else {
        eq_count = (eq_count == 500) ? 0 : eq_count + 1;
      }

      disp.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      disp.clear();
      for (int i = 0; i < GAME_HEIGHT; i += 1) {
        if (!done) disp.setRow(2, i, (uint8_t)(0x55 >> (i % 2)));
        disp.setRow(1, i, create_frame[i]);
        disp.setRow(1, curs_row, (uint8_t)(1 << curs_col) | 0x42 | create_frame[curs_row]);
        if (!done) disp.setRow(0, i, (uint8_t)(0x55 >> (i % 2)));      
      }
      disp.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      prev_input = game_data.input;
    }
  }
  

  delay(2000);

  quip.say(spt_READY_TO_START);
  delay(1000);
  quip.say(spa_THREE);
  delay(500);
  quip.say(spa_TWO);
  delay(500);
  quip.say(spa_ONE);
  delay(500);
  quip.say(spt_GO);


  player.sprite = sprite;
  player.emitter = emitter;
}

void game_over_animation() {
  uint8_t done = 0, num_empty = 0;
  while (!done) {
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
  
  
      if (i % 2 == 0) game_data.frame[i] >>= 1;
      else game_data.frame[i] <<= 1;
      delay(10);
      
      push_frame();
    }

    for (int i = 0; i < GAME_HEIGHT; i += 1) {
      if (!game_data.frame[i]) num_empty += 1;
    }

    if (num_empty == GAME_HEIGHT) done = 1;
    else num_empty = 0;
  }
  quip.say(spt_PLAY);
  quip.say(spa_AGAIN);
  quip.say(spt_IF);
  quip.say(spt_YOU);
  quip.say(DARE);
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
        player_attack();
        enemy_attack();
        // reset cd
        player.atk_timer.i_time = millis();
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
  uint32_t p_slice = 0, e_slice = 0;

  player.atk_timer.c_time = millis() - player.atk_timer.i_time;

  player.s_height = 0;
  for (int i = 0; i < SPRITE_HEIGHT; i += 1) {
    if (player.sprite & (0x000F << (i * SPRITE_WIDTH))) player.s_height += 1;
  }

  if (!(player.sprite & 0xF000)) {
    player.sprite <<= SPRITE_WIDTH;
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
          enemy_attack();
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
    game_over();
  }

  uint8_t alive_ct = game_data.num_enemies;
  for (int i = 0; i < game_data.num_enemies; i += 1) {
    if (enemies[i].sprite == 0) {
      alive_ct -= 1;
    } if (enemies[i].c_pos == 0) {
      game_over();
    }
  }

  if (alive_ct == 0) {
    if (game_data.cur_level == NUM_LEVELS) {
      game_data.g_state = GAME_OVER;
      quip.say(spt_YOU_WIN);
      game_over_animation();
    } else {
      game_data.g_state = GAME_OVER;
      game_data.cur_level += 1;
      quip.say(spt_NEXT);
      quip.say(spt_ROUND);
    }
  }
}

void update_frame() {
  uint8_t p_depth, e_depth, su_depth;
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

void player_attack() {
  for (int i = 0; i < GAME_HEIGHT; i += 1) {
    uint8_t p_depth = i - player.r_pos;
    if (p_depth >= 0 && p_depth < player.s_height) {
      uint32_t em_slice = get_sprite_slice(p_depth, player.emitter);
      em_slice <<= GAME_WIDTH - player.c_pos - SPRITE_WIDTH;
      game_data.p_proj[i] |= em_slice;
    }
  }
}

void enemy_attack() {
  for (int k = 0; k < game_data.num_enemies; k += 1) {
    for (int i = 0; i < GAME_HEIGHT; i += 1) {
      uint8_t e_depth = i - enemies[k].r_pos;
      uint32_t em_slice = get_sprite_slice(e_depth, enemies[k].emitter);
      em_slice <<= GAME_WIDTH - enemies[k].c_pos - SPRITE_WIDTH;

      // fire projectile
      game_data.e_proj[i] |= em_slice;

      enemies[k].atk_timer.i_time = millis();
    }
  }
}

void game_over() {
  game_data.g_state = GAME_OVER;
    
  quip.say(pacman_death);

  randomSeed(analogRead(0));
  uint16_t rand = random(0,100);
  if (rand < 25) {
    quip.say(sp4_MAYDAY);
  } else if (rand >= 25 && rand < 50) {
    quip.say(spt_TRY_AGAIN);
  } else if (rand >= 50 && rand < 75) {
    quip.say(sp2_POWER);
    quip.say(spa_DOWN);
  } else if (rand >= 75 && rand < 98) {
    quip.say(spa_ERROR);
    quip.say(spa_ERROR);
  } else if (rand >= 98) {
    quip.say(sp2_QUEBEC);
  }

  quip.say(spt_GAMES);
  quip.say(spt_OVER);

  game_over_animation();
}

void push_frame() {
  disp.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  disp.clear();
  // print_frame();
  for (int i = 0; i < GAME_HEIGHT; i += 1) {
    disp.setRow(2, i, (uint8_t)((game_data.frame[i] & 0x00FF0000) >> 16));
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