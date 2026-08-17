#pragma once

#include <time.h>

class P4RTC {

    public:
        time_t get_epoch();
        bool set_epoch(time_t epoch);
        bool enable_vbat_backup();

    private:
        bool _is_vbat_ready = false;
};
