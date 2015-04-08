#undef TRACE_SYSTEM
#define TRACE_SYSTEM workqueue

#if !defined(_TRACE_WORKQUEUE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_WORKQUEUE_H

#include <linux/tracepoint.h>
#include <linux/workqueue.h>

DECLARE_EVENT_CLASS(workqueue_work,

	TP_PROTO(struct work_struct *work),

	TP_ARGS(work),

	TP_STRUCT__entry(
		__field( void *,	work	)
	),

	TP_fast_assign(
		__entry->work		= work;
	),

	TP_printk("work struct %p", __entry->work)
);

/**
 * workqueue_queue_work - called when a work gets queued
 * @req_cpu:	the requested cpu
 * @cwq:	pointer to struct cpu_workqueue_struct
 * @work:	pointer to struct work_struct
 *
 * This event occurs when a work is queued immediately or once a
 * delayed work is actually queued on a workqueue (ie: once the delay
 * has been reached).
 */
TRACE_EVENT(workqueue_queue_work,

	TP_PROTO(unsigned int req_cpu, struct cpu_workqueue_struct *cwq,
		 struct work_struct *work),

	TP_ARGS(req_cpu, cwq, work),

	TP_STRUCT__entry(
		__field( void *,	work	)
		__field( void *,	function)
		__field( void *,	workqueue)
		__field( unsigned int,	req_cpu	)
		__field( unsigned int,	cpu	)
	),

	TP_fast_assign(
		__entry->work		= work;
		__entry->function	= work->func;
		__entry->workqueue	= cwq->wq;
		__entry->req_cpu	= req_cpu;
		__entry->cpu		= cwq->gcwq->cpu;
	),

	TP_printk("work struct=%p function=%pf workqueue=%p req_cpu=%u cpu=%u",
		  __entry->work, __entry->function, __entry->workqueue,
		  __entry->req_cpu, __entry->cpu)
);

/**
 * workqueue_activate_work - called when a work gets activated
 * @work:	pointer to struct work_struct
 *
 * This event occurs when a queued work is put on the active queue,
 * which happens immediately after queueing unless @max_active limit
 * is reached.
 */
DEFINE_EVENT(workqueue_work, workqueue_activate_work,

	TP_PROTO(struct work_struct *work),

	TP_ARGS(work)
);

/**
 * workqueue_execute_start - called immediately before the workqueue callback
 * @work:	pointer to struct work_struct
 *
 * Allows to track workqueue execution.
 */
TRACE_EVENT(workqueue_execute_start,

	TP_PROTO(struct work_struct *work),

	TP_ARGS(work),

	TP_STRUCT__entry(
		__field( void *,	work	)
		__field( void *,	function)
	),

	TP_fast_assign(
		__entry->work		= work;
		__entry->function	= work->func;
	),

	TP_printk("work struct %p: function %pf", __entry->work, __entry->function)
);

/**
 * workqueue_execute_end - called immediately before the workqueue callback
 * @work:	pointer to struct work_struct
 *
 * Allows to track workqueue execution.
 */
DEFINE_EVENT(workqueue_work, workqueue_execute_end,

	TP_PROTO(struct work_struct *work),

	TP_ARGS(work)
);

#endif /*  _TRACE_WORKQUEUE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
