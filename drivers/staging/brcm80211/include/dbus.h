/*
 * Copyright (c) 2010 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __DBUS_H__
#define __DBUS_H__

#ifdef BCMDBG
#define DBUSERR(args)        do { if (net_ratelimit()) printf args; } while (0)
#define DBUSTRACE(args)
#define DBUSDBGLOCK(args)

#else
#define DBUSTRACE(args)
#define DBUSERR(args)
#define DBUSDBGLOCK(args)
#endif

enum {
	DBUS_OK = 0,
	DBUS_ERR = -200,
	DBUS_ERR_TIMEOUT,
	DBUS_ERR_DISCONNECT,
	DBUS_ERR_NODEVICE,
	DBUS_ERR_UNSUPPORTED,
	DBUS_ERR_PENDING,
	DBUS_ERR_NOMEM,
	DBUS_ERR_TXFAIL,
	DBUS_ERR_TXTIMEOUT,
	DBUS_ERR_TXDROP,
	DBUS_ERR_RXFAIL,
	DBUS_ERR_RXDROP,
	DBUS_ERR_TXCTLFAIL,
	DBUS_ERR_RXCTLFAIL,
	DBUS_ERR_REG_PARAM,
	DBUS_STATUS_CANCELLED
};

#define ERR_CBMASK_TXFAIL		0x00000001
#define ERR_CBMASK_RXFAIL		0x00000002
#define ERR_CBMASK_ALL			0xFFFFFFFF

#define DBUS_CBCTL_WRITE		0
#define DBUS_CBCTL_READ			1

#define DBUS_TX_RETRY_LIMIT		3	/* retries for failed txirb */
#define DBUS_TX_TIMEOUT_INTERVAL	250	/* timeout for txirb complete, in ms */

#define DBUS_BUFFER_SIZE_TX	5000
#define DBUS_BUFFER_SIZE_RX	5000

#define DBUS_BUFFER_SIZE_TX_NOAGG	2048
#define DBUS_BUFFER_SIZE_RX_NOAGG	2048

/* DBUS types */
enum {
	DBUS_USB,
	DBUS_SDIO,
	DBUS_SPI,
	DBUS_UNKNOWN
};

enum dbus_state {
	DBUS_STATE_DL_PENDING,
	DBUS_STATE_DL_DONE,
	DBUS_STATE_UP,
	DBUS_STATE_DOWN,
	DBUS_STATE_PNP_FWDL,
	DBUS_STATE_DISCONNECT
};

enum dbus_pnp_state {
	DBUS_PNP_DISCONNECT,
	DBUS_PNP_SLEEP,
	DBUS_PNP_RESUME
};

typedef enum _DEVICE_SPEED {
	INVALID_SPEED = -1,
	LOW_SPEED = 1,		/* USB 1.1: 1.5 Mbps */
	FULL_SPEED,		/* USB 1.1: 12  Mbps */
	HIGH_SPEED,		/* USB 2.0: 480 Mbps */
	SUPER_SPEED,		/* USB 3.0: 4.8 Gbps */
} DEVICE_SPEED;

typedef struct {
	int bustype;
	int vid;
	int pid;
	int devid;
	int chiprev;		/* chip revsion number */
	int mtu;
	int nchan;		/* Data Channels */
} dbus_attrib_t;

/* FIX: Account for errors related to DBUS;
 * Let upper layer account for packets/bytes
 */
typedef struct {
	u32 rx_errors;
	u32 tx_errors;
	u32 rx_dropped;
	u32 tx_dropped;
} dbus_stats_t;

/*
 * Configurable BUS parameters
 */
typedef struct {
	bool rxctl_deferrespok;
} dbus_config_t;

struct dbus_callbacks;
struct exec_parms;

typedef void *(*probe_cb_t) (void *arg, const char *desc, u32 bustype,
			     u32 hdrlen);
typedef void (*disconnect_cb_t) (void *arg);
typedef void *(*exec_cb_t) (struct exec_parms *args);

/* Client callbacks registered during dbus_attach() */
typedef struct dbus_callbacks {
	void (*send_complete) (void *cbarg, void *info, int status);
	void (*recv_buf) (void *cbarg, u8 *buf, int len);
	void (*recv_pkt) (void *cbarg, void *pkt);
	void (*txflowcontrol) (void *cbarg, bool onoff);
	void (*errhandler) (void *cbarg, int err);
	void (*ctl_complete) (void *cbarg, int type, int status);
	void (*state_change) (void *cbarg, int state);
	void *(*pktget) (void *cbarg, uint len, bool send);
	void (*pktfree) (void *cbarg, void *p, bool send);
} dbus_callbacks_t;

struct dbus_pub;
struct bcmstrbuf;
struct dbus_irb;
struct dbus_irb_rx;
struct dbus_irb_tx;
struct dbus_intf_callbacks;

typedef struct {
	void *(*attach) (struct dbus_pub *pub, void *cbarg,
			 struct dbus_intf_callbacks *cbs);
	void (*detach) (struct dbus_pub *pub, void *bus);

	int (*up) (void *bus);
	int (*down) (void *bus);
	int (*send_irb) (void *bus, struct dbus_irb_tx *txirb);
	int (*recv_irb) (void *bus, struct dbus_irb_rx *rxirb);
	int (*cancel_irb) (void *bus, struct dbus_irb_tx *txirb);
	int (*send_ctl) (void *bus, u8 *buf, int len);
	int (*recv_ctl) (void *bus, u8 *buf, int len);
	int (*get_stats) (void *bus, dbus_stats_t *stats);
	int (*get_attrib) (void *bus, dbus_attrib_t *attrib);

	int (*pnp) (void *bus, int event);
	int (*remove) (void *bus);
	int (*resume) (void *bus);
	int (*suspend) (void *bus);
	int (*stop) (void *bus);
	int (*reset) (void *bus);

	/* Access to bus buffers directly */
	void *(*pktget) (void *bus, int len);
	void (*pktfree) (void *bus, void *pkt);

	int (*iovar_op) (void *bus, const char *name, void *params, int plen,
			 void *arg, int len, bool set);
	void (*dump) (void *bus, struct bcmstrbuf *strbuf);
	int (*set_config) (void *bus, dbus_config_t *config);
	int (*get_config) (void *bus, dbus_config_t *config);

	 bool(*device_exists) (void *bus);
	 bool(*dlneeded) (void *bus);
	int (*dlstart) (void *bus, u8 *fw, int len);
	int (*dlrun) (void *bus);
	 bool(*recv_needed) (void *bus);

	void *(*exec_rxlock) (void *bus, exec_cb_t func,
			      struct exec_parms *args);
	void *(*exec_txlock) (void *bus, exec_cb_t func,
			      struct exec_parms *args);

	int (*tx_timer_init) (void *bus);
	int (*tx_timer_start) (void *bus, uint timeout);
	int (*tx_timer_stop) (void *bus);

	int (*sched_dpc) (void *bus);
	int (*lock) (void *bus);
	int (*unlock) (void *bus);
	int (*sched_probe_cb) (void *bus);

	int (*shutdown) (void *bus);

	int (*recv_stop) (void *bus);
	int (*recv_resume) (void *bus);

	/* Add from the bottom */
} dbus_intf_t;

typedef struct dbus_pub {
	struct osl_info *osh;
	dbus_stats_t stats;
	dbus_attrib_t attrib;
	enum dbus_state busstate;
	DEVICE_SPEED device_speed;
	int ntxq, nrxq, rxsize;
	void *bus;
	struct shared_info *sh;
} dbus_pub_t;

#define BUS_INFO(bus, type) (((type *) bus)->pub->bus)

/*
 * Public Bus Function Interface
 */
extern int dbus_register(int vid, int pid, probe_cb_t prcb,
			 disconnect_cb_t discb, void *prarg, void *param1,
			 void *param2);
extern int dbus_deregister(void);

extern const dbus_pub_t *dbus_attach(struct osl_info *osh, int rxsize, int nrxq,
				     int ntxq, void *cbarg,
				     dbus_callbacks_t *cbs,
				     struct shared_info *sh);
extern void dbus_detach(const dbus_pub_t *pub);

extern int dbus_up(const dbus_pub_t *pub);
extern int dbus_down(const dbus_pub_t *pub);
extern int dbus_stop(const dbus_pub_t *pub);
extern int dbus_shutdown(const dbus_pub_t *pub);
extern void dbus_flowctrl_rx(const dbus_pub_t *pub, bool on);

extern int dbus_send_buf(const dbus_pub_t *pub, u8 *buf, int len,
			 void *info);
extern int dbus_send_pkt(const dbus_pub_t *pub, void *pkt, void *info);
extern int dbus_send_ctl(const dbus_pub_t *pub, u8 *buf, int len);
extern int dbus_recv_ctl(const dbus_pub_t *pub, u8 *buf, int len);

extern int dbus_get_stats(const dbus_pub_t *pub, dbus_stats_t *stats);
extern int dbus_get_attrib(const dbus_pub_t *pub, dbus_attrib_t *attrib);
extern int dbus_get_device_speed(const dbus_pub_t *pub);
extern int dbus_set_config(const dbus_pub_t *pub, dbus_config_t *config);
extern int dbus_get_config(const dbus_pub_t *pub, dbus_config_t *config);

extern void *dbus_pktget(const dbus_pub_t *pub, int len);
extern void dbus_pktfree(const dbus_pub_t *pub, void *pkt);

extern int dbus_set_errmask(const dbus_pub_t *pub, u32 mask);
extern int dbus_pnp_sleep(const dbus_pub_t *pub);
extern int dbus_pnp_resume(const dbus_pub_t *pub, int *fw_reload);
extern int dbus_pnp_disconnect(const dbus_pub_t *pub);

extern int dbus_iovar_op(const dbus_pub_t *pub, const char *name,
			 void *params, int plen, void *arg, int len, bool set);
#ifdef BCMDBG
extern void dbus_hist_dump(const dbus_pub_t *pub, struct bcmstrbuf *b);
#endif				/* BCMDBG */
/*
 * Private Common Bus Interface
 */

/* IO Request Block (IRB) */
typedef struct dbus_irb {
	struct dbus_irb *next;	/* it's casted from dbus_irb_tx or dbus_irb_rx struct */
} dbus_irb_t;

typedef struct dbus_irb_rx {
	struct dbus_irb irb;	/* Must be first */
	u8 *buf;
	int buf_len;
	int actual_len;
	void *pkt;
	void *info;
	void *arg;
} dbus_irb_rx_t;

typedef struct dbus_irb_tx {
	struct dbus_irb irb;	/* Must be first */
	u8 *buf;
	int len;
	void *pkt;
	int retry_count;
	void *info;
	void *arg;
} dbus_irb_tx_t;

/* DBUS interface callbacks are different from user callbacks
 * so, internally, different info can be passed to upper layer
 */
typedef struct dbus_intf_callbacks {
	void (*send_irb_timeout) (void *cbarg, dbus_irb_tx_t *txirb);
	void (*send_irb_complete) (void *cbarg, dbus_irb_tx_t *txirb,
				   int status);
	void (*recv_irb_complete) (void *cbarg, dbus_irb_rx_t *rxirb,
				   int status);
	void (*errhandler) (void *cbarg, int err);
	void (*ctl_complete) (void *cbarg, int type, int status);
	void (*state_change) (void *cbarg, int state);
	 bool(*isr) (void *cbarg, bool *wantdpc);
	 bool(*dpc) (void *cbarg, bool bounded);
	void (*watchdog) (void *cbarg);
	void *(*pktget) (void *cbarg, uint len, bool send);
	void (*pktfree) (void *cbarg, void *p, bool send);
	struct dbus_irb *(*getirb) (void *cbarg, bool send);
	void (*rxerr_indicate) (void *cbarg, bool on);
} dbus_intf_callbacks_t;

/*
 * Porting: To support new bus, port these functions below
 */

/*
 * Bus specific Interface
 * Implemented by dbus_usb.c/dbus_sdio.c
 */
extern int dbus_bus_register(int vid, int pid, probe_cb_t prcb,
			     disconnect_cb_t discb, void *prarg,
			     dbus_intf_t **intf, void *param1, void *param2);
extern int dbus_bus_deregister(void);

/*
 * Bus-specific and OS-specific Interface
 * Implemented by dbus_usb_[linux/ndis].c/dbus_sdio_[linux/ndis].c
 */
extern int dbus_bus_osl_register(int vid, int pid, probe_cb_t prcb,
				 disconnect_cb_t discb, void *prarg,
				 dbus_intf_t **intf, void *param1,
				 void *param2);
extern int dbus_bus_osl_deregister(void);

/*
 * Bus-specific, OS-specific, HW-specific Interface
 * Mainly for SDIO Host HW controller
 */
extern int dbus_bus_osl_hw_register(int vid, int pid, probe_cb_t prcb,
				    disconnect_cb_t discb, void *prarg,
				    dbus_intf_t **intf);
extern int dbus_bus_osl_hw_deregister(void);

#endif				/* __DBUS_H__ */
