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

#define MAX_ITER 32
#define TITLE_FONT_HEIGHT 8
#define NORMAL_FONT_HEIGHT 8
#define SMALL_FONT_HEIGHT 8



const char *choices[2] = {"Mandelbrot", "Julia"};
uint8_t selected_choice_idx = 0;

/* utils */
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

/* Header */
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

  /* Main text */
  // TODO: add while loop here, simplify all this DRY
  gfx_PrintStringXY("< Selected mode >",
                    (LCD_WIDTH - gfx_GetStringWidth("< Selected mode >"))/2,
                    (LCD_HEIGHT/2 - NORMAL_FONT_HEIGHT));

}

void update_fractal_choice() {
  gfx_SetColor(0);
  // erase old text
  gfx_FillRectangle(0, (LCD_HEIGHT/2 + NORMAL_FONT_HEIGHT), LCD_WIDTH, NORMAL_FONT_HEIGHT);
  gfx_SetTextFGColor(230);  // light yellow
  gfx_PrintStringXY(choices[selected_choice_idx],
                    (LCD_WIDTH - gfx_GetStringWidth(choices[selected_choice_idx]))/2,
                    (LCD_HEIGHT/2 + NORMAL_FONT_HEIGHT));
}


/* math stuff */
int calculate_divergence(float x, float y, float cx, float cy, char* fractal_type) {
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
      // |z|<2 => |z| doesn't converge, return the steps for color
      return n + 1;
    }
    b = 2 * a * b + cy;
    a = u - t + cx;
  }
  return 0;
}

void draw_fractal(char* fractal_type) {  // TODO: add color argument
  uint8_t current_color;
  int x, y, diverges_in;
  /* float a, b, cx, cy, t, u; */

  gfx_FillScreen(gfx_black);
  gfx_SetColor(0xFF);
  for (x = 0; x < LCD_WIDTH; x++) {
    for (y = 0; y < LCD_HEIGHT && kb_Data[6] != kb_Clear; y++) {
      kb_Scan();
      /* diverges_in = diverges_mandelbrot(x, y); */
      // TODO find good c value
      // −0.835 − 0.2321
      diverges_in = calculate_divergence(x, y, -0.835, -0.2321, fractal_type);
      /* dbg_sprintf(dbgout, "n: %d\n", diverges_in); */
      if (diverges_in == 0) {
        gfx_SetColor(0xFF); // inside of the fractal
      } else {
        gfx_SetColor(diverges_in * 8);
      }
      gfx_SetPixel(x, y);
    }
  }
}

int main(void) {
  // input detection
  bool right;
  bool left;
  kb_key_t arrows;
  int8_t prevKey;

  /* Seed the random numbers */
  srand(rtc_Time());
  gfx_Begin();
  gfx_FillScreen(gfx_black);

  print_main_menu();
  update_fractal_choice();

  prevKey = 0;  // needed to avoid repetitions
  do {
    // Check for arrows
    arrows = kb_Data[7];
    right = arrows & kb_Right;
    left = arrows & kb_Left;
    if ((left && prevKey != -1) || (right && prevKey != 1)) {
      // maxval is the number of elements minus one
      selected_choice_idx = cyclic_var_shift(selected_choice_idx, right ? 1 : -1, 1);
      update_fractal_choice();
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
