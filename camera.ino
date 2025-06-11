/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */
#include "globals.h"
#include "lv_conf.h"
#include "esp_camera.h"
#include <lvgl.h>
#include <stdio.h>
#include "sd_fns.h"
#include "camera_params.h"
#include "img.h"
#include "ui_fns.h"


static void on_capture(lv_event_t * e) {
  capture_pressed = true;
}

static void on_usb(lv_event_t* e) {
  ((Arduino_ST7789*)gfx)->displayOff();
  lvgl_lock(-1);
  bus->endWrite();
  digitalWrite(PIN_NUM_LCD_CS, HIGH);
  vTaskDelay(pdMS_TO_TICKS(5));

  MSC.vendorID("PllSuns");       //max 8 chars
  MSC.productID("PixArtCam");    //max 16 chars
  MSC.productRevision("1.0");  //max 4 chars
  MSC.onStartStop(onStartStop);
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);

  MSC.mediaPresent(true);
  MSC.isWritable(true);  // true if writable, false if read-only

  Serial.println("Mount USB");
  Serial.println("SD Size: ");
  Serial.println(SD.cardSize());
  Serial.println("Sector Size: ");
  Serial.println(SD.sectorSize());

  vTaskDelay(pdMS_TO_TICKS(10));

  if (!USB.begin()) Serial.println("USB Begin Failed");
  if (!MSC.begin(SD.numSectors() , SD.sectorSize())) Serial.println("USBMC Begin Failed");
  
}

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  if (DEBUG) Serial.printf(buf);
  if (DEBUG) Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, LCD_H_RES, LCD_V_RES);
#else
  gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, LCD_H_RES, LCD_V_RES);
#endif
  lv_disp_flush_ready(disp_drv);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  uint16_t touchpad_x;
  uint16_t touchpad_y;
  bsp_touch_read();

  if (bsp_touch_get_coordinates(&touchpad_x, &touchpad_y)) {
    data->point.x = touchpad_x;
    data->point.y = touchpad_y;
    data->state = LV_INDEV_STATE_PRESSED;
    showing_captured = false;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

}

void lvgl_camera_ui_init(lv_obj_t *parent) {
  img_camera = lv_img_create(parent);
  lv_obj_align(img_camera, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_pos(img_camera, 0, 0);
  lv_obj_set_scroll_dir(parent, LV_DIR_NONE);

  lv_obj_set_style_pad_top(img_camera, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(img_camera, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(img_camera, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(img_camera, 0, LV_PART_MAIN);
}

void update_voltage() {
  char str_buffer[20];
  float voltage;
  uint16_t analogValue = analogRead(5);
  voltage = 3.3 / 4096 * analogValue * 3;
  if (batt_voltage == 0) batt_voltage = voltage;
  else batt_voltage = 0.98 * batt_voltage + voltage * 0.02;
  sprintf(str_buffer, "%d \%", int(((batt_voltage - 3) / 0.7) * 100));
  strcat(str_buffer, "%");
  lv_label_set_text(batt_label, str_buffer);
}

static void task(void *param) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_1;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_HQVGA;
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    vTaskDelete(NULL);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_hmirror(s, 1);

  init_settings_page();

  camera_fb_t *pic;
  lv_img_dsc_t img_dsc;
  img_dsc.header.always_zero = 0;
  img_dsc.header.w = QVGA_W;
  img_dsc.header.h = QVGA_H;
  img_dsc.data_size = QVGA_W * QVGA_H * 2;
  img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  img_dsc.data = NULL;

  while (1) {
    pic = esp_camera_fb_get();

    if (capture_pressed) {
      capture_pressed = false;
      if (!sd_active) {
        save_to_sd(pic);
      }
    }

    if (showing_captured && processed_img_disp_buf) {
      img_dsc.data = processed_img_disp_buf;
      if (lvgl_lock(-1)) {
        lv_img_set_src(img_camera, &img_dsc);
        lvgl_unlock();
      }
    }
    else if (NULL != pic) {
      img_dsc.data = pic->buf;
      if (lvgl_lock(-1)) {
        lv_img_set_src(img_camera, &img_dsc);
        lvgl_unlock();
      }

    }
    update_voltage();

    esp_camera_fb_return(pic);
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  vTaskDelete(NULL);
}

void setup() {

  Serial.begin(115200);
  lvgl_api_mux = xSemaphoreCreateRecursiveMutex();
  Serial.println("Arduino_GFX LVGL_Arduino_v8 example ");
  String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

  processed_img_disp_buf = (uint8_t*)malloc(QVGA_W * QVGA_H * 2);
  
  
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  //init cs pin for sd
  pinMode(PIN_NUM_SD_CS, OUTPUT);
  digitalWrite(PIN_NUM_SD_CS, HIGH);

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  
  }
  //Init SD
  SPI.begin(PIN_NUM_SD_SCLK, PIN_NUM_SD_MISO, PIN_NUM_SD_MOSI, PIN_NUM_SD_CS);
  if (!SD.begin(PIN_NUM_SD_CS, SPI, 80000000))
  {
      Serial.println("SD Card Mount Failed");
      return;
  }
  else Serial.println("SD Card Mount Succeeded.");

#ifdef PIN_NUM_LCD_BL
  ledcAttach(PIN_NUM_LCD_BL, LEDC_FREQ, LEDC_TIMER_10_BIT);
  ledcWrite(PIN_NUM_LCD_BL, (1 << LEDC_TIMER_10_BIT) / 100 * 80);
#endif

  // Init touch device
  Wire.begin(PIN_NUM_TP_SDA, PIN_NUM_TP_SCL);
  bsp_touch_init(&Wire, gfx->getRotation(), gfx->width(), gfx->height());
  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  disp_draw_buf = (lv_color_t *)heap_caps_malloc(LCD_H_RES * LCD_V_RES * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(LCD_H_RES * LCD_V_RES * 2, MALLOC_CAP_8BIT);
  }

  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, LCD_H_RES * LCD_V_RES);
    
    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = gfx->width();
    disp_drv.ver_res = gfx->height();
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.direct_mode = true;

    lv_disp_drv_register(&disp_drv);

    /* Initialize the (dummy) input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
  }

  ui_pages[UIScreen::MAIN] = lv_obj_create(NULL);
  ui_pages[UIScreen::PALETTE_LIST] = lv_obj_create(NULL);
  ui_pages[UIScreen::CAMERA_PARAMS] = lv_obj_create(NULL);

  lvgl_camera_ui_init(ui_pages[UIScreen::MAIN]);
  lv_scr_load(ui_pages[UIScreen::MAIN]);
  gfx->fillScreen(BLACK);
  
  init_main_page();
  init_palette();
  init_palette_ui();
  load_last_palette();
  find_next_file_index();

  Serial.println("Setup done");
  
  xTaskCreatePinnedToCore(
    task,
    "lvgl_app_task",
    1024 * 10,
    NULL,
    1,
    NULL,
    0);
}

void loop() {
  if (lvgl_lock(-1)) {
    lv_timer_handler(); /* let the fxUI do its work */
    lvgl_unlock();
  }
  vTaskDelay(pdMS_TO_TICKS(5));
}
