#include "common.hpp"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        printf("usage: %s [job type]\n", argv[0]);
        return 1;
    }

    // job dispatch: desearch pipeline
    thread th;
    if (0 == strcmp(argv[1], "crawler")) {
        th = thread(crawler_loop);
        printf("[%d] crawler_loop created\n", getpid());
    } else if (0 == strcmp(argv[1], "indexer")) {
        th = thread(indexer_loop);
        printf("[%d] indexer_loop created\n", getpid());
    } else if (0 == strcmp(argv[1], "reducer")) {
        th = thread(reducer_loop);
        printf("[%d] reducer_loop created\n", getpid());
    } else if (0 == strcmp(argv[1], "querier")) {
        th = thread(querier_loop, argv[2], stoi(argv[3], nullptr, 10));
        printf("[%d] querier_loop created\n", getpid());
    }
    th.join();

    return 0;
}
