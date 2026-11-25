; The driver and the sinks of the rebuilt `formatter` module.
;
; The original driver sits at linear 0xd68c0 and holds 223 instructions and
; 641 bytes.  It calls the field parser at 0xd6b41, the conversion dispatcher
; at 0xd6e59, pads the field, and emits every character through the caller
; sink in ecx.  The register use and the jump encodings are hand-tuned, so the
; driver is a transliteration, not a C rebuild.
;
; The driver keeps the original contract: eax = destination handle, edx =
; format, ebx = argument cell, ecx = sink.  The sink is called with eax =
; address of the field record and edx = the character.  The record holds the
; fields at the original offsets:
;
;   +0x00 destination handle      +0x04 width, then leading fill
;   +0x08 precision, then body    +0x0c zero pad
;   +0x10 output count            +0x14 flags
;   +0x15 conversion              +0x16 fill
;   +0x17 prefix                  +0x20 empty string
;
; The parser and the conversion core are C (`formatter.c`).  `format_convert_`
; shuffles the original dispatcher registers into the C convention and returns
; the data selector in edx, because the driver emits through es.
.386
.model flat
public formatter_
public format_convert_
public formatter_stream_sink_
extrn format_parse_:near
extrn format_convert_c_:near
.code
formatter_ proc near
        push esi
        push edi
        push es
        push ebp
        sub esp, 0x4c
        db 89h, 0DEh                    ; mov esi, ebx, the 89h form the original compiler emits
        mov dword ptr [esp + 0x44], ecx
        db 30h, 0DBh                    ; xor bl, bl, the 30h form the original compiler emits
        mov dword ptr [esp + 0x28], eax
        mov byte ptr [esp + 0x48], bl
        mov byte ptr [esp + 0x3c], bl
        db 31h, 0DBh                    ; xor ebx, ebx, the 31h form the original compiler emits
        db 89h, 0D7h                    ; mov edi, edx, the 89h form the original compiler emits
        mov dword ptr [esp + 0x38], ebx
        mov ebp, 4
        jmp short fmt_loop
fmt_parse:
        lea ebx, [esp + 0x28]
        db 89h, 0F2h                    ; mov edx, esi, the 89h form the original compiler emits
        call format_parse_
        db 89h, 0C7h                    ; mov edi, eax, the 89h form the original compiler emits
        mov al, byte ptr [eax]
        inc edi
        mov byte ptr [esp + 0x3d], al
        test al, al
        je fmt_done
        cmp al, 0x6e
        jne fmt_convert
        mov bl, byte ptr [esp + 0x3c]
        test bl, 0x20
        je short fmt_n_short
        test bl, 0x80
        je short fmt_n_long_check_near
        mov ecx, dword ptr [esi]
        add ecx, 8
        mov dword ptr [esi], ecx
        les edx, fword ptr [ecx - 8]
fmt_n_long_store:
        mov eax, dword ptr [esp + 0x38]
        mov dword ptr es:[edx], eax
        jmp short fmt_loop
fmt_n_long_check_near:
        test bl, 0x40
        je short fmt_n_long_default
        mov ebx, dword ptr [esi]
        db 01h, 0EBh                    ; add ebx, ebp, the 01h form the original compiler emits
        mov dword ptr [esi], ebx
        mov edx, dword ptr [ebx - 4]
fmt_n_store_dword:
        mov eax, dword ptr [esp + 0x38]
        mov dword ptr [edx], eax
fmt_loop:
        mov cl, byte ptr [edi]
        test cl, cl
        je fmt_done
        lea eax, [edi + 1]
        cmp cl, 0x25
        je short fmt_parse
        db 89h, 0C7h                    ; mov edi, eax, the 89h form the original compiler emits
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        lea eax, [esp + 0x28]
        db 88h, 0CAh                    ; mov dl, cl, the 88h form the original compiler emits
        call dword ptr [esp + 0x44]
        jmp short fmt_loop
fmt_n_long_default:
        mov edx, dword ptr [esi]
        db 01h, 0EAh                    ; add edx, ebp, the 01h form the original compiler emits
        mov dword ptr [esi], edx
        mov edx, dword ptr [edx - 4]
        jmp short fmt_n_store_dword
fmt_n_short:
        test bl, 0x10
        je short fmt_n_default
        test bl, 0x80
        je short fmt_n_short_check_near
        mov eax, dword ptr [esi]
        add eax, 8
        mov dword ptr [esi], eax
        les edx, fword ptr [eax - 8]
        mov eax, dword ptr [esp + 0x38]
        mov word ptr es:[edx], ax
        jmp short fmt_loop
fmt_n_short_check_near:
        test bl, 0x40
        je short fmt_n_short_default
        mov ecx, dword ptr [esi]
        db 01h, 0E9h                    ; add ecx, ebp, the 01h form the original compiler emits
        mov dword ptr [esi], ecx
        mov edx, dword ptr [ecx - 4]
fmt_n_short_store:
        mov eax, dword ptr [esp + 0x38]
        mov word ptr [edx], ax
        jmp short fmt_loop
fmt_n_short_default:
        mov ebx, dword ptr [esi]
        db 01h, 0EBh                    ; add ebx, ebp, the 01h form the original compiler emits
        mov dword ptr [esi], ebx
        mov edx, dword ptr [ebx - 4]
        jmp short fmt_n_short_store
fmt_n_default:
        test bl, 0x80
        je short fmt_n_default_check_near
        mov edx, dword ptr [esi]
        add edx, 8
        mov dword ptr [esi], edx
        les edx, fword ptr [edx - 8]
        jmp fmt_n_long_store
fmt_n_default_check_near:
        test bl, 0x40
        je short fmt_n_default_else
        mov eax, dword ptr [esi]
        db 01h, 0E8h                    ; add eax, ebp, the 01h form the original compiler emits
        mov dword ptr [esi], eax
        mov edx, dword ptr [eax - 4]
        jmp fmt_n_store_dword
fmt_n_default_else:
        mov ecx, dword ptr [esi]
        db 01h, 0E9h                    ; add ecx, ebp, the 01h form the original compiler emits
        mov dword ptr [esi], ecx
        mov edx, dword ptr [ecx - 4]
        jmp fmt_n_store_dword
fmt_convert:
        db 89h, 0E0h                    ; mov eax, esp, the 89h form the original compiler emits
        lea ebx, [esp + 0x28]
        lea ecx, [esp + 0x48]
        db 89h, 0F2h                    ; mov edx, esi, the 89h form the original compiler emits
        call format_convert_
        mov es, edx
        mov dl, byte ptr [esp + 0x3c]
        db 89h, 0C3h                    ; mov ebx, eax, the 89h form the original compiler emits
        test dl, 8
        jne short fmt_after_leading
        cmp byte ptr [esp + 0x3e], 0x20
        jne short fmt_after_leading
fmt_leading:
        mov eax, dword ptr [esp + 0x2c]
        dec eax
        mov dword ptr [esp + 0x2c], eax
        test eax, eax
        jl short fmt_after_leading
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        lea eax, [esp + 0x28]
        mov dl, byte ptr [esp + 0x3e]
        call dword ptr [esp + 0x44]
        jmp short fmt_leading
fmt_after_leading:
        lea ecx, [esp + 0x3f]
fmt_prefix:
        mov al, byte ptr [ecx]
        test al, al
        je short fmt_zero_pad
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        db 88h, 0C2h                    ; mov dl, al, the 88h form the original compiler emits
        lea eax, [esp + 0x28]
        inc ecx
        call dword ptr [esp + 0x44]
        jmp short fmt_prefix
fmt_zero_pad:
        mov eax, dword ptr [esp + 0x34]
        lea ecx, [eax - 1]
        mov dword ptr [esp + 0x34], ecx
        test eax, eax
        je short fmt_after_zero
        lea eax, [esp + 0x28]
        mov edx, 0x30
        call dword ptr [esp + 0x44]
        jmp short fmt_zero_pad
fmt_after_zero:
        test byte ptr [esp + 0x3c], 8
        jne short fmt_body
        cmp byte ptr [esp + 0x3e], 0x20
        je short fmt_body
fmt_fill:
        mov eax, dword ptr [esp + 0x2c]
        dec eax
        mov dword ptr [esp + 0x2c], eax
        test eax, eax
        jl short fmt_body
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        lea eax, [esp + 0x28]
        mov dl, byte ptr [esp + 0x3e]
        call dword ptr [esp + 0x44]
        jmp short fmt_fill
fmt_body:
        mov dh, byte ptr [esp + 0x3d]
        cmp dh, 0x73
        je short fmt_string_body
        cmp dh, 0x53
        jne short fmt_plain_body
fmt_string_body:
        test byte ptr [esp + 0x3c], 0x20
        je short fmt_narrow_body
fmt_wide_body:
        mov eax, dword ptr [esp + 0x30]
        lea ecx, [eax - 1]
        mov dword ptr [esp + 0x30], ecx
        test eax, eax
        je short fmt_trailing
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        lea eax, [esp + 0x28]
        mov dl, byte ptr es:[ebx]
        call dword ptr [esp + 0x44]
        add ebx, 2
        jmp short fmt_wide_body
fmt_narrow_body:
        mov eax, dword ptr [esp + 0x30]
        lea edx, [eax - 1]
        mov dword ptr [esp + 0x30], edx
        test eax, eax
        je short fmt_trailing
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        lea eax, [esp + 0x28]
        mov dl, byte ptr es:[ebx]
        call dword ptr [esp + 0x44]
        inc ebx
        jmp short fmt_narrow_body
fmt_plain_body:
        mov eax, dword ptr [esp + 0x30]
        lea ecx, [eax - 1]
        mov dword ptr [esp + 0x30], ecx
        test eax, eax
        je short fmt_trailing
        db 31h, 0D2h                    ; xor edx, edx, the 31h form the original compiler emits
        lea eax, [esp + 0x28]
        mov dl, byte ptr es:[ebx]
        call dword ptr [esp + 0x44]
        inc ebx
        jmp short fmt_plain_body
fmt_trailing:
        test byte ptr [esp + 0x3c], 8
        je fmt_loop
        cmp dword ptr [esp + 0x2c], 0
        jle fmt_loop
fmt_trail_loop:
        mov eax, dword ptr [esp + 0x2c]
        lea edx, [eax - 1]
        mov dword ptr [esp + 0x2c], edx
        test eax, eax
        je fmt_loop
        lea eax, [esp + 0x28]
        mov edx, 0x20
        call dword ptr [esp + 0x44]
        jmp short fmt_trail_loop
fmt_done:
        mov eax, dword ptr [esp + 0x38]
        add esp, 0x4c
        pop ebp
        pop es
        pop edi
        pop esi
        ret
formatter_ endp

; Shuffle the original dispatcher registers into the C convention.
;
; In:  eax = scratch, ebx = field record, ecx = empty string, edx = arguments
; Out: eax = text, edx = the data selector
;
; The driver sets ebx from the returned text and does not use ecx after the
; call.  esi, edi, and ebp are saved because the driver keeps the argument
; cell in esi and the argument stride in ebp.
format_convert_ proc near
        push esi
        push edi
        push ebp
        xchg ebx, edx
        xchg ebx, ecx
        call format_convert_c_
        mov edx, ds
        pop ebp
        pop edi
        pop esi
        ret
format_convert_ endp

; The stream sink of the rebuilt `formatter` module.
;
; The original `__fprtf` sink at 0xd428e calls the Watcom `fputc` at 0xde2db.
; The rebuilt `__fprtf` keeps the `FILE` record, but the stream buffer helpers
; are compatibility stubs, so this sink writes the byte through the DOS handle
; at `FILE + 0x00`.  The output is unbuffered, which is enough for the rebuilt
; `printf` paths and keeps the stream path independent of the buffer state.
;
; The driver calls the sink with eax = the field record and edx = the
; character, and it expects eax, ecx, and edx to change.
formatter_stream_sink_ proc near
        push ebx
        push ecx
        push edx
        push esi
        mov esi, eax
        mov ebx, dword ptr [esi]
        mov ebx, dword ptr [ebx]
        push edx
        mov edx, esp
        mov ecx, 1
        mov eax, 0x4000
        int 0x21
        add esp, 4
        inc dword ptr [esi + 0x10]
        pop esi
        pop edx
        pop ecx
        pop ebx
        ret
formatter_stream_sink_ endp
end
