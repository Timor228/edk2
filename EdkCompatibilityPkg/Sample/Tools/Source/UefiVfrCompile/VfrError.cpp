#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "VfrError.h"

static SVFR_ERROR_HANDLE VFR_ERROR_HANDLE_TABLE [] = {
  { VFR_RETURN_SUCCESS, NULL },
  { VFR_RETURN_ERROR_SKIPED, NULL },
  { VFR_RETURN_FATAL_ERROR, "fatal error!!" },

  { VFR_RETURN_MISMATCHED, "unexpected token" },
  { VFR_RETURN_INVALID_PARAMETER, "Invalid parameter" },
  { VFR_RETURN_OUT_FOR_RESOURCES, "system out of memory" },
  { VFR_RETURN_UNSUPPORTED, "unsupported" },
  { VFR_RETURN_REDEFINED, "already defined" },
  { VFR_RETURN_FORMID_REDEFINED, "form id already defined" },
  { VFR_RETURN_QUESTIONID_REDEFINED, "question id already defined" },
  { VFR_RETURN_VARSTOREID_REDEFINED, "varstore id already defined" },
  { VFR_RETURN_UNDEFINED, "undefined" },
  { VFR_RETURN_VAR_NOTDEFINED_BY_QUESTION, "some variable has not defined by a question"},
  { VFR_RETURN_GET_EFIVARSTORE_ERROR, "get efi varstore error"},
  { VFR_RETURN_EFIVARSTORE_USE_ERROR, "can not use the efi varstore like this" },
  { VFR_RETURN_EFIVARSTORE_SIZE_ERROR, "unsupport efi varstore size should be <= 8 bytes" },
  { VFR_RETURN_GET_NVVARSTORE_ERROR, "get name value varstore error" },
  { VFR_RETURN_QVAR_REUSE, "variable reused by more than one question" }, 
  { VFR_RETURN_FLAGS_UNSUPPORTED, "flags unsupported" }, 
  { VFR_RETURN_ERROR_ARRARY_NUM, "array number error" },
  { VFR_RETURN_DATA_STRING_ERROR, "data field string error or not support"},
  { VFR_RETURN_CODEUNDEFINED, "Undefined Error Code" }
};

CVfrErrorHandle::CVfrErrorHandle (
  VOID
  )
{
  mScopeRecordListHead = NULL;
  mScopeRecordListTail = NULL;
  mVfrErrorHandleTable = VFR_ERROR_HANDLE_TABLE;
}

CVfrErrorHandle::~CVfrErrorHandle (
  VOID
  )
{
  SVfrFileScopeRecord *pNode = NULL;

  while (mScopeRecordListHead != NULL) {
    pNode = mScopeRecordListHead;
    mScopeRecordListHead = mScopeRecordListHead->mNext;
    delete pNode;
  }

  mScopeRecordListHead = NULL;
  mScopeRecordListTail = NULL;
  mVfrErrorHandleTable = NULL;
}

SVfrFileScopeRecord::SVfrFileScopeRecord (
  IN INT8     *Record, 
  IN UINT32   LineNum
  )
{
  UINT32      Index;
  INT8        *FileName = NULL;
  INT8        *Str      = NULL;

  mWholeScopeLine      = LineNum;
  mNext                = NULL;

  Str = strchr (Record, ' ');
  mScopeLineStart = atoi (++Str);

  Str = strchr (Str, '\"');
  FileName = ++Str;

  while((Str = strstr (FileName, "\\\\")) != NULL) {
    FileName = Str + 2;
  }
  if ((mFileName = new INT8[strlen(FileName)]) != NULL) {
    for (Index = 0; FileName[Index] != '\"'; Index++) {
      mFileName[Index] = FileName[Index];
    }
    mFileName[Index] = '\0';
  }

  return;
}

SVfrFileScopeRecord::~SVfrFileScopeRecord (
  VOID
  )
{
  if (mFileName != NULL) {
    delete mFileName;
  }
}

VOID
CVfrErrorHandle::ParseFileScopeRecord (
  IN INT8      *Record, 
  IN UINT32    WholeScopeLine
  )
{
  INT8                *FullPathName = NULL;
  SVfrFileScopeRecord *pNode        = NULL;

  if (Record == NULL) {
    return;
  }

  if ((pNode = new SVfrFileScopeRecord(Record, WholeScopeLine)) == NULL) {
    return;
  }

  if (mScopeRecordListHead == NULL) {
    mScopeRecordListTail = mScopeRecordListHead = pNode;
  } else {
    mScopeRecordListTail->mNext = pNode;
    mScopeRecordListTail        = pNode;
  }
}

VOID
CVfrErrorHandle::GetFileNameLineNum (
  IN  UINT32 LineNum,
  OUT INT8   **FileName,
  OUT UINT32 *FileLine
  )
{
  SVfrFileScopeRecord *pNode    = NULL;

  if ((FileName == NULL) || (FileLine == NULL)) {
    return;
  }

  *FileName = NULL;
  *FileLine = 0xFFFFFFFF;

  for (pNode = mScopeRecordListHead; pNode->mNext != NULL; pNode = pNode->mNext) {
    if ((LineNum > pNode->mWholeScopeLine) && (pNode->mNext->mWholeScopeLine > LineNum)) {
      *FileName = pNode->mFileName;
      *FileLine = LineNum - pNode->mWholeScopeLine + pNode->mScopeLineStart - 1;
      return ;
    }
  }

  *FileName = pNode->mFileName;
  *FileLine = LineNum - pNode->mWholeScopeLine + pNode->mScopeLineStart - 1;
}

VOID
CVfrErrorHandle::PrintError (
  IN UINT32               LineNum,
  IN INT8                 *TokName,
  IN INT8                 *ErrorMsg
  )
{
  INT8                   *FileName = NULL;
  UINT32                 FileLine;

  GetFileNameLineNum (LineNum, &FileName, &FileLine);
  printf ("%s line %d: error %s %s\n", FileName, FileLine, TokName, ErrorMsg);
}

UINT8
CVfrErrorHandle::HandleError (
  IN EFI_VFR_RETURN_CODE  ErrorCode,
  IN UINT32               LineNum,
  IN INT8                 *TokName
  )
{
  UINT32                 Index;
  INT8                   *FileName = NULL;
  UINT32                 FileLine;
  INT8                   *ErrorMsg = NULL;

  if (mVfrErrorHandleTable == NULL) {
    return 1;
  }

  for (Index = 0; mVfrErrorHandleTable[Index].mErrorCode != VFR_RETURN_CODEUNDEFINED; Index++) {
    if (ErrorCode == mVfrErrorHandleTable[Index].mErrorCode) {
      ErrorMsg = mVfrErrorHandleTable[Index].mErrorMsg;
      break;
    }
  }

  if (ErrorMsg != NULL) {
    GetFileNameLineNum (LineNum, &FileName, &FileLine);
    printf ("%s line %d: error %s %s\n", FileName, FileLine, TokName, ErrorMsg);
    return 1;
  } else {
    return 0;
  }
}

CVfrErrorHandle gCVfrErrorHandle;
