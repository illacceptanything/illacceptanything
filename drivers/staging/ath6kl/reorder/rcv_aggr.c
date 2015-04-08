/*
 *
 * Copyright (c) 2010 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
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
 *
 */

#ifdef  ATH_AR6K_11N_SUPPORT

#include <a_config.h>
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <a_debug.h>
#include "pkt_log.h"
#include "aggr_recv_api.h"
#include "aggr_rx_internal.h"
#include "wmi.h"

extern A_STATUS
wmi_dot3_2_dix(void *osbuf);

static void
aggr_slice_amsdu(AGGR_INFO *p_aggr, RXTID *rxtid, void **osbuf);

static void
aggr_timeout(A_ATH_TIMER arg);

static void
aggr_deque_frms(AGGR_INFO *p_aggr, A_UINT8 tid, A_UINT16 seq_no, A_UINT8 order);

static void
aggr_dispatch_frames(AGGR_INFO *p_aggr, A_NETBUF_QUEUE_T *q);

static void *
aggr_get_osbuf(AGGR_INFO *p_aggr);

void *
aggr_init(ALLOC_NETBUFS netbuf_allocator)
{
    AGGR_INFO   *p_aggr = NULL;
    RXTID *rxtid;
    A_UINT8 i;
    A_STATUS status = A_OK;

    A_PRINTF("In aggr_init..\n");

    do {
        p_aggr = A_MALLOC(sizeof(AGGR_INFO));
        if(!p_aggr) {
            A_PRINTF("Failed to allocate memory for aggr_node\n");
            status = A_ERROR;
            break;
        }

        /* Init timer and data structures */
        A_MEMZERO(p_aggr, sizeof(AGGR_INFO));
        p_aggr->aggr_sz = AGGR_SZ_DEFAULT;
        A_INIT_TIMER(&p_aggr->timer, aggr_timeout, p_aggr);
        p_aggr->timerScheduled = FALSE;
        A_NETBUF_QUEUE_INIT(&p_aggr->freeQ);

        p_aggr->netbuf_allocator = netbuf_allocator;
        p_aggr->netbuf_allocator(&p_aggr->freeQ, AGGR_NUM_OF_FREE_NETBUFS);

        for(i = 0; i < NUM_OF_TIDS; i++) {
            rxtid = AGGR_GET_RXTID(p_aggr, i);
            rxtid->aggr = FALSE;
            rxtid->progress = FALSE;
            rxtid->timerMon = FALSE;
            A_NETBUF_QUEUE_INIT(&rxtid->q);
            A_MUTEX_INIT(&rxtid->lock);
        }
    }while(FALSE);

    A_PRINTF("going out of aggr_init..status %s\n",
                    (status == A_OK) ? "OK":"Error");

    if(status != A_OK) {
        /* Cleanup */
        aggr_module_destroy(p_aggr);
    }
    return ((status == A_OK) ? p_aggr : NULL);
}

/* utility function to clear rx hold_q for a tid */
static void
aggr_delete_tid_state(AGGR_INFO *p_aggr, A_UINT8 tid)
{
    RXTID *rxtid;
    RXTID_STATS *stats;

    A_ASSERT(tid < NUM_OF_TIDS && p_aggr);

    rxtid = AGGR_GET_RXTID(p_aggr, tid);
    stats = AGGR_GET_RXTID_STATS(p_aggr, tid);

    if(rxtid->aggr) {
        aggr_deque_frms(p_aggr, tid, 0, ALL_SEQNO);
    }

    rxtid->aggr = FALSE;
    rxtid->progress = FALSE;
    rxtid->timerMon = FALSE;
    rxtid->win_sz = 0;
    rxtid->seq_next = 0;
    rxtid->hold_q_sz = 0;

    if(rxtid->hold_q) {
        A_FREE(rxtid->hold_q);
        rxtid->hold_q = NULL;
    }

    A_MEMZERO(stats, sizeof(RXTID_STATS));
}

void
aggr_module_destroy(void *cntxt)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;
    RXTID *rxtid;
    A_UINT8 i, k;
    A_PRINTF("%s(): aggr = %p\n",_A_FUNCNAME_, p_aggr);
    A_ASSERT(p_aggr);

    if(p_aggr) {
        if(p_aggr->timerScheduled) {
            A_UNTIMEOUT(&p_aggr->timer);
            p_aggr->timerScheduled = FALSE;
        }

        for(i = 0; i < NUM_OF_TIDS; i++) {
            rxtid = AGGR_GET_RXTID(p_aggr, i);
            /* Free the hold q contents and hold_q*/
            if(rxtid->hold_q) {
                for(k = 0; k< rxtid->hold_q_sz; k++) {
                    if(rxtid->hold_q[k].osbuf) {
                        A_NETBUF_FREE(rxtid->hold_q[k].osbuf);
                    }
                }
                A_FREE(rxtid->hold_q);
            }
            /* Free the dispatch q contents*/
            while(A_NETBUF_QUEUE_SIZE(&rxtid->q)) {
                A_NETBUF_FREE(A_NETBUF_DEQUEUE(&rxtid->q));
            }
            if (A_IS_MUTEX_VALID(&rxtid->lock)) {
                A_MUTEX_DELETE(&rxtid->lock);
            }
        }
        /* free the freeQ and its contents*/
        while(A_NETBUF_QUEUE_SIZE(&p_aggr->freeQ)) {
            A_NETBUF_FREE(A_NETBUF_DEQUEUE(&p_aggr->freeQ));
        }
        A_FREE(p_aggr);
    }
    A_PRINTF("out aggr_module_destroy\n");
}


void
aggr_register_rx_dispatcher(void *cntxt, void * dev, RX_CALLBACK fn)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;

    A_ASSERT(p_aggr && fn && dev);

    p_aggr->rx_fn = fn;
    p_aggr->dev = dev;
}


void
aggr_process_bar(void *cntxt, A_UINT8 tid, A_UINT16 seq_no)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;
    RXTID_STATS *stats;

    A_ASSERT(p_aggr);
    stats = AGGR_GET_RXTID_STATS(p_aggr, tid);
    stats->num_bar++;

    aggr_deque_frms(p_aggr, tid, seq_no, ALL_SEQNO);
}


void
aggr_recv_addba_req_evt(void *cntxt, A_UINT8 tid, A_UINT16 seq_no, A_UINT8 win_sz)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;
    RXTID *rxtid;
    RXTID_STATS *stats;

    A_ASSERT(p_aggr);
    rxtid = AGGR_GET_RXTID(p_aggr, tid);
    stats = AGGR_GET_RXTID_STATS(p_aggr, tid);

    A_PRINTF("%s(): win_sz = %d aggr %d\n", _A_FUNCNAME_, win_sz, rxtid->aggr);
    if(win_sz < AGGR_WIN_SZ_MIN || win_sz > AGGR_WIN_SZ_MAX) {
        A_PRINTF("win_sz %d, tid %d\n", win_sz, tid);
    }

    if(rxtid->aggr) {
        /* Just go and  deliver all the frames up from this
         * queue, as if we got DELBA and re-initialize the queue
         */
        aggr_delete_tid_state(p_aggr, tid);
    }

    rxtid->seq_next = seq_no;
    /* create these queues, only upon receiving of ADDBA for a
     * tid, reducing memory requirement
     */
    rxtid->hold_q = A_MALLOC(HOLD_Q_SZ(win_sz));
    if((rxtid->hold_q == NULL)) {
        A_PRINTF("Failed to allocate memory, tid = %d\n", tid);
        A_ASSERT(0);
    }
    A_MEMZERO(rxtid->hold_q, HOLD_Q_SZ(win_sz));

    /* Update rxtid for the window sz */
    rxtid->win_sz = win_sz;
    /* hold_q_sz inicates the depth of holding q - which  is
     * a factor of win_sz. Compute once, as it will be used often
     */
    rxtid->hold_q_sz = TID_WINDOW_SZ(win_sz);
    /* There should be no frames on q - even when second ADDBA comes in.
     * If aggr was previously ON on this tid, we would have cleaned up
     * the q
     */
    if(A_NETBUF_QUEUE_SIZE(&rxtid->q) != 0) {
        A_PRINTF("ERROR: Frames still on queue ?\n");
        A_ASSERT(0);
    }

    rxtid->aggr = TRUE;
}

void
aggr_recv_delba_req_evt(void *cntxt, A_UINT8 tid)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;
    RXTID *rxtid;

    A_ASSERT(p_aggr);
    A_PRINTF("%s(): tid %d\n", _A_FUNCNAME_, tid);

    rxtid = AGGR_GET_RXTID(p_aggr, tid);

    if(rxtid->aggr) {
        aggr_delete_tid_state(p_aggr, tid);
    }
}

static void
aggr_deque_frms(AGGR_INFO *p_aggr, A_UINT8 tid, A_UINT16 seq_no, A_UINT8 order)
{
    RXTID *rxtid;
    OSBUF_HOLD_Q *node;
    A_UINT16 idx, idx_end, seq_end;
    RXTID_STATS *stats;

    A_ASSERT(p_aggr);
    rxtid = AGGR_GET_RXTID(p_aggr, tid);
    stats = AGGR_GET_RXTID_STATS(p_aggr, tid);

    /* idx is absolute location for first frame */
    idx = AGGR_WIN_IDX(rxtid->seq_next, rxtid->hold_q_sz);

    /* idx_end is typically the last possible frame in the window,
     * but changes to 'the' seq_no, when BAR comes. If seq_no
     * is non-zero, we will go up to that and stop.
     * Note: last seq no in current window will occupy the same
     * index position as index that is just previous to start.
     * An imp point : if win_sz is 7, for seq_no space of 4095,
     * then, there would be holes when sequence wrap around occurs.
     * Target should judiciously choose the win_sz, based on
     * this condition. For 4095, (TID_WINDOW_SZ = 2 x win_sz
     * 2, 4, 8, 16 win_sz works fine).
     * We must deque from "idx" to "idx_end", including both.
     */
    seq_end = (seq_no) ? seq_no : rxtid->seq_next;
    idx_end = AGGR_WIN_IDX(seq_end, rxtid->hold_q_sz);

    /* Critical section begins */
    A_MUTEX_LOCK(&rxtid->lock);
    do {

        node = &rxtid->hold_q[idx];

        if((order == CONTIGUOUS_SEQNO) && (!node->osbuf))
            break;

        /* chain frames and deliver frames bcos:
         *  1. either the frames are in order and window is contiguous, OR
         *  2. we need to deque frames, irrespective of holes
         */
        if(node->osbuf) {
            if(node->is_amsdu) {
                aggr_slice_amsdu(p_aggr, rxtid, &node->osbuf);
            } else {
                A_NETBUF_ENQUEUE(&rxtid->q, node->osbuf);
            }
            node->osbuf = NULL;
        } else {
            stats->num_hole++;
        }

        /* window is moving */
        rxtid->seq_next = IEEE80211_NEXT_SEQ_NO(rxtid->seq_next);
        idx = AGGR_WIN_IDX(rxtid->seq_next, rxtid->hold_q_sz);
    } while(idx != idx_end);
    /* Critical section ends */
    A_MUTEX_UNLOCK(&rxtid->lock);

    stats->num_delivered += A_NETBUF_QUEUE_SIZE(&rxtid->q);
    aggr_dispatch_frames(p_aggr, &rxtid->q);
}

static void *
aggr_get_osbuf(AGGR_INFO *p_aggr)
{
    void *buf = NULL;

    /* Starving for buffers?  get more from OS
     *  check for low netbuffers( < 1/4 AGGR_NUM_OF_FREE_NETBUFS) :
     *      re-allocate bufs if so
     * allocate a free buf from freeQ
     */
    if (A_NETBUF_QUEUE_SIZE(&p_aggr->freeQ) < (AGGR_NUM_OF_FREE_NETBUFS >> 2)) {
        p_aggr->netbuf_allocator(&p_aggr->freeQ, AGGR_NUM_OF_FREE_NETBUFS);
    }

    if (A_NETBUF_QUEUE_SIZE(&p_aggr->freeQ)) {
        buf = A_NETBUF_DEQUEUE(&p_aggr->freeQ);
    }

    return buf;
}


static void
aggr_slice_amsdu(AGGR_INFO *p_aggr, RXTID *rxtid, void **osbuf)
{
    void *new_buf;
    A_UINT16 frame_8023_len, payload_8023_len, mac_hdr_len, amsdu_len;
    A_UINT8 *framep;

    /* Frame format at this point:
     *  [DIX hdr | 802.3 | 802.3 | ... | 802.3]
     *
     * Strip the DIX header.
     * Iterate through the osbuf and do:
     *  grab a free netbuf from freeQ
     *  find the start and end of a frame
     *  copy it to netbuf(Vista can do better here)
     *  convert all msdu's(802.3) frames to upper layer format - os routine
     *      -for now lets convert from 802.3 to dix
     *  enque this to dispatch q of tid
     * repeat
     * free the osbuf - to OS. It's been sliced.
     */

    mac_hdr_len = sizeof(ATH_MAC_HDR);
    framep = A_NETBUF_DATA(*osbuf) + mac_hdr_len;
    amsdu_len = A_NETBUF_LEN(*osbuf) - mac_hdr_len;

    while(amsdu_len > mac_hdr_len) {
        /* Begin of a 802.3 frame */
        payload_8023_len = A_BE2CPU16(((ATH_MAC_HDR *)framep)->typeOrLen);
#define MAX_MSDU_SUBFRAME_PAYLOAD_LEN 1508
#define MIN_MSDU_SUBFRAME_PAYLOAD_LEN 46
        if(payload_8023_len < MIN_MSDU_SUBFRAME_PAYLOAD_LEN || payload_8023_len > MAX_MSDU_SUBFRAME_PAYLOAD_LEN) {
            A_PRINTF("802.3 AMSDU frame bound check failed. len %d\n", payload_8023_len);
            break;
        }
        frame_8023_len = payload_8023_len + mac_hdr_len;
        new_buf = aggr_get_osbuf(p_aggr);
        if(new_buf == NULL) {
            A_PRINTF("No buffer available \n");
            break;
        }

        A_MEMCPY(A_NETBUF_DATA(new_buf), framep, frame_8023_len);
        A_NETBUF_PUT(new_buf, frame_8023_len);
        if (wmi_dot3_2_dix(new_buf) != A_OK) {
            A_PRINTF("dot3_2_dix err..\n");
            A_NETBUF_FREE(new_buf);
            break;
        }

        A_NETBUF_ENQUEUE(&rxtid->q, new_buf);

        /* Is this the last subframe within this aggregate ? */
        if ((amsdu_len - frame_8023_len) == 0) {
            break;
        }

        /* Add the length of A-MSDU subframe padding bytes -
         * Round to nearest word.
         */
        frame_8023_len = ((frame_8023_len + 3) & ~3);

        framep += frame_8023_len;
        amsdu_len -= frame_8023_len;
    }

    A_NETBUF_FREE(*osbuf);
    *osbuf = NULL;
}

void
aggr_process_recv_frm(void *cntxt, A_UINT8 tid, A_UINT16 seq_no, A_BOOL is_amsdu, void **osbuf)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;
    RXTID *rxtid;
    RXTID_STATS *stats;
    A_UINT16 idx, st, cur, end;
    A_UINT16 *log_idx;
    OSBUF_HOLD_Q *node;
    PACKET_LOG *log;

    A_ASSERT(p_aggr);
    A_ASSERT(tid < NUM_OF_TIDS);

    rxtid = AGGR_GET_RXTID(p_aggr, tid);
    stats = AGGR_GET_RXTID_STATS(p_aggr, tid);

    stats->num_into_aggr++;

    if(!rxtid->aggr) {
        if(is_amsdu) {
            aggr_slice_amsdu(p_aggr, rxtid, osbuf);
            stats->num_amsdu++;
            aggr_dispatch_frames(p_aggr, &rxtid->q);
        }
        return;
    }

    /* Check the incoming sequence no, if it's in the window */
    st = rxtid->seq_next;
    cur = seq_no;
    end = (st + rxtid->hold_q_sz-1) & IEEE80211_MAX_SEQ_NO;
    /* Log the pkt info for future analysis */
    log = &p_aggr->pkt_log;
    log_idx = &log->last_idx;
    log->info[*log_idx].cur = cur;
    log->info[*log_idx].st = st;
    log->info[*log_idx].end = end;
    *log_idx = IEEE80211_NEXT_SEQ_NO(*log_idx);

    if(((st < end) && (cur < st || cur > end)) ||
      ((st > end) && (cur > end) && (cur < st))) {
        /* the cur frame is outside the window. Since we know
         * our target would not do this without reason it must
         * be assumed that the window has moved for some valid reason.
         * Therefore, we dequeue all frames and start fresh.
         */
        A_UINT16 extended_end;

        extended_end = (end + rxtid->hold_q_sz-1) & IEEE80211_MAX_SEQ_NO;

        if(((end < extended_end) && (cur < end || cur > extended_end)) ||
           ((end > extended_end) && (cur > extended_end) && (cur < end))) {
            // dequeue all frames in queue and shift window to new frame
            aggr_deque_frms(p_aggr, tid, 0, ALL_SEQNO);
            //set window start so that new frame is last frame in window
            if(cur >= rxtid->hold_q_sz-1) {
                rxtid->seq_next = cur - (rxtid->hold_q_sz-1);
            }else{
                rxtid->seq_next = IEEE80211_MAX_SEQ_NO - (rxtid->hold_q_sz-2 - cur);
            }
        } else {
            // dequeue only those frames that are outside the new shifted window
            if(cur >= rxtid->hold_q_sz-1) {
                st = cur - (rxtid->hold_q_sz-1);
            }else{
                st = IEEE80211_MAX_SEQ_NO - (rxtid->hold_q_sz-2 - cur);
            }

            aggr_deque_frms(p_aggr, tid, st, ALL_SEQNO);
        }

        stats->num_oow++;
    }

    idx = AGGR_WIN_IDX(seq_no, rxtid->hold_q_sz);

    /*enque the frame, in hold_q */
    node = &rxtid->hold_q[idx];

    A_MUTEX_LOCK(&rxtid->lock);
    if(node->osbuf) {
        /* Is the cur frame duplicate or something beyond our
         * window(hold_q -> which is 2x, already)?
         * 1. Duplicate is easy - drop incoming frame.
         * 2. Not falling in current sliding window.
         *  2a. is the frame_seq_no preceding current tid_seq_no?
         *      -> drop the frame. perhaps sender did not get our ACK.
         *         this is taken care of above.
         *  2b. is the frame_seq_no beyond window(st, TID_WINDOW_SZ);
         *      -> Taken care of it above, by moving window forward.
         *
         */
        A_NETBUF_FREE(node->osbuf);
        stats->num_dups++;
    }

    node->osbuf = *osbuf;
    node->is_amsdu = is_amsdu;
    node->seq_no = seq_no;
    if(node->is_amsdu) {
        stats->num_amsdu++;
    } else {
        stats->num_mpdu++;
    }
    A_MUTEX_UNLOCK(&rxtid->lock);

    *osbuf = NULL;
    aggr_deque_frms(p_aggr, tid, 0, CONTIGUOUS_SEQNO);

    if(p_aggr->timerScheduled) {
        rxtid->progress = TRUE;
    }else{
        for(idx=0 ; idx<rxtid->hold_q_sz ; idx++) {
            if(rxtid->hold_q[idx].osbuf) {
                /* there is a frame in the queue and no timer so
                 * start a timer to ensure that the frame doesn't remain
                 * stuck forever. */
                p_aggr->timerScheduled = TRUE;
                A_TIMEOUT_MS(&p_aggr->timer, AGGR_RX_TIMEOUT, 0);
                rxtid->progress = FALSE;
                rxtid->timerMon = TRUE;
                break;
            }
        }
    }
}

/*
 * aggr_reset_state -- Called when it is deemed necessary to clear the aggregate
 *  hold Q state.  Examples include when a Connect event or disconnect event is
 *  received.
 */
void
aggr_reset_state(void *cntxt)
{
    A_UINT8 tid;
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;

    A_ASSERT(p_aggr);

    for(tid=0 ; tid<NUM_OF_TIDS ; tid++) {
        aggr_delete_tid_state(p_aggr, tid);
    }
}


static void
aggr_timeout(A_ATH_TIMER arg)
{
    A_UINT8 i,j;
    AGGR_INFO *p_aggr = (AGGR_INFO *)arg;
    RXTID   *rxtid;
    RXTID_STATS *stats;
    /*
     * If the q for which the timer was originally started has
     * not progressed then it is necessary to dequeue all the
     * contained frames so that they are not held forever.
     */
    for(i = 0; i < NUM_OF_TIDS; i++) {
        rxtid = AGGR_GET_RXTID(p_aggr, i);
        stats = AGGR_GET_RXTID_STATS(p_aggr, i);

        if(rxtid->aggr == FALSE ||
           rxtid->timerMon == FALSE ||
           rxtid->progress == TRUE) {
            continue;
        }
        // dequeue all frames in for this tid
        stats->num_timeouts++;
        A_PRINTF("TO: st %d end %d\n", rxtid->seq_next, ((rxtid->seq_next + rxtid->hold_q_sz-1) & IEEE80211_MAX_SEQ_NO));
        aggr_deque_frms(p_aggr, i, 0, ALL_SEQNO);
    }

    p_aggr->timerScheduled = FALSE;
    // determine whether a new timer should be started.
    for(i = 0; i < NUM_OF_TIDS; i++) {
        rxtid = AGGR_GET_RXTID(p_aggr, i);

        if(rxtid->aggr == TRUE && rxtid->hold_q) {
            for(j = 0 ; j < rxtid->hold_q_sz ; j++)
            {
                if(rxtid->hold_q[j].osbuf)
                {
                    p_aggr->timerScheduled = TRUE;
                    rxtid->timerMon = TRUE;
                    rxtid->progress = FALSE;
                    break;
                }
            }

            if(j >= rxtid->hold_q_sz) {
                rxtid->timerMon = FALSE;
            }
        }
    }

    if(p_aggr->timerScheduled) {
        /* Rearm the timer*/
        A_TIMEOUT_MS(&p_aggr->timer, AGGR_RX_TIMEOUT, 0);
    }

}

static void
aggr_dispatch_frames(AGGR_INFO *p_aggr, A_NETBUF_QUEUE_T *q)
{
    void *osbuf;

    while((osbuf = A_NETBUF_DEQUEUE(q))) {
        p_aggr->rx_fn(p_aggr->dev, osbuf);
    }
}

void
aggr_dump_stats(void *cntxt, PACKET_LOG **log_buf)
{
    AGGR_INFO *p_aggr = (AGGR_INFO *)cntxt;
    RXTID   *rxtid;
    RXTID_STATS *stats;
    A_UINT8 i;

    *log_buf = &p_aggr->pkt_log;
    A_PRINTF("\n\n================================================\n");
    A_PRINTF("tid: num_into_aggr, dups, oow, mpdu, amsdu, delivered, timeouts, holes, bar, seq_next\n");
    for(i = 0; i < NUM_OF_TIDS; i++) {
        stats = AGGR_GET_RXTID_STATS(p_aggr, i);
        rxtid = AGGR_GET_RXTID(p_aggr, i);
        A_PRINTF("%d: %d %d %d %d %d %d %d %d %d : %d\n", i, stats->num_into_aggr, stats->num_dups,
                    stats->num_oow, stats->num_mpdu,
                    stats->num_amsdu, stats->num_delivered, stats->num_timeouts,
                    stats->num_hole, stats->num_bar,
                    rxtid->seq_next);
    }
    A_PRINTF("================================================\n\n");

}

#endif  /* ATH_AR6K_11N_SUPPORT */
