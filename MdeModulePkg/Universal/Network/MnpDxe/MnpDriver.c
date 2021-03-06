/** @file
  Implementation of driver entry point and driver binding protocol.

Copyright (c) 2005 - 2008, Intel Corporation. <BR> 
All rights reserved. This program and the accompanying materials are licensed 
and made available under the terms and conditions of the BSD License which 
accompanies this distribution. The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php 

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MnpDriver.h"
#include "MnpImpl.h"


EFI_DRIVER_BINDING_PROTOCOL gMnpDriverBinding = {
  MnpDriverBindingSupported,
  MnpDriverBindingStart,
  MnpDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test.
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific 
                                   child device to start.

  @retval EFI_SUCCESS              This driver supports this device.
  @retval EFI_ALREADY_STARTED      This driver is already running on this device.
  @retval Others                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;

  //
  // Test to see if MNP is already installed.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test to open the Simple Network protocol BY_DRIVER.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Snp,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the openned SNP protocol.
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiSimpleNetworkProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}


/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make drivers as small 
  as possible, there are a few calling restrictions for this service.
  ConnectController() must follow these calling restrictions. If any other
  agent wishes to call Start() it must also follow these calling restrictions.

  @param[in]       This                 Protocol instance pointer.
  @param[in]       ControllerHandle     Handle of device to bind driver to.
  @param[in]       RemainingDevicePath  Optional parameter use to pick a specific 
                                        child device to start.

  @retval EFI_SUCCESS           This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED   This driver is already running on ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Mnp Service Data.
  @retval Others                This driver does not support this device.
  
**/
EFI_STATUS
EFIAPI
MnpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;
  MNP_SERVICE_DATA  *MnpServiceData;
  BOOLEAN           MnpInitialized;

  MnpInitialized  = FALSE;

  MnpServiceData  = AllocateZeroPool (sizeof (MNP_SERVICE_DATA));
  if (MnpServiceData == NULL) {
    DEBUG ((EFI_D_ERROR, "MnpDriverBindingStart(): Failed to allocate the Mnp Service Data.\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the Mnp Service Data.
  //
  Status = MnpInitializeServiceData (MnpServiceData, This->DriverBindingHandle, ControllerHandle);
  if (EFI_ERROR (Status)) {

    DEBUG ((EFI_D_ERROR, "MnpDriverBindingStart: MnpInitializeServiceData failed, %r.\n",Status));
    goto ErrorExit;
  }

  MnpInitialized = TRUE;

  //
  // Install the MNP Service Binding Protocol.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  &MnpServiceData->ServiceBinding,
                  NULL
                  );

ErrorExit:

  if (EFI_ERROR (Status)) {

    if (MnpInitialized) {
      //
      // Flush the Mnp Service Data.
      //
      MnpFlushServiceData (MnpServiceData, This->DriverBindingHandle);
    }

    gBS->FreePool (MnpServiceData);
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to make drivers as 
  small as possible, there are a few calling restrictions for this service. 
  DisconnectController() must follow these calling restrictions. If any other 
  agent wishes to call Stop() it must also follow these calling restrictions.
  
  @param[in]  This               Protocol instance pointer.
  @param[in]  ControllerHandle   Handle of device to stop driver on.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer. If 
                                 number of children is zero stop the entire 
								 bus driver.
  @param[in]  ChildHandleBuffer  List of Child Handles to Stop.

  @retval EFI_SUCCESS            This driver is removed ControllerHandle.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  MNP_SERVICE_DATA              *MnpServiceData;
  MNP_INSTANCE_DATA             *Instance;

  //
  // Retrieve the MNP service binding protocol from the ControllerHandle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "MnpDriverBindingStop: Locate MNP Service Binding Protocol failed, %r.\n",
      Status)
      );
    return EFI_DEVICE_ERROR;
  }

  MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (ServiceBinding);

  if (NumberOfChildren == 0) {
    //
    // Uninstall the MNP Service Binding Protocol.
    //
    gBS->UninstallMultipleProtocolInterfaces (
           ControllerHandle,
           &gEfiManagedNetworkServiceBindingProtocolGuid,
           ServiceBinding,
           NULL
           );

    //
    // Flush the Mnp service data.
    //
    MnpFlushServiceData (MnpServiceData, This->DriverBindingHandle);

    gBS->FreePool (MnpServiceData);
  } else {
    while (!IsListEmpty (&MnpServiceData->ChildrenList)) {
      //
      // Don't use NetListRemoveHead here, the remove opreration will be done
      // in ServiceBindingDestroyChild.
      //
      Instance = NET_LIST_HEAD (
                   &MnpServiceData->ChildrenList,
                   MNP_INSTANCE_DATA,
                   InstEntry
                   );

      ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
    }
  }

  return Status;
}


/**
  Creates a child handle with a set of I/O services.

  @param[in]       This              Protocol instance pointer.
  @param[in, out]  ChildHandle       Pointer to the handle of the child to create. If
                                     it is NULL, then a new handle is created. If
									 it is not NULL, then the I/O services are added 
									 to the existing child handle.

  @retval EFI_SUCCES                 The protocol was added to ChildHandle. 
  @retval EFI_INVALID_PARAMETER      ChildHandle is NULL. 
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources availabe to 
                                     create the child.
  @retval Others                     The child handle was not created.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                *ChildHandle
  )
{
  EFI_STATUS         Status;
  MNP_SERVICE_DATA   *MnpServiceData;
  MNP_INSTANCE_DATA  *Instance;
  VOID               *Snp;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (This);

  //
  // Allocate buffer for the new instance.
  //
  Instance = AllocateZeroPool (sizeof (MNP_INSTANCE_DATA));
  if (Instance == NULL) {

    DEBUG ((EFI_D_ERROR, "MnpServiceBindingCreateChild: Faild to allocate memory for the new instance.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Init the instance data.
  //
  MnpInitializeInstanceData (MnpServiceData, Instance);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  &Instance->ManagedNetwork,
                  NULL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "MnpServiceBindingCreateChild: Failed to install the MNP protocol, %r.\n",
      Status)
      );
    goto ErrorExit;
  }

  //
  // Save the instance's childhandle.
  //
  Instance->Handle = *ChildHandle;

  Status = gBS->OpenProtocol (
                  MnpServiceData->ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Snp,
                  gMnpDriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Add the child instance into ChildrenList.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&MnpServiceData->ChildrenList, &Instance->InstEntry);
  MnpServiceData->ChildrenNumber++;

  gBS->RestoreTPL (OldTpl);

ErrorExit:

  if (EFI_ERROR (Status)) {

    if (Instance->Handle != NULL) {

      gBS->UninstallMultipleProtocolInterfaces (
            &gEfiManagedNetworkProtocolGuid,
            &Instance->ManagedNetwork,
            NULL
            );
    }

    gBS->FreePool (Instance);
  }

  return Status;
}


/**
  Destroys a child handle with a set of I/O services.
   
  The DestroyChild() function does the opposite of CreateChild(). It removes a 
  protocol that was installed by CreateChild() from ChildHandle. If the removed 
  protocol is the last protocol on ChildHandle, then ChildHandle is destroyed. 
   
  @param[in]  This               Pointer to the EFI_SERVICE_BINDING_PROTOCOL 
                                 instance.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCES             The protocol was removed from ChildHandle. 
  @retval EFI_UNSUPPORTED        ChildHandle does not support the protocol that
                                 is being removed.
  @retval EFI_INVALID_PARAMETER  ChildHandle is not a valid UEFI handle.
  @retval EFI_ACCESS_DENIED      The protocol could not be removed from the
                                 ChildHandle because its services are being
                                 used.
  @retval Others                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS                    Status;
  MNP_SERVICE_DATA              *MnpServiceData;
  EFI_MANAGED_NETWORK_PROTOCOL  *ManagedNetwork;
  MNP_INSTANCE_DATA             *Instance;
  EFI_TPL                       OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (This);

  //
  // Try to retrieve ManagedNetwork Protocol from ChildHandle.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &ManagedNetwork,
                  gMnpDriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {

    return EFI_UNSUPPORTED;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (ManagedNetwork);

  //
  // MnpServiceBindingDestroyChild may be called twice: first called by
  // MnpServiceBindingStop, second called by uninstalling the MNP protocol
  // in this ChildHandle. Use destroyed to make sure the resource clean code
  // will only excecute once.
  //
  if (Instance->Destroyed) {

    return EFI_SUCCESS;
  }

  Instance->Destroyed = TRUE;

  //
  // Close the Simple Network protocol.
  //
  gBS->CloseProtocol (
         MnpServiceData->ControllerHandle,
         &gEfiSimpleNetworkProtocolGuid,
         gMnpDriverBinding.DriverBindingHandle,
         ChildHandle
         );

  //
  // Uninstall the ManagedNetwork protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  &Instance->ManagedNetwork,
                  NULL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "MnpServiceBindingDestroyChild: Failed to uninstall the ManagedNetwork protocol, %r.\n",
      Status)
      );

    Instance->Destroyed = FALSE;
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Reset the configuration.
  //
  ManagedNetwork->Configure (ManagedNetwork, NULL);

  //
  // Try to flush the RcvdPacketQueue.
  //
  MnpFlushRcvdDataQueue (Instance);

  //
  // Clean the RxTokenMap.
  //
  NetMapClean (&Instance->RxTokenMap);

  //
  // Remove this instance from the ChildrenList.
  //
  RemoveEntryList (&Instance->InstEntry);
  MnpServiceData->ChildrenNumber--;

  gBS->RestoreTPL (OldTpl);

  gBS->FreePool (Instance);

  return Status;
}

/**
  The entry point for Mnp driver which installs the driver binding and component
  name protocol on its ImageHandle.

  @param[in]  ImageHandle  The image handle of the driver.
  @param[in]  SystemTable  The system table.

  @retval EFI_SUCCES       The driver binding and component name protocols are 
                           successfully installed.
  @retval Others           Other errors as indicated.

**/
EFI_STATUS
EFIAPI
MnpDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gMnpDriverBinding,
           ImageHandle,
           &gMnpComponentName,
           &gMnpComponentName2
           );
}
