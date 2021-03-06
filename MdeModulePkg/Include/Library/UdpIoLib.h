/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to access UDP service. It is used by both DHCP and MTFTP.

Copyright (c) 2006 - 2008, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UDP4IO_H_
#define _UDP4IO_H_

#include <Protocol/Udp4.h>

#include <Library/NetLib.h>

typedef struct _UDP_IO_PORT UDP_IO_PORT;

///
/// Signatures used by UdpIo Library.
///
typedef enum {
  UDP_IO_RX_SIGNATURE = SIGNATURE_32 ('U', 'D', 'P', 'R'),
  UDP_IO_TX_SIGNATURE = SIGNATURE_32 ('U', 'D', 'P', 'T'),
  UDP_IO_SIGNATURE    = SIGNATURE_32 ('U', 'D', 'P', 'I')
} UDP_IO_SIGNATURE_TYPE;

///
/// The Udp4 address pair.
///
typedef struct {
  IP4_ADDR                  LocalAddr;
  UINT16                    LocalPort;
  IP4_ADDR                  RemoteAddr;
  UINT16                    RemotePort;
} UDP_POINTS;

/**
  Prototype called when receiving or sending packets to or from a UDP point.

  This prototype is used by both receive and sending when calling
  UdpIoRecvDatagram() or UdpIoSendDatagram(). When receiving, Netbuf is allocated by the
  UDP access point and released by the user. When sending, the user allocates the the NetBuf, which is then
  provided to the callback as a reference.
  
  @param[in] Packet       Packet received or sent
  @param[in] Points       The Udp4 address pair corresponds to the Udp4 IO
  @param[in] IoStatus     Packet receiving or sending status
  @param[in] Context      User-defined data when calling UdpIoRecvDatagram() or
                          UdpIoSendDatagram()
**/
typedef
VOID
(*UDP_IO_CALLBACK) (
  IN NET_BUF                *Packet,
  IN UDP_POINTS             *Points,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );

///
/// This structure is used internally by the UdpIo Library.
///
/// Each receive request is wrapped in an UDP_RX_TOKEN. Upon completion,
/// the CallBack will be called. Only one receive request is sent to UDP at a
/// time. HeadLen gives the length of the application's header. UDP_IO will
/// make the application's header continuous before delivering up.
///
typedef struct {
  UINT32                    Signature;
  UDP_IO_PORT               *UdpIo;

  UDP_IO_CALLBACK           CallBack;
  VOID                      *Context;

  UINT32                    HeadLen;
  EFI_UDP4_COMPLETION_TOKEN UdpToken;
} UDP_RX_TOKEN;

///
/// This structure is used internally by UdpIo Library.
///
/// Each transmit request is wrapped in an UDP_TX_TOKEN. Upon completion,
/// the CallBack will be called. There can be several transmit requests. All transmit requests
/// are linked in a list.
///
typedef struct {
  UINT32                    Signature;
  LIST_ENTRY                Link;
  UDP_IO_PORT               *UdpIo;

  UDP_IO_CALLBACK           CallBack;
  NET_BUF                   *Packet;
  VOID                      *Context;

  EFI_UDP4_SESSION_DATA     UdpSession;
  EFI_IPv4_ADDRESS          Gateway;

  EFI_UDP4_COMPLETION_TOKEN UdpToken;
  EFI_UDP4_TRANSMIT_DATA    UdpTxData;
} UDP_TX_TOKEN;

///
/// Type defined as UDP_IO_PORT.
///
/// This data structure wraps the Udp4 instance and configuration. 
/// UdpIo Library uses this structure for all Udp4 operations.
///
struct _UDP_IO_PORT {
  UINT32                    Signature;
  LIST_ENTRY                Link;
  INTN                      RefCnt;

  //
  // Handle used to create/destory UDP child
  //
  EFI_HANDLE                Controller;
  EFI_HANDLE                Image;
  EFI_HANDLE                UdpHandle;

  EFI_UDP4_PROTOCOL         *Udp;           ///< The wrapped Udp4 instance.
  EFI_UDP4_CONFIG_DATA      UdpConfig;
  EFI_SIMPLE_NETWORK_MODE   SnpMode;

  LIST_ENTRY                SentDatagram;   ///< A list of UDP_TX_TOKEN.
  UDP_RX_TOKEN              *RecvRequest;
};

/**
  Prototype called when UdpIo Library configures a Udp4 instance.
  
  The prototype is set and called when creating a UDP_IO_PORT in UdpIoCreatePort().
  
  @param[in] UdpIo         The UDP_IO_PORT to configure
  @param[in] Context       User-defined data when calling UdpIoCreatePort()
  
  @retval EFI_SUCCESS  The configuration succeeded
  @retval Others       The UDP_IO_PORT fails to configure indicating
                       UdpIoCreatePort() should fail
**/
typedef
EFI_STATUS
(*UDP_IO_CONFIG) (
  IN UDP_IO_PORT            *UdpIo,
  IN VOID                   *Context
  );

/**
  The select function to decide whether to cancel the UDP_TX_TOKEN. 
  
  @param[in] Token        The UDP_TX_TOKEN to decide whether to cancel
  @param[in] Context      User-defined data in UdpIoCancelDgrams()
  
  @retval TRUE        Cancel the UDP_TX_TOKEN
  @retval FALSE       Do not cancel this UDP_TX_TOKEN

**/
typedef
BOOLEAN
(*UDP_IO_TO_CANCEL) (
  IN UDP_TX_TOKEN           *Token,
  IN VOID                   *Context
  );

/**
  Cancel all sent datagrams selected by the parameter ToCancel.
  If ToCancel is NULL, all the datagrams are cancelled.

  @param[in]  UdpIo                 The UDP_IO_PORT to cancel packet.
  @param[in]  IoStatus              The IoStatus to return to the packet owners.
  @param[in]  ToCancel              Sets the criteria for canceling a packet. 
  @param[in]  Context               The opaque parameter to the ToCancel.

**/
VOID
EFIAPI
UdpIoCancelDgrams (
  IN UDP_IO_PORT            *UdpIo,
  IN EFI_STATUS             IoStatus,
  IN UDP_IO_TO_CANCEL       ToCancel,        OPTIONAL
  IN VOID                   *Context
  );

/**
  Creates a UDP_IO_PORT to access the UDP service. It creates and configures
  a UDP child.
  
  This function:
  # locates the UDP service binding prototype on the Controller parameter
  # uses the UDP service binding prototype to create a UDP child (also known as a UDP instance)
  # configures the UDP child by calling Configure function prototype. 
  Any failures in creating or configuring the UDP child return NULL for failure. 

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
  );

/**
  Free the UDP_IO_PORT and all its related resources.
  
  The function cancels all sent datagrams and receive requests.

  @param[in]  UdpIo                 The UDP_IO_PORT to free.

  @retval EFI_SUCCESS           The UDP_IO_PORT is freed.

**/
EFI_STATUS
EFIAPI
UdpIoFreePort (
  IN  UDP_IO_PORT           *UdpIo
  );

/**
  Cleans up the UDP_IO_PORT without freeing it. Call this function 
  if you intend to later re-use the UDP_IO_PORT.
  
  This function releases all transmitted datagrams and receive requests and configures NULL for the UDP instance.

  @param[in]  UdpIo                 The UDP_IO_PORT to clean up.

**/
VOID
EFIAPI
UdpIoCleanPort (
  IN  UDP_IO_PORT           *UdpIo
  );

/**
  Sends a packet through the UDP_IO_PORT.
  
  The packet will be wrapped in UDP_TX_TOKEN. The function specific in the CallBack parameter will be called
  when the packet is sent. If specified, the optional parameter EndPoint overrides the default
  address pair.

  @param[in]  UdpIo                 The UDP_IO_PORT to send the packet through.
  @param[in]  Packet                The packet to send.
  @param[in]  EndPoint              The local and remote access point. Overrides the
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
  );

/**
  Cancel a single sent datagram.

  @param[in]  UdpIo                 The UDP_IO_PORT from which to cancel the packet 
  @param[in]  Packet                The packet to cancel

**/
VOID
EFIAPI
UdpIoCancelSentDatagram (
  IN  UDP_IO_PORT           *UdpIo,
  IN  NET_BUF               *Packet
  );

/**
  Issue a receive request to the UDP_IO_PORT.
  
  This function is called when upper-layer needs packet from UDP for processing.
  Only one receive request is acceptable at a time. Therefore, one common usage model is
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
  );
#endif
