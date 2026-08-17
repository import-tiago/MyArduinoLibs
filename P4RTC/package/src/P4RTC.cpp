#include "P4RTC.h"

#include <Arduino.h>
#include <sys/time.h>

extern "C"
{
#include "soc/pmu_reg.h"
#include "soc/soc.h"
}

enum {
    VBAT_MODE_VDDA = 0,
    VBAT_MODE_VBAT = 1,
    VBAT_MODE_AUTO = 2,
};

time_t P4RTC::get_epoch() {

    time_t now = 0;
    time(&now);

    return now;
}

bool P4RTC::set_epoch(time_t epoch) {

    const timeval now = {
      .tv_sec = epoch,
      .tv_usec = 0,
    };

    return settimeofday(&now, nullptr) == 0;
}

bool P4RTC::enable_vbat_backup() {

    if (_is_vbat_ready)
        return true;

    REG_SET_FIELD(PMU_HP_SLEEP_LP_DIG_POWER_REG, PMU_HP_SLEEP_VDDBAT_MODE, VBAT_MODE_AUTO);
    REG_SET_BIT(PMU_VDDBAT_CFG_REG, PMU_VDDBAT_SW_UPDATE);

    const uint32_t t0 = millis();
    while (REG_GET_FIELD(PMU_VDDBAT_CFG_REG, PMU_ANA_VDDBAT_MODE) != VBAT_MODE_AUTO) {

        if (millis() - t0 > 100)
            return false;

        delay(1);
    }

    _is_vbat_ready = true;

    return true;
}
