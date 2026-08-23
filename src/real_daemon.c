
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>
    #include <sys/mman.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <pthread.h>

    #define BUFFER_CAPACITY 4096
    #define PACKET_SIZE 128
    #define RECORD_SIZE 144
    #define INDEX_CAPACITY 262144
    #define EMPTY_HASH 0ULL

    typedef struct {
        uint8_t data[PACKET_SIZE];
    } Packet;

    typedef struct {
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        Packet buffer[BUFFER_CAPACITY];
    } SharedRingBuffer;

    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    typedef struct {
        uint64_t hash;
        uint64_t file_offset;
    } IndexNode;

    typedef struct {
        IndexNode nodes[INDEX_CAPACITY];
        _Atomic uint32_t count;
    } IndexTable;

    typedef struct {
        SharedRingBuffer* shm;
        IndexTable* index;
        int ingest_fd;
        int query_fd;
        int journal_fd;
        int ingest_port;
        int query_port;
        _Atomic bool is_running;
        _Atomic uint64_t global_seq;
        _Atomic uint64_t bytes_persisted;
        pthread_t worker_thread;
    } OpenHarnessDaemon;

    uint64_t compute_fnv1a_64(const uint8_t* data, size_t len) {
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    int index_insert(IndexTable* idx, uint64_t hash, uint64_t offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

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
        atomic_fetch_add(&idx->count, 1);
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

    // Bakgrunnstråd som tømmer SHM -> Hashing -> Journalfil på disk -> RAM Index
    void* daemon_storage_worker(void* arg) {
        OpenHarnessDaemon* d = (OpenHarnessDaemon*)arg;

        while (atomic_load(&d->is_running) || 
               atomic_load(&d->shm->head) != atomic_load(&d->shm->tail)) {
            
            uint32_t head = atomic_load_explicit(&d->shm->head, memory_order_relaxed);
            uint32_t tail = atomic_load_explicit(&d->shm->tail, memory_order_acquire);

            if (head == tail) {
                usleep(1000); // 1 ms ventetid om bufferen er tom
                continue;
            }

            while (head != tail) {
                uint32_t index = head & (BUFFER_CAPACITY - 1);

                JournalRecord rec;
                rec.sequence_id = atomic_fetch_add(&d->global_seq, 1) + 1;
                rec.payload_hash = compute_fnv1a_64(d->shm->buffer[index].data, PACKET_SIZE);
                memcpy(rec.payload, d->shm->buffer[index].data, PACKET_SIZE);

                off_t current_offset = lseek(d->journal_fd, 0, SEEK_END);
                ssize_t w = write(d->journal_fd, &rec, sizeof(JournalRecord));
                if (w == sizeof(JournalRecord)) {
                    index_insert(d->index, rec.payload_hash, (uint64_t)current_offset);
                    atomic_fetch_add(&d->bytes_persisted, sizeof(JournalRecord));
                }

                head++;
            }

            atomic_store_explicit(&d->shm->head, head, memory_order_release);
            fsync(d->journal_fd);
        }

        return NULL;
    }

    OpenHarnessDaemon* daemon_init(const char* journal_path, int ingest_port, int query_port) {
        OpenHarnessDaemon* d = (OpenHarnessDaemon*)malloc(sizeof(OpenHarnessDaemon));
        if (!d) return NULL;

        d->ingest_port = ingest_port;
        d->query_port = query_port;
        atomic_init(&d->is_running, true);
        atomic_init(&d->global_seq, 0);
        atomic_init(&d->bytes_persisted, 0);

        // 1. SHM Allokering
        d->shm = (SharedRingBuffer*)mmap(NULL, sizeof(SharedRingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (d->shm == MAP_FAILED) { free(d); return NULL; }
        atomic_init(&d->shm->head, 0);
        atomic_init(&d->shm->tail, 0);

        // 2. RAM Index Allokering
        d->index = (IndexTable*)calloc(1, sizeof(IndexTable));
        if (!d->index) { munmap(d->shm, sizeof(SharedRingBuffer)); free(d); return NULL; }

        // 3. Åpne journalfil
        d->journal_fd = open(journal_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (d->journal_fd < 0) {
            free(d->index);
            munmap(d->shm, sizeof(SharedRingBuffer));
            free(d);
            return NULL;
        }

        // 4. Ingest Socket (Port 8080)
        d->ingest_fd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(d->ingest_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int flags = fcntl(d->ingest_fd, F_GETFL, 0);
        fcntl(d->ingest_fd, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in ing_addr;
        ing_addr.sin_family = AF_INET;
        ing_addr.sin_addr.s_addr = INADDR_ANY;
        ing_addr.sin_port = htons(ingest_port);
        bind(d->ingest_fd, (struct sockaddr*)&ing_addr, sizeof(ing_addr));
        listen(d->ingest_fd, 128);

        // 5. Query Socket (Port 9090)
        d->query_fd = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(d->query_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        flags = fcntl(d->query_fd, F_GETFL, 0);
        fcntl(d->query_fd, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in q_addr;
        q_addr.sin_family = AF_INET;
        q_addr.sin_addr.s_addr = INADDR_ANY;
        q_addr.sin_port = htons(query_port);
        bind(d->query_fd, (struct sockaddr*)&q_addr, sizeof(q_addr));
        listen(d->query_fd, 128);

        // 6. Start Bakgrunnstråd for Disk I/O & Indexing
        pthread_create(&d->worker_thread, NULL, daemon_storage_worker, d);

        return d;
    }

    // Poll for Ingest pakke over TCP (Port 8080)
    int daemon_poll_ingest(OpenHarnessDaemon* d) {
        if (!d) return -1;

        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(d->ingest_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) return 0;

        uint8_t rx_buf[PACKET_SIZE];
        ssize_t bytes_read = read(client_fd, rx_buf, PACKET_SIZE);
        close(client_fd);

        if (bytes_read == PACKET_SIZE) {
            uint32_t tail = atomic_load_explicit(&d->shm->tail, memory_order_relaxed);
            uint32_t head = atomic_load_explicit(&d->shm->head, memory_order_acquire);

            if ((tail - head) < BUFFER_CAPACITY) {
                uint32_t index = tail & (BUFFER_CAPACITY - 1);
                memcpy(d->shm->buffer[index].data, rx_buf, PACKET_SIZE);
                atomic_store_explicit(&d->shm->tail, tail + 1, memory_order_release);
                return 1;
            }
        }
        return 0;
    }

    // Poll for Query over TCP (Port 9090)
    int daemon_poll_query(OpenHarnessDaemon* d) {
        if (!d) return -1;

        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_fd = accept(d->query_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) return 0;

        uint64_t req_hash = 0;
        ssize_t r = read(client_fd, &req_hash, sizeof(uint64_t));

        if (r == sizeof(uint64_t)) {
            uint64_t file_offset = 0;
            if (index_lookup(d->index, req_hash, &file_offset) == 0) {
                uint8_t record_buf[RECORD_SIZE];
                if (pread(d->journal_fd, record_buf, RECORD_SIZE, (off_t)file_offset) == RECORD_SIZE) {
                    uint8_t status = 0x00;
                    write(client_fd, &status, 1);
                    write(client_fd, record_buf, RECORD_SIZE);
                } else {
                    uint8_t status = 0x50;
                    write(client_fd, &status, 1);
                }
            } else {
                uint8_t status = 0x44;
                write(client_fd, &status, 1);
            }
        }

        close(client_fd);
        return 1;
    }

    uint64_t daemon_get_persisted_bytes(OpenHarnessDaemon* d) {
        return d ? atomic_load(&d->bytes_persisted) : 0;
    }

    uint32_t daemon_get_indexed_count(OpenHarnessDaemon* d) {
        return d ? atomic_load(&d->index->count) : 0;
    }

    void daemon_shutdown(OpenHarnessDaemon* d) {
        if (d) {
            atomic_store(&d->is_running, false);
            pthread_join(d->worker_thread, NULL);

            if (d->ingest_fd >= 0) close(d->ingest_fd);
            if (d->query_fd >= 0) close(d->query_fd);
            if (d->journal_fd >= 0) close(d->journal_fd);
            if (d->shm) munmap(d->shm, sizeof(SharedRingBuffer));
            if (d->index) free(d->index);
            free(d);
        }
    }
    