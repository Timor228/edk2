;------------------------------------------------------------------------------
;
; Copyright (c) 2008, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   CommonMacros.inc
;
; Abstract:
;
;   Common macros used in the ResetVector VTF module.
;
;------------------------------------------------------------------------------

%define ADDR16_OF(x) (0x10000 - fourGigabytes + x)
%define ADDR_OF(x) (0x100000000 - fourGigabytes + x)

%macro  callEdx 1
    mov     edx, ADDR_OF(%%returnLabel)
    jmp     %1
%%returnLabel:
%endmacro

%macro  OneTimeCall 1
    jmp     %1
%1 %+ OneTimerCallReturn:
%endmacro

%macro  OneTimeCallRet 1
    jmp     %1 %+ OneTimerCallReturn
%endmacro

StartOfResetVectorCode:

%define ADDR_OF_START_OF_RESET_CODE ADDR_OF(StartOfResetVectorCode)

