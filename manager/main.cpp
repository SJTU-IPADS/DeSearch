#include "config.hpp"
#include "manager.hpp"

#include <sw/redis++/redis++.h>
using namespace sw::redis;

#include <cstdio>
#include <unistd.h>
#include <string>

using namespace std;

static auto kb = Redis(KANBAN_ADDR);

int main(void)
{
    manager m;

    // define the maximum sharding number of index
    kb.set("SHARDING-MAX", to_string(INDEX_POOL_MAX));
    printf("There will be %d index shards.\n\n", INDEX_POOL_MAX);

    for (int epoch = 1; ;epoch++) {

#ifdef USE_STEEMIT_ITEM
        // Steemit task management
        static int steemit_block_id = START_STEEM_BLOCK_ID; // tune this value as you wish

        kb.set("Steemit-Block-ID", to_string(steemit_block_id));

        // skip the number of items that were obtained
        steemit_block_id += ITEMS_BULK_MAX*ITEMS_POOL_MAX;
#endif
        if (epoch <= 2) sleep(epoch);
        else sleep(TUNED_EPOCH_SECOND);
        // sleep(DEFAULT_EPOCH_SECOND);

        m.increment_epoch();
    }

    return 0;
}