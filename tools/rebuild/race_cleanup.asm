; The rebuilt `race_cleanup` module, hand-written to reproduce the original
; body.
;
; The original sits at linear 0x2a1b0 in STARTH.EXE and holds 24 instructions
; and 97 bytes: the saved EDX, the frame, the twelve cleanup calls, and the
; epilogue.
;
; The C form with `-of+` saves the frame first, adds `lea esp,[ebp-4]`, and
; reorders the argument setup.  This source holds the original body exactly.
; The call targets and the five data addresses are link-time operands, which
; the whole-image relocation rewrites.
.386
.model flat
public race_cleanup_
extern _DAT_000062ac : dword
extern _DAT_000062b0 : byte
extern _DAT_00006530 : byte
extern _DAT_00006350 : byte
extern _DAT_00000fed : byte
extern cleanup_01_ : near
extern cleanup_02_ : near
extern cleanup_03_ : near
extern cleanup_04_ : near
extern cleanup_05_ : near
extern cleanup_06_ : near
extern cleanup_07_ : near
extern cleanup_08_ : near
extern cleanup_09_ : near
extern cleanup_10_ : near
extern cleanup_11_ : near
extern cleanup_12_ : near
.code
race_cleanup_ proc near
        push edx
        push ebp
        ; `mov ebp,esp` in the store encoding (`89 e5`) that the original uses.
        db 89h, 0e5h
        call cleanup_01_
        call cleanup_02_
        call cleanup_03_
        call cleanup_04_
        mov edx, offset _DAT_00006350
        mov eax, offset _DAT_00006530
        call cleanup_05_
        mov eax, offset _DAT_00006530
        mov edx, offset _DAT_00000fed
        call cleanup_06_
        call cleanup_07_
        call cleanup_08_
        call cleanup_09_
        mov eax, _DAT_000062ac
        call cleanup_10_
        call cleanup_11_
        mov eax, offset _DAT_000062b0
        call cleanup_12_
        pop ebp
        pop edx
        ret
race_cleanup_ endp
end
