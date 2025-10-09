/*********************************************************************
 *  Meshtastic UI Colour Palette – adapted for Arduino_GFX
 *
 *  The original Meshtastic UI (web / mobile) defines a handful of
 *  semantic colours that are used throughout the app.  For an embedded
 *  display we simply map those names to RGB565 values that can be fed
 *  into `Arduino_GFX`.
 *********************************************************************/

#include "kode_dot_screen.h"
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
static Arduino_CO5300 *gfx      = nullptr;

/* ────────────────────── Meshtastic UI colour palette (RGB565) ───────
 *
 * These values were taken from the Meshtastic web‑app CSS and converted
 * to 16‑bit RGB565.  Feel free to tweak them if you want a slightly
 * different shade – just keep the names.
 */
#define MT_COLOR_PRIMARY      0x1E90FF   // #1e90ff – blue (used for background)
#define MT_COLOR_ACCENT       0xFFA500   // #ffa500 – orange (used for highlights)
#define MT_COLOR_TEXT_LIGHT   0xFFFFFF   // white
#define MT_COLOR_TEXT_DARK    0x000000   // black
#define MT_COLOR_SECONDARY    0x00CED1   // #00ced1 – turquoise (optional)

/* ────────────────────── Display task ────────────────────── */
static void displayTask(void *param) {
  Serial.println("[Display] Inicializando pantalla...");

  /* Create QSPI bus and the CO5300 driver instance */
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

  gfx->setRotation(0);
  gfx->setBrightness(255);   // full brightness
  gfx->displayOn();
  Serial.println("[Display] ✅ Pantalla inicializada");

  /* Main refresh loop */
  for (;;) {
    /* ── Fondo azul (primary) ─────────────────────── */
    gfx->fillScreen(MT_COLOR_PRIMARY);

    /* ── Texto centrado (accent) ───────────────────── */
    gfx->setTextSize(4);
    gfx->setTextColor(MT_COLOR_ACCENT);
    gfx->setCursor(65, 250);          // adjust as needed
    gfx->print("Hello World!");

    /* ── Contador dinámico (text light) ─────────────── */
    static int counter = 0;
    gfx->setTextSize(2);
    gfx->setTextColor(MT_COLOR_TEXT_LIGHT);
    gfx->setCursor(140, 320);
    gfx->printf("Count: %d", counter++);

    /* Optional: show a secondary colour somewhere
     * e.g. a small status icon or progress bar.
     */
    // gfx->fillRect(10, 10, 20, 20, MT_COLOR_SECONDARY);

    vTaskDelay(pdMS_TO_TICKS(1000));   // refresh every second
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
