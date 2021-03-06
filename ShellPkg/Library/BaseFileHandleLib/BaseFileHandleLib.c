/** @file
  Provides interface to EFI_FILE_HANDLE functionality.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/SimpleFileSystem.h>

#include <Guid/FileInfo.h>

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#define MAX_FILE_NAME_LEN 522 // (20 * (6+5+2))+1) unicode characters from EFI FAT spec (doubled for bytes)
#define FIND_XXXXX_FILE_BUFFER_SIZE (SIZE_OF_EFI_FILE_INFO + MAX_FILE_NAME_LEN)

/**
  This function will retrieve the information about the file for the handle 
  specified and store it in allocated pool memory.

  This function allocates a buffer to store the file's information. It is the 
  caller's responsibility to free the buffer

  @param  FileHandle  The file handle of the file for which information is 
  being requested.

  @retval NULL information could not be retrieved.

  @return the information about the file
**/
EFI_FILE_INFO*
EFIAPI
FileHandleGetInfo (
  IN EFI_FILE_HANDLE            FileHandle
  )
{
  EFI_GUID        FileInfoGuid;
  EFI_FILE_INFO   *pFileInfo;
  UINTN           FileInfoSize;
  EFI_STATUS      Status;

  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);

  //
  // Get the required size to allocate
  //
  FileInfoGuid = gEfiFileInfoGuid;
  FileInfoSize = 0;
  pFileInfo = NULL;
  Status = FileHandle->GetInfo(FileHandle, 
                               &FileInfoGuid, 
                               &FileInfoSize, 
                               pFileInfo);
  //
  // error is expected.  getting size to allocate
  //
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  pFileInfo = AllocateZeroPool(FileInfoSize);
  ASSERT (pFileInfo != NULL);
  //
  // now get the information
  //
  Status = FileHandle->GetInfo(FileHandle, 
                               &FileInfoGuid, 
                               &FileInfoSize, 
                               pFileInfo);
  //
  // if we got an error free the memory and return NULL
  //
  if (EFI_ERROR(Status)) {
    FreePool(pFileInfo);
    return NULL;
  }
  return (pFileInfo);
}

/**
  This function will set the information about the file for the opened handle 
  specified.

  @param  FileHandle            The file handle of the file for which information 
  is being set

  @param  FileInfo              The infotmation to set.

  @retval EFI_SUCCESS		The information was set.
  @retval EFI_UNSUPPORTED The InformationType is not known.
  @retval EFI_NO_MEDIA		The device has no medium.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	The file was opened read only.
  @retval EFI_VOLUME_FULL	The volume is full.
**/
EFI_STATUS
EFIAPI
FileHandleSetInfo (
  IN EFI_FILE_HANDLE  	        FileHandle,
  IN CONST EFI_FILE_INFO        *FileInfo
  )
{
  EFI_GUID        FileInfoGuid;
  
  //
  // ASSERT if the FileHandle or FileInfo is NULL
  //
  ASSERT (FileHandle != NULL);
  ASSERT (FileInfo   != NULL);

  FileInfoGuid = gEfiFileInfoGuid;
  //
  // Set the info
  //
  return (FileHandle->SetInfo(FileHandle, 
                              &FileInfoGuid,
                              (UINTN)FileInfo->Size,
                              (EFI_FILE_INFO*)FileInfo));
}  

/**
  This function reads information from an opened file.

  If FileHandle is not a directory, the function reads the requested number of 
  bytes from the file at the file's current position and returns them in Buffer. 
  If the read goes beyond the end of the file, the read length is truncated to the
  end of the file. The file's current position is increased by the number of bytes 
  returned.  If FileHandle is a directory, the function reads the directory entry 
  at the file's current position and returns the entry in Buffer. If the Buffer 
  is not large enough to hold the current directory entry, then 
  EFI_BUFFER_TOO_SMALL is returned and the current file position is not updated. 
  BufferSize is set to be the size of the buffer needed to read the entry. On 
  success, the current position is updated to the next directory entry. If there 
  are no more directory entries, the read returns a zero-length buffer. 
  EFI_FILE_INFO is the structure returned as the directory entry.

  @param FileHandle             the opened file handle
  @param BufferSize             on input the size of buffer in bytes.  on return 
                                the number of bytes written.
  @param Buffer                 the buffer to put read data into.

  @retval EFI_SUCCESS	          Data was read.
  @retval EFI_NO_MEDIA	        The device has no media.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL	Buffer is too small. ReadSize contains required 
                                size.

**/
EFI_STATUS
EFIAPI
FileHandleRead(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
  OUT VOID                      *Buffer
  )
{
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);

  //
  // Perform the read based on EFI_FILE_PROTOCOL
  //
  return (FileHandle->Read(FileHandle, BufferSize, Buffer));
}


/**
  Write data to a file.

  This function writes the specified number of bytes to the file at the current 
  file position. The current file position is advanced the actual number of bytes 
  written, which is returned in BufferSize. Partial writes only occur when there 
  has been a data error during the write attempt (such as "volume space full"). 
  The file is automatically grown to hold the data if required. Direct writes to 
  opened directories are not supported.

  @param FileHandle           The opened file for writing
  @param BufferSize           on input the number of bytes in Buffer.  On output
                              the number of bytes written.
  @param Buffer               the buffer containing data to write is stored.

 @retval EFI_SUCCESS	        Data was written.
 @retval EFI_UNSUPPORTED	    Writes to an open directory are not supported.
 @retval EFI_NO_MEDIA	        The device has no media.
 @retval EFI_DEVICE_ERROR	    The device reported an error.
 @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
 @retval EFI_WRITE_PROTECTED	The device is write-protected.
 @retval EFI_ACCESS_DENIED	  The file was open for read only.
 @retval EFI_VOLUME_FULL	    The volume is full.
**/
EFI_STATUS
EFIAPI
FileHandleWrite(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
  IN VOID                       *Buffer
  )
{
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);
  //
  // Perform the write based on EFI_FILE_PROTOCOL
  //
  return (FileHandle->Write(FileHandle, BufferSize, Buffer));
}

/** 
  Close an open file handle.

  This function closes a specified file handle. All "dirty" cached file data is 
  flushed to the device, and the file is closed. In all cases the handle is 
  closed.

@param FileHandle               the file handle to close.

@retval EFI_SUCCESS             the file handle was closed sucessfully.
**/
EFI_STATUS
EFIAPI
FileHandleClose (
  IN EFI_FILE_HANDLE            FileHandle
  )
{
  EFI_STATUS Status;
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);
  //
  // Perform the Close based on EFI_FILE_PROTOCOL
  //
  Status = FileHandle->Close(FileHandle);
  return Status;
}

/**
  Delete a file and close the handle

  This function closes and deletes a file. In all cases the file handle is closed.
  If the file cannot be deleted, the warning code EFI_WARN_DELETE_FAILURE is 
  returned, but the handle is still closed.

  @param FileHandle             the file handle to delete

  @retval EFI_SUCCESS           the file was closed sucessfully
  @retval EFI_WARN_DELETE_FAILURE the handle was closed, but the file was not 
                                deleted
  @retval INVALID_PARAMETER    	One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
FileHandleDelete (
  IN EFI_FILE_HANDLE		FileHandle
  )
{
  EFI_STATUS Status;
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);
  //
  // Perform the Delete based on EFI_FILE_PROTOCOL
  //
  Status = FileHandle->Delete(FileHandle);
  return Status;
}

/**
  Set the current position in a file.

  This function sets the current file position for the handle to the position 
  supplied. With the exception of seeking to position 0xFFFFFFFFFFFFFFFF, only
  absolute positioning is supported, and seeking past the end of the file is 
  allowed (a subsequent write would grow the file). Seeking to position 
  0xFFFFFFFFFFFFFFFF causes the current position to be set to the end of the file.
  If FileHandle is a directory, the only position that may be set is zero. This 
  has the effect of starting the read process of the directory entries over.

  @param FileHandle             The file handle on which the position is being set
  @param Position               Byte position from begining of file

  @retval EFI_SUCCESS           Operation completed sucessfully.
  @retval EFI_UNSUPPORTED       the seek request for non-zero is not valid on 
                                directories.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
FileHandleSetPosition (
  IN EFI_FILE_HANDLE   	FileHandle,
  IN UINT64           	Position
  )
{
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);
  //
  // Perform the SetPosition based on EFI_FILE_PROTOCOL
  //
  return (FileHandle->SetPosition(FileHandle, Position));
}

/** 
  Gets a file's current position

  This function retrieves the current file position for the file handle. For 
  directories, the current file position has no meaning outside of the file 
  system driver and as such the operation is not supported. An error is returned
  if FileHandle is a directory.

  @param FileHandle             The open file handle on which to get the position.
  @param Position               Byte position from begining of file.

  @retval EFI_SUCCESS           the operation completed sucessfully.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       the request is not valid on directories.
**/
EFI_STATUS
EFIAPI
FileHandleGetPosition (
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Position
  )
{
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);
  //
  // Perform the GetPosition based on EFI_FILE_PROTOCOL
  //
  return (FileHandle->GetPosition(FileHandle, Position));
}
/**
  Flushes data on a file
  
  This function flushes all modified data associated with a file to a device.

  @param FileHandle             The file handle on which to flush data

  @retval EFI_SUCCESS           The data was flushed.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened for read only.
**/
EFI_STATUS
EFIAPI
FileHandleFlush (
  IN EFI_FILE_HANDLE            FileHandle
  )
{
  //
  // ASSERT if FileHandle is NULL
  //
  ASSERT (FileHandle != NULL);
  //
  // Perform the Flush based on EFI_FILE_PROTOCOL
  //
  return (FileHandle->Flush(FileHandle));
}

/**
  function to determine if a given handle is a directory handle

  if DirHandle is NULL then ASSERT()

  open the file information on the DirHandle and verify that the Attribute
  includes EFI_FILE_DIRECTORY bit set.

  @param DirHandle              Handle to open file

  @retval EFI_SUCCESS           DirHandle is a directory
  @retval EFI_INVALID_PARAMETER DirHandle did not have EFI_FILE_INFO available
  @retval EFI_NOT_FOUND         DirHandle is not a directory
**/
EFI_STATUS
EFIAPI
FileHandleIsDirectory (
  IN EFI_FILE_HANDLE            DirHandle
  )
{
  EFI_FILE_INFO *DirInfo;

  //
  // ASSERT if DirHandle is NULL
  //
  ASSERT(DirHandle != NULL);
  
  //
  // get the file information for DirHandle
  //
  DirInfo = FileHandleGetInfo (DirHandle);
  
  //
  // Parse DirInfo
  //
  if (DirInfo == NULL) {
    //
    // We got nothing...
    //
    return (EFI_INVALID_PARAMETER);
  } 
  if ((DirInfo->Attribute & EFI_FILE_DIRECTORY) == 0) {
    //
    // Attributes say this is not a directory
    //
    FreePool (DirInfo);
    return (EFI_NOT_FOUND);
  }
  //
  // all good...
  //
  FreePool (DirInfo);
  return (EFI_SUCCESS);
}

/**
  Retrieves the first file from a directory

  This function opens a directory and gets the first file's info in the 
  directory. Caller can use FileHandleFindNextFile() to get other files.  When 
  complete the caller is responsible for calling FreePool() on Buffer.

  @param DirHandle              The file handle of the directory to search
  @param Buffer                 Pointer to buffer for file's information

  @retval EFI_SUCCESS           Found the first file.
  @retval EFI_NOT_FOUND         Cannot find the directory.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @return Others                status of FileHandleGetInfo, FileHandleSetPosition,
                                or FileHandleRead
**/
EFI_STATUS
EFIAPI
FileHandleFindFirstFile (
  IN EFI_FILE_HANDLE            DirHandle,
  OUT EFI_FILE_INFO             **Buffer
  )
{
  EFI_STATUS    Status;
  UINTN         BufferSize;

  //
  // ASSERTs
  //
  ASSERT (DirHandle != NULL);
  ASSERT (Buffer != NULL);

  //
  // verify that DirHandle is a directory
  //
  Status = FileHandleIsDirectory(DirHandle);
  if (EFI_ERROR(Status)) {
    return (Status);
  } 

  //
  // reset to the begining of the directory 
  //
  Status = FileHandleSetPosition(DirHandle, 0);
  if (EFI_ERROR(Status)) {
    return (Status);
  } 

  //
  // Allocate a buffer sized to struct size + enough for the string at the end
  //
  BufferSize = FIND_XXXXX_FILE_BUFFER_SIZE;
  *Buffer = AllocateZeroPool(BufferSize);
  ASSERT (*Buffer != NULL);

  //
  // read in the info about the first file
  //
  Status = FileHandleRead (DirHandle, &BufferSize, *Buffer);
  ASSERT(Status != EFI_BUFFER_TOO_SMALL);
  if (EFI_ERROR(Status)) {
    FreePool(*Buffer);
    *Buffer = NULL;
    return (Status);
  }
  return (EFI_SUCCESS);
}
/**
  Retrieves the next file in a directory.

  To use this function, caller must call the FileHandleFindFirstFile() to get the 
  first file, and then use this function get other files. This function can be 
  called for several times to get each file's information in the directory. If 
  the call of FileHandleFindNextFile() got the last file in the directory, the next 
  call of this function has no file to get. *NoFile will be set to TRUE and the 
  Buffer memory will be automatically freed. 

  @param DirHandle              the file handle of the directory
  @param Buffer			            pointer to buffer for file's information
  @param NoFile			            pointer to boolean when last file is found

  @retval EFI_SUCCESS           Found the next file, or reached last file
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
**/
EFI_STATUS
EFIAPI
FileHandleFindNextFile(
  IN EFI_FILE_HANDLE             DirHandle,
  OUT EFI_FILE_INFO              *Buffer,
  OUT BOOLEAN                    *NoFile
  )
{
  EFI_STATUS    Status;
  UINTN         BufferSize;

  //
  // ASSERTs for DirHandle or Buffer or NoFile poitners being NULL
  //
  ASSERT (DirHandle != NULL);
  ASSERT (Buffer    != NULL);
  ASSERT (NoFile    != NULL);

  //
  // This BufferSize MUST stay equal to the originally allocated one in GetFirstFile
  //
  BufferSize = FIND_XXXXX_FILE_BUFFER_SIZE;

  //
  // read in the info about the next file
  //
  Status = FileHandleRead (DirHandle, &BufferSize, Buffer);
  ASSERT(Status != EFI_BUFFER_TOO_SMALL);
  if (EFI_ERROR(Status)) {
    return (Status);
  }

  //
  // If we read 0 bytes (but did not have erros) we already read in the last file.
  //
  if (BufferSize == 0) {
    FreePool(Buffer);
    *NoFile = TRUE;
  }

  return (EFI_SUCCESS);
}
/**
  Retrieve the size of a file.

  if FileHandle is NULL then ASSERT()
  if Size is NULL then ASSERT()

  This function extracts the file size info from the FileHandle's EFI_FILE_INFO 
  data.

  @param FileHandle             file handle from which size is retrieved
  @param Size                   pointer to size

  @retval EFI_SUCCESS           operation was completed sucessfully
  @retval EFI_DEVICE_ERROR      cannot access the file
**/
EFI_STATUS
EFIAPI
FileHandleGetSize (
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Size
  )
{
  EFI_FILE_INFO                 *FileInfo;

  //
  // ASSERT for FileHandle or Size being NULL
  //
  ASSERT (FileHandle != NULL);
  ASSERT (Size != NULL);
  
  //
  // get the FileInfo structure
  //
  FileInfo = FileHandleGetInfo(FileHandle);
  if (FileInfo == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // Assign the Size pointer to the correct value
  //
  *Size = FileInfo->FileSize;
  
  //
  // free the FileInfo memory
  //
  FreePool(FileInfo);

  return (EFI_SUCCESS);
}


/**
  Safely append (on the left) with automatic string resizing given length of Destination and 
  desired length of copy from Source.

  append the first D characters of Source to the end of Destination, where D is 
  the lesser of Count and the StrLen() of Source. If appending those D characters 
  will fit within Destination (whose Size is given as CurrentSize) and 
  still leave room for a null terminator, then those characters are appended, 
  starting at the original terminating null of Destination, and a new terminating 
  null is appended.

  If appending D characters onto Destination will result in a overflow of the size
  given in CurrentSize the string will be grown such that the copy can be performed
  and CurrentSize will be updated to the new size.

  If Source is NULL, there is nothing to append, just return the current buffer in 
  Destination.

  if Destination is NULL, then ASSERT()
  if Destination's current length (including NULL terminator) is already more then 
  CurrentSize, then ASSERT()

  @param[in,out] Destination   The String to append onto
  @param[in,out] CurrentSize   on call the number of bytes in Destination.  On 
                                return possibly the new size (still in bytes).  if NULL
                                then allocate whatever is needed.
  @param[in]      Source        The String to append from
  @param[in]      Count         Maximum number of characters to append.  if 0 then 
                                all are appended.

  @return Destination           return the resultant string.
**/
CHAR16* 
EFIAPI
StrnCatGrowLeft (
  IN OUT CHAR16           **Destination,
  IN OUT UINTN            *CurrentSize,
  IN     CONST CHAR16     *Source,
  IN     UINTN            Count
  ){
  UINTN DestinationStartSize;
  UINTN NewSize;
  UINTN CopySize;

  //
  // ASSERTs
  //
  ASSERT(Destination != NULL);

  //
  // If there's nothing to do then just return Destination
  //
  if (Source == NULL) {
    return (*Destination);
  }

  //
  // allow for NULL pointers address as Destination
  //
  if (*Destination != NULL) {
    ASSERT(CurrentSize != 0);
    DestinationStartSize = StrSize(*Destination);
    ASSERT(DestinationStartSize <= *CurrentSize);
  } else {
    DestinationStartSize = 0;
//    ASSERT(*CurrentSize == 0);
  }

  //
  // Append all of Source?
  //
  if (Count == 0) {
    Count = StrSize(Source);
  }

  //
  // Test and grow if required
  //
  if (CurrentSize != NULL) {
    NewSize = *CurrentSize;
    while (NewSize < (DestinationStartSize + Count)) {
      NewSize += 2 * Count;
    }
    *Destination = ReallocatePool(*CurrentSize, NewSize, *Destination);
    *CurrentSize = NewSize;
  } else {
    *Destination = AllocateZeroPool(Count+sizeof(CHAR16));
  }

  CopySize = StrSize(*Destination);
  CopyMem((*Destination)+((Count-2)/sizeof(CHAR16)), *Destination, CopySize);
  CopyMem(*Destination, Source, Count-2);
  return (*Destination);
}

/**
  Function to get a full filename given a EFI_FILE_HANDLE somewhere lower on the 
  directory 'stack'.

  if Handle is NULL, return EFI_INVALID_PARAMETER

  @param[in] Handle             Handle to the Directory or File to create path to.
  @param[out] FullFileName      pointer to pointer to generated full file name.  It 
                                is the responsibility of the caller to free this memory
                                with a call to FreePool().
  @retval EFI_SUCCESS           the operation was sucessful and the FullFileName is valid.
  @retval EFI_INVALID_PARAMETER Handle was NULL.
  @retval EFI_INVALID_PARAMETER FullFileName was NULL.
  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
EFIAPI
FileHandleGetFileName (
  IN CONST EFI_FILE_HANDLE      Handle,
  OUT CHAR16                    **FullFileName
  ){
  EFI_STATUS      Status;
  UINTN           Size;
  EFI_FILE_HANDLE CurrentHandle;
  EFI_FILE_HANDLE NextHigherHandle;
  EFI_FILE_INFO   *FileInfo;

  Size = 0;

  //
  // Check our parameters
  //
  if (FullFileName == NULL || Handle == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  *FullFileName = NULL;

  Status = Handle->Open(Handle, &CurrentHandle, L".", EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR(Status)) {
    //
    // Reverse out the current directory on the device
    //
    for (;;) {
      FileInfo = FileHandleGetInfo(CurrentHandle);
      if (FileInfo == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      } else {
        //
        // We got info... do we have a name? if yes preceed the current path with it...
        //
        if (StrLen (FileInfo->FileName) == 0) {
          if (*FullFileName == NULL) {
            *FullFileName = StrnCatGrowLeft(FullFileName, &Size, L"\\", 0);
          }
          FreePool(FileInfo);
          break;
        } else {
          if (*FullFileName == NULL) {
            *FullFileName = StrnCatGrowLeft(FullFileName, &Size, L"\\", 0);
          }
          *FullFileName = StrnCatGrowLeft(FullFileName, &Size, FileInfo->FileName, 0);
          *FullFileName = StrnCatGrowLeft(FullFileName, &Size, L"\\", 0);
          FreePool(FileInfo);
        }
      }
      //
      // Move to the parent directory
      //
      Status = CurrentHandle->Open (CurrentHandle, &NextHigherHandle, L"..", EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR (Status)) {
        break;
      }

      FileHandleClose(CurrentHandle);
      CurrentHandle = NextHigherHandle;
    }
  }

  if (CurrentHandle != NULL) {
    CurrentHandle->Close (CurrentHandle);
  }

  if (EFI_ERROR(Status) && *FullFileName != NULL) {
    FreePool(*FullFileName);
  }

  return (Status);
}

/**
  Function to read a single line (up to but not including the \n) from a file.

  @param[in]      Handle        FileHandle to read from
  @param[in,out]  Buffer        pointer to buffer to read into
  @param[in,out]  Size          pointer to number of bytes in buffer
  @param[in[      Truncate      if TRUE then allows for truncation of the line to fit.
                                if FALSE will reset the position to the begining of the 
                                line if the buffer is not large enough.

  @retval EFI_SUCCESS           the operation was sucessful.  the line is stored in 
                                Buffer.
  @retval EFI_INVALID_PARAMETER Handle was NULL.
  @retval EFI_INVALID_PARAMETER Buffer was NULL.
  @retval EFI_INVALID_PARAMETER Size was NULL.
  @retval EFI_BUFFER_TOO_SMALL  Size was not enough space to store the line.  
                                Size was updated to minimum space required.
  @sa FileHandleRead
**/
EFI_STATUS
EFIAPI
FileHandleReadLine(
  IN EFI_FILE_HANDLE            Handle,
  IN OUT VOID                   *Buffer,
  IN OUT UINTN                  *Size,
  IN BOOLEAN                    Truncate
  ){
  EFI_STATUS  Status;
  CHAR16      CharBuffer;
  UINTN       CharSize;
  UINTN       CountSoFar;
  UINT64      Position;


  if (Handle == NULL
    ||Buffer == NULL
    ||Size   == NULL
    ){
  return (EFI_INVALID_PARAMETER);
  }
  FileHandleGetPosition(Handle, &Position);

  for (CountSoFar = 0;;CountSoFar++){
    CharSize = sizeof(CharBuffer);
    Status = FileHandleRead(Handle, &CharSize, &CharBuffer);
    if (  EFI_ERROR(Status) 
       || CharSize == 0 
       || CharBuffer == '\n'
      ){
      break;
    }
    //
    // if we have space save it...
    //
    if ((CountSoFar+1)*sizeof(CHAR16) < *Size){
      ((CHAR16*)Buffer)[CountSoFar] = CharBuffer;
      ((CHAR16*)Buffer)[CountSoFar+1] = CHAR_NULL;
    }
  }

  //
  // if we ran out of space tell when...
  //
  if ((CountSoFar+1)*sizeof(CHAR16) > *Size){
    *Size = (CountSoFar+1)*sizeof(CHAR16);
    if (Truncate == FALSE) {
      FileHandleSetPosition(Handle, Position);
    } else {
      DEBUG((DEBUG_WARN, "The line was truncated in ReadLine"));
    }
    return (EFI_BUFFER_TOO_SMALL);
  }
  *Size = (CountSoFar+1)*sizeof(CHAR16);
  return (Status);
}

/**
  function to write a line of unicode text to a file.

  if Handle is NULL, ASSERT.
  if Buffer is NULL, do nothing.  (return SUCCESS)

  @param[in]     Handle         FileHandle to write to
  @param[in]     Buffer         Buffer to write

  @retval  EFI_SUCCESS          the data was written.
  @retval  other                failure.

  @sa FileHandleWrite
**/
EFI_STATUS
EFIAPI
FileHandleWriteLine(
  IN EFI_FILE_HANDLE Handle,
  IN CHAR16          *Buffer
  ){
  EFI_STATUS Status;
  UINTN      Size;

  ASSERT(Handle != NULL);

  if (Buffer == NULL) {
    return (EFI_SUCCESS);
  }

  Size = StrLen(Buffer);
  Status = FileHandleWrite(Handle, &Size, Buffer);
  if (EFI_ERROR(Status)) {
    return (Status);
  }
  Size = StrLen(L"\r\n");
  return FileHandleWrite(Handle, &Size, L"\r\n");
}
