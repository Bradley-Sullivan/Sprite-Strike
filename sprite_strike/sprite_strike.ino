#include "sprite_strike.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  init_io();

  init_game(0);

  disp.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (game_data.g_state & GAME_OVER) {  /* game reset */
    if (*PIN_B & A_BTN) {
      init_game(game_data.cur_level);
      game_data.g_state = INIT;
    }
  } else if (game_data.g_state & INIT) { /* character creation */
    create_character();
    game_data.g_state = PLAYING;
  } else if (game_data.g_state & PLAYING) { /* game playing */
    update_game();
    push_frame();

    // tx_frame_data();
  }
}

void init_io() {
  // input pins
  *DDR_B &= ~(U_BTN | D_BTN | R_BTN | L_BTN | A_BTN | B_BTN);
}

void tx_frame_data() {
  uint8_t tx_byte = 0;
  Serial.write(0x55);
  for (int i = 0; i < 32; i += 1) {
    tx_byte = 0;
    for (int k = 0; k < GAME_HEIGHT; k += 1) {
      uint32_t c_val = (game_data.frame[k] & (1 << i)) >> i;
      tx_byte |= c_val << k;
    }
    Serial.write(tx_byte);
    // p_bits(tx_byte, 8);
  }
  Serial.write(0x55);
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
