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

const char *choices[2] = {"Mandelbrot", "Julia"};
uint8_t selected_choice_idx = 0;
// similar file https://tiplanet.org/forum/archives/zipview.php?id=650118&zipFileID=1

int cyclic_var_shift(int v, int direction, int max_val) {
  if (v == 0 && direction == -1) {
    return max_val;
  } else if (v == max_val && direction == 1) {
    return 0;
  } else {
    return v + direction;
  }
}

/* Menu drawing functions */
// TODO make that global ?
void draw_fractal_choice() {
  gfx_SetColor(0);
  // erase old text
  gfx_FillRectangle(0, (LCD_HEIGHT/2 + NORMAL_FONT_HEIGHT), LCD_WIDTH, NORMAL_FONT_HEIGHT);
  gfx_SetTextFGColor(230);  // light yellow
  gfx_PrintStringXY(choices[selected_choice_idx],
                    (LCD_WIDTH - gfx_GetStringWidth(choices[selected_choice_idx]))/2,
                    (LCD_HEIGHT/2 + NORMAL_FONT_HEIGHT));
}

int diverges_julia(float x, float y) { //, float cx, float cy) {
  float a, b, t, u, cx, cy;
  int n;
  cx = -0.79;
  cy = 0.15;
  a = (x - LCD_WIDTH/2) / 80;
  b = (y - LCD_HEIGHT/2) / 80;
  /* a = (x - LCD_WIDTH) */
  for (n = 0; n < MAX_ITER; n++) {
    u = a * a;
    t = b * b;
    if (u + t > 4) {
      return n;
    }
    b = 2 * a * b + cy;
    a = u - t + cx;
  }
  return 0;
}

int diverges_mandelbrot(float x, float y) {
  // x and y are pixel coordinates
  float a, b, cx, cy, t, u;
  int n;
  a = cx = (x - LCD_WIDTH/2) / 80;
  b = cy = (y - LCD_HEIGHT/2) / 80;
  for (n = 0; n < MAX_ITER; n++) {
    u = a * a;
    t = b * b;
    if (u + t > 4) {
      // |z|<2 => |z| doesn't converge
      // returns in how many steps (for color)
      return n;
    }
    b = 2 * a * b + cy;
    a = u - t + cx;
  }
  return 0;
}

void draw_fractal(bool is_mandelbrot) {  // TODO: add color argument
  uint8_t current_color;
  int x, y, diverges_in;
  /* float a, b, cx, cy, t, u; */

  gfx_FillScreen(gfx_black);
  gfx_SetColor(0xFF);
  for (x = 0; x < LCD_WIDTH; x++) {
    for (y = 0; y < LCD_HEIGHT && kb_Data[6] != kb_Clear; y++) {
      kb_Scan();
      /* diverges_in = diverges_mandelbrot(x, y); */
      if (is_mandelbrot) {
        diverges_in = diverges_mandelbrot(x, y);
      } else {
        diverges_in = diverges_julia(x, y);
      }
      if (diverges_in != 0) {
        gfx_SetColor(0xFF); // inside of the fractal
      } else {
        dbg_sprintf(dbgout, "Color %d was passed", current_color);
        /* current_color = 256 - (MAX_ITER - n) * 16; */
        gfx_SetColor(gfx_Darken(0xFF, MAX_ITER - diverges_in));
        /* gfx_RGBTo1555(current_color, current_color, current_color)); */
      }
      gfx_SetPixel(x, y);
    }
  }
}

int main(void) {
  const char *title = "Fractal Generator";
  const int box_padding = 10;
  const int text_y_pos = 10;
  const int top_line_y = text_y_pos + box_padding + TITLE_FONT_HEIGHT;
  const char *bottom_messages[3] = {
    "Clear to exit", "Arrows to navigate", "2nd to start"
  };
  // input detection
  bool right;
  bool left;
  kb_key_t arrows;
  int8_t prevKey;

  dbg_sprintf(dbgout, "This is the start of a CEmu debugging test\n");
  /* int var = 10; */
  /* unsigned code = 3; */
  /* dbg_sprintf(dbgout, "Initialized some things...\n"); */
  /* dbg_sprintf(dbgout, "var value: %d\n", var); */
  /* dbg_sprintf(dbgerr, "PROGRAM ABORTED (code = %u)\n", code); */

  /* Seed the random numbers */
  srand(rtc_Time());
  gfx_Begin();
  gfx_FillScreen(gfx_black);

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

  draw_fractal_choice();

  /* Wait for key */
  prevKey = 0;  // needed to avoid repetitions
  do {
    arrows = kb_Data[7];
    right = arrows & kb_Right;
    left = arrows & kb_Left;
    if ((left && prevKey != -1) || (right && prevKey != 1)) {
      // maxval is the number of elements minus one
      selected_choice_idx = cyclic_var_shift(selected_choice_idx, right ? 1 : -1, 1);
      dbg_sprintf(dbgout, "Current index %d", selected_choice_idx);
      draw_fractal_choice();
    }
    prevKey = right ? 1 : (left ? -1 : 0);  // must do that here to avoid repetitions
  } while (kb_Data[1] != kb_2nd);

  switch(selected_choice_idx) {
  case 0:
    draw_fractal(true);
    break;
  case 1:
    draw_fractal(false);
    break;
  }

  while (!os_GetCSC());
  gfx_End();
  return 0;
}
