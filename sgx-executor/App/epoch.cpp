#include "common.hpp"

using namespace std;

extern uint8_t pkey[32];

string get_epoch(void)
{
    // get epoch string from Kanban
    auto _epoch_packet = kb.get("EPOCH");
    if (_epoch_packet) {
    } else {
        printf("The system hasn't been init'ed yet ...\n");
        return "-1";
    }

    string string_epoch_packet = *_epoch_packet;
    JSON epoch_packet = JSON::Load(string_epoch_packet);

    // tear sig and epoch apart
    uint8_t _sig[64];
    string string_sig = epoch_packet["SIG"].ToString();
    string string_epoch = epoch_packet["EPOCH"].ToString();

    // verify the epoch packet
    ed25519_string_to_sig(string_sig.data(), _sig);
    int res = ed25519_verify(pkey, (uint8_t *)string_epoch.data(), string_epoch.length(), _sig);
    // printf("EPOCH Verification %s\n", res ? "passed" : "failed");

    return res ? string_epoch : "-1";
}

int get_epoch_int(void)
{
    string string_epoch = get_epoch();
    return stoi(string_epoch, nullptr, 10);
}