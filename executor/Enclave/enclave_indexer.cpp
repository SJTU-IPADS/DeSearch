#include "enclave_common.hpp"
#include "stopwords.hpp"

// in-enclave data structures

static vector <string> i_inputs;
static vector <string> i_digest_tbl; // doc[1:] -> doc[1:256]
static unordered_map <DWORD, vector <DWORD> > i_index_tbl; // wordID -> docID[]

// [IN] items ---> [OUT] index
static 
void create_indexer_witness(const vector<string> &inputs,
    const void *index, size_t index_sz, string &witness)
{
    witness_context indexer_context;
    witness_init(indexer_context, "STEEMIT INDEXER");

    for (int pos = 0; pos < inputs.size(); pos++) {
        witness_input(indexer_context, inputs[pos].data(), inputs[pos].length());
    }
    witness_output(indexer_context, index, index_sz);
    witness_func(indexer_context, INDEXER_TAG, sizeof(INDEXER_TAG));
    witness_finalize(indexer_context, witness);
}

/**
    build index ready for use later on,
    like crawlers, this should be open to new index algorithms
*/

static
void yield_index(const int docID, const char *doc, const int docLen)
{
    // set up the tokenizer using Sphinx
    ISphTokenizer * pTok = CreateTokenizer();
    pTok->SetBuffer ( (BYTE*)doc, docLen );

    BYTE * sTok = NULL;
    while ( ( sTok = pTok->GetToken() )!=NULL ) {
        DWORD wordID = sphCRC32( (char*)sTok, strlen((char*)sTok ));
        // append to the docID list
        i_index_tbl[wordID].push_back(docID);
        #if 1 // deduplicate the same docID
        if (i_index_tbl[wordID].size() > 1 &&
            i_index_tbl[wordID][i_index_tbl[wordID].size()-1] ==
            i_index_tbl[wordID][i_index_tbl[wordID].size()-2])
            i_index_tbl[wordID].pop_back();
        #endif
    }
    // this is a must, or otherwise memory leaks
    SafeDelete ( pTok );
}

static
void build_index(void)
{
    // make up digest vector
    for ( auto inputs : i_inputs ) {
        JSON j_item_vector = JSON::Load(inputs);
        for ( auto & item : j_item_vector.ArrayRange() ) {
            i_digest_tbl.push_back( trim_string( item.dump() ) );
        }
    }

    // build the index using a trivial impl.
    // trick: start document ID from 1 such that 0 can be delimiter
    int docID = 1;
    for ( auto doc = i_digest_tbl.begin(); doc != i_digest_tbl.end(); ++doc ) {
        // yield index for this doc
        yield_index(docID, doc->c_str(), doc->length());
        docID += 1;

        // trick: only first 256 bytes remain as digests
        if (doc->length() > DIGEST_MAX_LENGTH)
            doc->resize(DIGEST_MAX_LENGTH);
    }

    // filter out stopwords
    for (auto word: stopwords) {
        i_index_tbl.erase(sphCRC32(word.data()));
    }
    char STX[] = { 0x2 };
    i_index_tbl.erase(sphCRC32(STX));

    // dump_digest(i_digest_tbl);
    // dump_index(i_index_tbl);
}

// items:string[] => index:binary
void ecall_build_index(
    void * index_inputs, int index_inputs_size,
    char * index_output, int * index_output_size,
    void * index_witness, int index_witness_size)
{
    string witness;

    // recover inputs list
    Binary2Digest((const char *)index_inputs, index_inputs_size, i_inputs);

    // make up the index
    build_index();

    // turn the index to binary, combined with digest
    int _output_size = 0;
    char *_output = Merge2Binary(i_digest_tbl, i_index_tbl, &_output_size);
    create_indexer_witness(i_inputs, _output, _output_size, witness);

    // make the results visible to outside
    if (*index_output_size > _output_size) {
        memcpy(index_output, _output, _output_size);
    }
    if (index_witness_size > witness.size()) {
        memcpy(index_witness, witness.data(), witness.size());
    }
    *index_output_size = _output_size;

    // stateless
    i_inputs.clear();
    i_digest_tbl.clear();
    i_index_tbl.clear();

    delete _output;
}