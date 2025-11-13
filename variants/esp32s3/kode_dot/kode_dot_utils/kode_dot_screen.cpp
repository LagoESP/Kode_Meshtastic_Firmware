/*********************************************************************
 *  Meshtastic UI Colour Palette – adapted for Arduino_GFX
 *
 *  The original Meshtastic UI (web / mobile) defines a handful of
 *  semantic colours that are used throughout the app.  For an embedded
 *  display we simply map those names to RGB565 values that can be fed
 *  into `Arduino_GFX`.
 *********************************************************************/

#include "kode_dot_screen.h"
#include "kode_dot_logo_meshtastic.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/* ────────────────────── Pin / resolution config ────────────────────── */
#define DSP_HOR_RES 410
#define DSP_VER_RES 502

/* QSPI pins – keep the same order you already use */
#define DSP_SCLK   17
#define DSP_SDIO0  15
#define DSP_SDIO1  14
#define DSP_SDIO2  16
#define DSP_SDIO3  10

/* Control pins */
#define DSP_RST    8
#define DSP_CS     9
#define DSP_BCKL   22

/* ────────────────────── Global objects ────────────────────── */
static Arduino_DataBus *gfxBus = nullptr;
static Arduino_CO5300 *gfx     = nullptr;

/* ────────────────────── Meshtastic UI colour palette (RGB565) ───────
 *
 * These values were taken from the Meshtastic web‑app CSS and converted
 * to 16‑bit RGB565.  Feel free to tweak them if you want a slightly
 * different shade – just keep the names.
 */
#define MT_COLOR_PRIMARY      0x6F52   // #1e90ff – blue (used for background)
#define MT_COLOR_ACCENT       0x2967   // #ffa500 – orange (used for highlights)
#define MT_COLOR_TEXT_LIGHT   0xFFFF   // white
#define MT_COLOR_TEXT_DARK    0x0000   // black

/* ────────────────────── Helper function to release only QSPI data pins ────────────────────── */
static void releaseQSPIDataPins() {
    // Only release the QSPI data pins that might conflict with radio
    // Keep control pins (CS, RST, BCKL) as they are to maintain display state
    
    // Set QSPI data pins to INPUT (high-impedance) to release them
    pinMode(DSP_SCLK,   INPUT);
    pinMode(DSP_SDIO0,  INPUT);
    pinMode(DSP_SDIO1,  INPUT);
    pinMode(DSP_SDIO2,  INPUT);
    pinMode(DSP_SDIO3,  INPUT);
    
    // Keep these pins configured as they are needed to maintain display state:
    // - DSP_CS: Keep as OUTPUT (but we won't actively use it)
    // - DSP_RST: Keep as OUTPUT (display needs reset line stable)
    // - DSP_BCKL: Keep as OUTPUT (backlight needs to stay on)
    
    // Small delay to let pins settle
    vTaskDelay(pdMS_TO_TICKS(10));
}
/* ────────────────────── Display task ────────────────────── */
static void displayTask(void *param) {
    Serial.println("[Display] Inicializando pantalla...");

    // Add a small delay to let other tasks run first
    vTaskDelay(pdMS_TO_TICKS(100));

    gfxBus = new Arduino_ESP32QSPI(DSP_CS, DSP_SCLK,
                                  DSP_SDIO0, DSP_SDIO1,
                                  DSP_SDIO2, DSP_SDIO3);
    gfx = new Arduino_CO5300(gfxBus, DSP_RST, 0, 0,
                            DSP_HOR_RES, DSP_VER_RES,
                            DSP_BCKL, 0, 0, 0);

    if (!gfx->begin()) {
      Serial.println("[Display] ❌ Error al iniciar el display");
      vTaskDelete(nullptr);
      return;
    }

    // Yield after heavy initialization
    vTaskDelay(pdMS_TO_TICKS(10));

    gfx->setRotation(0);
    gfx->setBrightness(80);
    gfx->displayOn();
    Serial.println("[Display] ✅ Pantalla inicializada");

    gfx->fillScreen(MT_COLOR_PRIMARY);
    // gfx->draw16bitRGBBitmap(0, 0, meshtastic_logo, meshtastic_logo_width, meshtastic_logo_height);
    
    // Add your text here (using hardcoded positions as before)
    int16_t height = DSP_VER_RES;
    int16_t lowerThirdY = (2 * height) / 3;
    gfx->fillRect(0, lowerThirdY, DSP_HOR_RES, height - lowerThirdY, MT_COLOR_PRIMARY);
    gfx->setTextColor(MT_COLOR_ACCENT);
    gfx->setTextWrap(false);
    gfx->setTextSize(3);
    gfx->setCursor(95, lowerThirdY + 20);
    gfx->print("Kode Dot Edition");
    gfx->setTextSize(2);
    gfx->setCursor(115, lowerThirdY + 50);
    gfx->print("Adapted by @Lagortinez");

    // Yield to ensure drawing completes
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Release QSPI bus pins (but keep display objects alive)
    // This allows the display to keep showing the image while freeing pins for radio
    releaseQSPIDataPins();
    
    Serial.println("[Display] ✅ Pines QSPI liberados, pantalla sigue activa");
    // DO NOT delete display objects - keep them alive
    // DO NOT call vTaskDelete - let this task stay alive to maintain display
    
    // Keep task alive indefinitely to maintain display state
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep forever, keeping display objects alive
    }
}
/* ────────────────────── Startup helper ────────────────────── */
void startDisplayTask() {
#if CONFIG_FREERTOS_UNICORE
  xTaskCreate(displayTask, "DisplayTask", 8192, nullptr, 1, nullptr);
#else
  xTaskCreatePinnedToCore(displayTask,
                          "DisplayTask",
                          8192,
                          nullptr,
                          1,
                          nullptr,
                          APP_CPU_NUM);
#endif
}
