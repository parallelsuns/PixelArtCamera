#ifndef SD_FNS
#define SD_FNS
#include "globals.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "USB.h"
#include "USBMSC.h"
#include "lv_conf.h"
#include "diskio.h"

//Wrapper exposes the _pdrv value
//which we need to call the underlying c fns, ff_sd_read, ff_sd_write
class SDWrapper : public fs::SDFS {
  public:
  uint8_t get_pdrv() const {
    return _pdrv;
  }
};

DRESULT ff_sd_read(uint8_t pdrv, uint8_t *buffer, DWORD sector, UINT count);
DRESULT ff_sd_write(uint8_t pdrv, const uint8_t *buffer, DWORD sector, UINT count);

static void process_img(uint8_t* img, size_t buf_sz);
static uint8_t* scale(uint8_t* img, size_t w, size_t h);
static void rgb888_to_rgb565(uint8_t* input_buf, const unsigned int im_height, const unsigned int im_width, uint8_t* output_buf);

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
  if (ff_sd_write(((const SDWrapper*)&SD)->get_pdrv(), (uint8_t*)buffer, lba, bufsize/SD.sectorSize()) == ESP_OK) return bufsize;
  else return 0;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
  if (ff_sd_read(((const SDWrapper*)&SD)->get_pdrv(), (uint8_t*)buffer, lba, bufsize/SD.sectorSize()) == ESP_OK) return bufsize;
  else return 0;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  if (DEBUG) Serial.printf("MSC START/STOP: power: %u, start: %u, eject: %u\n", power_condition, start, load_eject);
  //lvgl_unlock();
  return true;
}


static bool file_exists(const char* path) {
  sd_activate();
  bool res = SD.exists(path);
  sd_deactivate();
  return res;
}

static uint8_t* load_file_from_sd(const char* path, unsigned long* size, bool is_palette = false) {
  sd_activate();

  if (!SD.exists(path)) {
    sd_deactivate();
    return nullptr;
  }

  File file = SD.open(path, FILE_READ);
  if (DEBUG) Serial.println("Opening ");
  if (DEBUG) Serial.println(path);

  if (!file) {
    Serial.println("File Open Failed: ");
    sd_deactivate();
    return nullptr;
  }

  unsigned long filesize = file.size();
  //palette files are 8bytes per color, max 256 colors.
  //we check for 300 in case of add. data.
  if (filesize == 0 || (is_palette && filesize >= 8 * 300)) {
    Serial.println("File size invalid");
    sd_deactivate();
    return nullptr;
  }

  uint8_t* buf = (uint8_t*)malloc(filesize);
  long bytesread = file.read(buf, filesize);
  if (bytesread == -1) {
    free(buf);
    Serial.println("File read failed");
    sd_deactivate();
    return nullptr;
  }

  sd_deactivate();
  *size = filesize;
  return buf;
}

static void find_next_file_index() { 
  sd_activate();

  char str_buf[256];
  unsigned long size;
  uint8_t* buf = load_file_from_sd("/next", &size);
  if (buf == nullptr) { 
    curr_file_index = 1;
    sd_deactivate();
    return;
  }

  int highest = 0;
  memcpy(str_buf, buf, min(size, (unsigned long)255));
  str_buf[min(size,(unsigned long)255)] = 0;
  highest = strtol(str_buf, nullptr, 10);

  curr_file_index = highest;
  Serial.println("Last File Index: ");
  Serial.println(curr_file_index);

  sd_deactivate();
}

static void save_to_sd(const uint8_t* buf, size_t buf_sz, const char* path) {
  sd_activate();

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  if (DEBUG) Serial.println("SD Card Size: ");
  if (DEBUG) Serial.println(cardSize);

  File file = SD.open(path, FILE_WRITE);
  if (DEBUG) Serial.println("Opening ");
  if (DEBUG) Serial.println(path);
  if (!file) {
    Serial.println("File Open Failed: ");
    sd_deactivate();
    return;
  }
  size_t written = file.write(buf, buf_sz);
  if (written < buf_sz) {
    Serial.println("File Write did not complete. Total Bytes: ");
    Serial.println(buf_sz);
    Serial.println("Bytes Written: ");
    Serial.println(written);
    file.flush();
    file.close();
    sd_deactivate();
    return;
  }
  file.flush();
  file.close();
  sd_deactivate();
}

static void save_to_sd(camera_fb_t * pic) {
  lvgl_lock(-1);
  sd_active = true;
  uint index = curr_file_index+1;

  size_t width = pic->width;
  size_t height = pic->height;
  size_t rgb888_buf_sz = width*height*3*sizeof(uint8_t);
  if (!pic) {
    Serial.println("Failed to get camera buffer");
    sd_active = false;
    return;
  }
  uint8_t* rgb888buf = (uint8_t*)malloc(rgb888_buf_sz);
  bool res = fmt2rgb888(pic->buf, pic->len, pic->format, rgb888buf);
  //esp_camera_fb_return(pic);
  if (!res) {
    free(rgb888buf);
    Serial.println("Failed to convert to rgb888");
    sd_active = false;
    return;
  }

  process_img(rgb888buf, rgb888_buf_sz);
  rgb888_to_rgb565(rgb888buf, width, height, processed_img_disp_buf);

  uint8_t* scaled_buf = scale(rgb888buf, width, height);
  free(rgb888buf);

  uint8_t* jpg_buf;
  size_t jpg_buf_sz;
  res = fmt2jpg(scaled_buf, width*curr_scale_factor*height*curr_scale_factor*3*sizeof(uint8_t), width*curr_scale_factor, height*curr_scale_factor, PIXFORMAT_RGB888, 88, &jpg_buf, &jpg_buf_sz);
  if (!res) {
    free(scaled_buf);
    Serial.println("Failed to convert to jpg");
    sd_active = false;
    return;
  }
  free(scaled_buf);

  bus->endWrite();
  digitalWrite(PIN_NUM_LCD_CS, HIGH);
  digitalWrite(PIN_NUM_SD_CS, LOW);
  vTaskDelay(pdMS_TO_TICKS(10));

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  if (DEBUG) Serial.println("SD Card Size: ");
  if (DEBUG) Serial.println(cardSize);

  char buf[64];
  buf[0] = '/';
  itoa(index, buf+1, 10);
  strcat(buf+1, ".jpg");

  while (SD.exists(buf) && index < 10000) {
    ++index;
    itoa(index, buf+1, 10);
    strcat(buf+1, ".jpg");
  }

  File file = SD.open(buf, FILE_WRITE);
  if (DEBUG) Serial.println("Opening ");
  if (DEBUG) Serial.println(buf);
  if (!file) {
    Serial.println("File Open Failed: ");
    sd_active = false;
    free(jpg_buf);
    digitalWrite(PIN_NUM_SD_CS, HIGH);
    digitalWrite(PIN_NUM_LCD_CS, LOW);
    SPI.endTransaction();
    lvgl_unlock();
    return;
  }
  size_t written = file.write(jpg_buf, jpg_buf_sz);
  if (written < jpg_buf_sz) {
    Serial.println("File Write did not complete. Total Bytes: ");
    Serial.println(jpg_buf_sz);
    Serial.println("Bytes Written: ");
    Serial.println(written);
    file.close();
    free(jpg_buf);
    sd_active = false;
    digitalWrite(PIN_NUM_SD_CS, HIGH);
    digitalWrite(PIN_NUM_LCD_CS, LOW);
    SPI.endTransaction();
    lvgl_unlock();
    return;
  }
  file.flush();
  file.close();
  vTaskDelay(pdMS_TO_TICKS(50));

  File index_file = SD.open("/next", FILE_WRITE);
  if (index_file) {
    index_file.print(++curr_file_index, 10);
    index_file.close();
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  SPI.endTransaction();
  vTaskDelay(pdMS_TO_TICKS(25));
  digitalWrite(PIN_NUM_SD_CS, HIGH);
  digitalWrite(PIN_NUM_LCD_CS, LOW);
  showing_captured = true;
  vTaskDelay(pdMS_TO_TICKS(5));
  free(jpg_buf);
  sd_active = false;
  lvgl_unlock();
}
#endif