#ifndef SH1106_H
#define SH1106_H

#include "esp_err.h"
#include <stdint.h>

#define SH1106_WIDTH   128
#define SH1106_HEIGHT  64
#define SH1106_PAGES   (SH1106_HEIGHT / 8)

esp_err_t sh1106_init(void);
void sh1106_clear(void);
void sh1106_fill(uint8_t pattern);
void sh1106_update(void);

void sh1106_draw_char(uint8_t x, uint8_t page, char c);
void sh1106_draw_string(uint8_t page, const char *text);

#endif