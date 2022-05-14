#include "enclave_common.hpp"

extern uint8_t skey[];
extern uint8_t pkey[];

void witness_clear(witness_context &context)
{
    context.tag = context.out = context.bin = "";
    context.ins.clear();
    context.stage = 0;
}

void witness_init(witness_context &context, const string &tag)
{
    context.tag = tag;
    context.stage = 1;
}

// allow multiple inputs, so called MISO (multiple-input-single-output)
void witness_input(witness_context &context, const void *start, size_t size) // append an input
{
    char _in[65] = "";
    sha256_string(start, size, _in);
    context.ins.push_back(_in);
    context.stage = 2;
}

// note that if two witness_outputs are invoked, the latter will fail
void witness_output(witness_context &context, const void *start, size_t size) // append an output
{
    if (3 == context.stage) return;

    char _out[65] = "";
    sha256_string(start, size, _out);
    context.out = _out;
    context.stage = 3;
}

void witness_func(witness_context &context, const void *start, size_t size) // const void (*func)(void)
{
    if (4 == context.stage) return;

    char _bin[65] = "";
    sha256_string(start, size, _bin);
    context.bin = _bin;
    context.stage = 4;
}

void witness_finalize(witness_context &context, string & output_witness) // generate signature
{
    if (5 == context.stage) return;

    JSON json_witness;
    json_witness["TAG"] = context.tag;
    json_witness["IN"] = json::Array();

    int nr_inputs = context.ins.size();
    for (int i = 0; i < nr_inputs; i++) {
        json_witness["IN"].append( context.ins[i] );
    }
    json_witness["OUT"] = context.out;
    json_witness["FUNC"] = context.bin;

    // prepare to sign the witness
    uint8_t sig[65]  = "";
    ed25519_sign(sig, skey, (uint8_t *)json_witness.dump().data(), json_witness.dump().length());

    char _sig[129] = "";
    ed25519_sig_to_string(sig, _sig);
    json_witness["SIG"] = _sig;
    
    output_witness = json_witness.dump();

    context.stage = 5;
}
