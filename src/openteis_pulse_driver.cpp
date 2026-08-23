#include <stdint.h>
#include <stddef.h>

extern "C" {
    bool openteis_pulse_init() {
        return true;
    }

    int send_pulse_signal(double amplitude, int frequency_hz) {
        // Diskret spenningspuls-driver for FiiO KA13 / SGM8262-trinnet
        if (amplitude <= 0.0 || frequency_hz <= 0) {
            return -1;
        }
        return 0;
    }
}
