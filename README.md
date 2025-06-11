Heavily inspired by the [Pixless Camera](https://www.kickstarter.com/projects/carloandreini/pixless-camera), the Pixel Art Camera is a portable ‘toy’ camera that takes lo-resolution photos and posterizes the pixel colors to a small limited color palette, typically used for pixel art. This creates photos that resemble retro pixel art.

The camera supports .hex color palette files downloadable from https://lospec.com/palette-list.

The camera is built with a Waveshare ESP32-S3-Touch-LCD-2 development board, an OV5640 camera module and a 400 mAh Li-Po battery with a 3d printed enclosure.

For example images: https://parallelsuns.com/pixel-art-camera/

The code isn't very clean as this was just a fun side-project. You have been warned! 

Also, I did not bother with computing an accurate battery percentage based on the discharge curve or anything so the battery percentage indicator is VERY VERY approximate.

## Building

Build using Arduino IDE 2.3.6. You should set up the build environment following Waveshare's instructions provided here: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2#Environment_Setup.

Ensure you are able to build the example projects provided.

You'll need the cst816 library, which can be found as part of the 'offline component package' linked at the bottom of the page linked above.
## Requirements

| Library                                                                                     | Version | Remarks                                              |
| ------------------------------------------------------------------------------------------- | ------- | ---------------------------------------------------- |
| [LVGL](https://lvgl.io/)                                                                    | 8.4.0   |                                                      |
| [Arduino_GFX](https://github.com/moononournation/Arduino_GFX)                               | 1.5.9   |                                                      |
| [bsp_cst816](https://drive.google.com/drive/folders/1Pcs_A4FKWvdSHnz9lEBYqOpr-noTMbIv)      | N.A.    | Not available in Arduino IDE. Provided by Waveshare. |

## BOM

| Part                                                     | Source                                                                                                                               | Remarks                                                                                                               |
| -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------- |
| Waveshare ESP32-S3-Touch-LCD-2-C                         | https://www.waveshare.com/product/arduino/boards-kits/esp32/esp32-s3-touch-lcd-2.htm?sku=29668                                       |                                                                                                                       |
| OV5640 Camera Module                                     | https://www.waveshare.com/product/arduino/boards-kits/esp32/esp32-s3-touch-lcd-2.htm?sku=29668                                       | (Optionally bundled with the ESP32-S3-Touch-LCD-2-C from above URL)                                                   |
| 400 mAh 3.7v Li-Po Battery (dimensions approx 48x16x5mm) | https://kuriosity.sg/products/3-7v-li-po-lithium-polymer-battery-100mah-400mah-600mah-1600mah-2000mah-5000mah?variant=50332363096377 | Any Li-Po battery works. If you wish to use the 3d printed case STL the battery should fit within the give dimensions |
| SPST Rocker Switch (mounting hole 13x9mm)                | https://www.amazon.com/DIYhz-Environmental-Protection-Electrical-Products/dp/B07BPKZ2RG                                              | Switch should match given mounting dimensions to work with the 3d printed case STL                                    |
| 3d Printed Enclosure                                     | https://github.com/parallelsuns/PixelArtCamera/blob/main/PixelArtCamera_enclosure.stl                                                | PLA 15% infill should work just fine                                                                                  |

## Assembly

1. Print 3d printed enclosure.
2. Mount the rocker switch into case.
3. Insert battery into the compartment in the case.
4. Cut 1 lead from the battery and solder the rocker switch in between the 2 halves.
5. Connect the battery to VBAT and G on the ESP32 board.
6. Slide the board into the enclosure. Secure it with adhesive/screws.
7. Compile the firmware and flash it via USB.
