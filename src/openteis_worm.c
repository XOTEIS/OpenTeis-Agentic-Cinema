
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>

    #define PACKET_SIZE 128
    #define HASH_SIZE 32

    typedef struct {
        uint64_t sequence_id;
        uint8_t prev_chain_hash[HASH_SIZE];
        uint8_t payload[PACKET_SIZE];
        uint8_t current_hash[HASH_SIZE];
    } __attribute__((packed)) WormRecord;

    // Fysisk SW-implementasjon av SHA-256 for ubrytelig kjeding i C11
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

    void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
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

    void compute_sha256(const uint8_t* data, size_t len, uint8_t hash[32]) {
        uint32_t state[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        uint8_t buf[64];
        size_t i, pad_len;
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

    // Atomisk WORM Commit med SHA-256 Kjede-forsegling
    int worm_vault_commit_record(
        int fd, 
        uint64_t seq_id, 
        const uint8_t* prev_hash, 
        const uint8_t* payload, 
        uint8_t* out_current_hash
    ) {
        if (fd < 0 || !payload || !prev_hash || !out_current_hash) return -EINVAL;

        WormRecord rec;
        memset(&rec, 0, sizeof(WormRecord));
        rec.sequence_id = seq_id;
        memcpy(rec.prev_chain_hash, prev_hash, HASH_SIZE);
        memcpy(rec.payload, payload, PACKET_SIZE);

        // Forsegl kjedeelementet: H_n = SHA256(seq_id + prev_hash + payload)
        uint8_t sign_buf[8 + HASH_SIZE + PACKET_SIZE];
        memcpy(sign_buf, &seq_id, 8);
        memcpy(sign_buf + 8, prev_hash, HASH_SIZE);
        memcpy(sign_buf + 8 + HASH_SIZE, payload, PACKET_SIZE);

        compute_sha256(sign_buf, sizeof(sign_buf), rec.current_hash);
        memcpy(out_current_hash, rec.current_hash, HASH_SIZE);

        // Skriv atomisk til disk
        ssize_t written = write(fd, &rec, sizeof(WormRecord));
        if (written != sizeof(WormRecord)) return -EIO;

        fsync(fd); // Tving fysisk tømming til silisium/disk
        return 0;
    }

    // Validerer hele WORM-kjeden fra disk bit-for-bit
    int worm_vault_verify_chain(int fd, size_t total_records) {
        if (fd < 0) return -EINVAL;

        lseek(fd, 0, SEEK_SET);
        WormRecord rec;
        uint8_t expected_prev[HASH_SIZE] = {0}; // Første blokk har 0x00 som prev_hash

        for (size_t i = 0; i < total_records; i++) {
            ssize_t read_bytes = read(fd, &rec, sizeof(WormRecord));
            if (read_bytes != sizeof(WormRecord)) return -EIO;

            // Sjekk at prev_hash matcher forrige blokks current_hash
            if (memcmp(rec.prev_chain_hash, expected_prev, HASH_SIZE) != 0) {
                return -EBADMSG; // Kjedevridning / Manipulasjon oppdaget!
            }

            // Rekonstruer hash og sjekk integritet
            uint8_t sign_buf[8 + HASH_SIZE + PACKET_SIZE];
            memcpy(sign_buf, &rec.sequence_id, 8);
            memcpy(sign_buf + 8, rec.prev_chain_hash, HASH_SIZE);
            memcpy(sign_buf + 8 + HASH_SIZE, rec.payload, PACKET_SIZE);

            uint8_t calc_hash[HASH_SIZE];
            compute_sha256(sign_buf, sizeof(sign_buf), calc_hash);

            if (memcmp(rec.current_hash, calc_hash, HASH_SIZE) != 0) {
                return -EILSEQ; // Data korrumpert på disk!
            }

            memcpy(expected_prev, rec.current_hash, HASH_SIZE);
        }

        return 0; // Kjedevridning = 0, Integritet = 100%
    }
    