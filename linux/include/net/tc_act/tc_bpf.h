/*
 * Copyright (c) 2015 Jiri Pirko <jiri@resnulli.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __NET_TC_BPF_H
#define __NET_TC_BPF_H

#include <linux/filter.h>
#include <net/act_api.h>

struct tcf_bpf {
	struct tcf_common	common;
	struct bpf_prog		*filter;
	struct sock_filter	*bpf_ops;
	u16			bpf_num_ops;
};
#define to_bpf(a) \
	container_of(a->priv, struct tcf_bpf, common)

#endif /* __NET_TC_BPF_H */
