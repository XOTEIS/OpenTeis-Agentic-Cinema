
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <errno.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <unistd.h>

    #define INDEX_CAPACITY 131072
    #define EMPTY_HASH 0ULL
    #define RECORD_SIZE 144 // 8b seq + 8b hash + 128b payload

    typedef struct {
        uint64_t hash;
        uint64_t file_offset;
    } IndexNode;

    typedef struct {
        IndexNode nodes[INDEX_CAPACITY];
        uint32_t count;
    } IndexTable;

    typedef struct {
        IndexTable* index;
        int journal_fd;
        int server_fd;
        int port;
    } QueryEngine;

    IndexTable* index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        memset(idx->nodes, 0, sizeof(idx->nodes));
        idx->count = 0;
        return idx;
    }

    int index_insert(IndexTable* idx, uint64_t hash, uint64_t offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;
        if (idx->count >= (INDEX_CAPACITY / 2)) return -ENOSPC;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                idx->nodes[slot].file_offset = offset;
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
        }

        idx->nodes[slot].hash = hash;
        idx->nodes[slot].file_offset = offset;
        idx->count++;
        return 0;
    }

    int index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        uint32_t start_slot = slot;

        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                *out_offset = idx->nodes[slot].file_offset;
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
            if (slot == start_slot) break;
        }

        return -ENOENT;
    }

    QueryEngine* query_engine_create(const char* journal_path, int port) {
        QueryEngine* qe = (QueryEngine*)malloc(sizeof(QueryEngine));
        if (!qe) return NULL;

        qe->index = index_create();
        if (!qe->index) { free(qe); return NULL; }

        qe->journal_fd = open(journal_path, O_RDONLY);
        if (qe->journal_fd < 0) {
            free(qe->index);
            free(qe);
            return NULL;
        }

        qe->port = port;
        qe->server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (qe->server_fd < 0) {
            close(qe->journal_fd);
            free(qe->index);
            free(qe);
            return NULL;
        }

        int opt = 1;
        setsockopt(qe->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(qe->server_fd, (struct sockaddr*)&address, sizeof(address)) < 0 ||
            listen(qe->server_fd, 128) < 0) {
            close(qe->server_fd);
            close(qe->journal_fd);
            free(qe->index);
            free(qe);
            return NULL;
        }

        return qe;
    }

    // Indekserer eksisterende journalfil i RAM
    int query_engine_warmup(QueryEngine* qe) {
        if (!qe || qe->journal_fd < 0) return -1;

        off_t offset = 0;
        uint8_t buf[RECORD_SIZE];

        while (pread(qe->journal_fd, buf, RECORD_SIZE, offset) == RECORD_SIZE) {
            uint64_t hash_val;
            memcpy(&hash_val, buf + 8, sizeof(uint64_t)); // Offset 8 er payload_hash
            index_insert(qe->index, hash_val, (uint64_t)offset);
            offset += RECORD_SIZE;
        }

        return (int)qe->index->count;
    }

    // Behandler én enkelt binary lookup over TCP (Zero-Copy Read)
    int query_engine_handle_one(QueryEngine* qe) {
        if (!qe) return -1;

        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int client_fd = accept(qe->server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) return -1;

        // Les requested 64-bit hash direkte fra TCP-socket
        uint64_t req_hash = 0;
        ssize_t r = read(client_fd, &req_hash, sizeof(uint64_t));
        if (r != sizeof(uint64_t)) {
            close(client_fd);
            return -1;
        }

        uint64_t file_offset = 0;
        int lookup_res = index_lookup(qe->index, req_hash, &file_offset);

        if (lookup_res == 0) {
            // Slå opp i disken med pread() og send svar rett ut på socketen
            uint8_t record_buf[RECORD_SIZE];
            if (pread(qe->journal_fd, record_buf, RECORD_SIZE, (off_t)file_offset) == RECORD_SIZE) {
                uint8_t status_byte = 0x00; // Suksess status
                write(client_fd, &status_byte, 1);
                write(client_fd, record_buf, RECORD_SIZE);
            } else {
                uint8_t status_byte = 0x50; // Disk-feil
                write(client_fd, &status_byte, 1);
            }
        } else {
            uint8_t status_byte = 0x44; // Ikke funnet (404)
            write(client_fd, &status_byte, 1);
        }

        close(client_fd);
        return lookup_res;
    }

    void query_engine_free(QueryEngine* qe) {
        if (qe) {
            if (qe->server_fd >= 0) close(qe->server_fd);
            if (qe->journal_fd >= 0) close(qe->journal_fd);
            if (qe->index) free(qe->index);
            free(qe);
        }
    }
    