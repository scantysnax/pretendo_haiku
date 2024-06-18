; optimised blitters

bits 64

global blit_windowed_dirty_mmx
global blit_2x_dirty_mmx
global blit_2x_windowed_dirty_mmx
global blit_overlay

blit_windowed_dirty_mmx:
;	rdi		arg 1	source
;	rsi		arg 2	dirty buffer
;	rdx		arg 3	dest
;	rcx		arg 4	size
;	r8		arg 5	pixel width
	shr rcx, 4
	jz near .finish

.loop:
	movq mm0, [rdi]
	movq mm1, [rsi]
	movq mm2, [rdi+8]
	movq mm3, [rsi+8]
	
	pcmpeqd mm1, mm0
	pcmpeqd mm3, mm2
	
	psrld mm1, 31 		; ffffffffffffffff -> 0000000100000001
	psrld mm3, 31 		; ffffffffffffffff -> 0000000100000001
	pslld mm3, 1 		; 0000000100000001 -> 0000000200000002
	por mm1, mm3 		; 0000000300000003
	
	packsswb mm1, mm1 	; 0000000300000003 -> 00030003
	movd r10d, mm1
	cmp r10d, 0x00030003
	jz .skip32
	
	movq [rsi], mm0
	movq [rsi+8], mm2
	movq [rdx], mm0
	movq [rdx+8], mm2
	
.skip32:
	add rdi, 16
	add rsi, 16
	add rdx, 16
	sub rcx, 1
	jnz .loop
	jz near .finish

.finish:
	emms
ret

blit_2x_windowed_dirty_mmx:
;	rdi		arg 1	source
;	rsi		arg 2	dirty buffer
;	rdx		arg 3	dest
;	rcx		arg 4	size
;	r8		arg 5	pixel width
;	r9		arg 6	bytes per row
	shr rcx, 4
	
.loop:
	movq mm0, [rdi]
	movq mm1, [rsi]
	
	pcmpeqd mm1, mm0
	packsswb mm1, mm1
	movd r10d, mm1
	cmp r10d, 0xffffffff
	jz .skip
	
	movq [rsi], mm0
	movq mm1, mm0
	
	punpckldq mm0, mm0
	punpckhdq mm1, mm1
	movq [rdx], mm0
	movq [rdx+8], mm1
	
	movq [rdx+r9], mm0 		; remove for scanlines
	movq [rdx+r9+8], mm1 	; remove for scanlines

.skip:
	add rdi, 8
	add rsi, 8
	add rdx, 16
	dec rcx
	jnz .loop
	jz near .finish
	
.finish:
	emms
ret


blit_2x_dirty_mmx:
;	rdi		arg 1	dest		
;	rsi		arg 2	source
;	rdx		arg 3	dirty buffer
;	rcx		arg 4	bytes per row
;	r8		arg 5	size
	
	push rbx
	shr r8, 3
	
.loop:
	movq mm0, [rsi]
	movq mm1, [rdx]
	
	pcmpeqb mm1, mm0
	packsswb mm1, mm1
	movd ebx, mm1
	cmp ebx, 0xffffffff
	jz .skip
	
	movq [rdx], mm0
	movq mm1, mm0
	punpcklbw mm0, mm0
	punpckhbw mm1, mm1

	movq [rdi], mm0
	movq [rdi+8], mm1
	
	movq [rdi+rcx], mm0 	; next line
	movq [rdi+rcx+8], mm1
	
.skip:
	add rdi, 16
	add rsi, 8
	add rdx, 8
	sub r8, 1
	jnz .loop

	pop rbx
	emms
ret


blit_overlay:
	; not yet 
	; needs hardware accelerated video
ret
