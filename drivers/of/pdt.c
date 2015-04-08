/* pdt.c: OF PROM device tree support code.
 *
 * Paul Mackerras	August 1996.
 * Copyright (C) 1996-2005 Paul Mackerras.
 *
 *  Adapted for 64bit PowerPC by Dave Engebretsen and Peter Bergner.
 *    {engebret|bergner}@us.ibm.com
 *
 *  Adapted for sparc by David S. Miller davem@davemloft.net
 *  Adapted for multiple architectures by Andres Salomon <dilinger@queued.net>
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_pdt.h>
#include <asm/prom.h>

static struct of_pdt_ops *of_pdt_prom_ops __initdata;

void __initdata (*of_pdt_build_more)(struct device_node *dp,
		struct device_node ***nextp);

#if defined(CONFIG_SPARC)
unsigned int of_pdt_unique_id __initdata;

#define of_pdt_incr_unique_id(p) do { \
	(p)->unique_id = of_pdt_unique_id++; \
} while (0)

static inline const char *of_pdt_node_name(struct device_node *dp)
{
	return dp->path_component_name;
}

#else

static inline void of_pdt_incr_unique_id(void *p) { }
static inline void irq_trans_init(struct device_node *dp) { }

static inline const char *of_pdt_node_name(struct device_node *dp)
{
	return dp->name;
}

#endif /* !CONFIG_SPARC */

static struct property * __init of_pdt_build_one_prop(phandle node, char *prev,
					       char *special_name,
					       void *special_val,
					       int special_len)
{
	static struct property *tmp = NULL;
	struct property *p;
	int err;

	if (tmp) {
		p = tmp;
		memset(p, 0, sizeof(*p) + 32);
		tmp = NULL;
	} else {
		p = prom_early_alloc(sizeof(struct property) + 32);
		of_pdt_incr_unique_id(p);
	}

	p->name = (char *) (p + 1);
	if (special_name) {
		strcpy(p->name, special_name);
		p->length = special_len;
		p->value = prom_early_alloc(special_len);
		memcpy(p->value, special_val, special_len);
	} else {
		err = of_pdt_prom_ops->nextprop(node, prev, p->name);
		if (err) {
			tmp = p;
			return NULL;
		}
		p->length = of_pdt_prom_ops->getproplen(node, p->name);
		if (p->length <= 0) {
			p->length = 0;
		} else {
			int len;

			p->value = prom_early_alloc(p->length + 1);
			len = of_pdt_prom_ops->getproperty(node, p->name,
					p->value, p->length);
			if (len <= 0)
				p->length = 0;
			((unsigned char *)p->value)[p->length] = '\0';
		}
	}
	return p;
}

static struct property * __init of_pdt_build_prop_list(phandle node)
{
	struct property *head, *tail;

	head = tail = of_pdt_build_one_prop(node, NULL,
				     ".node", &node, sizeof(node));

	tail->next = of_pdt_build_one_prop(node, NULL, NULL, NULL, 0);
	tail = tail->next;
	while(tail) {
		tail->next = of_pdt_build_one_prop(node, tail->name,
					    NULL, NULL, 0);
		tail = tail->next;
	}

	return head;
}

static char * __init of_pdt_get_one_property(phandle node, const char *name)
{
	char *buf = "<NULL>";
	int len;

	len = of_pdt_prom_ops->getproplen(node, name);
	if (len > 0) {
		buf = prom_early_alloc(len);
		len = of_pdt_prom_ops->getproperty(node, name, buf, len);
	}

	return buf;
}

static char * __init of_pdt_try_pkg2path(phandle node)
{
	char *res, *buf = NULL;
	int len;

	if (!of_pdt_prom_ops->pkg2path)
		return NULL;

	if (of_pdt_prom_ops->pkg2path(node, buf, 0, &len))
		return NULL;
	buf = prom_early_alloc(len + 1);
	if (of_pdt_prom_ops->pkg2path(node, buf, len, &len)) {
		pr_err("%s: package-to-path failed\n", __func__);
		return NULL;
	}

	res = strrchr(buf, '/');
	if (!res) {
		pr_err("%s: couldn't find / in %s\n", __func__, buf);
		return NULL;
	}
	return res+1;
}

/*
 * When fetching the node's name, first try using package-to-path; if
 * that fails (either because the arch hasn't supplied a PROM callback,
 * or some other random failure), fall back to just looking at the node's
 * 'name' property.
 */
static char * __init of_pdt_build_name(phandle node)
{
	char *buf;

	buf = of_pdt_try_pkg2path(node);
	if (!buf)
		buf = of_pdt_get_one_property(node, "name");

	return buf;
}

static struct device_node * __init of_pdt_create_node(phandle node,
						    struct device_node *parent)
{
	struct device_node *dp;

	if (!node)
		return NULL;

	dp = prom_early_alloc(sizeof(*dp));
	of_pdt_incr_unique_id(dp);
	dp->parent = parent;

	kref_init(&dp->kref);

	dp->name = of_pdt_build_name(node);
	dp->type = of_pdt_get_one_property(node, "device_type");
	dp->phandle = node;

	dp->properties = of_pdt_build_prop_list(node);

	irq_trans_init(dp);

	return dp;
}

static char * __init of_pdt_build_full_name(struct device_node *dp)
{
	int len, ourlen, plen;
	char *n;

	plen = strlen(dp->parent->full_name);
	ourlen = strlen(of_pdt_node_name(dp));
	len = ourlen + plen + 2;

	n = prom_early_alloc(len);
	strcpy(n, dp->parent->full_name);
	if (!of_node_is_root(dp->parent)) {
		strcpy(n + plen, "/");
		plen++;
	}
	strcpy(n + plen, of_pdt_node_name(dp));

	return n;
}

static struct device_node * __init of_pdt_build_tree(struct device_node *parent,
						   phandle node,
						   struct device_node ***nextp)
{
	struct device_node *ret = NULL, *prev_sibling = NULL;
	struct device_node *dp;

	while (1) {
		dp = of_pdt_create_node(node, parent);
		if (!dp)
			break;

		if (prev_sibling)
			prev_sibling->sibling = dp;

		if (!ret)
			ret = dp;
		prev_sibling = dp;

		*(*nextp) = dp;
		*nextp = &dp->allnext;

#if defined(CONFIG_SPARC)
		dp->path_component_name = build_path_component(dp);
#endif
		dp->full_name = of_pdt_build_full_name(dp);

		dp->child = of_pdt_build_tree(dp,
				of_pdt_prom_ops->getchild(node), nextp);

		if (of_pdt_build_more)
			of_pdt_build_more(dp, nextp);

		node = of_pdt_prom_ops->getsibling(node);
	}

	return ret;
}

void __init of_pdt_build_devicetree(phandle root_node, struct of_pdt_ops *ops)
{
	struct device_node **nextp;

	BUG_ON(!ops);
	of_pdt_prom_ops = ops;

	allnodes = of_pdt_create_node(root_node, NULL);
#if defined(CONFIG_SPARC)
	allnodes->path_component_name = "";
#endif
	allnodes->full_name = "/";

	nextp = &allnodes->allnext;
	allnodes->child = of_pdt_build_tree(allnodes,
			of_pdt_prom_ops->getchild(allnodes->phandle), &nextp);
}
