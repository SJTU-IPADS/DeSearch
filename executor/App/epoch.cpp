#include "common.hpp"

using namespace std;

uint8_t pkey[32] = {
    0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
    0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
    0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
    0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
};

static
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