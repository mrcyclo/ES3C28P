#!/bin/bash

lv_font_conv \
  --no-compress \
  --font fonts/arial.ttf \
    -r 0x20-0x7F,0xA0-0x24F,0x1EA0-0x1EFF \
  --font "fonts/Font Awesome 7 Free-Solid-900.otf" \
    -r 0xF000-0xF8FF \
  --size 14 --bpp 4 --format lvgl \
  -o src/font_custom_merged.c \
  --lv-font-name font_custom_merged
