#ifndef __ZT_OASM_LIB__
#define __ZT_OASM_LIB__

#include "Block.hpp"

/*
	oset_value :
	On flag :
		- Copy value to dest
*/
extern "C"
void oset_value(uint32_t * dest, uint32_t value, uint32_t flag);

/*
	oset_value :
	On flag :
		- Set dest = dest + 1
*/
extern "C"
void oincrement_value(uint32_t * dest, uint32_t flag);

/*
	oblock_move_on_flag :
	On Flag :
		- Copy tree label res -> iter
		- Copy data chunk iter -> res 
		// Set iter_blk to dummy as well ? by placing N in id ?
*/
extern "C"
void oblock_move_on_flag(Block * res_blk, Block * iter_blk, uint32_t data_size, bool flag);

/*
	oset_block_as_dummy :
	On flag :
		- copy gN to block_id
*/
extern "C"
void oset_block_as_dummy(uint32_t * block_id, uint32_t gN, uint32_t flag);

extern "C"
void ostore_deepest(uint32_t treeLabel, uint32_t leaf, uint32_t * level_deepest, uint32_t blocksize);

/*
	ostore_deepest_round:	
	On Flag :
			 
*/
//extern "C" void ostore_deepest_round(uint32_t treeLabel, uint32_t flag, uint32_t *level_deepest);

/*
	pd_setdeepest:
	On Flag :
		- copy src -> deepest

*/
extern "C"
void pd_setdeepest(uint32_t * deepest, int32_t src, uint32_t flag);

extern "C"
void pt_settarget(uint32_t * target, int32_t * dest, int32_t * src, uint32_t flag);

extern "C"
void pt_set_target_position(int32_t * target_position, uint32_t pos_in_bucket, uint32_t * target_position_set_flag, uint32_t flag);

/*
	pt_set_src_dest
	On flag:
		- copy deepest[i-1] -> src
		- copy i -> dest
*/
extern "C"
void pt_set_src_dest(int32_t * src, int32_t * dest, int32_t deepest_of_i_1, uint32_t i, uint32_t flag_pt);
/*
	oset_return_value
	On Flag:
		-
		-
*/
//extern "C" void oset_return_value( uint32_t *return_value, uint32_t iter_data, uint32_t flag_ore, uint32_t* iter_data_ptr, uint32_t newleaf_nextlevel);

extern "C"
void oassign_newlabel(uint32_t * ptr_to_label, uint32_t newLabel, bool flag);
extern "C"
void ofix_recursion(unsigned char * ptr_to_data_in_block, bool flag, uint32_t newLabel, uint32_t * nextLeaf);
extern "C"
void omove(uint32_t i, uint32_t * item, uint32_t loc, uint32_t * leaf, uint32_t newLabel);
extern "C"
void stash_insert(void * iter_block, void * block, uint32_t extdata_size, bool flag, bool * block_written);

extern "C"
void stash_serialized_insert(unsigned char * iter_block, unsigned char * block, uint32_t extdata_size, bool flag, bool * block_written);

extern "C"
void oblock_move_to_bucket(unsigned char * serialized_block, Block * bucket_block, uint32_t extdata_size, bool flag, bool * written, uint32_t * posk);

extern "C"
void oset_goal_source(uint32_t level_in_path, uint32_t local_deepest_in_path, uint32_t flag, int32_t * src, int32_t * goal);
extern "C"
void oset_hold_dest(int32_t * hold, int32_t * dest, uint32_t * wflag, uint32_t flag);

/*
	omove_block:
	On Flag :
		- Move source->id -> dest->id
		- Move source->treeLabel -> dest->treeLabel
		- Move source->data -> dest->data
*/
extern "C"
void omove_block(Block * dest_block, Block * source_block, uint32_t BlockSize, uint32_t flag);
extern "C"
void omove_serialized_block(unsigned char * dest_block, unsigned char * source_block, uint32_t data_size, uint32_t flag);
extern "C"
void omove_buffer(unsigned char * dest, unsigned char * source, uint32_t buffersize, uint32_t flag);
extern "C"
void ocomp_set_flag(unsigned char * buff1, unsigned char * buff2, uint32_t buffersize, uint32_t * flag);
#endif
