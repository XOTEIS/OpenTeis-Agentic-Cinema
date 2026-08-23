
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <math.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdatomic.h>

    #define PACKET_SIZE 128
    #define HASH_SIZE 32
    #define MAX_STATES 8
    #define RING_CAPACITY 1024
    #define MAX_ALLOWED_ENTROPY 6.40f
    #define MIN_ALLOWED_ENTROPY 0.50f

    // --- STRUCT DEFINISJONER ---
    typedef struct {
        float entropy;
        uint32_t unique_bytes;
        bool is_valid;
        int status_code;
    } FilterResult;

    typedef struct {
        uint32_t state_id;
        float probability;
        uint8_t vector[PACKET_SIZE];
    } StateCandidate;

    typedef struct {
        uint32_t active_states;
        float system_entropy;
        uint32_t collapsed_state_id;
        bool is_collapsed;
        int status_code;
        uint8_t final_payload[PACKET_SIZE];
    } CollapseResult;

    typedef struct {
        uint64_t sequence_id;
        uint8_t prev_chain_hash[HASH_SIZE];
        uint8_t payload[PACKET_SIZE];
        uint8_t current_hash[HASH_SIZE];
    } __attribute__((packed)) WormRecord;

    typedef struct {
        uint64_t sequence_id;
        uint64_t timestamp_ns;
        uint8_t event_hash[HASH_SIZE];
        uint8_t payload[PACKET_SIZE];
    } SparkGapEvent;

    typedef struct {
        SparkGapEvent events[RING_CAPACITY];
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        _Atomic uint64_t dropped_count;
    } SparkGapBridge;

    static SparkGapBridge g_bridge;

    // --- SHA-256 IMPLEMENTASJON ---
    #define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
    #define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
    #define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
    #define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
    #define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
    #define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
    #define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

    static const uint32_t K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
    };

    static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
        uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
        for (i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
        for (; i < 64; ++i)
            m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

        a = state[0]; b = state[1]; c = state[2]; d = state[3];
        e = state[4]; f = state[5]; g = state[6]; h = state[7];

        for (i = 0; i < 64; ++i) {
            t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
            t2 = EP0(a) + MAJ(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    static void compute_sha256(const uint8_t* data, size_t len, uint8_t hash[32]) {
        uint32_t state[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        uint8_t buf[64];
        size_t i;
        uint64_t bits = len * 8;

        for (i = 0; i < len / 64; i++) {
            sha256_transform(state, data + i * 64);
        }

        size_t rem = len % 64;
        memcpy(buf, data + i * 64, rem);
        buf[rem] = 0x80;
        rem++;

        if (rem > 56) {
            memset(buf + rem, 0, 64 - rem);
            sha256_transform(state, buf);
            memset(buf, 0, 56);
        } else {
            memset(buf + rem, 0, 56 - rem);
        }

        for (int b = 7; b >= 0; b--) {
            buf[56 + (7 - b)] = (bits >> (b * 8)) & 0xFF;
        }
        sha256_transform(state, buf);

        for (i = 0; i < 8; i++) {
            hash[i * 4]     = (state[i] >> 24) & 0xFF;
            hash[i * 4 + 1] = (state[i] >> 16) & 0xFF;
            hash[i * 4 + 2] = (state[i] >> 8) & 0xFF;
            hash[i * 4 + 3] = state[i] & 0xFF;
        }
    }

    // --- MODUL 1: INNGANGSFILTER ---
    FilterResult openteis_dsp_analyze_payload(const uint8_t* payload, size_t len) {
        FilterResult res = {0.0f, 0, false, 0};
        if (!payload || len != PACKET_SIZE) {
            res.status_code = -EINVAL;
            return res;
        }

        uint32_t freq[256] = {0};
        for (size_t i = 0; i < len; i++) freq[payload[i]]++;

        float entropy = 0.0f;
        uint32_t unique = 0;
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                unique++;
                float p = (float)freq[i] / (float)len;
                entropy -= p * log2f(p);
            }
        }

        res.entropy = entropy;
        res.unique_bytes = unique;

        if (entropy > MAX_ALLOWED_ENTROPY) {
            res.is_valid = false;
            res.status_code = -EBADMSG;
        } else if (entropy < MIN_ALLOWED_ENTROPY) {
            res.is_valid = false;
            res.status_code = -EDOM;
        } else {
            res.is_valid = true;
            res.status_code = 0;
        }
        return res;
    }

    // --- MODUL 2: BØLGEKOLLAPS ---
    CollapseResult openteis_g13_collapse_state(const uint8_t* input_signal, size_t signal_len, uint8_t rule_mask) {
        CollapseResult res;
        memset(&res, 0, sizeof(CollapseResult));

        if (!input_signal || signal_len != PACKET_SIZE) {
            res.status_code = -EINVAL;
            return res;
        }

        StateCandidate candidates[MAX_STATES];
        float init_p = 1.0f / (float)MAX_STATES;

        for (uint32_t i = 0; i < MAX_STATES; i++) {
            candidates[i].state_id = 100 + i;
            candidates[i].probability = init_p;
            for (size_t j = 0; j < PACKET_SIZE; j++) {
                candidates[i].vector[j] = (input_signal[j] ^ (uint8_t)(i * 17)) + rule_mask;
            }
        }

        uint32_t valid_mask = 0;
        uint32_t remaining_count = 0;
        for (uint32_t i = 0; i < MAX_STATES; i++) {
            uint8_t check_byte = candidates[i].vector[0];
            if ((check_byte % (i + 1)) == (rule_mask % (i + 1))) {
                valid_mask |= (1 << i);
                remaining_count++;
            }
        }

        if (remaining_count == 0) {
            res.is_collapsed = false;
            res.status_code = -EILSEQ;
            return res;
        }

        uint32_t winner_idx = 0;
        for (uint32_t i = 0; i < MAX_STATES; i++) {
            if (valid_mask & (1 << i)) {
                winner_idx = i;
                break;
            }
        }

        res.active_states = 1;
        res.system_entropy = 0.0f;
        res.collapsed_state_id = candidates[winner_idx].state_id;
        res.is_collapsed = true;
        res.status_code = 0;
        memcpy(res.final_payload, candidates[winner_idx].vector, PACKET_SIZE);

        return res;
    }

    // --- MODUL 3 & 4: WORM VAULT & SPARK-GAP INIT ---
    void sparkgap_init(void) {
        atomic_store(&g_bridge.head, 0);
        atomic_store(&g_bridge.tail, 0);
        atomic_store(&g_bridge.dropped_count, 0);
        memset(g_bridge.events, 0, sizeof(g_bridge.events));
    }

    // --- FULLSTENDIG END-TO-END PIPELINE KONTROLLER ---
    int openteis_process_transaction(
        int vault_fd,
        uint64_t seq_id,
        uint64_t ts_ns,
        const uint8_t* raw_input,
        uint8_t rule_mask,
        uint8_t* inout_prev_hash
    ) {
        // 1. DSP Inngangsfilter
        FilterResult f_res = openteis_dsp_analyze_payload(raw_input, PACKET_SIZE);
        if (!f_res.is_valid) return f_res.status_code;

        // 2. G-13 Bølgekollaps Engine
        CollapseResult c_res = openteis_g13_collapse_state(raw_input, PACKET_SIZE, rule_mask);
        if (!c_res.is_collapsed) return c_res.status_code;

        // 3. WORM Vault Commit & SHA-256 Kjeding
        WormRecord rec;
        memset(&rec, 0, sizeof(WormRecord));
        rec.sequence_id = seq_id;
        memcpy(rec.prev_chain_hash, inout_prev_hash, HASH_SIZE);
        memcpy(rec.payload, c_res.final_payload, PACKET_SIZE);

        uint8_t sign_buf[8 + HASH_SIZE + PACKET_SIZE];
        memcpy(sign_buf, &seq_id, 8);
        memcpy(sign_buf + 8, inout_prev_hash, HASH_SIZE);
        memcpy(sign_buf + 8 + HASH_SIZE, c_res.final_payload, PACKET_SIZE);

        compute_sha256(sign_buf, sizeof(sign_buf), rec.current_hash);

        ssize_t written = write(vault_fd, &rec, sizeof(WormRecord));
        if (written != sizeof(WormRecord)) return -EIO;
        fsync(vault_fd);

        // Oppdater prev_hash for neste transaksjon i kjeden
        memcpy(inout_prev_hash, rec.current_hash, HASH_SIZE);

        // 4. Spark-Gap Lock-Free Event Emitting
        uint32_t current_head = atomic_load(&g_bridge.head);
        uint32_t current_tail = atomic_load(&g_bridge.tail);
        uint32_t next_head = (current_head + 1) % RING_CAPACITY;

        if (next_head == current_tail) {
            atomic_fetch_add(&g_bridge.dropped_count, 1);
            return -ENOBUFS;
        }

        SparkGapEvent* ev = &g_bridge.events[current_head];
        ev->sequence_id = seq_id;
        ev->timestamp_ns = ts_ns;
        memcpy(ev->event_hash, rec.current_hash, HASH_SIZE);
        memcpy(ev->payload, c_res.final_payload, PACKET_SIZE);

        atomic_store(&g_bridge.head, next_head);

        return 0;
    }

    int sparkgap_consume(SparkGapEvent* out_event) {
        if (!out_event) return -EINVAL;
        uint32_t current_head = atomic_load(&g_bridge.head);
        uint32_t current_tail = atomic_load(&g_bridge.tail);

        if (current_tail == current_head) return -EAGAIN;

        *out_event = g_bridge.events[current_tail];
        uint32_t next_tail = (current_tail + 1) % RING_CAPACITY;
        atomic_store(&g_bridge.tail, next_tail);
        return 0;
    }
    