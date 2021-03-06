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
#*    CpuInterrupt.S
#*  
#*   Abstract:
#*
#------------------------------------------------------------------------------

#PUBLIC SystemTimerHandler
#PUBLIC SystemExceptionHandler
#EXTERNDEF mExceptionCodeSize:DWORD

#EXTERN TimerHandler: NEAR
#EXTERN ExceptionHandler: NEAR
#EXTERN mTimerVector: DWORD

	.data
	ASM_GLOBAL ASM_PFX(mExceptionCodeSize)
ASM_PFX(mExceptionCodeSize): .long  9

	.text
	ASM_GLOBAL ASM_PFX(InitDescriptor)

ASM_PFX(InitDescriptor):
        movq    $GDT_BASE,%rax              # EAX=PHYSICAL address of gdt
        movq    %rax, gdtr + 2              # Put address of gdt into the gdtr
        lgdt    gdtr
        movq    $0x18, %rax
        movq    %rax, %gs
        movq    %rax, %fs
        movq    $IDT_BASE,%rax              # EAX=PHYSICAL address of idt
        movq    %rax, idtr + 2              # Put address of idt into the idtr
        lidt    idtr
        ret

# VOID
# InstallInterruptHandler (
#     UINTN Vector,
#     VOID  (*Handler)(VOID)
#     )
	ASM_GLOBAL ASM_PFX(InstallInterruptHandler)
ASM_PFX(InstallInterruptHandler):
#  Vector:DWORD @ 4(%esp)
#  Handler:DWORD @ 8(%esp)
        push     %rbx
        pushfq                              # save eflags
        cli                                 # turn off interrupts
        subq     $0x10, %rsp                # open some space on the stack
        movq     %rsp, %rbx
        
        sidt    (%rbx)                      # get fword address of IDT
        movq    2(%rbx), %rbx               # move offset of IDT into RBX
        addq    $0x10, %rsp                 # correct stack
        movq    %rcx, %rax                  # Get vector number
        shlq    $4, %rax                    # multiply by 16 to get offset
        addq    %rax, %rbx                  # add to IDT base to get entry
        movq    %rdx, %rax                  # load new address into IDT entry
        movw    %ax, (%rbx)                 # write bits 15..0 of offset
        shrq    $16, %rax                   # use ax to copy 31..16 to descriptors
        movw    %ax, 6(%rbx)                # write bits 31..16 of offset
        shrq    $16, %rax                   # use eax to copy 63..32 to descriptors
        movl    %eax, 8(%rbx)               # write bits 63..32 of offset
        popfq                               # restore flags (possible enabling interrupts)
        pop     %rbx
        ret

	.macro JmpCommonIdtEntry
    # jmp     commonIdtEntry - this must be hand coded to keep the assembler from
    #                          using a 8 bit reletive jump when the entries are
    #                          within 255 bytes of the common entry.  This must
    #                          be done to maintain the consistency of the size
    #                          of entry points...
    .byte     0xe9                        # jmp 16 bit reletive 
    .long     commonIdtEntry - . - 4      #  offset to jump to
	.endm

    .align 2
	ASM_GLOBAL ASM_PFX(SystemExceptionHandler)
ASM_PFX(SystemExceptionHandler):
INT0:
    push     $0x0      # push error code place holder on the stack
    push     $0x0 
    JmpCommonIdtEntry
#    db      0e9h                        # jmp 16 bit reletive 
#    dd      commonIdtEntry - $ - 4      #  offset to jump to
    
INT1:
    push     $0x0      # push error code place holder on the stack
    push     $0x1 
    JmpCommonIdtEntry
    
INT2:
    push     $0x0      # push error code place holder on the stack
    push     $0x2 
    JmpCommonIdtEntry
    
INT3:
    push     $0x0      # push error code place holder on the stack
    push     $0x3 
    JmpCommonIdtEntry
    
INT4:
    push     $0x0      # push error code place holder on the stack
    push     $0x4 
    JmpCommonIdtEntry
    
INT5:
    push     $0x0      # push error code place holder on the stack
    push     $0x5 
    JmpCommonIdtEntry
    
INT6:
    push     $0x0      # push error code place holder on the stack
    push     $0x6 
    JmpCommonIdtEntry
    
INT7:
    push     $0x0      # push error code place holder on the stack
    push     $0x7 
    JmpCommonIdtEntry
    
INT8:
#   Double fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push     $0x8 
    JmpCommonIdtEntry
    
INT9:
    push     $0x0      # push error code place holder on the stack
    push     $0x9 
    JmpCommonIdtEntry
    
INT10:
#   Invalid TSS causes an error code to be pushed so no phony push necessary
    nop
    nop
    push     $10
    JmpCommonIdtEntry
    
INT11:
#   Segment Not Present causes an error code to be pushed so no phony push necessary
    nop
    nop
    push     $11
    JmpCommonIdtEntry
    
INT12:
#   Stack fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push    $12
    JmpCommonIdtEntry
    
INT13:
#   GP fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push     $13
    JmpCommonIdtEntry
    
INT14:
#   Page fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    push     $14
    JmpCommonIdtEntry
    
INT15:
    push     $0x0      # push error code place holder on the stack
    push     $15
    JmpCommonIdtEntry
    
INT16:
    push     $0x0      # push error code place holder on the stack
    push     $16
    JmpCommonIdtEntry
    
INT17:
#   Alignment check causes an error code to be pushed so no phony push necessary
    nop
    nop
    push     $17
    JmpCommonIdtEntry
    
INT18:
    push     $0x0      # push error code place holder on the stack
    push     $18
    JmpCommonIdtEntry
    
INT19:
    push     $0x0      # push error code place holder on the stack
    push     $19
    JmpCommonIdtEntry

INTUnknown:
	.rept (32 - 20)
    push     $0x0      # push error code place holder on the stack
#    push    xxh     # push vector number
    .byte    0x6a
    .byte    ( . - INTUnknown - 3 ) / 9 + 20 # vector number
    JmpCommonIdtEntry
	.endr

	ASM_GLOBAL ASM_PFX(SystemTimerHandler)
ASM_PFX(SystemTimerHandler):
    push     $0
    push     $ASM_PFX(mTimerVector)
    JmpCommonIdtEntry

commonIdtEntry:
# +---------------------+
# +    EFlags           +
# +---------------------+
# +    CS               +
# +---------------------+
# +    EIP              +
# +---------------------+
# +    Error Code       +
# +---------------------+
# +    Vector Number    +
# +---------------------+
# +    EBP              +
# +---------------------+ <-- EBP

  cli
  push %rbp
  movq %rsp,%rbp

  #
  # Since here the stack pointer is 16-byte aligned, so
  # EFI_FX_SAVE_STATE_X64 of EFI_SYSTEM_CONTEXT_x64
  # is 16-byte aligned
  #       

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax#
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15#
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rax
  push %rcx
  push %rdx
  push %rbx
  push 6*8(%rbp)
  push (%rbp)
  push %rsi
  push %rdi
  
## UINT64  Gs, Fs, Es, Ds, Cs, Ss#  insure high 16 bits of each is zero
  movzx   7*8(%rbp), %rax
  push    %rax                      # for ss
  movzx   4*8(%rbp), %rax
  push    %rax                      # for cs
  movq    %ds, %rax
  push    %rax
  movq    %es, %rax
  push    %rax
  movq    %fs, %rax
  push    %rax
  movq    %gs, %rax
  push    %rax

## UINT64  Rip#
  push    3*8(%rbp)
  
## UINT64  Gdtr[2], Idtr[2]#
  subq    $16, %rsp
  sidt    (%rsp)
  subq    $16, %rsp
  sgdt    (%rsp)
  
## UINT64  Ldtr, Tr#
  xorq    %rax, %rax
  str     %ax
  push    %rax
  sldt    %ax
  push    %rax
  
## UINT64  RFlags#
  push    5*8(%rbp)

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8#
  movq    %cr8, %rax
  push    %rax
  movq    %cr4, %rax
  orq     $0x208, %rax
  movq    %rax, %cr4
  push    %rax
  movq    %cr3, %rax
  push    %rax
  movq    %cr2, %rax
  push    %rax
  xorq    %rax, %rax
  push    %rax
  movq    %cr0, %rax
  push    %rax

## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7#
  movq    %dr7, %rax
  push    %rax

## clear Dr7 while executing debugger itself
  xorq    %rax, %rax
  movq    %rax, %dr7

  movq    %dr6, %rax
  push    %rax
  
## insure all status bits in dr6 are clear...
  xorq    %rax, %rax
  movq    %rax, %dr6

  movq    %dr3, %rax
  push    %rax
  movq    %dr2, %rax
  push    %rax
  movq    %dr1, %rax
  push    %rax
  movq    %dr0, %rax
  push    %rax


## FX_SAVE_STATE_X64 FxSaveState#
  subq $512, %rsp
  movq %rsp, %rdi
  fxsave (%rdi)

## UINT64  ExceptionData#
  push    2*8 (%rbp) 
  
## call into exception handler
## Prepare parameter and call
  movq     1*8(%rbp), %rcx
  movq     %rsp, %rdx
  #
  # Per X64 calling convention, allocate maximum parameter stack space
  # and make sure RSP is 16-byte aligned
  #
  subq    $(4*8+8), %rsp
  cmpq    $32, %rcx
  jb      CallException
  call    ASM_PFX(TimerHandler)
  jmp     ExceptionDone
CallException:
  call    ASM_PFX(ExceptionHandler)
ExceptionDone:
  addq    $(4*8+8), %rsp

  cli
  
## UINT64  ExceptionData#
  addq    $8, %rsp

## FX_SAVE_STATE_X64 FxSaveState#
  movq     %rsp, %rsi
  fxrstor  (%esi)
  addq     $512, %rsp


## UINT64  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7#
  pop     %rax
  movq    %rax, %dr0
  pop     %rax
  movq    %rax, %dr1
  pop     %rax
  movq    %rax, %dr2
  pop     %rax
  movq    %rax, %dr3
## skip restore of dr6.  We cleared dr6 during the context save.
  addq    $8, %rsp
  pop     %rax
  movq    %rax, %dr7

## UINT64  Cr0, Cr1, Cr2, Cr3, Cr4, Cr8#
  pop     %rax
  movq    %rax, %cr0
  addq    $8, %rsp      # not for Cr1
  pop     %rax
  movq    %rax, %cr2
  pop     %rax
  movq    %rax, %cr3
  pop     %rax
  movq    %rax, %cr4
  pop     %rax
  mov     %rax, %cr8
  
## UINT64  RFlags#
  pop     5*8(%rbp) 

## UINT64  Ldtr, Tr#
## UINT64  Gdtr[2], Idtr[2]#
## Best not let anyone mess with these particular registers...
  addq     $48, %rsp

## UINT64  Rip#
  pop     3*8(%rbp)

## UINT64  Gs, Fs, Es, Ds, Cs, Ss#
  pop     %rax
  # mov     gs, rax # not for gs
  pop     %rax
  # mov     fs, rax # not for fs
  # (X64 will not use fs and gs, so we do not restore it)
  pop     %rax
  movq    %rax, %es
  pop     %rax
  movq    %rax, %ds
  pop     4*8(%rbp)       # for cs
  pop     7*8(%rbp)       # for ss

## UINT64  Rdi, Rsi, Rbp, Rsp, Rbx, Rdx, Rcx, Rax#
## UINT64  R8, R9, R10, R11, R12, R13, R14, R15#
  pop     %rdi
  pop     %rsi
  addq    $8, %rsp                    # not for rbp
  pop     6*8(%rbp)                   # for rsp
  pop     %rbx
  pop     %rdx
  pop     %rcx
  pop     %rax
  pop     %r8
  pop     %r9
  pop     %r10
  pop     %r11
  pop     %r12
  pop     %r13
  pop     %r14
  pop     %r15

  movq    %rbp, %rsp
  pop     %rbp
  addq    $16, %rsp
  iretq


##############################################################################
# data
##############################################################################

	.data
    .align 0x10

gdtr:	.short GDT_END - GDT_BASE - 1   # GDT limit
        .quad 0                         # (GDT base gets set above)
##############################################################################
#   global descriptor table (GDT)
##############################################################################

        .align 0x10

GDT_BASE:
# null descriptor
NULL_SEL            = .-GDT_BASE
        .short 0            # limit 15:0
        .short 0            # base 15:0
        .byte 0             # base 23:16
        .byte 0             # type
        .byte 0             # limit 19:16, flags
        .byte 0             # base 31:24

# linear data segment descriptor
LINEAR_SEL      = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0             # base 0
        .byte 0
        .byte 0x092          # present, ring 0, data, expand-up, writable
        .byte 0x0CF          # page-granular, 32-bit
        .byte 0

# linear code segment descriptor
LINEAR_CODE_SEL = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0             # base 0
        .byte 0
        .byte 0x09A          # present, ring 0, data, expand-up, writable
        .byte 0x0CF          # page-granular, 32-bit
        .byte 0

# system data segment descriptor
SYS_DATA_SEL    = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0             # base 0
        .byte 0
        .byte 0x092          # present, ring 0, data, expand-up, writable
        .byte 0x0CF          # page-granular, 32-bit
        .byte 0

# system code segment descriptor
SYS_CODE_SEL    = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0             # base 0
        .byte 0
        .byte 0x09A          # present, ring 0, data, expand-up, writable
        .byte 0x0CF          # page-granular, 32-bit
        .byte 0

# spare segment descriptor
SPARE3_SEL  = .-GDT_BASE
        .short 0            # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0             # present, ring 0, data, expand-up, writable
        .byte 0             # page-granular, 32-bit
        .byte 0

# spare segment descriptor
SPARE4_SEL  = .-GDT_BASE
        .short 0            # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0             # present, ring 0, data, expand-up, writable
        .byte 0             # page-granular, 32-bit
        .byte 0

# spare segment descriptor
SPARE5_SEL  = .-GDT_BASE
        .short 0            # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0             # present, ring 0, data, expand-up, writable
        .byte 0             # page-granular, 32-bit
        .byte 0

GDT_END:

        .align 0x4



idtr:	.short IDT_END - IDT_BASE - 1   # IDT limit
        .quad 0                         # (IDT base gets set above)
##############################################################################
#   interrupt descriptor table (IDT)
#
#   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
#       mappings.  This implementation only uses the system timer and all other
#       IRQs will remain masked.  The descriptors for vectors 33+ are provided
#       for convenience.
##############################################################################

#idt_tag .byte "IDT",0     
        .align 0x4

IDT_BASE:
# divide by zero (INT 0)
DIV_ZERO_SEL        = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# debug exception (INT 1)
DEBUG_EXCEPT_SEL    = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# NMI (INT 2)
NMI_SEL             = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# soft breakpoint (INT 3)
BREAKPOINT_SEL      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# overflow (INT 4)
OVERFLOW_SEL        = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# bounds check (INT 5)
BOUNDS_CHECK_SEL    = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# invalid opcode (INT 6)
INVALID_OPCODE_SEL  = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# device not available (INT 7)
DEV_NOT_AVAIL_SEL   = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# double fault (INT 8)
DOUBLE_FAULT_SEL    = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# invalid TSS (INT 0ah)
INVALID_TSS_SEL     = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# stack fault (INT 0ch)
STACK_FAULT_SEL     = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# general protection (INT 0dh)
GP_FAULT_SEL        = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# page fault (INT 0eh)
PAGE_FAULT_SEL      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# floating point error (INT 0x10)
FLT_POINT_ERR_SEL   = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# alignment check (INT 0x11)
ALIGNMENT_CHECK_SEL = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# machine check (INT 0x12)
MACHINE_CHECK_SEL   = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# SIMD floating-point exception (INT 0x13)
SIMD_EXCEPTION_SEL  = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

	.rept  (32 - 20)
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16
	.endr

# 72 unspecified descriptors
	.rept 72 * 8
        .byte 0
	.endr
        
# IRQ 0 (System timer) - (INT 0x68)
IRQ0_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 1 (8042 Keyboard controller) - (INT 0x69)
IRQ1_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 8 (RTC Alarm) - (INT 0x70)
IRQ8_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 9 - (INT 0x71)
IRQ9_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 10 - (INT 0x72)
IRQ10_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 11 - (INT 0x73)
IRQ11_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 12 (PS/2 mouse) - (INT 0x74)
IRQ12_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 13 (Floating point error) - (INT 0x75)
IRQ13_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 14 (Secondary IDE) - (INT 0x76)
IRQ14_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 15 (Primary IDE) - (INT 0x77)
IRQ15_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0             # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

	.rept 1 * 16
        .byte 0
	.endr

IDT_END:

