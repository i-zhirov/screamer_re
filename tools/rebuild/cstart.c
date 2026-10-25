/* The start symbol that the C compiler's main module references. */
#pragma off (check_stack)

void main_(void);
void _cstart(void);
#pragma aux _cstart = "jmp main_" parm [] modify exact [];
