#pragma once

// current all roles in desearch

#define CRAWLER_TAG  "cralwer"
#define INDEXER_TAG  "indexer"
#define REDUCER_TAG  "reducer"
#define QUERIER_TAG  "querier"
#define MANAGER_TAG  "manager"
#define AUDITOR_TAG  "auditor"

// testbed Kanban configuration

#define KANBAN_ADDR  "tcp://127.0.0.1:6379"

// magic numbers

#define DIGEST_MAX_LENGTH     (256)
#define INDEX_BLK_MAX_LENGTH  (512)
#define WITNESS_MAX_LENGTH    (1<<20)

// manager configuration

#define DEFAULT_EPOCH_SECOND (15 * 60)
#define TUNED_EPOCH_SECOND   (1  * 60)

// pipeline configuration

// NOTE: must be the power of 2
#define ITEMS_BULK_MAX 64   // crawler: a batch of crawled items
#define ITEMS_POOL_MAX 16   // crawler: how many item batches in an epoch
#define INDEX_BULK_MAX 4    // indexer: a batch of indexed docs
// manager, indexer, reducer, querier: how many shards to serve
// an ideal solution is to make this knob dynamic and tunable by all
#define INDEX_POOL_MAX (ITEMS_POOL_MAX / INDEX_BULK_MAX)

// crawler flag

#define USE_STEEMIT_ITEM
#define START_STEEM_BLOCK_ID (62396745)
