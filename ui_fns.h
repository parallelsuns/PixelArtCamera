#ifndef UI_FNS
#define UI_FNS
#include "lv_conf.h"
#include <lvgl.h>
#include <stdio.h>
#include "sd_fns.h"
#include "img.h"
#include "globals.h"


static void on_capture(lv_event_t * e);
static void on_usb(lv_event_t* e);

static int album_curr_index = 0;
static File album_dir;

static void on_open_palette_list(lv_event_t* e) {
  lv_scr_load(ui_pages[UIScreen::PALETTE_LIST]);
}
static void on_camera(lv_event_t* e) {
  lv_scr_load(ui_pages[UIScreen::CAMERA_PARAMS]);
}

static void init_palette_ui() {
  sd_activate();

  lv_obj_t* list = lv_list_create(ui_pages[UIScreen::PALETTE_LIST]);
  lv_obj_set_size(list, LCD_H_RES, LCD_V_RES);
  lv_obj_center(list);

  const char* path = "/palette/";

  if (!SD.exists(path)) {
    sd_deactivate();
    return;
  }

  File dir = SD.open(path, FILE_READ);

  if (!dir || !dir.isDirectory()) {
    dir.close();
    sd_deactivate();
    return;
  }

  lv_obj_t* btn = lv_btn_create(ui_pages[UIScreen::PALETTE_LIST]);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT,0,0);
  lv_obj_add_event_cb(btn, on_return_to_main, LV_EVENT_CLICKED, NULL);

  lv_obj_t * label = lv_label_create(btn);
  lv_label_set_text(label, "BACK");

  while(1) {
    File file = dir.openNextFile();
    if (!file) {
      dir.close();
      sd_deactivate();
      return;
    }
    lv_obj_t* btn = lv_btn_create(list);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID,0,0);
    lv_obj_add_event_cb(btn, on_choose_palette, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, file.name());

    file.close();
  }
  dir.close();
  sd_deactivate();
}

static void load_last_palette() {
  unsigned long buf_size = 0;
  char str_buf[256];
  uint8_t* buf = load_file_from_sd("/last_palette", &buf_size);
  if (buf == nullptr) return;
  memcpy(str_buf, buf, min((unsigned long)255, buf_size));
  str_buf[buf_size] = 0;
  load_palette(str_buf);
  free(buf);
}

static void init_main_page() {
  lv_obj_t * label;


  lv_obj_t * btn1 = lv_btn_create(ui_pages[UIScreen::MAIN]);
  lv_obj_add_event_cb(btn1, on_camera, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btn1, LV_ALIGN_TOP_LEFT, 0, 0);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "Camera");
  lv_obj_center(label);

  lv_obj_t * btn2 = lv_btn_create(ui_pages[UIScreen::MAIN]);
  lv_obj_add_event_cb(btn2, on_usb, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btn2, LV_ALIGN_TOP_MID, 0, 0);
  label = lv_label_create(btn2);
  lv_label_set_text(label, "USB");
  lv_obj_center(label);

  lv_obj_t * btn4 = lv_btn_create(ui_pages[UIScreen::MAIN]);
  lv_obj_add_event_cb(btn4, on_capture, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btn4, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_height(btn4, lv_pct(20));
  label = lv_label_create(btn4);
  lv_label_set_text(label, "Capture");
  lv_obj_center(label);

  lv_obj_t * btn5 = lv_btn_create(ui_pages[UIScreen::MAIN]);
  lv_obj_add_event_cb(btn5, on_open_palette_list, LV_EVENT_CLICKED, NULL);
  lv_obj_align(btn5, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_height(btn5, lv_pct(20));
  label = lv_label_create(btn5);
  lv_label_set_text(label, "Color");
  lv_obj_center(label);

  batt_label = lv_label_create(ui_pages[UIScreen::MAIN]);
  lv_label_set_text(batt_label, "");
  /*Position the main label*/
  lv_obj_align(batt_label, LV_ALIGN_TOP_RIGHT, -5, 0);

  palette_label = lv_label_create(ui_pages[UIScreen::MAIN]);
  lv_label_set_text(palette_label, "");
  /*Position the main label*/
  lv_obj_align(palette_label, LV_ALIGN_CENTER, 0, -100);
}


#endif