/** @file
  Device Path Driver to produce DevPathUtilities Protocol, DevPathFromText Protocol
  and DevPathToText Protocol.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DevicePath.h"

EFI_HANDLE  mDevicePathHandle = NULL;

GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_DEVICE_PATH_UTILITIES_PROTOCOL mDevicePathUtilities = {
  GetDevicePathSizeProtocolInterface,
  DuplicateDevicePathProtocolInterface,
  AppendDevicePathProtocolInterface,
  AppendDeviceNodeProtocolInterface,
  AppendDevicePathInstanceProtocolInterface,
  GetNextDevicePathInstanceProtocolInterface,
  IsDevicePathMultiInstanceProtocolInterface,
  CreateDeviceNodeProtocolInterface
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_DEVICE_PATH_TO_TEXT_PROTOCOL   mDevicePathToText = {
  ConvertDeviceNodeToText,
  ConvertDevicePathToText
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL mDevicePathFromText = {
  ConvertTextToDeviceNode,
  ConvertTextToDevicePath
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_GUID mEfiDevicePathMessagingUartFlowControlGuid = DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL;
GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_GUID mEfiDevicePathMessagingSASGuid             = DEVICE_PATH_MESSAGING_SAS;



/**
  The user Entry Point for DevicePath module.

  This is the entry point for DevicePath module. It installs the UEFI Device Path Utility Protocol and
  optionally the Device Path to Text and Device Path from Text protocols based on feature flags.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Others            Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DevicePathEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;
  if (FeaturePcdGet (PcdDevicePathSupportDevicePathToText)) {
    if (FeaturePcdGet (PcdDevicePathSupportDevicePathFromText)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      &gEfiDevicePathToTextProtocolGuid,    &mDevicePathToText,
                      &gEfiDevicePathFromTextProtocolGuid,  &mDevicePathFromText,
                      NULL
                      );
    } else {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      &gEfiDevicePathToTextProtocolGuid,    &mDevicePathToText,
                      NULL
                      );
    }
  } else {
    if (FeaturePcdGet (PcdDevicePathSupportDevicePathFromText)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      &gEfiDevicePathFromTextProtocolGuid,  &mDevicePathFromText,
                      NULL
                      );
    } else {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mDevicePathHandle,
                      &gEfiDevicePathUtilitiesProtocolGuid, &mDevicePathUtilities,
                      NULL
                      );
    }
  }
  return Status;
}
