; optimised copies

bits 64

global mmx_copy
global mmx2_copy
global sse_copy

align 16


mmx_copy:
	xor rax, rax
	shr rdx, 3
	
.loop:
	movq mm0, [rsi+rax*8]
	movq [rdi+rax*8], mm0
	add rax, 1
	cmp rdx, rax
	jg .loop
	
	emms	
ret


mmx2_copy:
ret


sse_copy:
ret
