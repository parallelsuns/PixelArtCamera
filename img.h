#ifndef PALETTE_H
#define PALETTE_H
#include <lvgl.h>
#include "sd_fns.h"
#include "globals.h"

static bool load_palette(const char* path);

static void on_choose_palette(lv_event_t* e) {
  char path[256];
  lv_obj_t* btn = lv_event_get_target(e);
  strcpy(path, "/palette/");
  strcat(path, lv_label_get_text(lv_obj_get_child(btn,0)));
  Serial.println("Load Palette: ");
  Serial.println(path);
  load_palette(path);
  lv_scr_load(ui_pages[UIScreen::MAIN]);
}

static void init_palette() {
  //default palette if none selected
  curr_palette.len = 16;
  for (int i = 0; i < 16; ++i) {
    curr_palette.p[i].r = i*16;
    curr_palette.p[i].g = i*16;
    curr_palette.p[i].b = i*16;
  }
  char* name = "DEFAULT";
  memcpy(curr_palette.name, name, 7);
  curr_palette.name[7] = 0;
  lv_label_set_text(palette_label, curr_palette.name);
  for (int i = 0; i < 256; ++i) curr_palette.leds[i] = nullptr;
  lv_style_init(&curr_palette.style);
  lv_style_set_outline_width(&curr_palette.style,0);
  lv_style_set_border_width(&curr_palette.style,0);
}

static bool load_palette(const char* path) {
  Serial.println("loading palette file");
  unsigned long buf_size = 0;
  uint8_t* buf = load_file_from_sd(path, &buf_size, true);
  if (buf == nullptr) 
  {
    Serial.println("Failed to load palette file");
    return false;
  }

  if (buf_size > 256 * 8) buf_size = 256 * 8;
  curr_palette.len = buf_size / 8;
  uint8_t* buf_p = buf;

  char hex[5] = {'0', 'x', '0', '0', 0};

  //parse palette .hex file
  for (int i = 0; i < curr_palette.len; ++i) {
    hex[2] = *((char*)buf_p++);
    hex[3] = *((char*)buf_p++);
    curr_palette.p[i].r = (uint8_t)strtoul(hex, nullptr, 16);

    hex[2] = *((char*)buf_p++);
    hex[3] = *((char*)buf_p++);
    curr_palette.p[i].g = (uint8_t)strtoul(hex, nullptr, 16);

    hex[2] = *((char*)buf_p++);
    hex[3] = *((char*)buf_p++);
    curr_palette.p[i].b = (uint8_t)strtoul(hex, nullptr, 16);

    buf_p += 2;
  }

  //set label to filename
  size_t len = strlen(path);
  memcpy(curr_palette.name, path, len);
  curr_palette.name[len] = 0;
  lv_label_set_text(palette_label, curr_palette.name);

  save_to_sd((const uint8_t*)path, len, "/last_palette");

  //display palette colors
  //we limit to 32 colors as anymore seems to cause memory issues
  for (int i = 0; i < min((uint8_t)32, curr_palette.len); ++i) {
    if (curr_palette.leds[i] == nullptr) {
      curr_palette.leds[i] = lv_obj_create(ui_pages[UIScreen::MAIN]);
      lv_obj_set_size(curr_palette.leds[i], 8, 8);
      lv_obj_align(curr_palette.leds[i], LV_ALIGN_LEFT_MID, 70 + (8 * i) % 80, 95 + ((i*8) / 80) * 8);
      lv_obj_add_style(curr_palette.leds[i], &curr_palette.style, 0);
    }
    lv_obj_clear_flag(curr_palette.leds[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(curr_palette.leds[i], lv_color_make(curr_palette.p[i].r, curr_palette.p[i].g, curr_palette.p[i].b), LV_PART_MAIN);
  }
  for (int i = curr_palette.len; i < 256; ++i) {
    if (curr_palette.leds[i] != nullptr) lv_obj_add_flag(curr_palette.leds[i], LV_OBJ_FLAG_HIDDEN);
  }

  Serial.println("Palette loaded");
  return true;
}

Color HsvToRgb(const Color& hsv)
{
    Color rgb;
    unsigned char region, remainder, p, q, t;
    
    if (hsv.g == 0)
    {
        rgb.r = hsv.b;
        rgb.g = hsv.b;
        rgb.b = hsv.b;
        return rgb;
    }
    
    region = hsv.r / 43;
    remainder = (hsv.r - (region * 43)) * 6; 
    
    p = (hsv.b * (255 - hsv.g)) >> 8;
    q = (hsv.b * (255 - ((hsv.g * remainder) >> 8))) >> 8;
    t = (hsv.b * (255 - ((hsv.g * (255 - remainder)) >> 8))) >> 8;
    
    switch (region)
    {
        case 0:
            rgb.r = hsv.b; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.b; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.b; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.b;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.b;
            break;
        default:
            rgb.r = hsv.b; rgb.g = p; rgb.b = q;
            break;
    }
    
    return rgb;
}

Color RgbToHsv(const Color& rgb)
{
    Color hsv;
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);
    
    hsv.b = rgbMax;
    if (hsv.b == 0)
    {
        hsv.r = 0;
        hsv.g = 0;
        return hsv;
    }

    hsv.g = 255 * long(rgbMax - rgbMin) / hsv.b;
    if (hsv.g == 0)
    {
        hsv.r = 0;
        return hsv;
    }

    if (rgbMax == rgb.r)
        hsv.r = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
    else if (rgbMax == rgb.g)
        hsv.r = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
    else
        hsv.r = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

    return hsv;
}

static long get_weight(const Color& px, const Color& pal) {
    //nearest level
    long r_d = (long)px.r - (long)pal.r;
    long g_d = (long)px.g - (long)pal.g;
    long b_d = (long)px.b - (long)pal.b;
    return ((abs(r_d)+abs(g_d)+abs(b_d))/3)*10000;
}

static void process_img(uint8_t* img, size_t buf_sz) {
  Color* buf = (Color*)img;

  for (size_t i = 0; i < buf_sz/3; ++i) {
      unsigned long x = i % QVGA_W;
      unsigned long y = i / QVGA_W;

      size_t closest = 0;
      long smallest = INT_MAX;
      for (size_t n = 0; n < curr_palette.len; ++n) {
        long this_dist = get_weight(buf[i], curr_palette.p[n]);
        if (this_dist < smallest) {
          smallest = this_dist;
          closest = n;
        }
      }

      buf[i].b = curr_palette.p[closest].r;
      buf[i].g = curr_palette.p[closest].g;
      buf[i].r = curr_palette.p[closest].b;
    }
}

void upscale_nn(uint8_t* og, uint8_t* out, int w, int h, int scale) {
  Color* ogc = (Color*)og;
  Color* outc = (Color*)out;
  int new_h = scale * h;
  int new_w = scale * w;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      for (int n = 0; n < scale; ++n) {
        outc[(y * scale) * (w * scale) + (scale * x) + n] = ogc[y * w + x];
      }
    }
  }

  for (int y = 0; y < new_h; y += scale) {
    for (int n = 1; n < scale; ++n) {
      memcpy(outc + (y+n) * new_w, outc + y * new_w, new_w * sizeof(Color));
    }
  }
}

static uint8_t* scale(uint8_t* old_img, size_t w, size_t h) {
  if (!old_img) return nullptr;
  const int s = curr_scale_factor;
  uint8_t* new_img = (uint8_t*)malloc(w * s * h * s * 3 * sizeof(uint8_t));
  upscale_nn(old_img, new_img, w, h, s);
  return (uint8_t*)new_img;
}



static void rgb888_to_rgb565(uint8_t* input_buf, const unsigned int im_height, const unsigned int im_width, uint8_t* output_buf)
{
	unsigned int i;
	unsigned int r,g,b;
	unsigned char x1,x2;
	
	for(i=0; i <im_height*im_width ; i++)
	{
		r = ((Color*)input_buf)[i].b;
    g = ((Color*)input_buf)[i].g;
    b = ((Color*)input_buf)[i].r;
	
		x1=(r & 0xF8) | (g >> 5);
        x2=((g & 0x1C) << 3) | (b  >> 3);

#if (LV_COLOR_16_SWAP != 0)
    *(output_buf++) = x1;
    *(output_buf++) = x2;
#else
    *(output_buf++) = x2;
    *(output_buf++) = x1;
#endif
	}
}

#endif