#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   ReadDr1.S
#
# Abstract:
#
#   AsmReadDr1 function
#
# Notes:
#
#------------------------------------------------------------------------------

#include <EdkIIGlueBase.h>
#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# AsmReadDr1 (
#   VOID
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(AsmReadDr1)
.align 16
ASM_PFX(AsmReadDr1):
    mov     %dr1, %rax
    ret
