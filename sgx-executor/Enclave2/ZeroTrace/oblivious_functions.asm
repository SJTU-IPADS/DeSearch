;
;    ZeroTrace: Oblivious Memory Primitives from Intel SGX 
;    Copyright (C) 2018  Sajin (sshsshy)
;
;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, version 3 of the License.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with this program.  If not, see <https://www.gnu.org/licenses/>.
;

BITS 64

section .text
	global omove
	global pt_settarget
	global pd_setdeepest
	global oassign_newlabel
	global ofix_recursion
	global oset_goal_source
	global omove_serialized_block
  global omove_buffer
	global oset_hold_dest
	global oset_block_as_dummy
	global pt_set_target_position
	global pt_set_src_dest
	global oset_value
  global ocomp_set_flag
	global stash_serialized_insert
	global oincrement_value
  global oblock_move_to_bucket

omove:
		;command:	
		;omove(i,&(array[i]),loc,leaf,newLabel)
		;Linux : rdi,rsi,rdx,rcx,r8,r9

		;If the callee wishes to use registers RBP, RBX, and R12-R15,
		;it must restore their original values before returning control to the caller.
		;All others must be saved by the caller if it wishes to preserve their values

		push rbx
		push rbp
		push r12
		push r13
		push r14
		push r15

		mov r13d, dword [rsi]	; r13d holds array[i]

		mov ebx, dword [rcx]	; Move value in leaf to rbx
		cmp edi, edx		; Compare i and loc

		;if i == loc
		cmovz ebx, dword [rsi]	; move value pointed by rdx to rbx (array[i] to rbx )
		cmovz r13d, r8d		; move newLabel to array[i]

		mov dword [rcx], ebx	; Push in leaflabel/prevleaf to leaf
		mov dword [rsi], r13d	; Push in newLabel/array[i] to array[i]

		pop r15
		pop r14
		pop r13
		pop r12
		pop rbp
		pop rbx

		ret

oset_value:
	; oset_value(&dest, target[i], flag_t);
	; Linux : rdi,rsi,rdx,rcx,r8,r9

	mov r10d, [rdi]
	
	cmp edx, 1
	
	cmovz r10d, esi
	
	mov [rdi], r10d

	ret

ocomp_set_flag:
        ; Compare 2 buffers (buff1 and buff2) obliviously and set flag to true
	; if they are equal
	; ocomp_set_flag(buff1, buff2, buff_size, flag)
	; Linux : rdi,rsi,rdx,rcx,
	; Callee-saved : RBP, RBX, and R12–R15

	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	; Move ptr to data from buff1 and buff2
	mov r10, rdi
	mov r11, rsi

	;Store pointer to flag in some other register other than c
	mov r12, rcx
	mov r13, rdx

	;Set loop parameters
	mov ax, dx
	xor rdx, rdx
        xor rcx, rcx
	mov bx, 8
	div bx
	mov cx, ax

	; RCX will be lost for loop, so use RBP as flag
	; Set flag to 1 at start, and move flag back at end
        ; to location pointer at by rcx, with value from rbp (1 byte , so bpl)
	mov ebp, 1
        xor edx, edx

	; Loop to fetch iter & res chunks till blk_size
	loopstart_ocsf:
		mov r14, qword [r10]
		mov r15, qword [r11]
		cmp r14, r15		;Compare bytes of the buff1&2
		setz dl			;set r12l with 1 if r14 and r15 equal
		and bpl, dl		;OR comparison outcome flag with bpl
		add r10, 8
		add r11, 8
		loop loopstart_ocsf
	mov dword [r12], ebp
       
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	ret


	

oincrement_value:
	; oincrement_value(&value, flag_t);
	; Linux : rdi,rsi,rdx,rcx,r8,r9

	mov r10d, [rdi]
	mov r9d, r10d
	add r9d, 1

	cmp edx, 1
	
	cmovz r10d, r9d
	
	mov [rdi], r10d

	ret

oset_block_as_dummy:
		; oset_block_as_dummy(&block.id, gN, block.id==id)
		; Linux : rdi,rsi,rdx,rcx,r8,r9

		mov r10d, [rdi]
		
		cmp edx,1		

		cmovz r10d, esi
	
		mov [rdi], r10d		
		
		ret

pt_set_src_dest:
		; pt_set_src_dest(&src, &dest, deepest[i-1], i, flag_pt);
		; Linux : rdi,rsi,rdx,rcx,r8,r9
		
		mov r10d, [rdi]
		mov r11d, [rsi]
		
		cmp r8d, 1
		
		mov r9d, edx
		cmovz r10d, r9d
		
		mov r9d, ecx
		cmovz r11d, r9d

		mov [rdi], r10d
		mov [rsi], r11d
	
		ret

pd_setdeepest:
		; pd_setdeepest(&(deepest[i]), src, goal>=i);	
		; Linux : rdi,rsi,rdx,rcx,r8,r9
		; Callee-saved : RBP, RBX, and R12–R15

		;Load deepest[i]
		mov r10d, dword [rdi]		

		; Check Flag	
		cmp edx, 1
		
		; On Flag, move src to deepest[i]
		cmovz r10d, esi
		
		mov [rdi], r10d  

		ret


pt_set_target_position:
		; pt_set_target_position(&(target_pos[i]), k, &target_set, flag_stp);
		; Linux : rdi,rsi,rdx,rcx,r8,r9
	
		
		mov r10d, [rdi]
		mov r11d, [rdx]
		mov eax, 1
		
		cmp ecx , 1
		
		cmovz r10d, esi
		cmovz r11d, eax

		mov [rdi], r10d
		mov [rdx], r11d

		ret

pt_settarget:
		; pt_settarget(&(target[i]), &dest, &src, i==src);	
		; Linux : rdi,rsi,rdx,rcx,r8,r9
		; Callee-saved : RBP, RBX, and R12–R15

		; Load target[i] to rax
		mov eax, [rdi]
	
		; Load src to r9					
		mov r9d, [rdx]

		; Load dest to r10
		mov r10d, [rsi]

		;Check flag
		cmp ecx,1


		; On Flag
		; Set target[i] = dest
		cmovz eax, r10d	
		mov [rdi], eax

		; On Flag 
		; Set src = -1
		; Set dst = -1
		mov r8d , -1
		cmovz r9d, r8d
		cmovz r10d, r8d
		mov [rdx], r9d
		mov [rsi], r10d

		ret
		
		

oset_hold_dest:		
		; Take inputs,  1 ptr to hold, 2 ptr to dest 3 ptr to flag_write, 4 flag,	
		; Linux : rdi,rsi,rdx,rcx,r8,r9
		; Callee-saved : RBP, RBX, and R12–R15

		mov r9d, dword[rdi]
		mov r10d, dword[rsi]
		mov r11d, dword[rdx]
		mov r8d, -1
	
		;Check flag
		cmp ecx,1
					

		cmovz r9d, r8d
		cmovz r10d, r8d
		mov r8d, 1
		cmovz r11d, r8d
		
		mov dword[rdi], r9d
		mov dword[rsi], r10d
		mov dword[rdx], r11d

		ret


omove_serialized_block:
		; Take inputs,  1 ptr to dest_block, 2 ptr to source_block, 3 data_size, 4 flag
		; Linux : 	rdi,rsi,rdx,rcx->rbp

		; Callee-saved : RBP, RBX, and R12–R15

		push rbx
		push rbp
		push r12
		push r13
		push r14
		push r15

		; Move ptr to data from serialized_dest_block and serialized_source_blk
		mov R10, rdi
		mov R11, rsi

		add r10, 24
		add r11, 24
		; Extract treelabels from dest_block and source_blk
		mov R12d, dword [rdi+20]
		mov R13d, dword [rsi+20]

		;RCX will be lost for loop, store flag from rcx to rbp (1 byte , so bpl)
		mov bpl, cl

		; Oblivious evaluation of flag
		cmp bpl, 1

		; Set source_block.treelabel -> dest_block.treelabel,  if flag is set
		cmovz r12d,r13d
		mov dword [rdi+20], r12d

		; Extract id from block_hold and iter_blk
		mov R12d, dword [rdi+16]
		mov R13d, dword [rsi+16]

		; Set source_block.id -> dest_block.id if flag is set
		cmovz r12d,r13d
		mov dword [rdi+16], r12d

		;Set loop parameters
		mov ax, dx
		xor rdx, rdx
		mov bx, 8
		div bx
		mov cx, ax

		; Loop to fetch iter & res chunks till blk_size
		loopstart3:
			cmp bpl, 1
			mov r14, qword [r10]
			mov r15, qword [r11]
			cmovz r14, r15 				;r14 / r15 based on the compare
			mov qword [r10], r14
			add r10, 8
			add r11, 8
			loop loopstart3

		pop r15
		pop r14
		pop r13
		pop r12
		pop rbp
		pop rbx

		ret

omove_buffer:
	; Take inputs,  1 ptr to dest_buffer, 2 ptr to source_buffer, 3 buffer_size, 4 flag
	; Linux : 	rdi,rsi,rdx,rcx->rbp

	; Callee-saved : RBP, RBX, and R12–R15

	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	; Move ptr to data from serialized_dest_block and serialized_source_blk
	mov r10, rdi
	mov r11, rsi

	;RCX will be lost for loop, store flag from rcx to rbp (1 byte , so bpl)
	mov bpl, cl

	; Oblivious evaluation of flag
	cmp bpl, 1

	;Set loop parameters
	mov ax, dx
	xor rdx, rdx
	mov bx, 8
	div bx
	mov cx, ax

	; Loop to fetch iter & res chunks till blk_size
	loopstart5:
		cmp bpl, 1
		mov r14, qword [r10]
		mov r15, qword [r11]
		cmovz r14, r15 				;r14 / r15 based on the compare
		mov qword [r10], r14
		add r10, 8
		add r11, 8
		loop loopstart5

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	ret



oassign_newlabel:
	; Take inputs,  1 ptr to block->label, 2 newlabel, 3 flag
	; Windows : rcx,rdx,r8 ,r9
	; Linux : rdi,rsi,rdx,rcx->rbp
	; Callee-saved : RBP, RBX, and R12–R15

	mov r9d, dword[rdi]
	cmp rdx, 1
	cmovz r9d, esi
	mov dword[rdi], r9d

	ret

ofix_recursion:
	
	; Take inputs,  1 ptr to block->data->ptr, 2 flag, 3 newlabel, 4 *nextleaf		
	; Windows : rcx,rdx,r8 ,r9
	; Linux : rdi,rsi,rdx,rcx,r8,r9
	; Callee-saved : RBP, RBX, and R12–R15

	mov r9d, dword[rcx]
	mov r8d, dword[rdi]		
	mov eax, dword[rdi]
	cmp rsi,1
	cmovz r9d, r8d
	cmovz eax, edx

	mov dword[rcx], r9d
	mov dword[rdi], eax		

	ret

oset_goal_source:
	; Take inputs,  1 level_in_path, 2 value_to_put_i_goal 3 flag, 4 ptr to src, 5 ptr to goal		
	; Linux : rdi,rsi,rdx,rcx,r8,r9
	; Callee-saved : RBP, RBX, and R12–R15

	mov r10d, dword[rcx]
	mov r11d, dword[r8]

	;Check flag
	cmp edx,1
	
	;
	cmovz r10d, edi
	cmovz r11d, esi
	
	mov dword[rcx], r10d
	mov dword[r8], r11d

	ret

stash_serialized_insert:
	; Linux : rdi,rsi,rdx,rcx,r8,r9
	; Calle-save registers : rbp, rbx, r12, r13, r14, r15.
	; Stash has the value of bool block_written

	; block : data, id, treelabel, r

	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	mov rbp,rcx

	;Linux_NOTES : r12 should hold ptr to block_written (r8 by Linux calling convention)

	; Ptr to data of iter
	mov r10, rdi
	add r10, 24		

	; Pointer block-data in r11		
	mov r11, rsi
	add r11, 24

	;Setup for transfer to written_flag
	xor rax,rax
	xor rbx,rbx

	; Check flag (r9)
	cmp bpl, 1

	; Set block_written flag if flag is valid else leave present_value unchanged
	mov al, 1
	mov bl, byte [r8]
	cmovz rbx, rax
	mov byte [r8], bl

	; obliviously move id
	mov r12d, dword [rdi+16]
	mov r13d, dword [rsi+16]
	cmovz r12d, r13d
	mov dword [rdi+16], r12d

	;obliviously move tree label
	mov r12d, dword [rdi+20]
	mov r13d, dword [rsi+20]
	cmovz r12d, r13d
	mov dword [rdi+20], r12d

	;Set loop parameters
	mov ax, dx
	xor rdx, rdx
	mov bx, 8
	div bx
	mov cx, ax

	;Oblivious transfer to iter-block's data
	loop_stash_insert2:
		;Check flag (r9)
		cmp bpl, 1
		mov r14, qword [r10]
		mov r15, qword [r11]
		cmovz r14, r15 				;r14 / r15 based on the compare
		mov qword [r10], r14
		add r10, 8
		add r11, 8
		loop loop_stash_insert2

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	ret


oblock_move_to_bucket:
; node *iter, Block *bucket_blocks[l], dataSize, flag, *sblock_written, *posk

		; Windows: rcx,rdx,r8 ,r9 , from stack : r12, r13
		
		; node *iter   , Block *bucket_blocks[l],   dataSize, 	flag, *sblock_written, *posk
		; Linux  : RDI,		RSI,    		RDX,	 RCX,	     R8,	  R9

		;Find and store bool sblock_written from the stack to a register
		;mov r11, rsp
		;add rsp, 8*(5)
		;mov r12, qword [rsp]	; r12 holds ptr to sblock_written
		;mov r13, qword [rsp+8]	; r13 holds ptr to posk
		;mov rsp, r11

		push rbx
		push rbp
		push r12
		push r13
		push r14
		push r15
		
		;Flag moved to rbp
		mov rbp, rcx

		;Replacement for occupied flag 
		mov r15d, dword[rsi+8]
		mov r11d, dword[rdi+16]

		; Check flag (r9)
		cmp bpl, 1

		; Pointer to data's of both blocks
		mov r10, rdi		
		add r10, 24		

		mov r11, qword [rsi]

		mov rbx, 1
		mov rax, 0

		; Check flag (r9)
		cmp bpl, 1
		
		;If flag, set sblock_written flag
		cmovz rax, rbx
		mov byte [r8], al

		;If flag, increment posk
		mov rax,0
		mov rbx,1
		cmovz rax,rbx
		add dword [r9], eax

		;Because of add flag registers are lost, reset using cmp
		cmp bpl, 1

		; obliviously move id
		mov r12d, dword [rdi+16]
		mov r13d, dword [rsi+8]
		cmovz r13d, r12d
		mov dword [rsi+8], r13d

		;If flag, set stash-block's id to gN
		cmovz r15d, r11d
		mov dword[rdi+16], r15d

		;obliviously move tree label
		mov r12d, dword [rdi+20]
		mov r13d, dword [rsi+12]
		cmovz r13d, r12d
		mov dword [rsi+12], r13d

		;Set loop parameters
		mov ax, dx
		xor rdx, rdx
		mov bx, 8
		div bx
		mov cx, ax

		;Oblivious transfer to iter-block's data
		loop_stash_insert_rebuild:
			cmp bpl, 1
			mov r14, qword [r10]
			mov r15, qword [r11]
			cmovz r15, r14 				;r14 / r15 based on the compare
			mov qword [r11], r15
			add r10, 8
			add r11, 8
			loop loop_stash_insert_rebuild

		pop r15
		pop r14
		pop r13
		pop r12
		pop rbp
		pop rbx

		ret

