#include "configuration.h"

#ifdef KODE_DOT


#include "kode_dot_utils/kode_dot_screen.h"

void lateInitVariant()
{
    LOG_DEBUG("Kode Dot tracker lateInitVariant");
    startDisplayTask();
    LOG_DEBUG("Kode Dot tracker lateInitVariant done");
}

#endif