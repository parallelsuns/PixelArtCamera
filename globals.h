#ifndef INCLUDEH
#define INCLUDEH

#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "bsp_cst816.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "USB.h"
#include "USBMSC.h"

#define INT_MAX 21474836

//GPIO Defs
#define PWDN_GPIO_NUM 17   //power down is not used
#define RESET_GPIO_NUM -1  //software reset will be performed
#define XCLK_GPIO_NUM 8
#define SIOD_GPIO_NUM 21
#define SIOC_GPIO_NUM 16
#define Y9_GPIO_NUM 2
#define Y8_GPIO_NUM 7
#define Y7_GPIO_NUM 10
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 11
#define Y4_GPIO_NUM 15
#define Y3_GPIO_NUM 13
#define Y2_GPIO_NUM 12
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 4
#define PCLK_GPIO_NUM 9
#define PIN_NUM_LCD_SCLK 39
#define PIN_NUM_LCD_MOSI 38
#define PIN_NUM_LCD_MISO 40
#define PIN_NUM_LCD_DC 42
#define PIN_NUM_LCD_RST -1
#define PIN_NUM_LCD_CS 45
#define PIN_NUM_SD_SCLK 39
#define PIN_NUM_SD_MOSI 38
#define PIN_NUM_SD_MISO 40
#define PIN_NUM_SD_CS 41
#define PIN_NUM_LCD_BL 1
#define PIN_NUM_TP_SDA 48
#define PIN_NUM_TP_SCL 47
#define LCD_ROTATION 0
#define LEDC_FREQ 5000
#define LEDC_TIMER_10_BIT 10

//LCD RES
#define LCD_H_RES 240
#define LCD_V_RES 320
//Camera Res
#define QVGA_W 240
#define QVGA_H 176

//mutex for lvgl's render loop
static SemaphoreHandle_t lvgl_api_mux = NULL;

//LCD SPI Bus and Wrapper Class
static Arduino_DataBus *bus = new Arduino_ESP32SPI(
  PIN_NUM_LCD_DC /* DC */, PIN_NUM_LCD_CS /* CS */,
  PIN_NUM_LCD_SCLK /* SCK */, PIN_NUM_LCD_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, FSPI, true);
static Arduino_GFX *gfx = new Arduino_ST7789(
  bus, PIN_NUM_LCD_RST /* RST */, LCD_ROTATION /* rotation */, true /* IPS */,
  LCD_H_RES /* width */, LCD_V_RES /* height */);

//USB Mass Storage Controller
static USBMSC MSC;

enum UIScreen {
  MAIN,
  PALETTE_LIST,
  ALBUM, //unimplemented
  CAMERA_PARAMS,
  IMG_FX, //unimplemented
  INVALID,
};

//ui page objects
static lv_obj_t* ui_pages[UIScreen::INVALID];
static UIScreen current_ui_page = UIScreen::MAIN;

//main page ui objects
static lv_obj_t* img_camera;
static lv_obj_t* batt_label;
static lv_obj_t* palette_label;
static lv_obj_t* album_img;
static float batt_voltage = 0; //current battery voltage estimate
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
static bool capture_pressed = false; //true when capture button is pressed, back to false after img processed
static uint8_t* processed_img_disp_buf = nullptr;
static bool showing_captured = false;

//camera params page ui objects
static lv_obj_t * wb_label;

//image processing vars
static int curr_scale_factor = 4; //img is scaled by this factor before saving to jpg, beyond 4 we run the risk of running out of memory
static int dither_pattern = 0; //unimplemented
static int8_t dither_strength = 0; //unimplemented

//sd card saving vars
static bool sd_active = false;
static int curr_file_index = 0;

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct Palette { 
  uint8_t len;
  Color p[256];
  char name[256];
  lv_obj_t* leds[256];
  lv_style_t style;
};

static Palette curr_palette;

//locks lvgl mutex
static bool lvgl_lock(int timeout_ms) {
  // Convert timeout in milliseconds to FreeRTOS ticks
  // If `timeout_ms` is set to -1, the program will block until the condition is met
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTakeRecursive(lvgl_api_mux, timeout_ticks) == pdTRUE;
}

//unlock lvgl mutex
static void lvgl_unlock(void) {
  xSemaphoreGiveRecursive(lvgl_api_mux);
}

static void on_return_to_main(lv_event_t* e) {
  lv_scr_load(ui_pages[UIScreen::MAIN]);
}

static void sd_activate() {
  lvgl_lock(-1);
  sd_active = true;
  bus->endWrite();
  digitalWrite(PIN_NUM_LCD_CS, HIGH);
  digitalWrite(PIN_NUM_SD_CS, LOW);
  vTaskDelay(pdMS_TO_TICKS(10));
}

static void sd_deactivate() {
  digitalWrite(PIN_NUM_SD_CS, HIGH);
  digitalWrite(PIN_NUM_LCD_CS, LOW);
  SPI.endTransaction();
  lvgl_unlock();
  sd_active = false;
}

#endif