#include "common.hpp"

void ecall_parse_items(
    void * app_input, int app_input_size,
    void * app_output, int app_output_size,
    void * app_witness, int app_witness_size
);

// a word generator for stress test only
// should not be used in a multiple-node network
// because of its non-deterministic property
static
string get_fake_item()
{
    // initialize the random number generator
    random_device rd;
    mt19937 rng(rd());

    // compose a random string
    vector < string > word_pool = { "steemit", "desearch", "jam", "hotpot", "pie", "lemon", "brownie"};
    shuffle(word_pool.begin(), word_pool.end(), rng);

    string item = "";
    int item_len = 3;
    while (item_len--) {
        item += word_pool[item_len] + " ";
    }
    item += to_string( get_epoch_int() ); // tricks: ensure no same input exists

    return item;
}

/**
    Steemit is a twitter-like social media whose posts are stored on the
    Steem blockchain. Steem is fast blockchain with new Txn in 3 seconds.

    Desearch allows more user-defined crawlers because of the growing DApps.
*/

// XXX: this numbder is obtained from Kanban
static int __steemit_block_id, steemit_block_id;
static
string get_steemit_item()
{

    string item = "";

    char cmd[256] = "";
    snprintf(cmd, sizeof(cmd), "curl -s --data '{\"jsonrpc\":\"2.0\", \"method\":\"block_api.get_block\", \"params\":{\"block_num\":%d}, \"id\": 1}' https://api.steem.fans", steemit_block_id);

    FILE * pp = popen(cmd, "r");
    assert(NULL != pp);

    char tmp[1024];
    while ( fgets(tmp, sizeof(tmp), pp) != NULL ) {
        item += tmp;
    }
    pclose(pp);

    return item;
}

static
string o_get_item()
{
#ifdef USE_STEEMIT_ITEM
    return get_steemit_item();
#else
    return get_fake_item();
#endif
}

static
void do_crawl(void)
{
    int int_current_epoch = get_epoch_int();
    if (-1 == int_current_epoch) return;
    string current_epoch = to_string(int_current_epoch);

#ifdef USE_STEEMIT_ITEM
    auto offset = kb.get("Steemit-Block-ID");
    __steemit_block_id = stoi(*offset, nullptr, 10);
#endif

    for (int i = 1; i <= ITEMS_POOL_MAX; ++i) {

        // must check if the item exists; skip ones that other did
        // format: ITEMS-#EPOCH[n]-v[m]
        string item_key = "ITEMS-#EPOCH" + current_epoch + "-v" + to_string(i);
        if (true == o_check_key(item_key)) {
            continue;
        }

        // [IN] download ITEMS_BULK_MAX items in a batch
        vector <string> item_candidates;
        for (int j = 1; j <= ITEMS_BULK_MAX; ++j) {
#ifdef USE_STEEMIT_ITEM
            steemit_block_id = __steemit_block_id + (i-1)*ITEMS_BULK_MAX+j;
            printf("steemit_block_id %d\n", steemit_block_id);
#endif
            string item = o_get_item();
            item_candidates.push_back( item );
        }

#define OUTPUT_MAX_LENGTH     (16*1024*1024) // 16MB
        // [FUNC] parse items and generate witnesses
        int input_size = 0;
        char * input = Digest2Binary(item_candidates, input_size);

        char * output = new char [OUTPUT_MAX_LENGTH];
        memset(output, 0x0, OUTPUT_MAX_LENGTH);

        char * crawler_witness = new char [WITNESS_MAX_LENGTH];
        memset(crawler_witness, 0x0, WITNESS_MAX_LENGTH);

        ecall_parse_items(input, input_size, output, OUTPUT_MAX_LENGTH, crawler_witness, WITNESS_MAX_LENGTH);

        // [OUT] upload items and corresponding witnesses to kanban
        o_put_key(item_key, output, strlen(output));
        o_append_witness(current_epoch, crawler_witness);

        printf("[%d] %s o_append_witness\n", getpid(), __func__);
        // printf("%s: %s ==> %s\n", __func__, item_key.data(), output);

        delete output, crawler_witness;
    }
}

// a cralwer executor simply runs as a daemon
void crawler_loop(void)
{
    for (;;) {
        do_crawl();
        sleep(1);
    }
}
