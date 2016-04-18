#include "nrf51_rtc.h"

int nrf51_rtc::set_time(unsigned int rawtime) {
    // set the current time from a parameter
    time_base = rawtime;
    rtc_previous = int (NRF_RTC1->COUNTER) / ticks_per_second;
    return 0;
}

unsigned int nrf51_rtc::time() {
    // get the current time... and update the underlying variables for tracking time
    //  ...this routine must be called regularly, before the 24b RTC register can wrap around:  
    //  t < size of register / (LFCLK_FREQUENCY / (NRF_RTC1->PRESCALER + 1))
    //    size of register = 2^24 ticks 
    //    LFCLK_FREQUENCY = 2^15 cycles/sec 
    //    NRF_RTC1->PRESCALER = 0 -- as (currently) set by mbed library!
    //  = (2^24)/(2^15/1) = 2^9 seconds = 512 seconds, ~ 8.5 minutes
    unsigned int rtc_now = (NRF_RTC1->COUNTER) / ticks_per_second;
    unsigned int delta_seconds = ((rtc_now + counter_size_in_seconds) - rtc_previous) % counter_size_in_seconds;
    time_base = time_base + (time_t) delta_seconds;
    rtc_previous = rtc_now;
    return time_base;
}

void nrf51_rtc::update_rtc() {
    // for use as interrupt routine, same as "time" but doesn't return a value (as req'd for interrupt)
    //  ...but this is useless since the compiler seems to be unable to correctly attach a member function to the ticker interrupt
    //  ...the exact same interrupt routine attached to a ticker works fine when declared outside the method
    nrf51_rtc::time();
}

nrf51_rtc::nrf51_rtc() {
    rtc_previous=0;
    time_base=0;
    #define LFCLK_FREQUENCY 0x8000
    #define RTC_COUNTER_SIZE 0x1000000
    ticks_per_second = LFCLK_FREQUENCY / (NRF_RTC1->PRESCALER + 1);
    counter_size_in_seconds = RTC_COUNTER_SIZE / ticks_per_second;
    
    // Ticker inside the method doesn't work:  the interrupt attachment to the ticker messes up the world (code goes to the weeds)
    //  ... not needed if "rtc.time()" is called frequently enough (once per 512 seconds)
    //  ... this scheme (ticker+interrupt routine to update rtc) works correctly when implemented at the next level up
    //Ticker rtc_ticker;
    //#define RTC_UPDATE 1
    //rtc_ticker.attach(this,&nrf51_rtc::update_rtc, RTC_UPDATE); // update the time regularly (must be < duration of 24b timer)
}
