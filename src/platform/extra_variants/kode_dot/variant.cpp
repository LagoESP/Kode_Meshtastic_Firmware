#include "configuration.h"

#ifdef KODE_DOT
#include "kode_dot_utils/kode_dot_screen.h"

extern "C" bool verifyRollbackLater();  // must use C linkage to match the weak symbol

extern "C" bool verifyRollbackLater() {
  return true;  // never auto-validate OTAs
}

extern "C" void __wrap_esp_ota_mark_app_valid_cancel_rollback(void);

extern "C" void __wrap_esp_ota_mark_app_valid_cancel_rollback(void) {
  // Do nothing, effectively disabling the OTA validation.
}

void lateInitVariant()
{
    LOG_DEBUG("Kode Dot tracker lateInitVariant");

    LOG_DEBUG("Kode Dot tracker lateInitVariant done");
}

#endif