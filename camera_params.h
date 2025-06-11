#ifndef CAMERA_H
#define CAMERA_H
#include <lvgl.h>
#include <stdio.h>
#include "esp_camera.h"

static void save_settings() {
  int8_t buf[14];
  sensor_t * info = esp_camera_sensor_get();
  buf[0] = (int8_t)info->status.ae_level;
  buf[1] = (int8_t)info->status.aec;
  buf[2] = (int8_t)info->status.agc_gain;
  buf[3] = (int8_t)info->status.agc;
  buf[4] = (int8_t)info->status.brightness;
  buf[5] = (int8_t)info->status.contrast;
  buf[6] = (int8_t)info->status.saturation;
  buf[7] = (int8_t)info->status.sharpness;
  buf[8] = (int8_t)info->status.wb_mode;
  buf[9] = 0; //unused
  buf[10] = (int8_t)dither_pattern;
  buf[11] = (int8_t)dither_strength;
  buf[12] = 0; //unused
  save_to_sd((uint8_t*)buf, 12, "/settings");
}

static void load_settings() {
  unsigned long sz = 0;
  sensor_t * info = esp_camera_sensor_get();
  int8_t* buf = (int8_t*)load_file_from_sd("/settings", &sz, false);
  if (!buf) return;
  if (sz > 0 && info->set_ae_level) info->set_ae_level(info, buf[0]);
  if (sz > 1 && info->set_exposure_ctrl) info->set_exposure_ctrl(info, buf[1]);
  if (sz > 2 && info->set_agc_gain) info->set_agc_gain(info, buf[2]);
  if (sz > 3 && info->set_gain_ctrl) info->set_gain_ctrl(info, buf[3]);
  if (sz > 4 && info->set_brightness) info->set_brightness(info, buf[4]);
  if (sz > 5 && info->set_contrast) info->set_contrast(info, buf[5]);
  if (sz > 6 && info->set_saturation) info->set_saturation(info, buf[6]);
  if (sz > 7 && info->set_sharpness) info->set_sharpness(info, buf[7]);
  if (sz > 8 && info->set_wb_mode) info->set_wb_mode(info, buf[8]);
  if (sz > 10) dither_pattern = buf[10];
  if (sz > 11) dither_strength = buf[11];
  free(buf);
}

static void on_ae_lvl(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_ae_level) {
    info->set_ae_level(info, lv_slider_get_value(slider));
    Serial.println("on_ae_lvl");
    save_settings();
  }
}

static void on_ae_on(lv_event_t* e) {
  lv_obj_t* cb = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_exposure_ctrl) {
    info->set_exposure_ctrl(info, lv_obj_has_state(cb, LV_STATE_CHECKED) ? 1 : 0);
    Serial.println("on_ae_on");
    save_settings();
  }
}

static void on_ag_lvl(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_agc_gain) {
    info->set_agc_gain(info, lv_slider_get_value(slider));
    Serial.println("on_ag_lvl");
    save_settings();
  }
}

static void on_ag_on(lv_event_t* e) {
  lv_obj_t* cb = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_gain_ctrl) {
    info->set_gain_ctrl(info, lv_obj_has_state(cb, LV_STATE_CHECKED) ? 1 : 0);
    Serial.println("on_ag_on");
    save_settings();
  }
}

static void on_brightness(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_brightness) {
    info->set_brightness(info, lv_slider_get_value(slider));
    Serial.println("on_brightness");
    save_settings();
  }
}


static void on_contrast(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_contrast) {
    info->set_contrast(info, lv_slider_get_value(slider));
    Serial.println("on_contrast");
    save_settings();
  }
}

static void on_saturation(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_saturation) {
    info->set_saturation(info, lv_slider_get_value(slider));
    Serial.println("on_saturation");
    save_settings();
  }
}

static void on_sharpness(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_sharpness) {
    info->set_sharpness(info, lv_slider_get_value(slider));
    Serial.println("on_sharpness");
    save_settings();
  }
}
static void on_dither_strength(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  dither_strength = lv_slider_get_value(slider);
  Serial.println("on_dither_strength");
  save_settings();
}

static void on_dither_patt(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  dither_pattern = lv_slider_get_value(slider);
  Serial.println("on_dither_patt");
  save_settings();
}

static void update_wb_label(uint8_t mode) {
  switch(mode) {
    case 1: 
    lv_label_set_text(wb_label, "White Bal: Sunny");
    break;
    case 2: 
    lv_label_set_text(wb_label, "White Bal: Cloudy");
    break;
    case 3: 
    lv_label_set_text(wb_label, "White Bal: Office");
    break;
    case 4: 
    lv_label_set_text(wb_label, "White Bal: Home");
    break;
    default: 
    lv_label_set_text(wb_label, "White Bal: Auto");
    break;
  }
}

static void on_wb(lv_event_t* e) {
  lv_obj_t* slider = lv_event_get_target(e);
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_wb_mode) {
    info->set_wb_mode(info, lv_slider_get_value(slider));
    Serial.println("on_wb");
    update_wb_label((uint8_t)lv_slider_get_value(slider));
    save_settings();
  }
}

static void on_cam_reset(lv_event_t* e) {
  sensor_t * info = esp_camera_sensor_get();
  if (info->set_ae_level) info->set_ae_level(info, 0);
  if (info->set_exposure_ctrl) info->set_exposure_ctrl(info, 1);
  if (info->set_gain_ctrl) info->set_gain_ctrl(info, 1);
  if (info->set_agc_gain) info->set_agc_gain(info, 0);
  if (info->set_brightness) info->set_brightness(info, 0);
  if (info->set_contrast) info->set_contrast(info, 0);
  if (info->set_saturation) info->set_saturation(info, 0);
  if (info->set_sharpness) info->set_sharpness(info, 0);
  if (info->set_wb_mode) info->set_wb_mode(info, 0);
  dither_strength = 0;
  dither_pattern = 0;
  save_settings();
}

static void init_settings_page() {
  sensor_t * info = esp_camera_sensor_get();
  if (!info) {
    Serial.println("no sensor");
    return;
  }

  load_settings();

  lv_obj_t* list = lv_list_create(ui_pages[UIScreen::CAMERA_PARAMS]);
  lv_obj_set_size(list, LCD_H_RES, LCD_V_RES);
  lv_obj_center(list);
  lv_obj_set_style_pad_row(list, 10, 00);
  lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLL_MOMENTUM);

  lv_obj_t* btn = lv_btn_create(list);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT,0,0);
  lv_obj_add_event_cb(btn, on_return_to_main, LV_EVENT_CLICKED, NULL);
  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, "Back");
  lv_obj_center(label);

  lv_obj_t * slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,-5,5);
  lv_slider_set_value(slider, (int)info->status.ae_level, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_ae_lvl, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_t * slider_label = lv_label_create(list);
  lv_label_set_text(slider_label, "Exposure Lvl");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);
  lv_list_add_text(list, "");

  lv_obj_t * cb;
  cb = lv_checkbox_create(list);
  lv_checkbox_set_text(cb, "Auto Exposure On");
  if (info->status.aec) lv_obj_add_state(cb, LV_STATE_CHECKED);
  lv_obj_add_event_cb(cb, on_ae_on, LV_EVENT_VALUE_CHANGED, NULL);
  lv_list_add_text(list, "");

  slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,0,64);
  lv_slider_set_value(slider, (int)info->status.agc_gain, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_ag_lvl, LV_EVENT_VALUE_CHANGED, NULL);
  slider_label = lv_label_create(list);
  lv_label_set_text(slider_label, "Gain Lvl");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);
  lv_list_add_text(list, "");
  
  cb = lv_checkbox_create(list);
  lv_checkbox_set_text(cb, "Auto Gain On");
  if (info->status.agc) lv_obj_add_state(cb, LV_STATE_CHECKED);
  lv_obj_add_event_cb(cb, on_ag_on, LV_EVENT_VALUE_CHANGED, NULL);
  lv_list_add_text(list, "");

  slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,-3,3);
  lv_slider_set_value(slider, (int)info->status.brightness, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_brightness, LV_EVENT_VALUE_CHANGED, NULL);
  slider_label = lv_label_create(list);
  lv_label_set_text(slider_label, "Brightness");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);
  lv_list_add_text(list, "");

  slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,-3,3);
  lv_slider_set_value(slider, (int)info->status.contrast, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_contrast, LV_EVENT_VALUE_CHANGED, NULL);
  slider_label = lv_label_create(list);
  lv_label_set_text(slider_label, "Contrast");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);
  lv_list_add_text(list, "");

  slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,-4,4);
  lv_slider_set_value(slider, (int)info->status.saturation, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_saturation, LV_EVENT_VALUE_CHANGED, NULL);
  slider_label = lv_label_create(list);
  lv_label_set_text(slider_label, "Saturation");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -10);
  lv_list_add_text(list, "");

  slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,-3,3);
  lv_slider_set_value(slider, (int)info->status.sharpness, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_sharpness, LV_EVENT_VALUE_CHANGED, NULL);
  slider_label = lv_label_create(list);
  lv_label_set_text(slider_label, "Sharpness");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -30);
  lv_list_add_text(list, "");

  slider = lv_slider_create(list);
  lv_obj_center(slider);
  lv_obj_set_size(slider, LCD_H_RES-20, 10);
  lv_slider_set_range(slider,0,4);
  lv_slider_set_value(slider, (int)info->status.wb_mode, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, on_wb, LV_EVENT_VALUE_CHANGED, NULL);
  wb_label = lv_label_create(list);
  lv_obj_align_to(wb_label, slider, LV_ALIGN_OUT_TOP_MID, 0, -30);
  update_wb_label(info->status.wb_mode);
  lv_list_add_text(list, "");
}

#endif