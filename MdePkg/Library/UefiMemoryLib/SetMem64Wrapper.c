/** @file
  SetMem64() implementation.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  The following BaseMemoryLib instances contain the same copy of this file:

    BaseMemoryLib
    BaseMemoryLibMmx
    BaseMemoryLibSse2
    BaseMemoryLibRepStr
    BaseMemoryLibOptDxe
    BaseMemoryLibOptPei
    PeiMemoryLib
    DxeMemoryLib

**/


#include "MemLibInternals.h"

/**
  Fills a target buffer with a 64-bit value, and returns the target buffer.

  This function fills Length bytes of Buffer with the 64-bit value specified by
  Value, and returns Buffer. Value is repeated every 64-bits in for Length
  bytes of Buffer.

  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().
  If Buffer is not aligned on a 64-bit boundary, then ASSERT().
  If Length is not aligned on a 64-bit boundary, then ASSERT().

  @param  Buffer  Pointer to the target buffer to fill.
  @param  Length  Number of bytes in Buffer to fill.
  @param  Value   Value with which to fill Length bytes of Buffer.

  @return Buffer.

**/
VOID *
EFIAPI
SetMem64 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT64  Value
  )
{
  if (Length == 0) {
    return Buffer;
  }

  ASSERT (Buffer != NULL);
  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)Buffer));
  ASSERT ((((UINTN)Buffer) & (sizeof (Value) - 1)) == 0);
  ASSERT ((Length & (sizeof (Value) - 1)) == 0);

  return InternalMemSetMem64 (Buffer, Length / sizeof (Value), Value);
}
