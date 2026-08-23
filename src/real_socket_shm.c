
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

    #define BUFFER_CAPACITY 2048
    #define PACKET_SIZE 128

    typedef struct {
        uint8_t data[PACKET_SIZE];
    } Packet;

    typedef struct {
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        Packet buffer[BUFFER_CAPACITY];
    } SharedRingBuffer;

    typedef struct {
        SharedRingBuffer* shm;
        int server_fd;
        int port;
        _Atomic bool is_running;
    } SocketServer;

    SharedRingBuffer* shm_create() {
        size_t size = sizeof(SharedRingBuffer);
        void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) return NULL;

        SharedRingBuffer* shm = (SharedRingBuffer*)addr;
        atomic_init(&shm->head, 0);
        atomic_init(&shm->tail, 0);
        memset(shm->buffer, 0, sizeof(shm->buffer));
        return shm;
    }

    int shm_push(SharedRingBuffer* shm, const uint8_t* src_data) {
        uint32_t current_tail = atomic_load_explicit(&shm->tail, memory_order_relaxed);
        uint32_t current_head = atomic_load_explicit(&shm->head, memory_order_acquire);

        if ((current_tail - current_head) >= BUFFER_CAPACITY) return -ENOSPC;

        uint32_t index = current_tail & (BUFFER_CAPACITY - 1);
        memcpy(shm->buffer[index].data, src_data, PACKET_SIZE);

        atomic_store_explicit(&shm->tail, current_tail + 1, memory_order_release);
        return 0;
    }

    int shm_pop(SharedRingBuffer* shm, uint8_t* dest_data) {
        uint32_t current_head = atomic_load_explicit(&shm->head, memory_order_relaxed);
        uint32_t current_tail = atomic_load_explicit(&shm->tail, memory_order_acquire);

        if (current_head == current_tail) return -EAGAIN;

        uint32_t index = current_head & (BUFFER_CAPACITY - 1);
        memcpy(dest_data, shm->buffer[index].data, PACKET_SIZE);

        atomic_store_explicit(&shm->head, current_head + 1, memory_order_release);
        return 0;
    }

    SocketServer* socket_server_create(int port) {
        SocketServer* srv = (SocketServer*)malloc(sizeof(SocketServer));
        if (!srv) return NULL;

        srv->shm = shm_create();
        if (!srv->shm) {
            free(srv);
            return NULL;
        }

        srv->port = port;
        atomic_init(&srv->is_running, true);

        // Opprett POSIX TCP socket
        srv->server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (srv->server_fd < 0) {
            munmap(srv->shm, sizeof(SharedRingBuffer));
            free(srv);
            return NULL;
        }

        int opt = 1;
        setsockopt(srv->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Sett socket til non-blocking
        int flags = fcntl(srv->server_fd, F_GETFL, 0);
        fcntl(srv->server_fd, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(srv->server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(srv->server_fd);
            munmap(srv->shm, sizeof(SharedRingBuffer));
            free(srv);
            return NULL;
        }

        if (listen(srv->server_fd, 128) < 0) {
            close(srv->server_fd);
            munmap(srv->shm, sizeof(SharedRingBuffer));
            free(srv);
            return NULL;
        }

        return srv;
    }

    // Lytter på nettverket og sluser pakker inn i SHM
    int socket_server_poll(SocketServer* srv) {
        if (!srv || srv->server_fd < 0) return -1;

        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int client_fd = accept(srv->server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0; // Ingen nye klienter ennå
            return -1;
        }

        // Les pakke fra klient
        uint8_t rx_buf[PACKET_SIZE];
        ssize_t bytes_read = read(client_fd, rx_buf, PACKET_SIZE);
        if (bytes_read == PACKET_SIZE) {
            shm_push(srv->shm, rx_buf);
        }

        close(client_fd);
        return (bytes_read == PACKET_SIZE) ? 1 : 0;
    }

    SharedRingBuffer* socket_server_get_shm(SocketServer* srv) {
        return srv ? srv->shm : NULL;
    }

    void socket_server_free(SocketServer* srv) {
        if (srv) {
            if (srv->server_fd >= 0) close(srv->server_fd);
            if (srv->shm) munmap(srv->shm, sizeof(SharedRingBuffer));
            free(srv);
        }
    }
    