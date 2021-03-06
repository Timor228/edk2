#------------------------------------------------------------------------------
#*
#*   Copyright 2006, Intel Corporation                                                         
#*   All rights reserved. This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*    efi64.asm
#*  
#*   Abstract:
#*
#------------------------------------------------------------------------------

##############################################################################
# Now in 64-bit long mode.
##############################################################################

        .486: 
        .stack: 
        .code: 
        .org 0x21000

.equ                 DEFAULT_HANDLER_SIZE, INT1 - INT0

.macro jmpCommonIdtEntry  
    # jmp     commonIdtEntry - this must be hand coded to keep the assembler from
    #                          using a 8 bit reletive jump when the entries are
    #                          within 255 bytes of the common entry.  This must
    #                          be done to maintain the consistency of the size
    #                          of entry points...
    .byte   0xe9               			 # jmp 16 bit relative
		.long   commonIdtEntry - . - 4   # offset to jump to
.endm


Start:  

    movl    $0x001fffe8,%esp # make final stack aligned

    # set OSFXSR and OSXMMEXCPT because some code will use XMM register
    .byte 0xf
    .byte 0x20
    .byte 0xe0
#    mov rax, cr4
    btsl $9,%eax
    btsl $0xa,%eax
    .byte 0xf
    .byte 0x22
    .byte 0xe0
#    mov cr4, rax

    call    ClearScreen

    # Populate IDT with meaningful offsets for exception handlers...
		sidt    Idtr 
		

		movl    Halt, %eax
    movl    %eax,%ebx                   # use bx to copy 15..0 to descriptors
    shrl    $16,%eax                    # use ax to copy 31..16 to descriptors 
                                        # 63..32 of descriptors is 0
    movl    $0x78,%ecx                  # 78h IDT entries to initialize with unique entry points (exceptions)
		movl    (Idtr + 2), %esi
    movl    (%esi),%edi

LOOP_1:                                         # loop through all IDT entries exception handlers and initialize to default handler
    movw    %bx, (%edi)                         # write bits 15..0 of offset
    movw    $0x38, 2(%edi)                      # SYS_CODE_SEL64 from GDT
    movw    $(0x0e00 | 0x8000), 4(%edi)     		# type = 386 interrupt gate, present
    movw    %ax, 6(%edi)                        # write bits 31..16 of offset
    movl    $0, 8(%edi)                         # write bits 31..16 of offset  
    addl    $16, %edi                           # move up to next descriptor
    addw    DEFAULT_HANDLER_SIZE, %bx           # move to next entry point
    loopl   LOOP_1                              # loop back through again until all descriptors are initialized

    ## at this point edi contains the offset of the descriptor for INT 20
    ## and bx contains the low 16 bits of the offset of the default handler
    ## so initialize all the rest of the descriptors with these two values...
#    mov     ecx, 101                            ; there are 100 descriptors left (INT 20 (14h) - INT 119 (77h)
#@@:                                             ; loop through all IDT entries exception handlers and initialize to default handler
#    mov     word ptr [edi], bx                  ; write bits 15..0 of offset
#    mov     word ptr [edi+2], 38h               ; SYS_CODE64_SEL from GDT
#    mov     word ptr [edi+4], 0e00h OR 8000h    ; type = 386 interrupt gate, present
#    mov     word ptr [edi+6], ax                ; write bits 31..16 of offset
#    mov     dword ptr [edi+8], 0                ; write bits 63..32 of offset
#    add     edi, 16                             ; move up to next descriptor
#    loop    @b                                  ; loop back through again until all descriptors are initialized


##  DUMP    location of IDT and several of the descriptors
#    mov     ecx, 8
#    mov     eax, [offset Idtr + 2]
#    mov     eax, [eax]
#    mov     edi, 0b8000h
#    call    PrintQword
#    mov     esi, eax
#    mov     edi, 0b80a0h
#    jmp     OuterLoop

##    
## just for fun, let's do a software interrupt to see if we correctly land in the exception handler...
#    mov     eax, 011111111h
#    mov     ebx, 022222222h
#    mov     ecx, 033333333h
#    mov     edx, 044444444h
#    mov     ebp, 055555555h
#    mov     esi, 066666666h
#    mov     edi, 077777777h
#    push    011111111h
#    push    022222222h
#    push    033333333h
#    int     119

    movl    $0x22000,%esi               # esi = 22000
    movl    0x14(%esi),%eax             # eax = [22014]
    addl    %eax,%esi                   # esi = 22000 + [22014] = Base of EFILDR.C
    movl    0x3c(%esi),%ebp             # ebp = [22000 + [22014] + 3c] = NT Image Header for EFILDR.C
    addl    %esi,%ebp
    movl    0x30(%ebp),%edi             # edi = [[22000 + [22014] + 3c] + 2c] = ImageBase (63..32 is zero, ignore)
    movl    0x28(%ebp),%eax             # eax = [[22000 + [22014] + 3c] + 24] = EntryPoint
    addl    %edi,%eax                   # eax = ImageBase + EntryPoint
		movl    %ebx, EfiLdrOffset          
    movl    %eax, (%ebx)                # Modify far jump instruction for correct entry point

		movw    6(%ebp), %bx                # bx = Number of sections
    xorl    %eax,%eax
		movw    0x14(%ebp), %ax             # ax = Optional Header Size
    addl    %eax,%ebp
    addl    $0x18,%ebp                  # ebp = Start of 1st Section

SectionLoop: 
    pushl   %esi                        # Save Base of EFILDR.C
    pushl   %edi                        # Save ImageBase
    addl    0x14(%ebp),%esi             # esi = Base of EFILDR.C + PointerToRawData
    addl    0x0c(%ebp),%edi             # edi = ImageBase + VirtualAddress
    movl    0x10(%ebp),%ecx             # ecs = SizeOfRawData

    cld
    shrl    $2,%ecx
    rep
    movsl

    popl    %edi                        # Restore ImageBase
    popl    %esi                        # Restore Base of EFILDR.C

    addw    $0x28,%bp                   # ebp = ebp + 028h = Pointer to next section record
    .byte 0x66
    .byte 0xff
    .byte 0xcb
#    dec     bx
    cmpw    $0,%bx
    jne     SectionLoop

		movl    (Idtr), %eax                  # get size of IDT
		movzx		(%edx), %eax
    .byte 0xff
    .byte 0xc0
#    inc     eax
 		addl    2(%edx), %eax								 # add to base of IDT to get location of memory map...
    xorl    %ecx,%ecx
    movl    %eax,%ecx                    # put argument to RCX

    .byte 0x48
    .byte 0xc7
    .byte 0xc0
EfiLdrOffset: 
    .long 0x00401000                    # Offset of EFILDR
#   mov rax, 401000h
    .byte 0x50
#   push rax

# ret
    .byte 0xc3

#    db      "**** DEFAULT IDT ENTRY ***",0
    .align 0x2
Halt: 
INT0: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x0
    jmpCommonIdtEntry 
#    db      0e9h                        ; jmp 16 bit reletive 
#    dd      commonIdtEntry - $ - 4      ;  offset to jump to

INT1: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x1
    jmpCommonIdtEntry 

INT2: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x2
    jmpCommonIdtEntry 

INT3: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x3
    jmpCommonIdtEntry 

INT4: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x4
    jmpCommonIdtEntry 

INT5: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x5
    jmpCommonIdtEntry 

INT6: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x6
    jmpCommonIdtEntry 

INT7: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x7
    jmpCommonIdtEntry 

INT8: 
#   Double fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $0x8
    jmpCommonIdtEntry 

INT9: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $0x9
    jmpCommonIdtEntry 

INT10: 
#   Invalid TSS causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $10
    jmpCommonIdtEntry 

INT11: 
#   Segment Not Present causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $11
    jmpCommonIdtEntry 

INT12: 
#   Stack fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $12
    jmpCommonIdtEntry 

INT13: 
#   GP fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $13
    jmpCommonIdtEntry 

INT14: 
#   Page fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $14
    jmpCommonIdtEntry 

INT15: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $15
    jmpCommonIdtEntry 

INT16: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $16
    jmpCommonIdtEntry 

INT17: 
#   Alignment check causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl   $17
    jmpCommonIdtEntry 

INT18: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $18
    jmpCommonIdtEntry 

INT19: 
    pushl   $0x0    # push error code place holder on the stack
    pushl   $19
    jmpCommonIdtEntry 

INTUnknown: 
.rept  (0x78 - 20)
    pushl   $0x0    # push error code place holder on the stack
#    push    xxh     ; push vector number
    .byte 0x6a
    .byte      ( . - INTUnknown - 3 ) / 9 + 20   # vector number
    jmpCommonIdtEntry 
.endr

commonIdtEntry: 
    pushl   %eax
    pushl   %ecx
    pushl   %edx
    pushl   %ebx
    pushl   %esp
    pushl   %ebp
    pushl   %esi
    pushl   %edi
    .byte 0x41
    .byte 0x50
#    push    r8
    .byte 0x41
    .byte 0x51
#    push    r9
    .byte 0x41
    .byte 0x52
#    push    r10
    .byte 0x41
    .byte 0x53
#    push    r11
    .byte 0x41
    .byte 0x54
#    push    r12
    .byte 0x41
    .byte 0x55
#    push    r13
    .byte 0x41
    .byte 0x56
#    push    r14
    .byte 0x41
    .byte 0x57
#    push    r15
    .byte 0x48
    movl    %esp,%ebp
#    mov     rbp, rsp

##
##  At this point the stack looks like this:
##
##      Calling SS
##      Calling RSP
##      rflags
##      Calling CS
##      Calling RIP
##      Error code or 0
##      Int num or 0ffh for unknown int num
##      rax
##      rcx
##      rdx
##      rbx
##      rsp
##      rbp
##      rsi
##      rdi
##      r8
##      r9
##      r10
##      r11
##      r12
##      r13
##      r14
##      r15 <------- RSP, RBP
##      

    call    ClearScreen
    mov     String1, %esi 
    call    PrintString
    .byte 0x48
    movl    16*8(%ebp),%eax       ## move Int number into RAX 
    .byte 0x48
    cmpl    $18,%eax
    ja      PrintDefaultString
PrintExceptionString: 
    shll    $3,%eax             ## multiply by 8 to get offset from StringTable to actual string address
		addl    StringTable, %eax
    movl    (%eax),%esi
    jmp     PrintTheString
PrintDefaultString: 
		movl    IntUnknownString, %esi
    # patch Int number
    movl    %eax,%edx
    call    A2C
    movb    %al,1(%esi)
    movl    %edx,%eax
    shrl    $4,%eax
    call    A2C
    movb    %al,(%esi)
PrintTheString:        
    call    PrintString
		movl    String2, %esi
    call    PrintString
    .byte 0x48
    movl    19*8(%ebp),%eax    # CS
    call    PrintQword
    movb    $':', %al
		movb    %al, (%edi)
    addl    $2,%edi
    .byte 0x48
    movl    18*8(%ebp),%eax    # RIP
    call    PrintQword
		movl    String3, %esi
    call    PrintString

    movl    $0xb8140,%edi

		movl    StringRax, %esi
    call    PrintString
    .byte 0x48
    movl    15*8(%ebp),%eax
    call    PrintQword

		movl    StringRcx, %esi
    call    PrintString
    .byte 0x48
    movl    14*8(%ebp),%eax
    call    PrintQword

		movl    StringRdx, %esi
    call    PrintString
    .byte 0x48
    movl    13*8(%ebp),%eax
    call    PrintQword

    movl    $0xb81e0,%edi

		movl    StringRbx, %esi
    call    PrintString
    .byte 0x48
    movl    12*8(%ebp),%eax
    call    PrintQword

		movl    StringRsp, %esi
    call    PrintString
    .byte 0x48
    movl    21*8(%ebp),%eax
    call    PrintQword

		movl    StringRbp, %esi
    call    PrintString
    .byte 0x48
    movl    10*8(%ebp),%eax
    call    PrintQword

    movl    $0xb8280,%edi

		movl    StringRsi, %esi
    call    PrintString
    .byte 0x48
    movl    9*8(%ebp),%eax
    call    PrintQword

		movl    StringRdi, %esi
    call    PrintString
    .byte 0x48
    movl    8*8(%ebp),%eax
    call    PrintQword

		movl    StringEcode, %esi
    call    PrintString
    .byte 0x48
    movl    17*8(%ebp),%eax
    call    PrintQword

    movl    $0xb8320,%edi

		movl    StringR8, %esi
    call    PrintString
    .byte 0x48
    movl    7*8(%ebp),%eax
    call    PrintQword

		movl    StringR9, %esi
    call    PrintString
    .byte 0x48
    movl    6*8(%ebp),%eax
    call    PrintQword

		movl    StringR10, %esi
    call    PrintString
    .byte 0x48
    movl    5*8(%ebp),%eax
    call    PrintQword

    movl    $0xb83c0,%edi

		movl    StringR11, %esi
    call    PrintString
    .byte 0x48
    movl    4*8(%ebp),%eax
    call    PrintQword

		movl    StringR12, %esi
    call    PrintString
    .byte 0x48
    movl    3*8(%ebp),%eax
    call    PrintQword

		movl    StringR13, %esi
    call    PrintString
    .byte 0x48
    movl    2*8(%ebp),%eax
    call    PrintQword

    movl    $0xb8460,%edi

		movl    StringR14, %esi
    call    PrintString
    .byte 0x48
    movl    1*8(%ebp),%eax
    call    PrintQword

		movl    StringR15, %esi
    call    PrintString
    .byte 0x48
    movl    0*8(%ebp),%eax
    call    PrintQword

		movl    StringSs, %esi
    call    PrintString
    .byte 0x48
    movl    22*8(%ebp),%eax
    call    PrintQword

    movl    $0xb8500,%edi

		movl    StringRflags, %esi
    call    PrintString
    .byte 0x48
    movl    20*8(%ebp),%eax
    call    PrintQword

    movl    $0xb8640,%edi

    movl    %ebp,%esi
    addl    $23*8,%esi
    movl    $4,%ecx


OuterLoop: 
    pushl   %ecx
    movl    $4,%ecx
    .byte 0x48
    movl    %edi,%edx

InnerLoop: 
    .byte 0x48
    movl    (%esi),%eax
    call    PrintQword
    addl    $8,%esi
		mov			$0x00, %al 
    movb    %al,(%edi)
    addl    $2,%edi
    loop    InnerLoop

    popl    %ecx
    addl    $0xa0,%edx
    movl    %edx,%edi
    loop    OuterLoop


    movl    $0xb8960,%edi

    .byte 0x48
    movl    18*8(%ebp),%eax  # RIP
    subl    $8*8,%eax
    .byte 0x48
    movl    %eax,%esi       # esi = rip - 8 QWORD linear (total 16 QWORD)

    movl    $4,%ecx

OuterLoop1: 
    pushl   %ecx
    movl    $4,%ecx
    movl    %edi,%edx

InnerLoop1: 
    .byte 0x48
    movl    (%esi),%eax
    call    PrintQword
    addl    $8,%esi
		movb		$0x00, %al
    movb    %al,(%edi)
    addl    $2,%edi
    loop    InnerLoop1

    popl    %ecx
    addl    $0xa0,%edx
    movl    %edx,%edi
    loop    OuterLoop1



    #wbinvd
LN_C1:    
    jmp     LN_C1

#
# return
#
    movl    %ebp,%esp
#    mov     rsp, rbp
    .byte 0x41
    .byte 0x5f
#    pop    r15
    .byte 0x41
    .byte 0x5e
#    pop    r14
    .byte 0x41
    .byte 0x5d
#    pop    r13
    .byte 0x41
    .byte 0x5c
#    pop    r12
    .byte 0x41
    .byte 0x5b
#    pop    r11
    .byte 0x41
    .byte 0x5a
#    pop    r10
    .byte 0x41
    .byte 0x59
#    pop    r9
    .byte 0x41
    .byte 0x58
#    pop    r8
    popl   %edi
    popl   %esi
    popl   %ebp
    popl   %eax # esp
    popl   %ebx
    popl   %edx
    popl   %ecx
    popl   %eax

    .byte 0x48
    .byte 0x83
    .byte 0xc4
    .byte 0x10
#    add    esp, 16 ; error code and INT number

    .byte 0x48
    .byte 0xcf
#    iretq

PrintString: 
    pushl   %eax
LN_C2: 
		movb    (%esi), %al
    cmpb    $0,%al
    je      LN_C3
 		movb    %al, (%edi)
    .byte 0xff
    .byte 0xc6
#    inc     esi
    addl    $2,%edi
    jmp     LN_C2
LN_C3: 
    popl    %eax
    ret

## RAX contains qword to print
## RDI contains memory location (screen location) to print it to
PrintQword: 
    pushl   %ecx
    pushl   %ebx
    pushl   %eax

    .byte 0x48
    .byte 0xc7
    .byte 0xc1
    .long 16
#    mov     rcx, 16
looptop: 
    .byte 0x48
    roll    $4,%eax
    movb    %al,%bl
    andb    $0xf,%bl
    addb    $'0', %bl
    cmpb    $'9', %bl
    jle     @f
    addb    $7,%bl
@@: 
		movb %bl, (%edi)
    addl    $2,%edi
    loop    looptop
    #wbinvd

    popl    %eax
    popl    %ebx
    popl    %ecx
    ret

ClearScreen: 
    pushl   %eax
    pushl   %ecx

		movb 		$0x00, %al
    movb    $0xc,%ah
    movl    $0xb8000,%edi
    movl    $80*24,%ecx
LN_C4: 
		movw	  %ax, (%edi)
    addl    $2,%edi
    loop    LN_C4
    movl    $0xb8000,%edi

    popl    %ecx
    popl    %eax

    ret

A2C: 
    andb    $0xf,%al
    addb    $'0', %al
    cmpb    $'9', %al
    jle     @f
    addb    $7,%al
LN_C5: 
    ret

String1:            .asciz      "*** INT "

Int0String:         .asciz      "00h Divide by 0 -"
Int1String:         .asciz      "01h Debug exception -"
Int2String:         .asciz      "02h NMI -"
Int3String:         .asciz      "03h Breakpoint -"
Int4String:         .asciz      "04h Overflow -"
Int5String:         .asciz      "05h Bound -"
Int6String:         .asciz      "06h Invalid opcode -"
Int7String:         .asciz      "07h Device not available -"
Int8String:         .asciz      "08h Double fault -"
Int9String:         .asciz      "09h Coprocessor seg overrun (reserved) -"
Int10String:        .asciz      "0Ah Invalid TSS -"
Int11String:        .asciz      "0Bh Segment not present -"
Int12String:        .asciz      "0Ch Stack fault -"
Int13String:        .asciz      "0Dh General protection fault -"
Int14String:        .asciz      "0Eh Page fault -"
Int15String:        .asciz      "0Fh (Intel reserved) -"
Int16String:        .asciz      "10h Floating point error -"
Int17String:        .asciz      "11h Alignment check -"
Int18String:        .asciz      "12h Machine check -"
Int19String:        .asciz      "13h SIMD Floating-Point Exception -"
IntUnknownString:   .asciz      "??h Unknown interrupt -"

StringTable:   .long  Int0String, Int1String, Int2String, Int3String,    \
                      Int4String, Int5String, Int6String, Int7String,    \
                      Int8String, Int9String, Int10String, Int11String,  \
                      Int12String, Int13String, Int14String, Int15String,\
                      Int16String, Int17String, Int18String, Int19String

String2:           .asciz  " HALT!! *** ("
String3:           .asciz  ")"
StringRax:         .asciz  "RAX="
StringRcx:         .asciz  " RCX="
StringRdx:         .asciz  " RDX="
StringRbx:         .asciz  "RBX="
StringRsp:         .asciz  " RSP="
StringRbp:         .asciz  " RBP="
StringRsi:         .asciz  "RSI="
StringRdi:         .asciz  " RDI="
StringEcode:       .asciz  " ECODE="
StringR8:          .asciz  "R8 ="
StringR9:          .asciz  " R9 ="
StringR10:         .asciz  " R10="
StringR11:         .asciz  "R11="
StringR12:         .asciz  " R12="
StringR13:         .asciz  " R13="
StringR14:         .asciz  "R14="
StringR15:         .asciz  " R15="
StringSs:          .asciz  " SS ="
StringRflags:      .asciz  "RFLAGS="

Idtr:        .float  0
            .float  0

    .org 0x21ffe
BlockSignature: 
    .word 0xaa55

