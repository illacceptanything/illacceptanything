//------------------------------------------------------------------------------
// <copyright file="ar6k_prot_hciUart.c" company="Atheros">
//    Copyright (c) 2007-2010 Atheros Corporation.  All rights reserved.
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
//------------------------------------------------------------------------------
//==============================================================================
// Protocol module for use in bridging HCI-UART packets over the GMBOX interface
//
// Author(s): ="Atheros"
//==============================================================================
#include "a_config.h"
#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "../htc_debug.h"
#include "hif.h"
#include "htc_packet.h"
#include "ar6k.h"
#include "hci_transport_api.h"
#include "gmboxif.h"
#include "ar6000_diag.h"
#include "hw/apb_map.h"
#include "hw/mbox_reg.h"

#ifdef ATH_AR6K_ENABLE_GMBOX
#define HCI_UART_COMMAND_PKT 0x01
#define HCI_UART_ACL_PKT     0x02
#define HCI_UART_SCO_PKT     0x03
#define HCI_UART_EVENT_PKT   0x04

#define HCI_RECV_WAIT_BUFFERS (1 << 0)

#define HCI_SEND_WAIT_CREDITS (1 << 0)

#define HCI_UART_BRIDGE_CREDIT_SIZE     128

#define CREDIT_POLL_COUNT       256

#define HCI_DELAY_PER_INTERVAL_MS 10 
#define BTON_TIMEOUT_MS           500
#define BTOFF_TIMEOUT_MS          500
#define BAUD_TIMEOUT_MS           1
#define BTPWRSAV_TIMEOUT_MS       1  

typedef struct {
    HCI_TRANSPORT_CONFIG_INFO   HCIConfig;
    A_BOOL                      HCIAttached;
    A_BOOL                      HCIStopped;
    A_UINT32                    RecvStateFlags;
    A_UINT32                    SendStateFlags;
    HCI_TRANSPORT_PACKET_TYPE   WaitBufferType;
    HTC_PACKET_QUEUE            SendQueue;         /* write queue holding HCI Command and ACL packets */
    HTC_PACKET_QUEUE            HCIACLRecvBuffers;  /* recv queue holding buffers for incomming ACL packets */
    HTC_PACKET_QUEUE            HCIEventBuffers;    /* recv queue holding buffers for incomming event packets */
    AR6K_DEVICE                 *pDev;
    A_MUTEX_T                   HCIRxLock;
    A_MUTEX_T                   HCITxLock;
    int                         CreditsMax;
    int                         CreditsConsumed;
    int                         CreditsAvailable;
    int                         CreditSize;
    int                         CreditsCurrentSeek;
    int                         SendProcessCount;
} GMBOX_PROTO_HCI_UART;

#define LOCK_HCI_RX(t)   A_MUTEX_LOCK(&(t)->HCIRxLock);
#define UNLOCK_HCI_RX(t) A_MUTEX_UNLOCK(&(t)->HCIRxLock);
#define LOCK_HCI_TX(t)   A_MUTEX_LOCK(&(t)->HCITxLock);
#define UNLOCK_HCI_TX(t) A_MUTEX_UNLOCK(&(t)->HCITxLock);

#define DO_HCI_RECV_INDICATION(p,pt) \
{   AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI: Indicate Recv on packet:0x%lX status:%d len:%d type:%d \n",  \
      (unsigned long)(pt),(pt)->Status, A_SUCCESS((pt)->Status) ? (pt)->ActualLength : 0, HCI_GET_PACKET_TYPE(pt))); \
    (p)->HCIConfig.pHCIPktRecv((p)->HCIConfig.pContext, (pt));                                 \
}

#define DO_HCI_SEND_INDICATION(p,pt) \
{   AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: Indicate Send on packet:0x%lX status:%d type:%d \n",  \
            (unsigned long)(pt),(pt)->Status,HCI_GET_PACKET_TYPE(pt)));                             \
    (p)->HCIConfig.pHCISendComplete((p)->HCIConfig.pContext, (pt));                            \
}
    
static A_STATUS HCITrySend(GMBOX_PROTO_HCI_UART *pProt, HTC_PACKET *pPacket, A_BOOL Synchronous);

static void HCIUartCleanup(GMBOX_PROTO_HCI_UART *pProtocol)
{
    A_ASSERT(pProtocol != NULL);
    
    A_MUTEX_DELETE(&pProtocol->HCIRxLock);
    A_MUTEX_DELETE(&pProtocol->HCITxLock);
        
    A_FREE(pProtocol);    
}

static A_STATUS InitTxCreditState(GMBOX_PROTO_HCI_UART *pProt)
{
    A_STATUS    status;
    int         credits;
    int         creditPollCount = CREDIT_POLL_COUNT;
    A_BOOL      gotCredits = FALSE;

    pProt->CreditsConsumed = 0;
    
    do {    
        
        if (pProt->CreditsMax != 0) {
            /* we can only call this only once per target reset */
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HCI: InitTxCreditState - already called!  \n"));
            A_ASSERT(FALSE);
            status = A_EINVAL;
            break; 
        }
        
        /* read the credit counter. At startup the target will set the credit counter
         * to the max available, we read this in a loop because it may take
         * multiple credit counter reads to get all credits  */
                 
        while (creditPollCount) {
            
            credits = 0;

            status = DevGMboxReadCreditCounter(pProt->pDev, PROC_IO_SYNC, &credits);
    
            if (A_FAILED(status)) {
                break;    
            }
            
            if (!gotCredits && (0 == credits)) {
                creditPollCount--;
                AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: credit is 0, retrying (%d)  \n",creditPollCount));
                A_MDELAY(HCI_DELAY_PER_INTERVAL_MS);
                continue;  
            } else {
                gotCredits = TRUE;    
            }
            
            if (0 == credits) {
                break;    
            }
            
            pProt->CreditsMax += credits;
        }
        
        if (A_FAILED(status)) {
            break;    
        }
        
        if (0 == creditPollCount) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,
                    ("** HCI : Failed to get credits! GMBOX Target was not available \n"));        
            status = A_ERROR;
            break;
        }
        
            /* now get the size */
        status = DevGMboxReadCreditSize(pProt->pDev, &pProt->CreditSize);
        
        if (A_FAILED(status)) {
            break;    
        }
               
    } while (FALSE);
    
    if (A_SUCCESS(status)) {
        pProt->CreditsAvailable = pProt->CreditsMax;
        AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("HCI : InitTxCreditState - credits avail: %d, size: %d \n",
            pProt->CreditsAvailable, pProt->CreditSize));    
    }    
    
    return status;
}

static A_STATUS CreditsAvailableCallback(void *pContext, int Credits, A_BOOL CreditIRQEnabled)
{
    GMBOX_PROTO_HCI_UART *pProt = (GMBOX_PROTO_HCI_UART *)pContext;    
    A_BOOL               enableCreditIrq = FALSE;   
    A_BOOL               disableCreditIrq = FALSE;
    A_BOOL               doPendingSends = FALSE;
    A_STATUS             status = A_OK;
    
    /** this callback is called under 2 conditions:
     *   1. The credit IRQ interrupt was enabled and signaled.
     *   2. A credit counter read completed.
     * 
     *   The function must not assume that the calling context can block !
     */
     
    AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("+CreditsAvailableCallback (Credits:%d, IRQ:%s) \n",
                Credits, CreditIRQEnabled ? "ON" : "OFF"));
    
    LOCK_HCI_TX(pProt);
    
    do {
        
        if (0 == Credits) {
            if (!CreditIRQEnabled) {
                    /* enable credit IRQ */
                enableCreditIrq = TRUE;    
            }
            break;
        }
        
        AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: current credit state, consumed:%d available:%d max:%d seek:%d\n",
                         pProt->CreditsConsumed, 
                         pProt->CreditsAvailable,  
                         pProt->CreditsMax,
                         pProt->CreditsCurrentSeek));
                         
        pProt->CreditsAvailable += Credits;
        A_ASSERT(pProt->CreditsAvailable <= pProt->CreditsMax);
        pProt->CreditsConsumed  -= Credits;
        A_ASSERT(pProt->CreditsConsumed >= 0);
            
        AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: new credit state, consumed:%d available:%d max:%d seek:%d\n",
                         pProt->CreditsConsumed, 
                         pProt->CreditsAvailable,  
                         pProt->CreditsMax,
                         pProt->CreditsCurrentSeek));
        
        if (pProt->CreditsAvailable >= pProt->CreditsCurrentSeek) {
                /* we have enough credits to fullfill at least 1 packet waiting in the queue */
            pProt->CreditsCurrentSeek = 0;
            pProt->SendStateFlags &= ~HCI_SEND_WAIT_CREDITS;  
            doPendingSends = TRUE;  
            if (CreditIRQEnabled) {
                    /* credit IRQ was enabled, we shouldn't need it anymore */
                disableCreditIrq = TRUE;    
            }      
        } else {
                /* not enough credits yet, enable credit IRQ if we haven't already */
            if (!CreditIRQEnabled) {               
                enableCreditIrq = TRUE;    
            }    
        }
                      
    } while (FALSE);
    
    UNLOCK_HCI_TX(pProt);

    if (enableCreditIrq) {
        AR_DEBUG_PRINTF(ATH_DEBUG_RECV,(" Enabling credit count IRQ...\n"));
            /* must use async only */
        status = DevGMboxIRQAction(pProt->pDev, GMBOX_CREDIT_IRQ_ENABLE, PROC_IO_ASYNC);    
    } else if (disableCreditIrq) {
            /* must use async only */
        AR_DEBUG_PRINTF(ATH_DEBUG_RECV,(" Disabling credit count IRQ...\n"));
        status = DevGMboxIRQAction(pProt->pDev, GMBOX_CREDIT_IRQ_DISABLE, PROC_IO_ASYNC); 
    }
       
    if (doPendingSends) {
        HCITrySend(pProt, NULL, FALSE);
    }

    AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("+CreditsAvailableCallback \n"));
    return status;
}

static INLINE void NotifyTransportFailure(GMBOX_PROTO_HCI_UART  *pProt, A_STATUS status)
{
    if (pProt->HCIConfig.TransportFailure != NULL) {
        pProt->HCIConfig.TransportFailure(pProt->HCIConfig.pContext, status);
    }
}

static void FailureCallback(void *pContext, A_STATUS Status)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)pContext; 
    
        /* target assertion occured */           
    NotifyTransportFailure(pProt, Status);  
}

static void StateDumpCallback(void *pContext)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)pContext;
   
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("============ HCIUart State ======================\n"));    
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("RecvStateFlags   :  0x%X \n",pProt->RecvStateFlags));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("SendStateFlags   :  0x%X \n",pProt->SendStateFlags));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("WaitBufferType   :  %d   \n",pProt->WaitBufferType));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("SendQueue Depth  :  %d   \n",HTC_PACKET_QUEUE_DEPTH(&pProt->SendQueue)));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("CreditsMax       :  %d   \n",pProt->CreditsMax));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("CreditsConsumed  :  %d   \n",pProt->CreditsConsumed));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("CreditsAvailable :  %d   \n",pProt->CreditsAvailable));
    AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("==================================================\n"));
}

static A_STATUS HCIUartMessagePending(void *pContext, A_UINT8 LookAheadBytes[], int ValidBytes)
{
    GMBOX_PROTO_HCI_UART        *pProt = (GMBOX_PROTO_HCI_UART *)pContext;
    A_STATUS                    status = A_OK;
    int                         totalRecvLength = 0;
    HCI_TRANSPORT_PACKET_TYPE   pktType = HCI_PACKET_INVALID;
    A_BOOL                      recvRefillCalled = FALSE;
    A_BOOL                      blockRecv = FALSE;
    HTC_PACKET                  *pPacket = NULL;
    
    /** caller guarantees that this is a fully block-able context (synch I/O is allowed) */
    
    AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("+HCIUartMessagePending Lookahead Bytes:%d \n",ValidBytes));
    
    LOCK_HCI_RX(pProt);
        
    do {
    
        if (ValidBytes < 3) {
                /* not enough for ACL or event header */
            break;    
        }    
        
        if ((LookAheadBytes[0] == HCI_UART_ACL_PKT) && (ValidBytes < 5)) {
                /* not enough for ACL data header */
            break;    
        }
                
        switch (LookAheadBytes[0]) {       
            case HCI_UART_EVENT_PKT:
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI Event: %d param length: %d \n",
                        LookAheadBytes[1], LookAheadBytes[2]));
                totalRecvLength = LookAheadBytes[2];
                totalRecvLength += 3; /* add type + event code + length field */
                pktType = HCI_EVENT_TYPE;      
                break;
            case HCI_UART_ACL_PKT:                
                totalRecvLength = (LookAheadBytes[4] << 8) | LookAheadBytes[3];                
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI ACL: conn:0x%X length: %d \n",
                        ((LookAheadBytes[2] & 0xF0) << 8) | LookAheadBytes[1], totalRecvLength));
                totalRecvLength += 5; /* add type + connection handle + length field */
                pktType = HCI_ACL_TYPE;           
                break;        
            default:
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**Invalid HCI packet type: %d \n",LookAheadBytes[0]));
                status = A_EPROTO;
                break;
        }
        
        if (A_FAILED(status)) {
            break;    
        }
                
        if (pProt->HCIConfig.pHCIPktRecvAlloc != NULL) {
            UNLOCK_HCI_RX(pProt);
                /* user is using a per-packet allocation callback */
            pPacket = pProt->HCIConfig.pHCIPktRecvAlloc(pProt->HCIConfig.pContext,
                                                        pktType,
                                                        totalRecvLength);
            LOCK_HCI_RX(pProt);
    
        } else {
            HTC_PACKET_QUEUE *pQueue;
                /* user is using a refill handler that can refill multiple HTC buffers */
            
                /* select buffer queue */
            if (pktType == HCI_ACL_TYPE) {
                pQueue = &pProt->HCIACLRecvBuffers;    
            } else {
                pQueue = &pProt->HCIEventBuffers;              
            }    
            
            if (HTC_QUEUE_EMPTY(pQueue)) {
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,
                    ("** HCI pkt type: %d has no buffers available calling allocation handler \n", 
                    pktType));
                    /* check for refill handler */
                if (pProt->HCIConfig.pHCIPktRecvRefill != NULL) {
                    recvRefillCalled = TRUE;
                    UNLOCK_HCI_RX(pProt);
                        /* call the re-fill handler */
                    pProt->HCIConfig.pHCIPktRecvRefill(pProt->HCIConfig.pContext,
                                                       pktType,
                                                       0);
                    LOCK_HCI_RX(pProt);
                        /* check if we have more buffers */
                    pPacket = HTC_PACKET_DEQUEUE(pQueue);
                        /* fall through */
                }
            } else {
                pPacket = HTC_PACKET_DEQUEUE(pQueue);
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,
                    ("HCI pkt type: %d now has %d recv buffers left \n", 
                            pktType, HTC_PACKET_QUEUE_DEPTH(pQueue)));    
            }
        }
     
        if (NULL == pPacket) {
            AR_DEBUG_PRINTF(ATH_DEBUG_RECV,
                    ("** HCI pkt type: %d has no buffers available stopping recv...\n", pktType));
                /* this is not an error, we simply need to mark that we are waiting for buffers.*/
            pProt->RecvStateFlags |= HCI_RECV_WAIT_BUFFERS;
            pProt->WaitBufferType = pktType;
            blockRecv = TRUE;
            break;
        }
        
        if (totalRecvLength > (int)pPacket->BufferLength) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI-UART pkt: %d requires %d bytes (%d buffer bytes avail) ! \n",
                LookAheadBytes[0], totalRecvLength, pPacket->BufferLength));
            status = A_EINVAL;
            break;
        }
        
    } while (FALSE);
    
    UNLOCK_HCI_RX(pProt);
    
        /* locks are released, we can go fetch the packet */
        
    do {
        
        if (A_FAILED(status) || (NULL == pPacket)) {
            break;    
        } 
        
            /* do this synchronously, we don't need to be fast here */
        pPacket->Completion = NULL;
        
        AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI : getting recv packet len:%d hci-uart-type: %s \n",
                totalRecvLength, (LookAheadBytes[0] == HCI_UART_EVENT_PKT) ? "EVENT" : "ACL"));
                
        status = DevGMboxRead(pProt->pDev, pPacket, totalRecvLength);     
        
        if (A_FAILED(status)) {
            break;    
        }
        
        if (pPacket->pBuffer[0] != LookAheadBytes[0]) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI buffer does not contain expected packet type: %d ! \n",
                        pPacket->pBuffer[0]));
            status = A_EPROTO;
            break;   
        }
        
        if (pPacket->pBuffer[0] == HCI_UART_EVENT_PKT) {
                /* validate event header fields */
            if ((pPacket->pBuffer[1] != LookAheadBytes[1]) ||
                (pPacket->pBuffer[2] != LookAheadBytes[2])) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI buffer does not match lookahead! \n"));
                DebugDumpBytes(LookAheadBytes, 3, "Expected HCI-UART Header");  
                DebugDumpBytes(pPacket->pBuffer, 3, "** Bad HCI-UART Header");  
                status = A_EPROTO;
                break;       
            }   
        } else if (pPacket->pBuffer[0] == HCI_UART_ACL_PKT) {
                /* validate acl header fields */
            if ((pPacket->pBuffer[1] != LookAheadBytes[1]) ||
                (pPacket->pBuffer[2] != LookAheadBytes[2]) ||
                (pPacket->pBuffer[3] != LookAheadBytes[3]) ||
                (pPacket->pBuffer[4] != LookAheadBytes[4])) {
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("** HCI buffer does not match lookahead! \n"));
                DebugDumpBytes(LookAheadBytes, 5, "Expected HCI-UART Header");  
                DebugDumpBytes(pPacket->pBuffer, 5, "** Bad HCI-UART Header");  
                status = A_EPROTO;
                break;       
            }   
        }
        
            /* adjust buffer to move past packet ID */
        pPacket->pBuffer++;
        pPacket->ActualLength = totalRecvLength - 1;
        pPacket->Status = A_OK;
            /* indicate packet */
        DO_HCI_RECV_INDICATION(pProt,pPacket);
        pPacket = NULL;
        
            /* check if we need to refill recv buffers */        
        if ((pProt->HCIConfig.pHCIPktRecvRefill != NULL) && !recvRefillCalled) {           
            HTC_PACKET_QUEUE *pQueue;
            int              watermark;

            if (pktType == HCI_ACL_TYPE) {
                watermark = pProt->HCIConfig.ACLRecvBufferWaterMark;
                pQueue = &pProt->HCIACLRecvBuffers;    
            } else {
                watermark = pProt->HCIConfig.EventRecvBufferWaterMark;     
                pQueue = &pProt->HCIEventBuffers;        
            }    
            
            if (HTC_PACKET_QUEUE_DEPTH(pQueue) < watermark) {
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,
                    ("** HCI pkt type: %d watermark hit (%d) current:%d \n", 
                    pktType, watermark, HTC_PACKET_QUEUE_DEPTH(pQueue)));
                    /* call the re-fill handler */
                pProt->HCIConfig.pHCIPktRecvRefill(pProt->HCIConfig.pContext,
                                                   pktType,
                                                   HTC_PACKET_QUEUE_DEPTH(pQueue));
            }
        }   
        
    } while (FALSE);
        
        /* check if we need to disable the reciever */
    if (A_FAILED(status) || blockRecv) {
        DevGMboxIRQAction(pProt->pDev, GMBOX_RECV_IRQ_DISABLE, PROC_IO_SYNC); 
    }
    
        /* see if we need to recycle the recv buffer */    
    if (A_FAILED(status) && (pPacket != NULL)) {
        HTC_PACKET_QUEUE queue;
        
        if (A_EPROTO == status) {
            DebugDumpBytes(pPacket->pBuffer, totalRecvLength, "Bad HCI-UART Recv packet");    
        }
            /* recycle packet */
        HTC_PACKET_RESET_RX(pPacket);
        INIT_HTC_PACKET_QUEUE_AND_ADD(&queue,pPacket);
        HCI_TransportAddReceivePkts(pProt,&queue);
        NotifyTransportFailure(pProt,status);    
    }
    
 
    AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("-HCIUartMessagePending \n"));
    
    return status;
}

static void HCISendPacketCompletion(void *Context, HTC_PACKET *pPacket)
{
    GMBOX_PROTO_HCI_UART *pProt = (GMBOX_PROTO_HCI_UART *)Context;
    AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("+HCISendPacketCompletion (pPacket:0x%lX) \n",(unsigned long)pPacket));
    
    if (A_FAILED(pPacket->Status)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,(" Send Packet (0x%lX) failed: %d , len:%d \n",
            (unsigned long)pPacket, pPacket->Status, pPacket->ActualLength));        
    }
    
    DO_HCI_SEND_INDICATION(pProt,pPacket);
    
    AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("+HCISendPacketCompletion \n"));
}

static A_STATUS SeekCreditsSynch(GMBOX_PROTO_HCI_UART *pProt)
{
    A_STATUS status = A_OK;
    int      credits;
    int      retry = 100;
    
    while (TRUE) {                
        credits = 0;
        status =  DevGMboxReadCreditCounter(pProt->pDev, PROC_IO_SYNC, &credits);   
        if (A_FAILED(status)) {
            break;    
        }
        LOCK_HCI_TX(pProt);
        pProt->CreditsAvailable += credits;        
        pProt->CreditsConsumed -= credits;        
        if (pProt->CreditsAvailable >= pProt->CreditsCurrentSeek) {
            pProt->CreditsCurrentSeek = 0;
            UNLOCK_HCI_TX(pProt);
            break;    
        }
        UNLOCK_HCI_TX(pProt);
        retry--;
        if (0 == retry) {
            status = A_EBUSY;
            break;    
        }
        A_MDELAY(20);
    }   
    
    return status;
}

static A_STATUS HCITrySend(GMBOX_PROTO_HCI_UART *pProt, HTC_PACKET *pPacket, A_BOOL Synchronous)
{   
    A_STATUS    status = A_OK;
    int         transferLength;
    int         creditsRequired, remainder;
    A_UINT8     hciUartType;
    A_BOOL      synchSendComplete = FALSE;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("+HCITrySend (pPacket:0x%lX) %s \n",(unsigned long)pPacket,
            Synchronous ? "SYNC" :"ASYNC"));
    
    LOCK_HCI_TX(pProt);
     
        /* increment write processing count on entry */    
    pProt->SendProcessCount++;
        
    do {
                                             
        if (pProt->HCIStopped) {
            status = A_ECANCELED;
            break;     
        }   
         
        if (pPacket != NULL) {  
                /* packet was supplied */     
            if (Synchronous) {
                    /* in synchronous mode, the send queue can only hold 1 packet */
                if (!HTC_QUEUE_EMPTY(&pProt->SendQueue)) {
                    status = A_EBUSY;
                    A_ASSERT(FALSE);
                    break;    
                }             
                
                if (pProt->SendProcessCount > 1) {
                        /* another thread or task is draining the TX queues  */
                    status = A_EBUSY;
                    A_ASSERT(FALSE);
                    break;
                } 
                  
                HTC_PACKET_ENQUEUE(&pProt->SendQueue,pPacket);
                
            } else {
                    /* see if adding this packet hits the max depth (asynchronous mode only) */
                if ((pProt->HCIConfig.MaxSendQueueDepth > 0) && 
                    ((HTC_PACKET_QUEUE_DEPTH(&pProt->SendQueue) + 1) >= pProt->HCIConfig.MaxSendQueueDepth)) {
                    AR_DEBUG_PRINTF(ATH_DEBUG_SEND, ("HCI Send queue is full, Depth:%d, Max:%d \n",
                            HTC_PACKET_QUEUE_DEPTH(&pProt->SendQueue), 
                            pProt->HCIConfig.MaxSendQueueDepth));
                        /* queue will be full, invoke any callbacks to determine what action to take */
                    if (pProt->HCIConfig.pHCISendFull != NULL) {
                        AR_DEBUG_PRINTF(ATH_DEBUG_SEND, 
                                    ("HCI : Calling driver's send full callback.... \n"));
                        if (pProt->HCIConfig.pHCISendFull(pProt->HCIConfig.pContext,
                                                          pPacket) == HCI_SEND_FULL_DROP) {
                                /* drop it */
                            status = A_NO_RESOURCE;      
                            break;
                        }
                    }               
                }
          
                HTC_PACKET_ENQUEUE(&pProt->SendQueue,pPacket);
            }

        }
               
        if (pProt->SendStateFlags & HCI_SEND_WAIT_CREDITS) {
            break;   
        }

        if (pProt->SendProcessCount > 1) {
                /* another thread or task is draining the TX queues  */
            break;
        }
    
        /***** beyond this point only 1 thread may enter ******/
           
        /* now drain the send queue for transmission as long as we have enough
         * credits */
        while (!HTC_QUEUE_EMPTY(&pProt->SendQueue)) {
            
            pPacket = HTC_PACKET_DEQUEUE(&pProt->SendQueue);

            switch (HCI_GET_PACKET_TYPE(pPacket)) {            
                case HCI_COMMAND_TYPE:
                    hciUartType = HCI_UART_COMMAND_PKT;
                    break;
                case HCI_ACL_TYPE:
                    hciUartType = HCI_UART_ACL_PKT;
                    break;
                default:
                    status = A_EINVAL;
                    A_ASSERT(FALSE);
                    break;
            }
                       
            if (A_FAILED(status)) {
                break;   
            }
            
            AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: Got head packet:0x%lX , Type:%d  Length: %d Remaining Queue Depth: %d\n",
                (unsigned long)pPacket, HCI_GET_PACKET_TYPE(pPacket), pPacket->ActualLength, 
                HTC_PACKET_QUEUE_DEPTH(&pProt->SendQueue)));
        
            transferLength = 1;  /* UART type header is 1 byte */
            transferLength += pPacket->ActualLength;
            transferLength = DEV_CALC_SEND_PADDED_LEN(pProt->pDev, transferLength);
                   
                /* figure out how many credits this message requires */
            creditsRequired = transferLength / pProt->CreditSize;
            remainder = transferLength % pProt->CreditSize;

            if (remainder) {
                creditsRequired++;
            }

            AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: Creds Required:%d   Got:%d\n",
                            creditsRequired, pProt->CreditsAvailable));
            
            if (creditsRequired > pProt->CreditsAvailable) {
                if (Synchronous) {
                        /* in synchronous mode we need to seek credits in synchronously */
                    pProt->CreditsCurrentSeek = creditsRequired;
                    UNLOCK_HCI_TX(pProt);
                    status = SeekCreditsSynch(pProt);
                    LOCK_HCI_TX(pProt);
                    if (A_FAILED(status)) {
                        break;    
                    }                    
                    /* fall through and continue processing this send op */                    
                } else {
                        /* not enough credits, queue back to the head */
                    HTC_PACKET_ENQUEUE_TO_HEAD(&pProt->SendQueue,pPacket);
                        /* waiting for credits */
                    pProt->SendStateFlags |= HCI_SEND_WAIT_CREDITS;
                        /* provide a hint to reduce attempts to re-send if credits are dribbling back
                         * this hint is the short fall of credits */
                    pProt->CreditsCurrentSeek = creditsRequired;
                    AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: packet:0x%lX placed back in queue. head packet needs: %d credits \n",
                                        (unsigned long)pPacket, pProt->CreditsCurrentSeek));
                    pPacket = NULL;
                    UNLOCK_HCI_TX(pProt);
                    
                        /* schedule a credit counter read, our CreditsAvailableCallback callback will be called
                         * with the result */   
                    DevGMboxReadCreditCounter(pProt->pDev, PROC_IO_ASYNC, NULL);
                             
                    LOCK_HCI_TX(pProt);
                    break;              
                }          
            }
        
                /* caller guarantees some head room */
            pPacket->pBuffer--;
            pPacket->pBuffer[0] = hciUartType;
            
            pProt->CreditsAvailable -= creditsRequired;
            pProt->CreditsConsumed  += creditsRequired;
            A_ASSERT(pProt->CreditsConsumed <= pProt->CreditsMax);
            
            AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("HCI: new credit state: consumed:%d   available:%d max:%d\n",
                             pProt->CreditsConsumed, pProt->CreditsAvailable,  pProt->CreditsMax));
            
            UNLOCK_HCI_TX(pProt);   
            
                /* write it out */   
            if (Synchronous) {                            
                pPacket->Completion = NULL;
                pPacket->pContext = NULL;         
            } else {                       
                pPacket->Completion = HCISendPacketCompletion;
                pPacket->pContext = pProt;    
            }
            
            status = DevGMboxWrite(pProt->pDev,pPacket,transferLength);            
            if (Synchronous) {            
                synchSendComplete = TRUE;
            } else {
                pPacket = NULL;    
            }
            
            LOCK_HCI_TX(pProt);
              
        }
        
    } while (FALSE);
        
    pProt->SendProcessCount--;
    A_ASSERT(pProt->SendProcessCount >= 0);
    UNLOCK_HCI_TX(pProt);
    
    if (Synchronous) {
        A_ASSERT(pPacket != NULL);
        if (A_SUCCESS(status) && (!synchSendComplete)) {
            status = A_EBUSY;
            A_ASSERT(FALSE);
            LOCK_HCI_TX(pProt);
            if (pPacket->ListLink.pNext != NULL) {
                    /* remove from the queue */
                HTC_PACKET_REMOVE(&pProt->SendQueue,pPacket);
            }
            UNLOCK_HCI_TX(pProt);
        }
    } else {   
        if (A_FAILED(status) && (pPacket != NULL)) {
            pPacket->Status = status;
            DO_HCI_SEND_INDICATION(pProt,pPacket); 
        }
    }
        
    AR_DEBUG_PRINTF(ATH_DEBUG_SEND,("-HCITrySend:  \n"));
    return status;    
}

static void FlushSendQueue(GMBOX_PROTO_HCI_UART *pProt)
{
    HTC_PACKET          *pPacket;
    HTC_PACKET_QUEUE    discardQueue;
    
    INIT_HTC_PACKET_QUEUE(&discardQueue);
    
    LOCK_HCI_TX(pProt);
    
    if (!HTC_QUEUE_EMPTY(&pProt->SendQueue)) {
        HTC_PACKET_QUEUE_TRANSFER_TO_TAIL(&discardQueue,&pProt->SendQueue);    
    }
    
    UNLOCK_HCI_TX(pProt);
    
        /* discard packets */
    while (!HTC_QUEUE_EMPTY(&discardQueue)) {
        pPacket = HTC_PACKET_DEQUEUE(&discardQueue);   
        pPacket->Status = A_ECANCELED;
        DO_HCI_SEND_INDICATION(pProt,pPacket);
    }
    
}

static void FlushRecvBuffers(GMBOX_PROTO_HCI_UART *pProt)
{
    HTC_PACKET_QUEUE discardQueue;
    HTC_PACKET *pPacket;
    
    INIT_HTC_PACKET_QUEUE(&discardQueue);
    
    LOCK_HCI_RX(pProt);
        /*transfer list items from ACL and event buffer queues to the discard queue */       
    if (!HTC_QUEUE_EMPTY(&pProt->HCIACLRecvBuffers)) {
        HTC_PACKET_QUEUE_TRANSFER_TO_TAIL(&discardQueue,&pProt->HCIACLRecvBuffers);    
    }
    if (!HTC_QUEUE_EMPTY(&pProt->HCIEventBuffers)) {
        HTC_PACKET_QUEUE_TRANSFER_TO_TAIL(&discardQueue,&pProt->HCIEventBuffers);    
    }
    UNLOCK_HCI_RX(pProt);
    
        /* now empty the discard queue */
    while (!HTC_QUEUE_EMPTY(&discardQueue)) {
        pPacket = HTC_PACKET_DEQUEUE(&discardQueue);      
        pPacket->Status = A_ECANCELED;
        DO_HCI_RECV_INDICATION(pProt,pPacket);
    }
    
}

/*** protocol module install entry point ***/

A_STATUS GMboxProtocolInstall(AR6K_DEVICE *pDev)
{
    A_STATUS                status = A_OK;
    GMBOX_PROTO_HCI_UART    *pProtocol = NULL;
        
    do {
        
        pProtocol = A_MALLOC(sizeof(GMBOX_PROTO_HCI_UART));
        
        if (NULL == pProtocol) {
            status = A_NO_MEMORY;
            break;    
        }
        
        A_MEMZERO(pProtocol, sizeof(*pProtocol));
        pProtocol->pDev = pDev;
        INIT_HTC_PACKET_QUEUE(&pProtocol->SendQueue);
        INIT_HTC_PACKET_QUEUE(&pProtocol->HCIACLRecvBuffers);
        INIT_HTC_PACKET_QUEUE(&pProtocol->HCIEventBuffers);
        A_MUTEX_INIT(&pProtocol->HCIRxLock);
        A_MUTEX_INIT(&pProtocol->HCITxLock);
     
    } while (FALSE);
    
    if (A_SUCCESS(status)) {
        LOCK_AR6K(pDev);
        DEV_GMBOX_SET_PROTOCOL(pDev,
                               HCIUartMessagePending,
                               CreditsAvailableCallback,
                               FailureCallback,
                               StateDumpCallback,
                               pProtocol);
        UNLOCK_AR6K(pDev);
    } else {
        if (pProtocol != NULL) {
            HCIUartCleanup(pProtocol);    
        }    
    }
    
    return status;    
}

/*** protocol module uninstall entry point ***/
void GMboxProtocolUninstall(AR6K_DEVICE *pDev)
{
    GMBOX_PROTO_HCI_UART *pProtocol = (GMBOX_PROTO_HCI_UART *)DEV_GMBOX_GET_PROTOCOL(pDev);
    
    if (pProtocol != NULL) {
        
            /* notify anyone attached */    
        if (pProtocol->HCIAttached) {
            A_ASSERT(pProtocol->HCIConfig.TransportRemoved != NULL);
            pProtocol->HCIConfig.TransportRemoved(pProtocol->HCIConfig.pContext);
            pProtocol->HCIAttached = FALSE;    
        }
        
        HCIUartCleanup(pProtocol);
        DEV_GMBOX_SET_PROTOCOL(pDev,NULL,NULL,NULL,NULL,NULL);       
    }
    
}

static A_STATUS NotifyTransportReady(GMBOX_PROTO_HCI_UART  *pProt)
{
    HCI_TRANSPORT_PROPERTIES props;
    A_STATUS                 status = A_OK;
    
    do {
        
        A_MEMZERO(&props,sizeof(props));
        
            /* HCI UART only needs one extra byte at the head to indicate the packet TYPE */
        props.HeadRoom = 1;
        props.TailRoom = 0;
        props.IOBlockPad = pProt->pDev->BlockSize;
        if (pProt->HCIAttached) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ANY,("HCI: notifying attached client to transport... \n"));    
            A_ASSERT(pProt->HCIConfig.TransportReady != NULL);
            status = pProt->HCIConfig.TransportReady(pProt,
                                                    &props,
                                                    pProt->HCIConfig.pContext);
        }
        
    } while (FALSE);
    
    return status;
}

/***********  HCI UART protocol implementation ************************************************/

HCI_TRANSPORT_HANDLE HCI_TransportAttach(void *HTCHandle, HCI_TRANSPORT_CONFIG_INFO *pInfo)
{
    GMBOX_PROTO_HCI_UART  *pProtocol = NULL; 
    AR6K_DEVICE           *pDev;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("+HCI_TransportAttach \n"));
    
    pDev = HTCGetAR6KDevice(HTCHandle);
    
    LOCK_AR6K(pDev);
    
    do {
        
        pProtocol = (GMBOX_PROTO_HCI_UART *)DEV_GMBOX_GET_PROTOCOL(pDev);
        
        if (NULL == pProtocol) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("GMBOX protocol not installed! \n"));
            break;
        }
        
        if (pProtocol->HCIAttached) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("GMBOX protocol already attached! \n"));
            break;    
        }
        
        A_MEMCPY(&pProtocol->HCIConfig, pInfo, sizeof(HCI_TRANSPORT_CONFIG_INFO));
        
        A_ASSERT(pProtocol->HCIConfig.pHCIPktRecv != NULL);
        A_ASSERT(pProtocol->HCIConfig.pHCISendComplete != NULL);
        
        pProtocol->HCIAttached = TRUE;
        
    } while (FALSE);
    
    UNLOCK_AR6K(pDev);
    
    if (pProtocol != NULL) {
            /* TODO ... should we use a worker? */
        NotifyTransportReady(pProtocol);
    }
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("-HCI_TransportAttach (0x%lX) \n",(unsigned long)pProtocol));
    return (HCI_TRANSPORT_HANDLE)pProtocol;
}

void HCI_TransportDetach(HCI_TRANSPORT_HANDLE HciTrans)
{
    GMBOX_PROTO_HCI_UART  *pProtocol = (GMBOX_PROTO_HCI_UART *)HciTrans; 
    AR6K_DEVICE           *pDev = pProtocol->pDev;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("+HCI_TransportDetach \n"));
    
    LOCK_AR6K(pDev);
    if (!pProtocol->HCIAttached) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("GMBOX protocol not attached! \n"));
        UNLOCK_AR6K(pDev);
        return;
    }
    pProtocol->HCIAttached = FALSE;
    UNLOCK_AR6K(pDev);
    
    HCI_TransportStop(HciTrans);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("-HCI_TransportAttach \n"));
}

A_STATUS HCI_TransportAddReceivePkts(HCI_TRANSPORT_HANDLE HciTrans, HTC_PACKET_QUEUE *pQueue)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans; 
    A_STATUS              status = A_OK;
    A_BOOL                unblockRecv = FALSE;
    HTC_PACKET            *pPacket;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("+HCI_TransportAddReceivePkt \n"));
    
    LOCK_HCI_RX(pProt);
   
    do {
         
        if (pProt->HCIStopped) {
            status = A_ECANCELED;
            break;    
        }
        
        pPacket = HTC_GET_PKT_AT_HEAD(pQueue);
        
        if (NULL == pPacket) {
            status = A_EINVAL;
            break;    
        }
        
        AR_DEBUG_PRINTF(ATH_DEBUG_RECV,(" HCI recv packet added, type :%d, len:%d num:%d \n",
                        HCI_GET_PACKET_TYPE(pPacket), pPacket->BufferLength, HTC_PACKET_QUEUE_DEPTH(pQueue)));
                        
        if (HCI_GET_PACKET_TYPE(pPacket) == HCI_EVENT_TYPE) {
            HTC_PACKET_QUEUE_TRANSFER_TO_TAIL(&pProt->HCIEventBuffers, pQueue);
        } else if (HCI_GET_PACKET_TYPE(pPacket) == HCI_ACL_TYPE) {
            HTC_PACKET_QUEUE_TRANSFER_TO_TAIL(&pProt->HCIACLRecvBuffers, pQueue);    
        } else {
            status = A_EINVAL;
            break;    
        }
        
        if (pProt->RecvStateFlags & HCI_RECV_WAIT_BUFFERS) {
            if (pProt->WaitBufferType == HCI_GET_PACKET_TYPE(pPacket)) {
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,(" HCI recv was blocked on packet type :%d, unblocking.. \n",
                        pProt->WaitBufferType));
                pProt->RecvStateFlags &= ~HCI_RECV_WAIT_BUFFERS;
                pProt->WaitBufferType = HCI_PACKET_INVALID;
                unblockRecv = TRUE;
            }
        }
        
    } while (FALSE);
    
    UNLOCK_HCI_RX(pProt);
    
    if (A_FAILED(status)) {
        while (!HTC_QUEUE_EMPTY(pQueue)) {
            pPacket = HTC_PACKET_DEQUEUE(pQueue);      
            pPacket->Status = A_ECANCELED;
            DO_HCI_RECV_INDICATION(pProt,pPacket);
        }   
    }
    
    if (unblockRecv) {
        DevGMboxIRQAction(pProt->pDev, GMBOX_RECV_IRQ_ENABLE, PROC_IO_ASYNC);
    }
    
    AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("-HCI_TransportAddReceivePkt \n"));
    
    return A_OK;    
}

A_STATUS HCI_TransportSendPkt(HCI_TRANSPORT_HANDLE HciTrans, HTC_PACKET *pPacket, A_BOOL Synchronous)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans;  
    
    return HCITrySend(pProt,pPacket,Synchronous);
}

void HCI_TransportStop(HCI_TRANSPORT_HANDLE HciTrans)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans; 
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("+HCI_TransportStop \n"));
     
    LOCK_AR6K(pProt->pDev);
    if (pProt->HCIStopped) {
        UNLOCK_AR6K(pProt->pDev);
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("-HCI_TransportStop \n"));
        return;    
    }
    pProt->HCIStopped = TRUE;
    UNLOCK_AR6K(pProt->pDev);
     
        /* disable interrupts */
    DevGMboxIRQAction(pProt->pDev, GMBOX_DISABLE_ALL, PROC_IO_SYNC);
    FlushSendQueue(pProt);
    FlushRecvBuffers(pProt);
    
        /* signal bridge side to power down BT */
    DevGMboxSetTargetInterrupt(pProt->pDev, MBOX_SIG_HCI_BRIDGE_BT_OFF, BTOFF_TIMEOUT_MS);
           
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("-HCI_TransportStop \n"));
}

A_STATUS HCI_TransportStart(HCI_TRANSPORT_HANDLE HciTrans)
{
    A_STATUS              status;
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("+HCI_TransportStart \n"));
    
        /* set stopped in case we have a problem in starting */
    pProt->HCIStopped = TRUE;
    
    do {
        
        status = InitTxCreditState(pProt);   
        
        if (A_FAILED(status)) {
            break;    
        }     
        
        status = DevGMboxIRQAction(pProt->pDev, GMBOX_ERRORS_IRQ_ENABLE, PROC_IO_SYNC);   
        
        if (A_FAILED(status)) {
            break;   
        } 
            /* enable recv */   
        status = DevGMboxIRQAction(pProt->pDev, GMBOX_RECV_IRQ_ENABLE, PROC_IO_SYNC);
        
        if (A_FAILED(status)) {
            break;   
        } 
            /* signal bridge side to power up BT */
        status = DevGMboxSetTargetInterrupt(pProt->pDev, MBOX_SIG_HCI_BRIDGE_BT_ON, BTON_TIMEOUT_MS);
        
        if (A_FAILED(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HCI_TransportStart : Failed to trigger BT ON \n"));
            break;   
        } 
        
            /* we made it */
        pProt->HCIStopped = FALSE;
        
    } while (FALSE);
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC,("-HCI_TransportStart \n"));
    
    return status;
}

A_STATUS HCI_TransportEnableDisableAsyncRecv(HCI_TRANSPORT_HANDLE HciTrans, A_BOOL Enable)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans;
    return DevGMboxIRQAction(pProt->pDev, 
                             Enable ? GMBOX_RECV_IRQ_ENABLE : GMBOX_RECV_IRQ_DISABLE, 
                             PROC_IO_SYNC);
                             
}

A_STATUS HCI_TransportRecvHCIEventSync(HCI_TRANSPORT_HANDLE HciTrans,
                                       HTC_PACKET           *pPacket,
                                       int                  MaxPollMS)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans;
    A_STATUS              status = A_OK;
    A_UINT8               lookAhead[8];
    int                   bytes;
    int                   totalRecvLength;
    
    MaxPollMS = MaxPollMS / 16;
    
    if (MaxPollMS < 2) {
        MaxPollMS = 2;    
    }
    
    while (MaxPollMS) {
        
        bytes = sizeof(lookAhead);
        status = DevGMboxRecvLookAheadPeek(pProt->pDev,lookAhead,&bytes);
        if (A_FAILED(status)) {
            break;    
        }        
                
        if (bytes < 3) {
            AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI recv poll got bytes: %d, retry : %d \n",
                        bytes, MaxPollMS));
            A_MDELAY(16);
            MaxPollMS--;        
            continue;
        }
        
        totalRecvLength = 0;
        switch (lookAhead[0]) {       
            case HCI_UART_EVENT_PKT:
                AR_DEBUG_PRINTF(ATH_DEBUG_RECV,("HCI Event: %d param length: %d \n",
                        lookAhead[1], lookAhead[2]));
                totalRecvLength = lookAhead[2];
                totalRecvLength += 3; /* add type + event code + length field */
                break;
            default:
                AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**Invalid HCI packet type: %d \n",lookAhead[0]));
                status = A_EPROTO;
                break;
        }
        
        if (A_FAILED(status)) {
            break;    
        }
        
        pPacket->Completion = NULL;
        status = DevGMboxRead(pProt->pDev,pPacket,totalRecvLength); 
        if (A_FAILED(status)) {
            break;    
        }
        
        pPacket->pBuffer++;
        pPacket->ActualLength = totalRecvLength - 1;
        pPacket->Status = A_OK;        
        break; 
    }
    
    if (MaxPollMS == 0) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HCI recv poll timeout! \n"));
        status = A_ERROR;    
    }
    
    return status;
}

#define LSB_SCRATCH_IDX     4
#define MSB_SCRATCH_IDX     5
A_STATUS HCI_TransportSetBaudRate(HCI_TRANSPORT_HANDLE HciTrans, A_UINT32 Baud)
{
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans;
    HIF_DEVICE *pHIFDevice = (HIF_DEVICE *)(pProt->pDev->HIFDevice);
    A_UINT32 scaledBaud, scratchAddr;
    A_STATUS status = A_OK;

    /* Divide the desired baud rate by 100
     * Store the LSB in the local scratch register 4 and the MSB in the local
     * scratch register 5 for the target to read
     */
    scratchAddr = MBOX_BASE_ADDRESS | (LOCAL_SCRATCH_ADDRESS + 4 * LSB_SCRATCH_IDX);
    scaledBaud = (Baud / 100) & LOCAL_SCRATCH_VALUE_MASK;
    status = ar6000_WriteRegDiag(pHIFDevice, &scratchAddr, &scaledBaud);                     
    scratchAddr = MBOX_BASE_ADDRESS | (LOCAL_SCRATCH_ADDRESS + 4 * MSB_SCRATCH_IDX);
    scaledBaud = ((Baud / 100) >> (LOCAL_SCRATCH_VALUE_MSB+1)) & LOCAL_SCRATCH_VALUE_MASK;
    status |= ar6000_WriteRegDiag(pHIFDevice, &scratchAddr, &scaledBaud);                     
    if (A_OK != status) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Failed to set up baud rate in scratch register!"));            
        return status;
    }

    /* Now interrupt the target to tell it about the baud rate */
    status = DevGMboxSetTargetInterrupt(pProt->pDev, MBOX_SIG_HCI_BRIDGE_BAUD_SET, BAUD_TIMEOUT_MS);
    if (A_OK != status) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Failed to tell target to change baud rate!"));            
    }
    
    return status;
}

A_STATUS HCI_TransportEnablePowerMgmt(HCI_TRANSPORT_HANDLE HciTrans, A_BOOL Enable)
{
    A_STATUS status;
    GMBOX_PROTO_HCI_UART  *pProt = (GMBOX_PROTO_HCI_UART *)HciTrans;
                             
    if (Enable) {
        status = DevGMboxSetTargetInterrupt(pProt->pDev, MBOX_SIG_HCI_BRIDGE_PWR_SAV_ON, BTPWRSAV_TIMEOUT_MS);
    } else {
        status = DevGMboxSetTargetInterrupt(pProt->pDev, MBOX_SIG_HCI_BRIDGE_PWR_SAV_OFF, BTPWRSAV_TIMEOUT_MS);
    }

    if (A_FAILED(status)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("Failed to enable/disable HCI power management!\n"));
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HCI power management enabled/disabled!\n"));
    }

    return status;
}

#endif  //ATH_AR6K_ENABLE_GMBOX

