#include <linux/list.h>
#include <linux/compiler.h>
#include <linux/string.h>
#include "ordered-events.h"
#include "evlist.h"
#include "session.h"
#include "asm/bug.h"
#include "debug.h"

#define pr_N(n, fmt, ...) \
	eprintf(n, debug_ordered_events, fmt, ##__VA_ARGS__)

#define pr(fmt, ...) pr_N(1, pr_fmt(fmt), ##__VA_ARGS__)

static void queue_event(struct ordered_events *oe, struct ordered_event *new)
{
	struct ordered_event *last = oe->last;
	u64 timestamp = new->timestamp;
	struct list_head *p;

	++oe->nr_events;
	oe->last = new;

	pr_oe_time2(timestamp, "queue_event nr_events %u\n", oe->nr_events);

	if (!last) {
		list_add(&new->list, &oe->events);
		oe->max_timestamp = timestamp;
		return;
	}

	/*
	 * last event might point to some random place in the list as it's
	 * the last queued event. We expect that the new event is close to
	 * this.
	 */
	if (last->timestamp <= timestamp) {
		while (last->timestamp <= timestamp) {
			p = last->list.next;
			if (p == &oe->events) {
				list_add_tail(&new->list, &oe->events);
				oe->max_timestamp = timestamp;
				return;
			}
			last = list_entry(p, struct ordered_event, list);
		}
		list_add_tail(&new->list, &last->list);
	} else {
		while (last->timestamp > timestamp) {
			p = last->list.prev;
			if (p == &oe->events) {
				list_add(&new->list, &oe->events);
				return;
			}
			last = list_entry(p, struct ordered_event, list);
		}
		list_add(&new->list, &last->list);
	}
}

static union perf_event *__dup_event(struct ordered_events *oe,
				     union perf_event *event)
{
	union perf_event *new_event = NULL;

	if (oe->cur_alloc_size < oe->max_alloc_size) {
		new_event = memdup(event, event->header.size);
		if (new_event)
			oe->cur_alloc_size += event->header.size;
	}

	return new_event;
}

static union perf_event *dup_event(struct ordered_events *oe,
				   union perf_event *event)
{
	return oe->copy_on_queue ? __dup_event(oe, event) : event;
}

static void free_dup_event(struct ordered_events *oe, union perf_event *event)
{
	if (oe->copy_on_queue) {
		oe->cur_alloc_size -= event->header.size;
		free(event);
	}
}

#define MAX_SAMPLE_BUFFER	(64 * 1024 / sizeof(struct ordered_event))
static struct ordered_event *alloc_event(struct ordered_events *oe,
					 union perf_event *event)
{
	struct list_head *cache = &oe->cache;
	struct ordered_event *new = NULL;
	union perf_event *new_event;

	new_event = dup_event(oe, event);
	if (!new_event)
		return NULL;

	if (!list_empty(cache)) {
		new = list_entry(cache->next, struct ordered_event, list);
		list_del(&new->list);
	} else if (oe->buffer) {
		new = oe->buffer + oe->buffer_idx;
		if (++oe->buffer_idx == MAX_SAMPLE_BUFFER)
			oe->buffer = NULL;
	} else if (oe->cur_alloc_size < oe->max_alloc_size) {
		size_t size = MAX_SAMPLE_BUFFER * sizeof(*new);

		oe->buffer = malloc(size);
		if (!oe->buffer) {
			free_dup_event(oe, new_event);
			return NULL;
		}

		pr("alloc size %" PRIu64 "B (+%zu), max %" PRIu64 "B\n",
		   oe->cur_alloc_size, size, oe->max_alloc_size);

		oe->cur_alloc_size += size;
		list_add(&oe->buffer->list, &oe->to_free);

		/* First entry is abused to maintain the to_free list. */
		oe->buffer_idx = 2;
		new = oe->buffer + 1;
	} else {
		pr("allocation limit reached %" PRIu64 "B\n", oe->max_alloc_size);
	}

	new->event = new_event;
	return new;
}

struct ordered_event *
ordered_events__new(struct ordered_events *oe, u64 timestamp,
		    union perf_event *event)
{
	struct ordered_event *new;

	new = alloc_event(oe, event);
	if (new) {
		new->timestamp = timestamp;
		queue_event(oe, new);
	}

	return new;
}

void ordered_events__delete(struct ordered_events *oe, struct ordered_event *event)
{
	list_move(&event->list, &oe->cache);
	oe->nr_events--;
	free_dup_event(oe, event->event);
}

static int __ordered_events__flush(struct perf_session *s,
				   struct perf_tool *tool)
{
	struct ordered_events *oe = &s->ordered_events;
	struct list_head *head = &oe->events;
	struct ordered_event *tmp, *iter;
	struct perf_sample sample;
	u64 limit = oe->next_flush;
	u64 last_ts = oe->last ? oe->last->timestamp : 0ULL;
	bool show_progress = limit == ULLONG_MAX;
	struct ui_progress prog;
	int ret;

	if (!tool->ordered_events || !limit)
		return 0;

	if (show_progress)
		ui_progress__init(&prog, oe->nr_events, "Processing time ordered events...");

	list_for_each_entry_safe(iter, tmp, head, list) {
		if (session_done())
			return 0;

		if (iter->timestamp > limit)
			break;

		ret = perf_evlist__parse_sample(s->evlist, iter->event, &sample);
		if (ret)
			pr_err("Can't parse sample, err = %d\n", ret);
		else {
			ret = perf_session__deliver_event(s, iter->event, &sample, tool,
							  iter->file_offset);
			if (ret)
				return ret;
		}

		ordered_events__delete(oe, iter);
		oe->last_flush = iter->timestamp;

		if (show_progress)
			ui_progress__update(&prog, 1);
	}

	if (list_empty(head))
		oe->last = NULL;
	else if (last_ts <= limit)
		oe->last = list_entry(head->prev, struct ordered_event, list);

	return 0;
}

int ordered_events__flush(struct perf_session *s, struct perf_tool *tool,
			  enum oe_flush how)
{
	struct ordered_events *oe = &s->ordered_events;
	static const char * const str[] = {
		"NONE",
		"FINAL",
		"ROUND",
		"HALF ",
	};
	int err;

	switch (how) {
	case OE_FLUSH__FINAL:
		oe->next_flush = ULLONG_MAX;
		break;

	case OE_FLUSH__HALF:
	{
		struct ordered_event *first, *last;
		struct list_head *head = &oe->events;

		first = list_entry(head->next, struct ordered_event, list);
		last = oe->last;

		/* Warn if we are called before any event got allocated. */
		if (WARN_ONCE(!last || list_empty(head), "empty queue"))
			return 0;

		oe->next_flush  = first->timestamp;
		oe->next_flush += (last->timestamp - first->timestamp) / 2;
		break;
	}

	case OE_FLUSH__ROUND:
	case OE_FLUSH__NONE:
	default:
		break;
	};

	pr_oe_time(oe->next_flush, "next_flush - ordered_events__flush PRE  %s, nr_events %u\n",
		   str[how], oe->nr_events);
	pr_oe_time(oe->max_timestamp, "max_timestamp\n");

	err = __ordered_events__flush(s, tool);

	if (!err) {
		if (how == OE_FLUSH__ROUND)
			oe->next_flush = oe->max_timestamp;

		oe->last_flush_type = how;
	}

	pr_oe_time(oe->next_flush, "next_flush - ordered_events__flush POST %s, nr_events %u\n",
		   str[how], oe->nr_events);
	pr_oe_time(oe->last_flush, "last_flush\n");

	return err;
}

void ordered_events__init(struct ordered_events *oe)
{
	INIT_LIST_HEAD(&oe->events);
	INIT_LIST_HEAD(&oe->cache);
	INIT_LIST_HEAD(&oe->to_free);
	oe->max_alloc_size = (u64) -1;
	oe->cur_alloc_size = 0;
}

void ordered_events__free(struct ordered_events *oe)
{
	while (!list_empty(&oe->to_free)) {
		struct ordered_event *event;

		event = list_entry(oe->to_free.next, struct ordered_event, list);
		list_del(&event->list);
		free_dup_event(oe, event->event);
		free(event);
	}
}
