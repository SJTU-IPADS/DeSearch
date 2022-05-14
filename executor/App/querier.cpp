#include "common.hpp"
#include "live_node.hpp"

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 10240

void ecall_querier_setup(const char * b_index_input, const int b_index_input_size);
void ecall_do_query(
    const char * querier_request, int querier_request_size,
    char * querier_response, int querier_response_size
);

static
int get_sharding_max()
{
    auto value = kb.get("SHARDING-MAX");
    return stoi(*value, nullptr, 10);
}

static
int get_sharding_counter()
{
    return kb.incr("SHARD_COUNTER");
}

static int id;
void querier_setup(const char* addr, int port)
{
    int current_epoch_int = get_epoch_int();
    if (current_epoch_int < 3) return;
    
    int max_partiton = get_sharding_max();
    id = get_sharding_counter() % max_partiton + 1;
    string global_index_key = "GLOBAL-INDEX-#EPOCH" + to_string(current_epoch_int-1) + "-v" + to_string(id);
    printf("[%d] %s: I'm serving the partiton %s\n", getpid(), __func__, global_index_key.data());

    static char * b_index_input;
    static int b_index_input_size;
    o_get_key(global_index_key, b_index_input, b_index_input_size);

    ecall_querier_setup(b_index_input, b_index_input_size);

    // register itself to the global Kanban; note that test only
    JSON self_addr_info;
    self_addr_info["addr"] = addr;
    self_addr_info["port"] = port;
    register_live_querier(to_string(current_epoch_int), to_string(id), self_addr_info.dump().data());

    delete b_index_input;
}

static
void do_query(
    const char * request, int request_size,
    char * response, int response_size
)
{
    int current_epoch_int = get_epoch_int();
    if (current_epoch_int < 3) return; // reject when epoch < 3

    ecall_do_query(request, request_size, response, response_size);
}

static int last_epoch = 0;
static
void do_update(const char* addr, int port)
{
    int current_epoch_int;

    for (;;) { // main loop to monitor epoch update
        current_epoch_int = get_epoch_int();

        // must wait until epoch >= 3
        if (current_epoch_int < 3) {
            sleep(1);
            continue;
        }

        if (current_epoch_int != last_epoch) { // we need to update the setup
            querier_setup(addr, port);
            last_epoch = current_epoch_int;
        }
        sleep(1);
    }
}

void querier_loop(const char* addr, int port)
{
    // launch a daemon thread to monitor epoch and update index/digest at background
    thread querier_monitor = thread(do_update, addr, port);

    int server_sockfd;
    int client_sockfd;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int addr_len = sizeof(client_addr);

    char recvbuf[BUFFER_SIZE], sendbuf[BUFFER_SIZE];
    ssize_t data_length;

    if ( (server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int on = 1;
    if ( (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) <0 ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if ( ::bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if ( listen(server_sockfd, 1) < 0 ) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // a daemon to listen to all possible requests
    for (;;) {
        printf("[%d] %s listening on port: %d\n", getpid(), __func__, port);

        client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
        if ( client_sockfd < 0 ) {
            perror("accept");
            continue;
        }

        data_length = recv(client_sockfd, recvbuf, BUFFER_SIZE, 0);
        recvbuf[data_length] = '\0';
        memset(sendbuf, 0x0, BUFFER_SIZE);

        do_query(recvbuf, data_length, sendbuf, BUFFER_SIZE);

        // TODO: use openssl AES-GCM for secure channel
        send(client_sockfd, sendbuf, strlen(sendbuf), 0);
    }
out:
    close(client_sockfd);
    close(server_sockfd);

    querier_monitor.join();
}