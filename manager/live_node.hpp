/**
 * This is a very simple way to coordinate clients and queriers
 * intended for tests only
 * 
 * A real-world deployment should involve Enclave RA process between manager
 * and new queriers to ensure no fake nodes are joining; and a heartbeat machanism
 * to ping-pong between manager and live queriers to update the live node list
 */

#include <algorithm>
#include <random>
#include <string>
#include <vector>

using namespace std;

// this impl. uses a straightforward method to register a querier exexcutor
void register_live_querier(const string& epoch, const string& partition, const string& addr)
{
    kb.rpush("NODE-#EPOCH" + epoch + "-v" + partition, addr);
}

// simply return a list of live querier exexcutors
// random pick nodes in each partition
// if a certain partition contains zero live node, still return the list
// because users might also want the results, and they can learn from the witness verification process
// NOTE: this impl. cannot detect failure querier, which may yield incomplete list
void get_live_querier(const string& epoch, vector<string>& live_list)
{
    // initialize the random number generator for shuffling
    random_device rd;
    mt19937 rng(rd());

    for (int partition = 1; partition <= INDEX_POOL_MAX; partition++) {
        vector<string> partition_list;
        kb.lrange("NODE-#EPOCH" + epoch + "-v" + to_string(partition),
            0, -1, std::back_inserter(partition_list));
        shuffle(partition_list.begin(), partition_list.end(), rng);
        if (partition_list.size() > 0)
            live_list.push_back(partition_list[0]);
    }
}