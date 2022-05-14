typedef unsigned int DWORD;

// NOTE: the called must free the space
static inline
char *Digest2Binary(const vector <string> &digest, int &size)
{
    int sum_size = 0;
    for ( auto & di : digest ) {
        sum_size += di.length() + 1; // add space for '0' as delimiter
    }

    char *ret = new char[sum_size];
    memset(ret, 0, sum_size);

    char *bin = ret;
    for ( auto & di : digest ) {
        strncpy(bin, di.data(), di.size());
        bin += di.size() + 1;
    }

    size = sum_size;
    return ret;
}

static inline
void Binary2Digest(const char *digest_bin, const int digest_size,
    vector <string> &digest_tbl)
{
#define TEMP_DIGEST_LEN (64*1024)
    string digest = "";
    char c_digest[TEMP_DIGEST_LEN] = "";
    int pos = 0;

    for ( int i = 0; i < digest_size; ++i ) {
        if (0 != *(digest_bin + i)) {
            c_digest[pos++] = *(digest_bin + i);
        } else {
            digest = c_digest;
            digest_tbl.push_back(digest);

            pos = 0;
            memset(c_digest, 0, sizeof(c_digest));
        }
    }
}

// NOTE: the called must free the space
static inline
char *Index2Binary(const unordered_map <DWORD, vector <DWORD> > &index, int &size)
{
    vector < DWORD > flatten_index;
    flatten_index.clear();

    for ( auto & i : index ) {
        flatten_index.push_back(i.first);
        for ( auto & j : i.second ) {
            flatten_index.push_back(j);
        }
        flatten_index.push_back(0); // delimiter
    }

    // marshall into a BYTE array
    int new_size = flatten_index.size() * (sizeof(DWORD)/sizeof(uint8_t));
    char *ret = new char[new_size];
    char *src = (char *)&flatten_index[0];
    memcpy(ret, src, new_size);
    size = new_size;

    return ret;
}

static inline
void Binary2Index(const char *index_bin, int index_size,
    unordered_map <DWORD, vector <DWORD> > &index_tbl)
{
    // re-construct the new index
    int recovery_size = index_size / (sizeof(DWORD)/sizeof(uint8_t));
    vector < DWORD > recovery_vector;
    recovery_vector.resize(recovery_size);
    char *recovery_dst = (char *)&recovery_vector[0];
    memcpy(recovery_dst, index_bin, index_size);

    // unmarshall into an index table
    bool is_first = true;
    DWORD key;
    for ( auto & i : recovery_vector ) {
        if (true == is_first) { key = i; is_first = false; }
        else {
            if (0 == i) { is_first = true; }
            else { index_tbl[key].push_back(i); }
        }
    }
}

static
char * Merge2Binary(const vector <string> digest_tbl,
    const unordered_map <DWORD, vector <DWORD> > index_tbl,
    int * b_output_size)
{
    // make digest a binary file
    int b_digest_size = 0;
    char * b_digest = Digest2Binary(digest_tbl, b_digest_size);

    // make index a binary file
    int b_index_size = 0;
    char * b_index = Index2Binary(index_tbl, b_index_size);

    // union into one file
    *b_output_size = sizeof(int) + b_index_size + b_digest_size;
    char *b_output = new char[*b_output_size];
    memcpy(b_output, &b_index_size, sizeof(int)); // write index size
    memcpy(b_output + sizeof(int), b_index, b_index_size);
    memcpy(b_output + sizeof(int) + b_index_size, b_digest, b_digest_size);

    delete b_index, b_digest;
    return b_output;
}