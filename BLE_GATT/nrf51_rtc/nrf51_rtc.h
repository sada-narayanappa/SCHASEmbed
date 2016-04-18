#ifndef NRF51_RTC_H
#define NRF51_RTC_H

#include "mbed.h"

static class nrf51_rtc {
    // class to create equivalent of time() and set_time()
    //   ...depends upon RTC1 to be running and to never stop -- a byproduct of instantiation of mbed library's "ticker"
    public:
    
        nrf51_rtc();
        unsigned int time();
        int set_time(unsigned int rawtime);
        void update_rtc(); // similar to "time" but doesn't return the value, just updates underlying variables
    
    private:
    
        unsigned int time_base;
        unsigned int rtc_previous;
        unsigned int ticks_per_second, counter_size_in_seconds;

} rtc;
#endif