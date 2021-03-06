/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  Debug.h

Abstract:

Revision History:

**/

#ifndef _EFILDR_DEBUG_H_
#define _EFILDR_DEBUG_H_

VOID
PrintHeader (
  CHAR8 Char
  );

VOID 
PrintValue (
  UINT32 Value
  );

VOID
PrintValue64 (
  UINT64 Value
  );

VOID 
PrintString (
  CHAR8 *String
  );

VOID 
ClearScreen (
  VOID
  );

#endif
