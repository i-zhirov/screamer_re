; The rebuilt `engine_shutdown` module, hand-written to reproduce the
; original body.
;
; The original sits at linear 0x128ac in STARTH.EXE and holds 14 instructions
; and 52 bytes: the frame `push ebp` and `mov ebp,esp`, the eight calls, the
; argument setup `xor eax,eax` for the value 0 and `mov eax,0x62b0` for the
; fourth call, and the epilogue `pop ebp` and `ret`.
;
; The C build with `-of+` adds one `mov esp,ebp` in the epilogue, because
; OpenWatcom V2 always restores ESP and the original compiler omits the restore
; when nothing moved.  This source holds the original body exactly.  The
; differing operands are the eight call targets and the one data address, which
; the oracle normalizes as link-time addresses.
;
; The original `mov eax,0x62b0` carries a 4-byte internal fixup to object 3 at
; offset 0x62b0, so the operand is a relocatable address.  This source takes the
; address of the data symbol `DAT_000062b0` from the harness, so the rebuilt
; instruction carries the same fixup.
.386
.model flat
public engine_shutdown_
extern _DAT_000062b0 : byte
extern f1_ : near
extern f2_ : near
extern f3_ : near
extern f4_ : near
extern f5_ : near
extern f6_ : near
extern f7_ : near
extern f8_ : near
.code
engine_shutdown_ proc near
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        call f1_
        call f2_
        ; `xor eax,eax` in the store encoding (`31 c0`) that the original uses,
        ; not the assembler's `33 c0` form.
        db 31h, 0c0h
        call f3_
        mov eax, offset _DAT_000062b0
        call f4_
        call f5_
        call f6_
        call f7_
        call f8_
        pop ebp
        ret
engine_shutdown_ endp
end
