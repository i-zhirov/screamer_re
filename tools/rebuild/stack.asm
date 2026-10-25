; The stack segment that the DOS/4G extender needs (the error 1317).
STACK SEGMENT STACK 'STACK'
        db 4000h dup (?)
STACK ENDS
        end
