	bits 32
	section .text USE32

	; eax: vector ptr, edx: matrix ptr
	global vec3_xform_
vec3_xform_:
	push ebp
	mov ebp, esp
	sub esp, 8
	push ebx
	push ecx
	push esi
	push edi

	mov ebx, edx	; matrix to ebx
	mov edi, eax	; vertex to edi

%macro MULROW3 0
	mov eax, [edi]		; eax <- X
	imul dword [ebx]
	shrd eax, edx, 16
	mov ecx, eax
	mov eax, [edi + 4]	; eax <- Y
	imul dword [ebx + 4]
	shrd eax, edx, 16
	add ecx, eax
	mov eax, [edi + 8]	; eax <- Z
	imul dword [ebx + 8]
	shrd eax, edx, 16
	add eax, ecx
	add eax, [ebx + 12]	; add last column
%endmacro

	MULROW3
	mov [ebp - 8], eax
	add ebx, 16	; next matrix row
	MULROW3
	mov [ebp - 4], eax
	add ebx, 16
	MULROW3
	mov [edi + 8], eax	; move Z into place
	mov dword [edi + 12], 10000h
	; move XY into place
	lea esi, [ebp - 8]
	movsd
	movsd

	pop edi
	pop esi
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret


	; eax: vector ptr, edx: matrix ptr
	global vec4_xform_
vec4_xform_:
	push ebp
	mov ebp, esp
	sub esp, 12
	push ebx
	push ecx
	push esi
	push edi

	mov ebx, edx	; matrix to ebx
	mov edi, eax	; vertex to edi

%macro MULROW 0
	mov eax, [edi]		; eax <- X
	imul dword [ebx]
	shrd eax, edx, 16
	mov ecx, eax
	mov eax, [edi + 4]	; eax <- Y
	imul dword [ebx + 4]
	shrd eax, edx, 16
	add ecx, eax
	mov eax, [edi + 8]	; eax <- Z
	imul dword [ebx + 8]
	shrd eax, edx, 16
	add ecx, eax
	mov eax, [edi + 12]	; eax <- W
	imul dword [ebx + 12]
	shrd eax, edx, 16
	add eax, ecx
%endmacro

	MULROW
	mov [ebp - 12], eax
	add ebx, 16	; next matrix row
	MULROW
	mov [ebp - 8], eax
	add ebx, 16
	MULROW
	mov [ebp - 4], eax
	add ebx, 16
	MULROW
	mov [edi + 12], eax	; move W into place
	; move XYZ into place
	lea esi, [ebp - 12]
	movsd
	movsd
	movsd

	pop edi
	pop esi
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret


	; eax: ma ptr, edx: mb ptr
	global mat_mult_
mat_mult_:
	push ebp
	mov ebp, esp
	sub esp, 64
	push ebx
	push ecx
	push esi
	push edi

	mov ebx, edx	; mb to ebx
	mov edi, eax	; ma to edi

%macro MULROWCOL 0
	mov eax, [edi]			; eax <- row[0]
	imul dword [ebx]		; mul col[0]
	shrd eax, edx, 16
	mov ecx, eax
	mov eax, [edi + 4]		; eax <- row[1]
	imul dword [ebx + 16]	; mul col[1]
	shrd eax, edx, 16
	add ecx, eax
	mov eax, [edi + 8]		; eax <- row[2]
	imul dword [ebx + 32]	; mul col[2]
	shrd eax, edx, 16
	add ecx, eax
	mov eax, [edi + 12]		; eax <- row[3]
	imul dword [ebx + 48]	; mul col[3]
	shrd eax, edx, 16
	add eax, ecx			; dot product in eax
%endmacro

	; row 0
	MULROWCOL
	mov [ebp - 64], eax
	add ebx, 4				; next matrix b column
	MULROWCOL
	mov [ebp - 60], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 56], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 52], eax
	sub ebx, 12				; return to mb column 0
	add edi, 16				; next matrix a row

	; row 1
	MULROWCOL
	mov [ebp - 48], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 44], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 40], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 36], eax
	sub ebx, 12			; return to mb column 0
	add edi, 16			; next matrix a row
	
	; row 2
	MULROWCOL
	mov [ebp - 32], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 28], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 24], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 20], eax
	sub ebx, 12			; return to mb column 0
	add edi, 16			; next matrix a row
	
	; row 3
	MULROWCOL
	mov [ebp - 16], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 12], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 8], eax
	add ebx, 4
	MULROWCOL
	mov [ebp - 4], eax
	
   	sub edi, 48		; return edi to the start of matrix a
	lea esi, [ebp - 64]	; esi to the temp matrix on stack
	mov ecx, 16
	rep movsd
	
	pop edi
	pop esi
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret

; vi:ft=nasm ts=8 sts=8 sw=8:
	ret
