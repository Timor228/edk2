/** @file
  Help functions to access UDP service, it is used by both the DHCP and MTFTP.
  
Copyright (c) 2005 - 2007, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/Udp4.h>

#include <Library/UdpIoLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DpcLib.h>


/**
  Free a UDP_TX_TOKEN. The TX event is closed.

  @param[in]  Token                 The UDP_TX_TOKEN to release.

**/
VOID
UdpIoFreeTxToken (
  IN UDP_TX_TOKEN           *Token
  )
{
  gBS->CloseEvent (Token->UdpToken.Event);
  gBS->FreePool (Token);
}

/**
  Free a UDP_RX_TOKEN. The RX event is closed.

  @param[in]  Token                 The UDP_RX_TOKEN to release.

**/
VOID
UdpIoFreeRxToken (
  IN UDP_RX_TOKEN           *Token
  )
{
  gBS->CloseEvent (Token->UdpToken.Event);
  gBS->FreePool (Token);
}

/**
  The callback function when the packet is sent by UDP.
  
  It will remove the packet from the local list then call
  the packet owner's callback function set by UdpIoSendDatagram.

  @param[in]  Context               The UDP TX Token.

**/
VOID
EFIAPI
UdpIoOnDgramSentDpc (
  IN VOID                   *Context
  )
{
  UDP_TX_TOKEN              *Token;

  Token   = (UDP_TX_TOKEN *) Context;
  ASSERT (Token->Signature == UDP_IO_TX_SIGNATURE);

  RemoveEntryList (&Token->Link);
  Token->CallBack (Token->Packet, NULL, Token->UdpToken.Status, Token->Context);

  UdpIoFreeTxToken (Token);
}

/**
  Request UdpIoOnDgramSentDpc as a DPC at TPL_CALLBACK.
  
  @param[in]  Event                 The event signaled.
  @param[in]  Context               The UDP TX Token.

**/
VOID
EFIAPI
UdpIoOnDgramSent (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  //
  // Request UdpIoOnDgramSentDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, UdpIoOnDgramSentDpc, Context);
}

/**
  Recycle the received UDP data.

  @param[in]  Context               The UDP_RX_TOKEN.

**/
VOID
UdpIoRecycleDgram (
  IN VOID                   *Context
  )
{
  UDP_RX_TOKEN              *Token;

  Token = (UDP_RX_TOKEN *) Context;
  gBS->SignalEvent (Token->UdpToken.Packet.RxData->RecycleSignal);
  UdpIoFreeRxToken (Token);
}

/**
  The event handle for UDP receive request.
  
  It will build a NET_BUF from the recieved UDP data, then deliver it
  to the receiver.

  @param[in]  Context               The UDP RX token.

**/
VOID
EFIAPI
UdpIoOnDgramRcvdDpc (
  IN VOID                   *Context
  )
{
  EFI_UDP4_COMPLETION_TOKEN *UdpToken;
  EFI_UDP4_RECEIVE_DATA     *UdpRxData;
  EFI_UDP4_SESSION_DATA     *UdpSession;
  UDP_RX_TOKEN              *Token;
  UDP_POINTS                Points;
  NET_BUF                   *Netbuf;

  Token   = (UDP_RX_TOKEN *) Context;

  ASSERT ((Token->Signature == UDP_IO_RX_SIGNATURE) &&
          (Token == Token->UdpIo->RecvRequest));

  //
  // Clear the receive request first in case that the caller
  // wants to restart the receive in the callback.
  //
  Token->UdpIo->RecvRequest = NULL;

  UdpToken  = &Token->UdpToken;
  UdpRxData = UdpToken->Packet.RxData;

  if (EFI_ERROR (UdpToken->Status) || (UdpRxData == NULL)) {
    if (UdpToken->Status != EFI_ABORTED) {
      //
      // Invoke the CallBack only if the reception is not actively aborted.
      //
      Token->CallBack (NULL, NULL, UdpToken->Status, Token->Context);
    }

    UdpIoFreeRxToken (Token);
    return;
  }

  //
  // Build a NET_BUF from the UDP receive data, then deliver it up.
  //
  Netbuf = NetbufFromExt (
             (NET_FRAGMENT *) UdpRxData->FragmentTable,
             UdpRxData->FragmentCount,
             0,
             (UINT32) Token->HeadLen,
             UdpIoRecycleDgram,
             Token
             );

  if (Netbuf == NULL) {
    gBS->SignalEvent (UdpRxData->RecycleSignal);
    Token->CallBack (NULL, NULL, EFI_OUT_OF_RESOURCES, Token->Context);

    UdpIoFreeRxToken (Token);
    return;
  }

  UdpSession        = &UdpRxData->UdpSession;
  Points.LocalPort  = UdpSession->DestinationPort;
  Points.RemotePort = UdpSession->SourcePort;

  CopyMem (&Points.LocalAddr, &UdpSession->DestinationAddress, sizeof (IP4_ADDR));
  CopyMem (&Points.RemoteAddr, &UdpSession->SourceAddress, sizeof (IP4_ADDR));
  Points.LocalAddr  = NTOHL (Points.LocalAddr);
  Points.RemoteAddr = NTOHL (Points.RemoteAddr);

  Token->CallBack (Netbuf, &Points, EFI_SUCCESS, Token->Context);
}

/**
  Request UdpIoOnDgramRcvdDpc() as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The UDP receive request event.
  @param[in]  Context               The UDP RX token.

**/
VOID
EFIAPI
UdpIoOnDgramRcvd (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  //
  // Request UdpIoOnDgramRcvdDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, UdpIoOnDgramRcvdDpc, Context);
}

/**
  Create a UDP_RX_TOKEN to wrap the request.

  @param[in]  UdpIo                 The UdpIo to receive packets from.
  @param[in]  CallBack              The function to call when receive finished.
  @param[in]  Context               The opaque parameter to the CallBack.
  @param[in]  HeadLen               The head length to reserver for the packet.

  @return The Wrapped request or NULL if failed to allocate resources or some errors happened.

**/
UDP_RX_TOKEN *
UdpIoCreateRxToken (
  IN UDP_IO_PORT            *UdpIo,
  IN UDP_IO_CALLBACK        CallBack,
  IN VOID                   *Context,
  IN UINT32                 HeadLen
  )
{
  UDP_RX_TOKEN              *Token;
  EFI_STATUS                Status;

  Token = AllocatePool (sizeof (UDP_RX_TOKEN));

  if (Token == NULL) {
    return NULL;
  }

  Token->Signature              = UDP_IO_RX_SIGNATURE;
  Token->UdpIo                  = UdpIo;
  Token->CallBack               = CallBack;
  Token->Context                = Context;
  Token->HeadLen                = HeadLen;

  Token->UdpToken.Status        = EFI_NOT_READY;
  Token->UdpToken.Packet.RxData = NULL;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UdpIoOnDgramRcvd,
                  Token,
                  &Token->UdpToken.Event
                  );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (Token);
    return NULL;
  }

  return Token;
}

/**
  Wrap a transmit request into a UDP_TX_TOKEN.

  @param[in]  UdpIo                 The UdpIo port to send packet to.
  @param[in]  Packet                The user's packet.
  @param[in]  EndPoint              The local and remote access point.
  @param[in]  Gateway               The overrided next hop.
  @param[in]  CallBack              The function to call when transmission completed.
  @param[in]  Context               The opaque parameter to the call back.

  @return The wrapped transmission request or NULL if failed to allocate resources 
          or for some errors.

**/
UDP_TX_TOKEN *
UdpIoWrapTx (
  IN UDP_IO_PORT            *UdpIo,
  IN NET_BUF                *Packet,
  IN UDP_POINTS             *EndPoint  OPTIONAL,
  IN IP4_ADDR               Gateway,
  IN UDP_IO_CALLBACK        CallBack,
  IN VOID                   *Context
  )
{
  UDP_TX_TOKEN              *Token;
  EFI_UDP4_COMPLETION_TOKEN *UdpToken;
  EFI_UDP4_TRANSMIT_DATA    *UdpTxData;
  EFI_STATUS                Status;
  UINT32                    Count;
  IP4_ADDR                  Ip;

  Token = AllocatePool (sizeof (UDP_TX_TOKEN) +
                           sizeof (EFI_UDP4_FRAGMENT_DATA) * (Packet->BlockOpNum - 1));

  if (Token == NULL) {
    return NULL;
  }

  Token->Signature  = UDP_IO_TX_SIGNATURE;
  InitializeListHead (&Token->Link);

  Token->UdpIo      = UdpIo;
  Token->CallBack   = CallBack;
  Token->Packet     = Packet;
  Token->Context    = Context;

  UdpToken          = &(Token->UdpToken);
  UdpToken->Status  = EFI_NOT_READY;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UdpIoOnDgramSent,
                  Token,
                  &UdpToken->Event
                  );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (Token);
    return NULL;
  }

  UdpTxData                 = &Token->UdpTxData;
  UdpToken->Packet.TxData   = UdpTxData;

  UdpTxData->UdpSessionData = NULL;
  UdpTxData->GatewayAddress = NULL;

  if (EndPoint != NULL) {
    Ip = HTONL (EndPoint->LocalAddr);
    CopyMem (&Token->UdpSession.SourceAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

    Ip = HTONL (EndPoint->RemoteAddr);
    CopyMem (&Token->UdpSession.DestinationAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

    Token->UdpSession.SourcePort      = EndPoint->LocalPort;
    Token->UdpSession.DestinationPort = EndPoint->RemotePort;
    UdpTxData->UdpSessionData         = &Token->UdpSession;
  }

  if (Gateway != 0) {
    Ip = HTONL (Gateway);
    CopyMem (&Token->Gateway, &Ip, sizeof (EFI_IPv4_ADDRESS));

    UdpTxData->GatewayAddress = &Token->Gateway;
  }

  UdpTxData->DataLength = Packet->TotalSize;
  Count                 = Packet->BlockOpNum;
  NetbufBuildExt (Packet, (NET_FRAGMENT *) UdpTxData->FragmentTable, &Count);
  UdpTxData->FragmentCount = Count;

  return Token;
}

/**
  Create a UDP_IO_PORT to access the UDP service. It will create and configure
  a UDP child.
  
  The function will locate the UDP service binding prototype on the Controller
  parameter and use it to create a UDP child (aka Udp instance). Then the UDP
  child will be configured by calling Configure function prototype. Any failures
  in creating or configure the UDP child will lead to the failure of UDP_IO_PORT
  creation.

  @param[in]  Controller            The controller that has the UDP service binding.
                                    protocol installed.
  @param[in]  Image                 The image handle for the driver.
  @param[in]  Configure             The function to configure the created UDP child.
  @param[in]  Context               The opaque parameter for the Configure funtion.

  @return Newly-created UDP_IO_PORT or NULL if failed.

**/
UDP_IO_PORT *
EFIAPI
UdpIoCreatePort (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  IN  UDP_IO_CONFIG         Configure,
  IN  VOID                  *Context
  )
{
  UDP_IO_PORT               *UdpIo;
  EFI_STATUS                Status;

  ASSERT (Configure != NULL);

  UdpIo = AllocatePool (sizeof (UDP_IO_PORT));

  if (UdpIo == NULL) {
    return NULL;
  }

  UdpIo->Signature    = UDP_IO_SIGNATURE;
  InitializeListHead (&UdpIo->Link);
  UdpIo->RefCnt       = 1;

  UdpIo->Controller   = Controller;
  UdpIo->Image        = Image;

  InitializeListHead (&UdpIo->SentDatagram);
  UdpIo->RecvRequest  = NULL;
  UdpIo->UdpHandle    = NULL;

  //
  // Create a UDP child then open and configure it
  //
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiUdp4ServiceBindingProtocolGuid,
             &UdpIo->UdpHandle
             );

  if (EFI_ERROR (Status)) {
    goto FREE_MEM;
  }

  Status = gBS->OpenProtocol (
                  UdpIo->UdpHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **) &UdpIo->Udp,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto FREE_CHILD;
  }

  if (EFI_ERROR (Configure (UdpIo, Context))) {
    goto CLOSE_PROTOCOL;
  }

  Status = UdpIo->Udp->GetModeData (UdpIo->Udp, NULL, NULL, NULL, &UdpIo->SnpMode);

  if (EFI_ERROR (Status)) {
    goto CLOSE_PROTOCOL;
  }

  return UdpIo;

CLOSE_PROTOCOL:
  gBS->CloseProtocol (UdpIo->UdpHandle, &gEfiUdp4ProtocolGuid, Image, Controller);

FREE_CHILD:
  NetLibDestroyServiceChild (
    Controller,
    Image,
    &gEfiUdp4ServiceBindingProtocolGuid,
    UdpIo->UdpHandle
    );

FREE_MEM:
  gBS->FreePool (UdpIo);
  return NULL;
}

/**
  Cancel all the sent datagram that pass the selection criteria of ToCancel.
  If ToCancel is NULL, all the datagrams are cancelled.

  @param[in]  UdpIo                 The UDP_IO_PORT to cancel packet.
  @param[in]  IoStatus              The IoStatus to return to the packet owners.
  @param[in]  ToCancel              The select funtion to test whether to cancel this
                                    packet or not.
  @param[in]  Context               The opaque parameter to the ToCancel.

**/
VOID
EFIAPI
UdpIoCancelDgrams (
  IN UDP_IO_PORT            *UdpIo,
  IN EFI_STATUS             IoStatus,
  IN UDP_IO_TO_CANCEL       ToCancel,        OPTIONAL
  IN VOID                   *Context
  )
{
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  UDP_TX_TOKEN              *Token;

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &UdpIo->SentDatagram) {
    Token = NET_LIST_USER_STRUCT (Entry, UDP_TX_TOKEN, Link);

    if ((ToCancel == NULL) || (ToCancel (Token, Context))) {
      UdpIo->Udp->Cancel (UdpIo->Udp, &Token->UdpToken);
    }
  }
}

/**
  Free the UDP_IO_PORT and all its related resources.
  
  The function will cancel all sent datagram and receive request.

  @param[in]  UdpIo                 The UDP_IO_PORT to free.

  @retval EFI_SUCCESS           The UDP_IO_PORT is freed.

**/
EFI_STATUS
EFIAPI
UdpIoFreePort (
  IN  UDP_IO_PORT           *UdpIo
  )
{
  UDP_RX_TOKEN  *RxToken;

  //
  // Cancel all the sent datagram and receive requests. The
  // callbacks of transmit requests are executed to allow the
  // caller to release the resource. The callback of receive
  // request are NOT executed. This is because it is most
  // likely that the current user of the UDP IO port is closing
  // itself.
  //
  UdpIoCancelDgrams (UdpIo, EFI_ABORTED, NULL, NULL);

  if ((RxToken = UdpIo->RecvRequest) != NULL) {
    UdpIo->Udp->Cancel (UdpIo->Udp, &RxToken->UdpToken);
  }

  //
  // Close then destory the UDP child
  //
  gBS->CloseProtocol (
         UdpIo->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         UdpIo->Image,
         UdpIo->Controller
         );

  NetLibDestroyServiceChild (
    UdpIo->Controller,
    UdpIo->Image,
    &gEfiUdp4ServiceBindingProtocolGuid,
    UdpIo->UdpHandle
    );

  if (!IsListEmpty(&UdpIo->Link)) {
    RemoveEntryList (&UdpIo->Link);
  }

  gBS->FreePool (UdpIo);
  return EFI_SUCCESS;
}


/**
  Clean up the UDP_IO_PORT without freeing it. The function is called when
  user wants to re-use the UDP_IO_PORT later.
  
  It will release all the transmitted datagrams and receive request. It will
  also configure NULL for the UDP instance.

  @param[in]  UdpIo                 The UDP_IO_PORT to clean up.

**/
VOID
EFIAPI
UdpIoCleanPort (
  IN  UDP_IO_PORT           *UdpIo
  )
{
  UDP_RX_TOKEN              *RxToken;

  //
  // Cancel all the sent datagram and receive requests.
  //
  UdpIoCancelDgrams (UdpIo, EFI_ABORTED, NULL, NULL);

  if ((RxToken = UdpIo->RecvRequest) != NULL) {
    UdpIo->Udp->Cancel (UdpIo->Udp, &RxToken->UdpToken);
  }

  UdpIo->Udp->Configure (UdpIo->Udp, NULL);
}

/**
  Send a packet through the UDP_IO_PORT.
  
  The packet will be wrapped in UDP_TX_TOKEN. Function Callback will be called
  when the packet is sent. The optional parameter EndPoint overrides the default
  address pair if specified.

  @param[in]  UdpIo                 The UDP_IO_PORT to send the packet through.
  @param[in]  Packet                The packet to send.
  @param[in]  EndPoint              The local and remote access point. Override the
                                    default address pair set during configuration.
  @param[in]  Gateway               The gateway to use.
  @param[in]  CallBack              The function being called when packet is
                                    transmitted or failed.
  @param[in]  Context               The opaque parameter passed to CallBack.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the packet.
  @retval EFI_SUCCESS           The packet is successfully delivered to UDP  for
                                transmission.

**/
EFI_STATUS
EFIAPI
UdpIoSendDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet,
  IN  UDP_POINTS            *EndPoint, OPTIONAL
  IN  IP4_ADDR              Gateway,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context
  )
{
  UDP_TX_TOKEN              *Token;
  EFI_STATUS                Status;

  Token = UdpIoWrapTx (UdpIo, Packet, EndPoint, Gateway, CallBack, Context);

  if (Token == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Insert the tx token into SendDatagram list before transmitting it. Remove
  // it from the list if the returned status is not EFI_SUCCESS.
  //
  InsertHeadList (&UdpIo->SentDatagram, &Token->Link);
  Status = UdpIo->Udp->Transmit (UdpIo->Udp, &Token->UdpToken);
  if (EFI_ERROR (Status)) {
    RemoveEntryList (&Token->Link);
    UdpIoFreeTxToken (Token);
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  The select function to cancel a single sent datagram.

  @param[in]  Token                 The UDP_TX_TOKEN to test against
  @param[in]  Context               The NET_BUF of the sent datagram

  @retval TRUE              The packet is to be cancelled.
  @retval FALSE             The packet is not to be cancelled.
**/
BOOLEAN
UdpIoCancelSingleDgram (
  IN UDP_TX_TOKEN           *Token,
  IN VOID                   *Context
  )
{
  NET_BUF                   *Packet;

  Packet = (NET_BUF *) Context;

  if (Token->Packet == Packet) {
    return TRUE;
  }

  return FALSE;
}

/**
  Cancel a single sent datagram.

  @param[in]  UdpIo                 The UDP_IO_PORT to cancel the packet from
  @param[in]  Packet                The packet to cancel

**/
VOID
EFIAPI
UdpIoCancelSentDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet
  )
{
  UdpIoCancelDgrams (UdpIo, EFI_ABORTED, UdpIoCancelSingleDgram, Packet);
}

/**
  Issue a receive request to the UDP_IO_PORT.
  
  This function is called when upper-layer needs packet from UDP for processing.
  Only one receive request is acceptable at a time so a common usage model is
  to invoke this function inside its Callback function when the former packet
  is processed.

  @param[in]  UdpIo                 The UDP_IO_PORT to receive the packet from.
  @param[in]  CallBack              The call back function to execute when the packet
                                    is received.
  @param[in]  Context               The opaque context passed to Callback.
  @param[in] HeadLen                The length of the upper-layer's protocol header.

  @retval EFI_ALREADY_STARTED   There is already a pending receive request. Only
                                one receive request is supported at a time.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate needed resources.
  @retval EFI_SUCCESS           The receive request is issued successfully.

**/
EFI_STATUS
EFIAPI
UdpIoRecvDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  UDP_IO_CALLBACK       CallBack,
  IN  VOID                  *Context,
  IN  UINT32                HeadLen
  )
{
  UDP_RX_TOKEN              *Token;
  EFI_STATUS                Status;

  if (UdpIo->RecvRequest != NULL) {
    return EFI_ALREADY_STARTED;
  }

  Token = UdpIoCreateRxToken (UdpIo, CallBack, Context, HeadLen);

  if (Token == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UdpIo->RecvRequest = Token;
  Status = UdpIo->Udp->Receive (UdpIo->Udp, &Token->UdpToken);

  if (EFI_ERROR (Status)) {
    UdpIo->RecvRequest = NULL;
    UdpIoFreeRxToken (Token);
  }

  return Status;
}
