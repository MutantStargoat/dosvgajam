	section .text use32
	bits 32

PITCH		equ (320 + 64) / 4

SC_ADDR		equ 3c4h	; sequence controller address register
CRTC_ADDR	equ 3d4h	; CRTC address register
MISC_ADDR	equ 3c2h	; miscellaneous output register
AC_PORT		equ 3c0h	; attribute controller reg/data port
STAT1_PORT	equ 3dah

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

	; reset attribute controller flip-flop
	mov dx, STAT1_PORT
	in al, dx

	; initial back buffer is the second page
	; XXX game-specific tweak: assume 88 byte scanlines for the guard band
	; and add a vertical guard band of 16 scanlines before each buffer
	; a0c40h/a7340h
	mov dword [_vga_backbuf], 0a7340h

	; set initial scanout address to page 0. if we never pageflip, we
	; can just draw to a0000 as usual and it will be visible.
	; This also makes sure the low byte is 0, because we're not touching it
	; while page flipping; we flip by toggling a few bits in the high byte.
	; XXX game-specific: start after the guard band
	mov dx, STAT1_PORT
.invb:	in al, dx
	and al, 8
	jnz .invb
	mov dx, CRTC_ADDR
	mov ah, [_vga_backbuf + 8]
	mov al, 0ch	; 0ch: start address high register
	mov ah, [_vga_backbuf]
	mov al, 0dh	; 0dh: start address low register
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

	global vga_setpitch_
vga_setpitch_:
	push edx
	mov [_vga_pitch], eax
	shr eax, 1	; in byte addressing scanline offset is this value * 2
	mov ah, al
	mov al, 13h	; offset register [13h]
	mov dx, CRTC_ADDR
	out dx, ax
	pop edx
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
	mov ecx, PITCH * 240 / 4
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
	; XXX game-specific: a0c40h/a7340h
	global vga_pgflip_
vga_pgflip_:
	push ebx
	push edx

	; set the current backbuffer as the new CRTC scanout start address
	mov ebx, [_vga_backbuf]
	mov bl, 0ch		; CRTC start address high register

	push eax
	; was vsync requested? if not skip the wait (but see next comment block)
	test eax, eax
	jz .nowait

	; wait for vblank. The CRTC address is latched at the end of vblank
	; so without waiting (if arg is 0), we will end up swapping pointers
	; without the flip having taken place for another frame, and proceed to
	; write all over the scanout buffer. But it's still useful for
	; performance measurements, even if it will flicker badly.

	mov dx, STAT1_PORT
.invbl:	in al, dx
	and al, 8
	jnz .invbl
.nowait:

	; make the change, only the high register needs to change
	mov dx, CRTC_ADDR
	mov ax, bx		; get previously prepared reg addr and value
	out dx, ax
	; flip the backbuffer pointer (a0c40h xor a7340h = 7f00h)
	xor word [_vga_backbuf], 7f00h

	pop eax
	test eax, eax
	jz .end
	; if vsync was requested, wait until vblank starts
	mov dx, STAT1_PORT
.waitbl:in al, dx
	and al, 8
	jz .waitbl

.end:	pop edx
	pop ebx
	ret

	; eax: xoffs
	; edx: yoffs
	global vga_scroll_
vga_scroll_:
	push ebx
	mov [_vga_xscroll], eax
	mov [_vga_yscroll], edx
	mov ebx, eax
	shr ebx, 2
	mov eax, edx
	imul eax, [_vga_pitch]
	add eax, ebx
	mov ebx, eax

	; make sure we're out of vblank (see above)
	mov dx, STAT1_PORT
.invbl:	in al, dx
	and al, 8
	jnz .invbl

	; change start address, will be latched during the next vblank
	mov dx, CRTC_ADDR
	mov al, 0ch	; start addr high
	out dx, ax
	mov ah, bl
	inc al		; start addr low
	out dx, ax

	; wait for vblank before setting the pixel-pan register, because it
	; acts immediately
	mov dx, STAT1_PORT
.novbl:	in al, dx
	and al, 8
	jz .novbl

	; start address change only x-scrolls in 4 column increments, use the
	; horizontal pixel pan register for the remainder
	; flip-flop is in address mode, due to the vblank check above (in stat1)
	mov dx, AC_PORT
	mov al, 33h	; horizontal pixel pan register ORed with video palette access
	out dx, al
	mov eax, [_vga_xscroll]
	and eax, 3
	shl eax, 1
	out dx, al

	pop ebx
	ret


	section .bss

	align 4
	global _vga_backbuf
_vga_backbuf dd 0
	global _vga_pitch
_vga_pitch dd 80
	global _vga_xscroll
_vga_xscroll dd 0
	global _vga_yscroll
_vga_yscroll dd 0

; vi:ft=nasm:
