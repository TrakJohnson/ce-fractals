#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <keypadc.h>
#include <debug.h>
#include <assert.h>

#define MAX_ITER 16
#define TITLE_FONT_HEIGHT 8
#define NORMAL_FONT_HEIGHT 8
#define SMALL_FONT_HEIGHT 8

// Timing
// Julia, res 32: 00:16 -> 7:02
// Julia, res 16:


const char *choices[2] = {"Mandelbrot", "Julia"};
uint8_t selected_choice_idx = 0;

const uint8_t blue_gradient_16[16] = {0, 24, 25, 26, 27, 28, 29, 30, 31, 63, 95, 127, 159, 191, 223, 255};
const uint8_t orange_gradient_16[16] = {0, 8, 40, 72, 104, 136, 168, 200, 232, 233, 234, 235, 236, 237, 238, 239};

/* utils */
char* concat(const char *s1, const char *s2) {
  // https://stackoverflow.com/questions/8465006/how-do-i-concatenate-two-strings-in-c/8465083
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

int cyclic_var_shift(int v, int direction, int max_val) {
  if (v == 0 && direction == -1) {
    return max_val;
  } else if (v == max_val && direction == 1) {
    return 0;
  } else {
    return v + direction;
  }
}

/* GUI */
void print_main_menu() {
  const char *title = "Fractal Generator";
  const int box_padding = 10;
  const int text_y_pos = 10;
  const int top_line_y = text_y_pos + box_padding + TITLE_FONT_HEIGHT;
  const char *bottom_messages[3] = {
    "Clear to exit", "<> to navigate", "Enter to start"
  };

  // Header
  gfx_SetColor(gfx_blue);
  gfx_Line(0, top_line_y, LCD_WIDTH, top_line_y);
  // Text
  gfx_SetTextFGColor(222);
  /* gfx_SetFontHeight(TITLE_FONT_HEIGHT); */
  /* gfx_SetFontSpacing(10); */
  gfx_PrintStringXY(title, (LCD_WIDTH - gfx_GetStringWidth(title)) / 2, text_y_pos);

  /* Footer */
  /* gfx_SetFontHeight(SMALL_FONT_HEIGHT); */
  gfx_PrintStringXY(bottom_messages[0], 0, LCD_HEIGHT - SMALL_FONT_HEIGHT);
  gfx_PrintStringXY(bottom_messages[1],
                    (LCD_WIDTH - gfx_GetStringWidth(bottom_messages[1])) / 2,
                    LCD_HEIGHT - SMALL_FONT_HEIGHT);
  gfx_PrintStringXY(bottom_messages[2],
                    LCD_WIDTH - gfx_GetStringWidth(bottom_messages[2]),
                    LCD_HEIGHT - SMALL_FONT_HEIGHT);
}

void print_select_block(int y_top, bool first_draw, char *description, const char *selected_value) {
  // Print description
  char *complete_desc = concat(concat("< ", description), " >"); // TODO this is stupid
  if (first_draw) {
    gfx_SetTextFGColor(222);
    gfx_PrintStringXY(complete_desc, (LCD_WIDTH - gfx_GetStringWidth(complete_desc)) / 2, y_top);
  } else {
    gfx_SetColor(0);
    gfx_FillRectangle(0, (y_top + 2 * NORMAL_FONT_HEIGHT), LCD_WIDTH, NORMAL_FONT_HEIGHT);
  }
  gfx_SetTextFGColor(230);  // light yellow
  gfx_PrintStringXY(selected_value, (LCD_WIDTH - gfx_GetStringWidth(selected_value))/2,
                    (y_top + 2 * NORMAL_FONT_HEIGHT));  // line spacing of 1
}

// Fractal
int calculate_divergence(float x, float y, float cx, float cy, const char* fractal_type) {
  float a, b, t, u;
  int n;
  a = (x - LCD_WIDTH/2) / 80;
  b = (y - LCD_HEIGHT/2) / 80;
  if (strcmp(fractal_type, "Mandelbrot") == 0) {
    cx = a;
    cy = b;
  }
  for (n = 0; n < MAX_ITER; n++) {
    u = a * a;
    t = b * b;
    if (u + t > 4) {
      // |z| < 2 => (|z| doesn't converge), return the steps for color
      return n + 1;
    }
    b = 2 * a * b + cy;
    a = u - t + cx;
  }
  return 0;
}

void draw_fractal(const char* fractal_type) {  // TODO: add color argument
  uint8_t current_color, previous_color, diverges_in;
  int x, y, previous_y_checkpoint;
  previous_y_checkpoint = 0;
  previous_color = 0;

  gfx_FillScreen(gfx_black);
  gfx_SetColor(0xFF);
  for (x = 0; x < LCD_WIDTH; x++) {
    for (y = 0; y < LCD_HEIGHT && kb_Data[6] != kb_Clear; y++) {
      kb_Scan();
      // TODO allow customizing the starting values
      diverges_in = calculate_divergence(x, y, -0.835, -0.2321, fractal_type);
      current_color = blue_gradient_16[diverges_in];
      if (current_color != previous_color) {
        gfx_SetColor(previous_color);
        gfx_Line(x, previous_y_checkpoint, x, y - 1);
        previous_y_checkpoint = y;
      }
      previous_color = current_color;
    }
    // finish the line to bottom before x++
    if (previous_y_checkpoint != LCD_HEIGHT - 1) {
      gfx_SetColor(current_color);
      gfx_Line(x, previous_y_checkpoint, x, LCD_HEIGHT - 1);
      previous_y_checkpoint = 0;
    }
  }
}
// Timer
// 13:44 -> 22:00 = 7min
// New
// 2:25 -> 9:08
int main(void) {
  // input detection
  bool right;
  bool left;
  kb_key_t arrows;
  int8_t prevKey;

  /* Seed the random numbers */
  srand(rtc_Time());
  gfx_Begin();
  /* gfx_SetDrawBuffer(); */
  gfx_FillScreen(gfx_black);

  print_main_menu();
  print_select_block(100, true, "Select type", choices[selected_choice_idx]);

  prevKey = 0;  // needed to avoid repetitions
  do {
    // Check for arrows
    arrows = kb_Data[7];
    right = arrows & kb_Right;
    left = arrows & kb_Left;
    if ((left && prevKey != -1) || (right && prevKey != 1)) {
      // maxval is the number of elements minus one
      selected_choice_idx = cyclic_var_shift(selected_choice_idx, right ? 1 : -1, 1);
      print_select_block(100, false, "Select type", choices[selected_choice_idx]);
    }
    prevKey = right ? 1 : (left ? -1 : 0);  // must do that here to avoid repetitions

    if (kb_Data[6] & kb_Clear) {
      gfx_End();
      return 0;
    }
  } while (kb_Data[6] != kb_Enter);

  draw_fractal(choices[selected_choice_idx]);

  while (!os_GetCSC());
  gfx_End();
  return 0;
}
