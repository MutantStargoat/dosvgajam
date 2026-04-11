	section .text use32
	bits 32

VGA_PITCH	equ 80h

SC_ADDR		equ 3c4h	; sequence controller address register
CRTC_ADDR	equ 3d4h	; CRTC address register
MISC_ADDR	equ 3c2h	; miscellaneous output register


%macro planemask 1
	mov dx, SC_ADDR
	mov ax, (2 | (%1 << 8))	; map mask reg (2), mask bits in high byte
	out dx, ax
%endmacro


	global vga_setmodex_
vga_setmodex_:
	pusha
	mov ax, 13h
	int 10h

	; disable chain-4 (bit 3 of sequencer memory mode register [4])
	mov dx, SC_ADDR
	mov ax, 0604h
	out dx, ax
	; select 25MHz dot clock & 60Hz vertical
	mov dx, MISC_ADDR
	mov ax, 0e3h
	out dx, al

	; -- set up CRTC --
	mov dx, CRTC_ADDR
	; unprotect registers 0-7
	mov al, 11h	; vsync end register [11h] bit 8 is the protect bit
	out dx, al
	inc dx
	in al, dx
	and al, 7fh
	out dx, al
	dec dx

	; set vertical total to 525 (480 visible, 45 non-visible)
	; ... vertical total bits 0-7, 8,9 in overflow reg (register [06h])
	mov ax, 0d06h
	out dx, ax
	; ... overflow register [07h] ...
	mov ax, 3e07h
	out dx, ax
	; disable double-scan, char cell 2 (maximum scan line register [09h])
	mov ax, 4109h
	out dx, ax
	; set vsync start to 490, bits 8,9 in overflow register.
	; (vertical retrace start register [10h])
	mov ax, 0ea10h
	out dx, ax
	; set vsync end to 12(?), bits 0-4, and reg0-7 protect (vertical retrace
	; end register [11h])
	mov ax, 8c11h
	out dx, ax
	; set vertical display end to 479 bits 0-7, bits 8,9 in overflow reg.
	; (vertical display end register [12h])
	mov ax, 0df12h
	out dx, ax
	; disable double-word addressing (bit 6 of CRTC underline location
	; register [14h])
	mov ax, 0014h
	out dx, ax
	; set vblank start to 487 bits 0-7, bit 8 in overflow reg. (start
	; vertical blank register [15h])
	mov ax, 0e715h
	out dx, ax
	; set vblank end to 518 (518 & 7f = 6) (end vertical blank reg. [16h])
	mov ax, 0616h
	out dx, ax
	; enable byte mode address generation (bit 6 of CRTC mode control
	; register [17h])
	mov ax, 0e317h
	out dx, ax

	; clear all 256kb of vram
	planemask 0fh		; enable all planes
	mov edi, 0a0000h
	mov ecx, 3fffh
	xor eax, eax
	rep stosd

	; initial back buffer is the second page
	mov dword [_vga_backbuf], 0a4b00h

	; set initial scanout address to page 0. if we never pageflip, we
	; can just draw to a0000 as usual and it will be visible.
	; This also makes sure the low byte is 0, because we're not touching it
	; while page flipping; we flip by toggling a few bits in the high byte.
	mov dx, 3dah
.invb:	in al, dx
	and al, 8
	jnz .invb
	mov dx, CRTC_ADDR
	mov ax, 000ch	; 0ch: start address high register
	mov ax, 000dh	; 0dh: start address low register
	out dx, ax

	popa
	xor eax, eax
	ret

	global vga_cleanup_
vga_cleanup_:
	pusha
	mov ax, 3
	int 10h
	popa
	ret

	global vga_setpal_
vga_setpal_:
	; call with the high bit of index set, to ignore the index
	mov bh, dl
	test ax, 8000h
	jnz .skip_dac_addr
	mov dx, 3c8h
	out dx, al
.skip_dac_addr:
	mov dx, 3c9h
	mov al, bh
	shr al, 2
	out dx, al
	mov al, bl
	shr al, 2
	out dx, al
	mov al, cl
	shr al, 2
	out dx, al
	ret

	; clear the framebuffer 4 pixels at a time
	global vga_clearfb_
vga_clearfb_:
	push ecx
	push edx
	push edi
	mov ah, al
	mov cx, ax
	bswap eax
	or ecx, eax
	planemask 0fh
	mov eax, ecx
	mov edi, [_vga_backbuf]
	mov ecx, 4800	; 4800 dwords * 4 planes * 4 bytes = 76800 pixels
	rep stosd
	pop edi
	pop edx
	pop ecx
	ret

	; blit a full framebuffer to video ram
	; eax: dest, edx: src
	global vga_blitfb_
vga_blitfb_:
	push ecx
	push edi

	mov edi, eax

%macro blitfb_plane_loop 1
	mov ecx, edx
	planemask %1
	mov edx, ecx
	mov cx, 320 * 240 / 16
%%top:	mov al, [edx + 12]
	mov ah, [edx + 8]
	bswap eax
	mov ah, [edx + 4]
	mov al, [edx]
	add edx, 16
	stosd
	dec cx
	jnz %%top
%endmacro

	blitfb_plane_loop 1
	sub edi, 320 * 240 / 4
	sub edx, 320 * 240 - 1
	blitfb_plane_loop 2
	sub edi, 320 * 240 / 4
	sub edx, 320 * 240 - 1
	blitfb_plane_loop 4
	sub edi, 320 * 240 / 4
	sub edx, 320 * 240 - 1
	blitfb_plane_loop 8

.end:	pop edi
	pop ecx
	ret

	; vga_backbuf is the linear address of the back buffer in video RAM
	; either a0000 or a4b00. Only the high byte needs ot be changed to flip
	; between them, and masking with ffff gives the CRTC start address.
	global vga_pgflip_
vga_pgflip_:
	push ebx
	push edx
	push eax
	; set the current backbuffer as the new CRTC scanout start address
	mov ebx, [_vga_backbuf]
	mov bl, 0ch		; CRTC start address high register

	; only proceed if we're out of vblank, otherwise we might think we've
	; set a new scanout address, but it might not be latched until the next
	; vblank, and we'll be drawing over the scanout buffer in the meantime.
	mov dx, 3dah
.wait:	in al, dx
	and al, 8
	jnz .wait

	mov dx, CRTC_ADDR
	mov ax, bx		; get previously prepared reg addr and value
	out dx, ax
	; clear low bits and flip the backbuffer pointer
	xor ax, 4b0ch
	mov [_vga_backbuf], ax

	pop eax
	; then, if vsync was requested, wait until we enter vblank
	test eax, eax
	jz .end
	mov dx, 3dah
.waitvblank:
	in al, dx
	and al, 8
	jz .waitvblank

.end:	pop edx
	pop ebx
	ret


	section .bss

	align 4
	global _vga_backbuf
_vga_backbuf dd 0

; vi:ft=nasm:
