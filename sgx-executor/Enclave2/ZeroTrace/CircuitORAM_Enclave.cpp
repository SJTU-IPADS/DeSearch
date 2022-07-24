#include "CircuitORAM_Enclave.hpp"

uint32_t reverseBits(uint32_t n)
{
    n = (n >> 1) & 0x55555555 | (n << 1) & 0xaaaaaaaa;
    n = (n >> 2) & 0x33333333 | (n << 2) & 0xcccccccc;
    n = (n >> 4) & 0x0f0f0f0f | (n << 4) & 0xf0f0f0f0;
    n = (n >> 8) & 0x00ff00ff | (n << 8) & 0xff00ff00;
    n = (n >> 16) & 0x0000ffff | (n << 16) & 0xffff0000;
    return n;
}

uint32_t firstkbits(uint32_t n, uint32_t k)
{
    return (n >> (32 - k));
}

void CircuitORAM::Initialize(uint32_t instance_id, uint8_t oram_type, uint8_t pZ, uint32_t pmax_blocks, uint32_t pdata_size, uint32_t pstash_size, uint32_t poblivious_flag, uint32_t precursion_data_size, uint8_t precursion_levels)
{
#ifdef BUILDTREE_DEBUG
    printf("In CircuitORAM::Initialize, Started Initialize\n");
#endif

    ORAMTree::SetParams(instance_id, oram_type, pZ, pmax_blocks, pdata_size, pstash_size, poblivious_flag, precursion_data_size, precursion_levels);
    ORAMTree::Initialize();

    uint32_t d_largest;
    if (recursion_levels == 1)
        d_largest = D_level[0];
    else
        d_largest = D_level[recursion_levels - 1];

    //deepest and target buffers also treat stash as level 0 of the logical path, hence +1
    deepest = (uint32_t*)malloc(sizeof(uint32_t) * (d_largest + 1));
    target = (uint32_t*)malloc(sizeof(uint32_t) * (d_largest + 1));
    deepest_position = (int32_t*)malloc((d_largest + 1) * sizeof(uint32_t));
    target_position = (int32_t*)malloc((d_largest + 1) * sizeof(uint32_t));
    serialized_block_hold = (unsigned char*)malloc(data_size + ADDITIONAL_METADATA_SIZE);
    serialized_block_write = (unsigned char*)malloc(data_size + ADDITIONAL_METADATA_SIZE);

    access_counter = (uint32_t*)malloc((recursion_levels) * sizeof(uint32_t));
    for (uint8_t i = 0; i < recursion_levels; i++)
        access_counter[i] = 0;

#ifdef BUILDTREE_DEBUG
    printf("Finished Initialize\n");
#endif
}

uint32_t* CircuitORAM::prepare_target(uint32_t leaf, unsigned char* serialized_path, uint32_t block_size, uint32_t level, uint32_t* deepest, int32_t* target_position)
{
    uint32_t N = N_level[level];
    uint32_t D = D_level[level];
    int32_t i, k;
    nodev2* listptr_t;
    int32_t src = -1, dest = -1;
    unsigned char* serialized_path_ptr = serialized_path;
    listptr_t = recursive_stash[level].getStart();

    for (i = D; i >= 0; i--) {
        target[i] = -1;

        // If i==src : target[i] = dest, dest = -1 , src = -1
        pt_settarget(&(target[i]), &dest, &src, i == src);

        //Fix for i = 0
        if (i == 0) {

        } else {
            uint32_t flag_empty_slot = 0;
            uint32_t target_position_set = 0;
            for (k = 0; k < Z; k++) {
                bool dummy_flag = isBlockDummy(serialized_path_ptr, gN);
                flag_empty_slot = (flag_empty_slot || dummy_flag);

                uint32_t flag_stp = dummy_flag && (!target_position_set);

                //if flag_stp: target_position[i] = k, target_set = 1
                pt_set_target_position(&(target_position[i]), k, &target_position_set, flag_stp);
                serialized_path_ptr += (block_size);
            }

            uint32_t flag_pt = (((dest == -1 && flag_empty_slot)) && (deepest[i] != -1));
            //if flag_pt : src = deepest[i], dest = i
            pt_set_src_dest(&src, &dest, deepest[i], i, flag_pt);
        }
    }

    return target;
}

// Root - to - leaf linear scan , NOTE: serialized_path from LS is filled in leaf to root
// Creates deepest[] , where deepest[i] stores the source level of the deepest block in path[0,...,i-1] that can reside in path[i]
uint32_t* CircuitORAM::prepare_deepest(uint32_t leaf, unsigned char* serialized_path, uint32_t block_size, uint32_t level, int32_t* deepest_position)
{
    uint32_t N = N_level[level];
    uint32_t D = D_level[level];
    int32_t i;
    uint32_t k;
    unsigned char* serialized_path_ptr = serialized_path + (Z * D - 1) * (block_size);
    uint32_t local_deepest = 0;
    int32_t goal = -1, src = -1;
    nodev2* listptr_t;

    listptr_t = recursive_stash[level].getStart();

    for (i = 0; i <= D; i++) {
        if (i == 0) {
            //deepest[stash]
            for (k = 0; k < stash_size; k++) {
                uint32_t l1 = getTreeLabel(listptr_t->serialized_block);
                uint32_t l2 = leaf;

#ifdef ACCESS_CORAM_DEBUG
                printf("(%d, %d) - local_deepest = %d\n", getTreeLabel(listptr_t->serialized_block), leaf, local_deepest);
#endif

                //level_in_path keeps track of which level in the path will this block fit into
                uint32_t level_in_path = D, position_in_path = 0;
                for (uint32_t j = 0; j < D; j++) {
                    uint32_t flag_deepest = (l1 == l2) && (!isBlockDummy(listptr_t->serialized_block, gN) && l1 > local_deepest);
                    oset_value(&local_deepest, l1, flag_deepest);
                    oset_value((uint32_t*)&(deepest_position[i]), k, flag_deepest);
                    oset_value(&position_in_path, level_in_path, flag_deepest);

#ifdef ACCESS_CORAM_DEBUG3
                    if (!isBlockDummy(listptr_t->serialized_block, gN)) {
                        printf("\t(%d, %d) - local_deepest = %d, level_in_path = %d\n",
                            l1, l2, local_deepest, level_in_path);
                    }
#endif

                    l1 = l1 >> 1;
                    l2 = l2 >> 1;
                    level_in_path -= 1;
                }

                oset_value((uint32_t*)&(deepest_position[i]), k, position_in_path > goal);
                oset_goal_source(i, position_in_path, (int32_t)position_in_path > goal && position_in_path > i, &src, &goal);
                listptr_t = listptr_t->next;
            }
        } else {
            //if goal > i, deepest[i] = goal
            deepest[i] = -1;
            uint32_t flag_pdsd = (goal >= i);
            pd_setdeepest(&(deepest[i]), src, flag_pdsd);
#ifdef ACCESS_CORAM_DEBUG3
            printf("i = %d, Goal is %d , src = %d, deepest[i] = %d\n", i, goal, src, deepest[i]);
#endif

            oset_value((uint32_t*)&src, (uint32_t)-1, goal == i);
            oset_value((uint32_t*)&goal, (uint32_t)-1, goal == i);

            for (k = 0; k < Z; k++) {
                bool dummy_flag = isBlockDummy(serialized_path_ptr, gN);
                uint32_t l1 = getTreeLabel(serialized_path_ptr);
                uint32_t l2 = leaf;
                uint32_t level_in_path = D, position_in_path = 0;

#ifdef ACCESS_CORAM_DEBUG
                printf("(%d, %d) - local_deepest = %d\n", getTreeLabel(serialized_path_ptr), leaf, local_deepest);
#endif

                //TODO: Did not make any index tweaks on this for loop in new revision
                // Might need to patch this.
                for (uint32_t j = 0; j < D - i; j++) {
                    uint32_t flag_deepest = (l1 == l2) && !dummy_flag && l1 > local_deepest;
                    oset_value(&local_deepest, l1, flag_deepest);
                    oset_value((uint32_t*)&(deepest_position[i]), k, flag_deepest);
                    oset_value(&position_in_path, level_in_path, flag_deepest);

#ifdef ACCESS_CORAM_DEBUG3
                    if (!dummy_flag)
                        printf("\t(%d, %d) - local_deepest = %d, deepest_position[%d] = %d\n", l1, l2, local_deepest, i - 1, deepest_position[i - 1]);
#endif
                    l1 = l1 >> 1;
                    l2 = l2 >> 1;
                    level_in_path -= 1;
                }
                oset_value((uint32_t*)&(deepest_position[i]), k, position_in_path > goal);
                oset_goal_source(i, position_in_path, (int32_t)position_in_path > goal && position_in_path > i, &src, &goal);
                serialized_path_ptr -= (block_size);
            }
        }
    }
    //deepest[0] = _|_
    deepest[0] = -1;
    return deepest;
}

//Pass from Root to leaf (Opposite direction of serialized_path)
void CircuitORAM::EvictOnceFast(uint32_t* deepest, uint32_t* target, int32_t* deepest_position, int32_t* target_position, unsigned char* serialized_path, unsigned char* path_hash, uint32_t level, unsigned char* new_path_hash, uint32_t leaf)
{
    uint32_t dlevel = D_level[level];
    uint32_t nlevel = N_level[level];
    int32_t hold = -1, dest = -1;
    uint32_t write_flag = 0;
    uint32_t tblock_size, tdata_size;

    //Check level and set block size
    if (recursion_levels == 1 || level == recursion_levels - 1) {
        tblock_size = data_size + ADDITIONAL_METADATA_SIZE;
        tdata_size = data_size;
    } else {
        tblock_size = recursion_data_size + ADDITIONAL_METADATA_SIZE;
        tdata_size = recursion_data_size;
    }

    unsigned char* serialized_path_ptr = serialized_path + (Z * dlevel - 1) * (tblock_size);
    unsigned char* serialized_path_ptr2 = serialized_path_ptr;

    nodev2* listptr_t;
    listptr_t = recursive_stash[level].getStart();
    write_flag = 0;

    for (uint32_t i = 0; i <= (dlevel); i++) {
        // Oblivious function for Setting Write flag, hold , dest and moving held_block to block_write
        uint32_t flag_1 = ((hold != -1) && (i == dest));
        omove_serialized_block(serialized_block_write, serialized_block_hold, tdata_size, flag_1);
        //oset_hold_dest : Set hold = -1, dest = -1, write_flag =1
        oset_hold_dest(&hold, &dest, &write_flag, flag_1);

        if (i == 0) {
            //Scan the blocks in Stash to pick deepest block from it
            for (uint32_t k = 0; k < stash_size; k++) {
                unsigned char* stash_block = listptr_t->serialized_block;
                uint32_t flag_hold = (k == deepest_position[i] && (target[i] != -1) && (!isBlockDummy(stash_block, gN)));

#ifdef DEBUG_EFO
                if (flag_hold)
                    printf("Held block ID = %d, treelabel = %d\n", getId(stash_block), getTreeLabel(stash_block));
#endif

                omove_serialized_block(serialized_block_hold, stash_block, tdata_size, flag_hold);
                oset_value(getIdPtr(stash_block), gN, flag_hold);
                oset_value((uint32_t*)&dest, target[i], flag_hold);
                oset_value((uint32_t*)&hold, target[i], flag_hold);
                listptr_t = listptr_t->next;
            }

        } else {
            //Scan the blocks in Path[i] to pick deepest block from it
            for (uint32_t k = 0; k < Z; k++) {
                uint32_t flag_hold = (k == deepest_position[i]) && (hold == -1) && (target[i] != -1);
                omove_serialized_block(serialized_block_hold, serialized_path_ptr, tdata_size, flag_hold);
#ifdef DEBUG_EFO
                if (flag_hold)
                    printf("Held Block in bucket, ID = %d, treelabel = %d\n", getId(serialized_block_hold), getTreeLabel(serialized_block_hold));
#endif
                uint32_t* temp_pos = (uint32_t*)serialized_path_ptr;
                oset_value((uint32_t*)&(temp_pos[4]), gN, flag_hold);
                oset_value((uint32_t*)&dest, target[i], flag_hold);
                oset_value((uint32_t*)&hold, target[i], flag_hold);
                serialized_path_ptr -= tblock_size;
            }
        }

        //if(target[i]!= -1)
        //	dest = target[i];
        uint32_t flag_t = ((int32_t)target[i] != -1);
        oset_value((uint32_t*)&dest, target[i], flag_t);

        if (i != 0) {
            uint32_t flag_w = 0;
            for (int32_t k = (Z - 1); k >= 0; k--) {
                flag_w = (write_flag && (k == target_position[i]));
#ifdef DEBUG_EFO
                if (flag_w)
                    printf("serialized_path_ptr2 : ID = %d, treeLabel = %d\n", getId(serialized_path_ptr),
                        getTreeLabel(serialized_path_ptr2));
#endif

                omove_serialized_block(serialized_path_ptr2, serialized_block_write, tdata_size, flag_w);

#ifdef DEBUG_EFO
                printf("i = %d, k = %d, write_flag = %d, flag_w = %d, writeblock_id = %d, writeblock_treelabel = %d\n", i, k, write_flag, flag_w,
                    getId(serialized_block_write), getTreeLabel(serialized_block_write));
                if (flag_w)
                    printf("serialized_path_ptr2 : ID = %d, treeLabel = %d\n", getId(serialized_path_ptr2),
                        getTreeLabel(serialized_path_ptr2));
#endif

                oset_value(&write_flag, (uint32_t)0, flag_w);

                // printf("EVERY PATHBLOCK: serialized_path_ptr2 : ID = %d, treeLabel = %d\n",getId(serialized_path_ptr2),
                // getTreeLabel(serialized_path_ptr2));
                serialized_path_ptr2 -= tblock_size;
            }
        }
    }
}

void CircuitORAM::CircuitORAM_FetchBlock(uint32_t* return_value, uint32_t leaf, uint32_t newleaf, char opType,
    uint32_t id, uint32_t position_in_id, uint32_t newleaf_nextlevel, uint32_t level, unsigned char* data_in, unsigned char* data_out)
{

    //FetchBlock over Path
    uint32_t nlevel = N_level[level];
    uint32_t dlevel = D_level[level];
    unsigned char* decrypted_path_ptr = decrypted_path;
    unsigned char* path_ptr;
    uint32_t i, k;
    uint8_t rt;
    uint32_t tblock_size, tdata_size;

    if (recursion_levels == 1 || level == recursion_levels - 1) {
        tblock_size = data_size + ADDITIONAL_METADATA_SIZE;
        tdata_size = data_size;
    } else {
        tblock_size = recursion_data_size + ADDITIONAL_METADATA_SIZE;
        tdata_size = recursion_data_size;
    }

#ifdef ACCESS_DEBUG
    printf("Fetched Path : \n");
    showPath_reverse(decrypted_path, Z, dlevel, data_size, leaf);
#endif

    for (i = 0; i < (Z * (dlevel)); i++) {
        if (oblivious_flag) {
            uint32_t block_id = getId(decrypted_path_ptr);
            // if(block_id==id&&level==recursion_levels)
            //	printf("Flag for move to serialized_result_block SET ! \n");
            omove_serialized_block(serialized_result_block, decrypted_path_ptr, data_size, block_id == id);
            oset_block_as_dummy(getIdPtr(decrypted_path_ptr), gN, block_id == id);
        } else {
            uint32_t block_id = getId(decrypted_path_ptr);
            if (block_id == id) {
                memcpy(serialized_result_block, decrypted_path_ptr, tblock_size);
                setId(decrypted_path_ptr, gN);
            }
        }
        decrypted_path_ptr += (tblock_size);
    }

    // FetchBlock over Stash
    nodev2* listptr_t;
    listptr_t = recursive_stash[level].getStart();

    if (oblivious_flag) {
        for (k = 0; k < stash_size; k++) {
            unsigned char* stash_block = listptr_t->serialized_block;
            uint32_t block_id = getId(stash_block);
            bool flag_move = (block_id == id && !isBlockDummy(stash_block, gN));
            omove_serialized_block(serialized_result_block, stash_block, data_size, flag_move);
            listptr_t = listptr_t->next;
        }
        if (level != recursion_levels - 1) {
            uint32_t* data_iter = (uint32_t*)getDataPtr(serialized_result_block);
            for (k = 0; k < x; k++) {
                uint32_t flag_ore = (position_in_id == k);
                oset_value(return_value, data_iter[k], flag_ore);
                oset_value(&(data_iter[k]), newleaf_nextlevel, flag_ore);
            }
        }
    } else {
        for (k = 0; k < stash_size; k++) {
            unsigned char* stash_block = listptr_t->serialized_block;
            if (getId(stash_block) == id) {
                memcpy(serialized_result_block, stash_block, tblock_size);
                setId(stash_block, gN);
            }
            listptr_t = listptr_t->next;
        }

        if (level != recursion_levels - 1) {
            uint32_t* data_iter = (uint32_t*)getDataPtr(serialized_result_block);
            data_iter[position_in_id] = newleaf_nextlevel;
        }
    }

    // Use serialized_result_block to perform the Access operation
    // Obliviously read to data_out or obliviously write data_in to serialized_result_block
    // before inserting it into stash.
    unsigned char* data_ptr = getDataPtr(serialized_result_block);
    bool flag_w = (opType == 'w');
    omove_buffer((unsigned char*)data_ptr, data_in, tdata_size, flag_w);
    bool flag_r = (opType == 'r');
    omove_buffer(data_out, (unsigned char*)data_ptr, tdata_size, flag_r);

#ifdef ACCESS_DEBUG
    printf("Path before encrypt and upload: \n");
    showPath_reverse(decrypted_path, Z, dlevel, data_size, leaf);
#endif

//Encrypt Path Module
#ifdef ENCRYPTION_ON
    encryptPath(decrypted_path, encrypted_path, (Z * (dlevel)), data_size);
#endif

//Path Integrity Module
#ifndef PASSIVE_ADVERSARY

#ifdef ENCRYPTION_ON
    path_ptr = encrypted_path;
#else
    path_ptr = decrypted_path;
#endif

    createNewPathHash(path_ptr, path_hash, new_path_hash, leaf, tblock_size, level);
#endif

    uint32_t path_size = tblock_size * Z * D_level[level];
    uint32_t new_path_hash_size = HASH_LENGTH * D_level[level];

#ifdef ENCRYPTION_ON
    uploadPath(leaf, encrypted_path, path_size, new_path_hash, new_path_hash_size, level);
#else
    uploadPath(leaf, decrypted_path, path_size, new_path_hash, new_path_hash_size, level);
#endif

    //Set newleaf for fetched_block
    setTreeLabel(serialized_result_block, newleaf);

    //Insert Fetched Block into Stash
    if (oblivious_flag) {
        recursive_stash[level].pass_insert(serialized_result_block, isBlockDummy(serialized_result_block, gN));
    } else {
        recursive_stash[level].insert(serialized_result_block);
    }

    if (level == recursion_levels) {
        recursive_stash[level].PerformAccessOperation(opType, id, newleaf, data_in, data_out);
    }
}

uint32_t CircuitORAM::access_oram_level(char opType, uint32_t leaf, uint32_t id, uint32_t position_in_id, uint32_t level, uint32_t newleaf, uint32_t newleaf_nextleaf, unsigned char* data_in, unsigned char* data_out)
{
    uint32_t return_value = -1;

    decrypted_path = downloadPath(leaf, path_hash, level);

    return_value = CircuitORAM_Access(opType, id, position_in_id, leaf, newleaf, newleaf_nextleaf, decrypted_path,
        path_hash, level, data_in, data_out);
    return return_value;
}

void CircuitORAM::EvictionRoutine(uint32_t leaf, uint32_t level)
{
    uint32_t dlevel = D_level[level];
    uint32_t nlevel = N_level[level];
    uint8_t rt;
    uint32_t i;
    uint64_t leaf_right, leaf_left, temp_n;
    unsigned char *eviction_path_left, *eviction_path_right;
    unsigned char* decrypted_path_ptr = decrypted_path;
    unsigned char* path_ptr;
    uint32_t tblock_size, tdata_size;
    if (recursion_levels == 1 || level == recursion_levels - 1) {
        tblock_size = data_size + ADDITIONAL_METADATA_SIZE;
        tdata_size = data_size;
    } else {
        tblock_size = recursion_data_size + ADDITIONAL_METADATA_SIZE;
        tdata_size = recursion_data_size;
    }

#ifdef SHOW_STASH_COUNT_DEBUG
    print_stash_count(level, nlevel);
#endif

#ifdef REVERSE_LEXICOGRAPHIC_EVICTION
    uint32_t ac = access_counter[level];
    uint32_t ll = firstkbits(reverseBits(ac), D_level[level] - 1);
    uint32_t lr = firstkbits(reverseBits(ac + 1), D_level[level] - 1);
    leaf_left = nlevel + ll;
    leaf_right = nlevel + lr;
    access_counter[level] += 2;
    if (ac == N_level[level])
        access_counter[level] = 0;
#else
    sgx_read_rand((unsigned char*)&temp_n, 4);
    leaf_left = nlevel + (temp_n % (nlevel / 2));
    sgx_read_rand((unsigned char*)&temp_n, 4);
    leaf_right = nlevel + (nlevel / 2) + (temp_n % (nlevel / 2));
#endif

    //Sample the leaves for eviction

    eviction_path_left = downloadPath(leaf_left, path_hash, level);
    for (uint32_t e = 0; e < dlevel + 1; e++) {
        deepest_position[e] = -1;
        target_position[e] = -1;
    }

#ifdef SHOW_STASH_CONTENTS
    if (level == recursion_levels - 1)
        recursive_stash[level].displayStashContents(nlevel, false);
#endif

#ifdef ACCESS_DEBUG
    printf("Level = %d, leaf_left = %d, Eviction_path_left:\n", level, leaf_left);
    showPath_reverse(eviction_path_left, Z, dlevel, data_size, leaf_left);
    print_stash_count(level, nlevel);
#endif

    // prepare_deepest() Create deepest[]
    deepest = prepare_deepest(leaf_left, eviction_path_left, tblock_size, level, deepest_position);

#ifdef ACCESS_CORAM_META_DEBUG
    printf("Printing Deepest : \n");
    for (i = 0; i <= dlevel; i++) {
        printf("Level : %d, %d, deepest_position = %d\n", i, deepest[i], (int32_t)(deepest_position[i]));
    }
#endif

    // prepare_target Create target[]
    target = prepare_target(leaf_left, eviction_path_left, tblock_size, level, deepest, target_position);

#ifdef ACCESS_CORAM_META_DEBUG
    printf("Printing Target : \n");
    for (i = 0; i <= dlevel; i++) {
        printf("%d , target_position = %d\n", target[i], (int32_t)(target_position[i]));
    }
#endif

    EvictOnceFast(deepest, target, deepest_position, target_position, eviction_path_left, path_hash, level, new_path_hash, leaf_left);

#ifdef ACCESS_DEBUG
    printf("\nLevel = %d, Eviction_path_left after Eviction:\n", level);
    if (level == recursion_levels)
        printf("Blocksize = %d\n", tblock_size);
    showPath_reverse(eviction_path_left, Z, dlevel, tdata_size, leaf_left);
    print_stash_count(level, nlevel);
#endif

#ifdef ENCRYPTION_ON
    encryptPath(eviction_path_left, encrypted_path, (Z * (dlevel)), tdata_size);
#endif

    uint32_t path_size = tblock_size * Z * D_level[level];
    uint32_t new_path_hash_size = HASH_LENGTH * D_level[level];
#ifdef ENCRYPTION_ON
    uploadPath(leaf_left, encrypted_path, path_size, new_path_hash, new_path_hash_size, level);
#else
    uploadPath(leaf_left, eviction_path_left, path_size, new_path_hash, new_path_hash_size, level);
#endif

#ifndef PASSIVE_ADVERSARY
#ifdef ENCRYPTION_ON
    path_ptr = encrypted_path;
#else
    path_ptr = eviction_path_left;
#endif

    createNewPathHash(path_ptr, path_hash, new_path_hash, leaf_left, tblock_size, level);

#endif

    eviction_path_right = downloadPath(leaf_right, path_hash, level);

#ifdef ACCESS_DEBUG
    printf("\nLevel = %d, leaf_right = %d, Eviction_path_right:\n", level, leaf_right);
    showPath_reverse(eviction_path_right, Z, dlevel, data_size, leaf_right);
    print_stash_count(level, nlevel);
#endif

    //Eviction Path Right
    for (uint32_t e = 0; e < dlevel + 1; e++) {
        deepest_position[e] = -1;
        target_position[e] = -1;
    }

    deepest = prepare_deepest(leaf_right, eviction_path_right, tblock_size, level, deepest_position);
    target = prepare_target(leaf_right, eviction_path_right, tblock_size, level, deepest, target_position);

#ifdef ACCESS_CORAM_META_DEBUG
    printf("Printing Deepest : \n");
    for (i = 0; i <= dlevel; i++) {
        printf("Level : %d,%d, deepest_position = %d\n", i, deepest[i], (int32_t)(deepest_position[i]));
    }
#endif

#ifdef ACCESS_CORAM_META_DEBUG
    printf("Printing Target : \n");
    for (i = 0; i <= dlevel; i++) {
        printf("%d , target_position = %d\n", target[i], (int32_t)(target_position[i]));
    }
#endif

    EvictOnceFast(deepest, target, deepest_position, target_position, eviction_path_right, path_hash, level, new_path_hash, leaf_right);
#ifdef ACCESS_DEBUG
    printf("\nLevel = %d, Eviction_path_right after Eviction:\n", level);
    showPath_reverse(eviction_path_right, Z, dlevel, tdata_size, leaf_right);
    print_stash_count(level, nlevel);
#endif

#ifdef ENCRYPTION_ON
    encryptPath(eviction_path_right, encrypted_path, Z * dlevel, tdata_size);
#endif

#ifndef PASSIVE_ADVERSARY

#ifdef ENCRYPTION_ON
    path_ptr = encrypted_path;
#else
    path_ptr = eviction_path_right;
#endif

    createNewPathHash(path_ptr, path_hash, new_path_hash, leaf_right, tblock_size, level);

#endif

#ifdef ENCRYPTION_ON
    uploadPath(leaf_right, encrypted_path, path_size, new_path_hash, new_path_hash_size, level);
#else
    uploadPath(leaf_right, eviction_path_right, path_size, new_path_hash, new_path_hash_size, level);
#endif

#ifdef SHOW_STASH_COUNT_DEBUG
    print_stash_count(level, nlevel);
#endif
}

uint32_t CircuitORAM::CircuitORAM_Access(char opType, uint32_t id, uint32_t position_in_id, uint32_t leaf, uint32_t newleaf, uint32_t newleaf_nextlevel, unsigned char* decrypted_path, unsigned char* path_hash, uint32_t level, unsigned char* data_in, unsigned char* data_out)
{

    uint32_t dlevel = D_level[level];
    uint64_t nlevel = N_level[level];
    uint32_t i, k;
    uint8_t rt;
    uint32_t return_value = 0;

#ifdef EXITLESS_MODE
    serialized_path = resp_struct->new_path;
    new_path_hash = resp_struct->new_path_hash;
#endif

    CircuitORAM_FetchBlock(&return_value, leaf, newleaf, opType, id, position_in_id, newleaf_nextlevel, level, data_in, data_out);

//Free Result_block (Application/Use result_block otherwise)
#ifdef ACCESS_DEBUG
    printf("\nResult_block ID = %d, label = %d, Return value = %d\n", getId(serialized_result_block),
        getTreeLabel(serialized_result_block), return_value);
    printf("Done with Fetch\n\n");
#endif

    EvictionRoutine(leaf, level);

    return return_value;
}

uint32_t CircuitORAM::access(uint32_t id, int32_t position_in_id, char opType, uint8_t level, unsigned char* data_in, unsigned char* data_out, uint32_t* prev_sampled_leaf)
{

    uint32_t leaf = 0;
    uint32_t nextLeaf;
    uint32_t id_adj;
    uint32_t newleaf;
    uint32_t newleaf_nextlevel = -1;
    unsigned char random_value[ID_SIZE_IN_BYTES];

    if (recursion_levels == 1) {
        sgx_status_t rt = SGX_SUCCESS;
        rt = sgx_read_rand((unsigned char*)random_value, ID_SIZE_IN_BYTES);
        uint32_t newleaf = (N_level[0]) + (*((uint32_t*)random_value) % N_level[0]);

        if (oblivious_flag) {
            //            oarray_search(posmap, id, & leaf, newleaf, max_blocks_level[0]);
            // WARNING: relax security guarantee for performance issue
            leaf = posmap[id];
            posmap[id] = newleaf;
        } else {
            leaf = posmap[id];
            posmap[id] = newleaf;
        }
        if (leaf == 0) {
            printf("BBBBBBBBBBBBBBBBBBBBBBBBBBBB Oh MY MY MY: running out of block space\n");
        }

        decrypted_path = downloadPath(leaf, path_hash, 0);

        CircuitORAM_Access(opType, id, -1, leaf, newleaf, -1, decrypted_path, path_hash, level, data_in, data_out);

    } else if (level == 0) {
        sgx_read_rand((unsigned char*)random_value, ID_SIZE_IN_BYTES);
        //To slot into one of the buckets of next level
        newleaf = (N_level[level + 1] - 1) + (*((uint32_t*)random_value) % (N_level[level + 1]));
        *prev_sampled_leaf = newleaf;

        if (oblivious_flag) {
            //            oarray_search(posmap, id, & leaf, newleaf, real_max_blocks_level[level]);
            // WARNING: relax security guarantee for performance issue
            leaf = posmap[id];
            posmap[id] = newleaf;
        } else {
            leaf = posmap[id];
            posmap[id] = newleaf;
        }

#ifdef ACCESS_DEBUG
        printf("access : Level = %d: \n Requested_id = %d, Corresponding leaf from posmap = %d, Newleaf assigned = %d,\n\n", level, id, leaf, newleaf);
#endif
        return leaf;
    } else if (level == recursion_levels) {
        //DataAccess for leaf.
        id_adj = id / x;
        position_in_id = id % x;
        leaf = access(id_adj, position_in_id, opType, level - 1, data_in, data_out, prev_sampled_leaf);
#ifdef ACCESS_DEBUG
        printf("access, Level = %d:  before access_oram_level : Block_id = %d, Newleaf = %d, Leaf from level = %d, Flag = %d\n", level, id, *prev_sampled_leaf, leaf, oblivious_flag);
#endif
        //ORAM ACCESS of recursion_levels is to fetch entire Data block, no position_in_id, hence -1)
        access_oram_level(opType, leaf, id, -1, level, *prev_sampled_leaf, -1, data_in, data_out);
        return 0;
    } else {
        id_adj = id / x;
        uint32_t nl_position_in_id = (level == 1) ? -1 : id % x;
        leaf = access(id_adj, nl_position_in_id, opType, level - 1, data_in, data_out, prev_sampled_leaf);

        //sampling leafs for a level ahead
        sgx_read_rand((unsigned char*)random_value, ID_SIZE_IN_BYTES);
        newleaf_nextlevel = (N_level[level + 1]) + (*((uint32_t*)random_value) % N_level[level + 1]);
        newleaf = *prev_sampled_leaf;
        *prev_sampled_leaf = newleaf_nextlevel;
#ifdef ACCESS_DEBUG
        printf("access, Level = %d :\n leaf = %d, block_id = %d, position_in_id = %d, newLeaf = %d, newleaf_nextlevel = %d\n", level, leaf, id, position_in_id, newleaf, newleaf_nextlevel);
#endif
        nextLeaf = access_oram_level(opType, leaf, id, position_in_id, level, newleaf, newleaf_nextlevel, data_in, data_out);
#ifdef ACCESS_DEBUG
        printf("access, Level = %d : \n nextLeaf from level = %d\n", level, nextLeaf);
#endif
        return nextLeaf;
    }
    return nextLeaf;
}

void CircuitORAM::Create(uint32_t instance_id, uint8_t oram_type, uint8_t pZ, uint32_t pmax_blocks, uint32_t pdata_size, uint32_t pstash_size, uint32_t poblivious_flag, uint32_t precursion_data_size, uint8_t precursion_levels)
{
    Initialize(instance_id, oram_type, pZ, pmax_blocks, pdata_size, pstash_size, poblivious_flag, precursion_data_size, precursion_levels);
}

void CircuitORAM::Access(uint32_t id, char opType, unsigned char* data_in, unsigned char* data_out)
{
    uint32_t prev_sampled_leaf = -1;
    access(id, -1, opType, recursion_levels - 1, data_in, data_out, &prev_sampled_leaf);
}
