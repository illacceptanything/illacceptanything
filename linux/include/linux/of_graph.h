/*
 * OF graph binding parsing helpers
 *
 * Copyright (C) 2012 - 2013 Samsung Electronics Co., Ltd.
 * Author: Sylwester Nawrocki <s.nawrocki@samsung.com>
 *
 * Copyright (C) 2012 Renesas Electronics Corp.
 * Author: Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_OF_GRAPH_H
#define __LINUX_OF_GRAPH_H

/**
 * struct of_endpoint - the OF graph endpoint data structure
 * @port: identifier (value of reg property) of a port this endpoint belongs to
 * @id: identifier (value of reg property) of this endpoint
 * @local_node: pointer to device_node of this endpoint
 */
struct of_endpoint {
	unsigned int port;
	unsigned int id;
	const struct device_node *local_node;
};

#ifdef CONFIG_OF
int of_graph_parse_endpoint(const struct device_node *node,
				struct of_endpoint *endpoint);
struct device_node *of_graph_get_next_endpoint(const struct device_node *parent,
					struct device_node *previous);
struct device_node *of_graph_get_remote_port_parent(
					const struct device_node *node);
struct device_node *of_graph_get_remote_port(const struct device_node *node);
#else

static inline int of_graph_parse_endpoint(const struct device_node *node,
					struct of_endpoint *endpoint)
{
	return -ENOSYS;
}

static inline struct device_node *of_graph_get_next_endpoint(
					const struct device_node *parent,
					struct device_node *previous)
{
	return NULL;
}

static inline struct device_node *of_graph_get_remote_port_parent(
					const struct device_node *node)
{
	return NULL;
}

static inline struct device_node *of_graph_get_remote_port(
					const struct device_node *node)
{
	return NULL;
}

#endif /* CONFIG_OF */

#endif /* __LINUX_OF_GRAPH_H */
