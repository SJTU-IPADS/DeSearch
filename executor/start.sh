#/bin/bash

### This is a demo showing how to launch desearch executors
### You may add more executors as you wish
### For queriers you need to assign different ports

./executor crawler &
./executor crawler &

./executor indexer &
./executor indexer &

./executor reducer &
./executor reducer &

### The number of querier needed depends on how many shards there are
./executor querier 127.0.0.1 12345 &
./executor querier 127.0.0.1 12346 &
./executor querier 127.0.0.1 12347 &
./executor querier 127.0.0.1 12348 &
./executor querier 127.0.0.1 12349 &
./executor querier 127.0.0.1 12350 &