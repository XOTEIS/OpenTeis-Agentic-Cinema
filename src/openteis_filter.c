
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <math.h>
    #include <errno.h>

    #define PACKET_SIZE 128
    #define MAX_ALLOWED_ENTROPY 6.40f  // Maksimal entropi for strukturerte 128-bytes payloads
    #define MIN_ALLOWED_ENTROPY 0.50f  // Grense mot flate/døde signaler

    typedef struct {
        float entropy;
        uint32_t unique_bytes;
        bool is_valid;
        int status_code;
    } FilterResult;

    FilterResult openteis_dsp_analyze_payload(const uint8_t* payload, size_t len) {
        FilterResult res = {0.0f, 0, false, 0};
        if (!payload || len != PACKET_SIZE) {
            res.status_code = -EINVAL;
            return res;
        }

        uint32_t freq[256] = {0};
        for (size_t i = 0; i < len; i++) {
            freq[payload[i]]++;
        }

        float entropy = 0.0f;
        uint32_t unique = 0;

        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                unique++;
                float p = (float)freq[i] / (float)len;
                entropy -= p * (log2f(p));
            }
        }

        res.entropy = entropy;
        res.unique_bytes = unique;

        if (entropy > MAX_ALLOWED_ENTROPY) {
            res.is_valid = false;
            res.status_code = -EBADMSG; // Høy-entropi ustrukturert støy
        } else if (entropy < MIN_ALLOWED_ENTROPY) {
            res.is_valid = false;
            res.status_code = -EDOM;    // Dødt/flatt signal (null-entropi)
        } else {
            res.is_valid = true;
            res.status_code = 0;        // Deterministisk strukturert signal
        }

        return res;
    }
    