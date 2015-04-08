/*
 * tick internal variable and functions used by low/high res code
 */
#include <linux/hrtimer.h>
#include <linux/tick.h>

#include "timekeeping.h"

extern seqlock_t jiffies_lock;

#define CS_NAME_LEN	32

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BUILD

#define TICK_DO_TIMER_NONE	-1
#define TICK_DO_TIMER_BOOT	-2

DECLARE_PER_CPU(struct tick_device, tick_cpu_device);
extern ktime_t tick_next_period;
extern ktime_t tick_period;
extern int tick_do_timer_cpu __read_mostly;

extern void tick_setup_periodic(struct clock_event_device *dev, int broadcast);
extern void tick_handle_periodic(struct clock_event_device *dev);
extern void tick_check_new_device(struct clock_event_device *dev);
extern void tick_handover_do_timer(int *cpup);
extern void tick_shutdown(unsigned int *cpup);
extern void tick_suspend(void);
extern void tick_resume(void);
extern bool tick_check_replacement(struct clock_event_device *curdev,
				   struct clock_event_device *newdev);
extern void tick_install_replacement(struct clock_event_device *dev);

extern void clockevents_shutdown(struct clock_event_device *dev);

extern ssize_t sysfs_get_uname(const char *buf, char *dst, size_t cnt);

/*
 * NO_HZ / high resolution timer shared code
 */
#ifdef CONFIG_TICK_ONESHOT
extern void tick_setup_oneshot(struct clock_event_device *newdev,
			       void (*handler)(struct clock_event_device *),
			       ktime_t nextevt);
extern int tick_program_event(ktime_t expires, int force);
extern void tick_oneshot_notify(void);
extern int tick_switch_to_oneshot(void (*handler)(struct clock_event_device *));
extern void tick_resume_oneshot(void);
# ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
extern void tick_broadcast_setup_oneshot(struct clock_event_device *bc);
extern int tick_broadcast_oneshot_control(unsigned long reason);
extern void tick_broadcast_switch_to_oneshot(void);
extern void tick_shutdown_broadcast_oneshot(unsigned int *cpup);
extern int tick_resume_broadcast_oneshot(struct clock_event_device *bc);
extern int tick_broadcast_oneshot_active(void);
extern void tick_check_oneshot_broadcast_this_cpu(void);
bool tick_broadcast_oneshot_available(void);
# else /* BROADCAST */
static inline void tick_broadcast_setup_oneshot(struct clock_event_device *bc)
{
	BUG();
}
static inline int tick_broadcast_oneshot_control(unsigned long reason) { return 0; }
static inline void tick_broadcast_switch_to_oneshot(void) { }
static inline void tick_shutdown_broadcast_oneshot(unsigned int *cpup) { }
static inline int tick_broadcast_oneshot_active(void) { return 0; }
static inline void tick_check_oneshot_broadcast_this_cpu(void) { }
static inline bool tick_broadcast_oneshot_available(void) { return true; }
# endif /* !BROADCAST */

#else /* !ONESHOT */
static inline
void tick_setup_oneshot(struct clock_event_device *newdev,
			void (*handler)(struct clock_event_device *),
			ktime_t nextevt)
{
	BUG();
}
static inline void tick_resume_oneshot(void)
{
	BUG();
}
static inline int tick_program_event(ktime_t expires, int force)
{
	return 0;
}
static inline void tick_oneshot_notify(void) { }
static inline void tick_broadcast_setup_oneshot(struct clock_event_device *bc)
{
	BUG();
}
static inline int tick_broadcast_oneshot_control(unsigned long reason) { return 0; }
static inline void tick_shutdown_broadcast_oneshot(unsigned int *cpup) { }
static inline int tick_resume_broadcast_oneshot(struct clock_event_device *bc)
{
	return 0;
}
static inline int tick_broadcast_oneshot_active(void) { return 0; }
static inline bool tick_broadcast_oneshot_available(void) { return false; }
#endif /* !TICK_ONESHOT */

/* NO_HZ_FULL internal */
#ifdef CONFIG_NO_HZ_FULL
extern void tick_nohz_init(void);
# else
static inline void tick_nohz_init(void) { }
#endif

/*
 * Broadcasting support
 */
#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
extern int tick_device_uses_broadcast(struct clock_event_device *dev, int cpu);
extern void tick_install_broadcast_device(struct clock_event_device *dev);
extern int tick_is_broadcast_device(struct clock_event_device *dev);
extern void tick_broadcast_on_off(unsigned long reason, int *oncpu);
extern void tick_shutdown_broadcast(unsigned int *cpup);
extern void tick_suspend_broadcast(void);
extern int tick_resume_broadcast(void);
extern void tick_broadcast_init(void);
extern void
tick_set_periodic_handler(struct clock_event_device *dev, int broadcast);
int tick_broadcast_update_freq(struct clock_event_device *dev, u32 freq);

#else /* !BROADCAST */

static inline void tick_install_broadcast_device(struct clock_event_device *dev)
{
}

static inline int tick_is_broadcast_device(struct clock_event_device *dev)
{
	return 0;
}
static inline int tick_device_uses_broadcast(struct clock_event_device *dev,
					     int cpu)
{
	return 0;
}
static inline void tick_do_periodic_broadcast(struct clock_event_device *d) { }
static inline void tick_broadcast_on_off(unsigned long reason, int *oncpu) { }
static inline void tick_shutdown_broadcast(unsigned int *cpup) { }
static inline void tick_suspend_broadcast(void) { }
static inline int tick_resume_broadcast(void) { return 0; }
static inline void tick_broadcast_init(void) { }
static inline int tick_broadcast_update_freq(struct clock_event_device *dev,
					     u32 freq) { return -ENODEV; }

/*
 * Set the periodic handler in non broadcast mode
 */
static inline void tick_set_periodic_handler(struct clock_event_device *dev,
					     int broadcast)
{
	dev->event_handler = tick_handle_periodic;
}
#endif /* !BROADCAST */

/*
 * Check, if the device is functional or a dummy for broadcast
 */
static inline int tick_device_is_functional(struct clock_event_device *dev)
{
	return !(dev->features & CLOCK_EVT_FEAT_DUMMY);
}

int __clockevents_update_freq(struct clock_event_device *dev, u32 freq);

#endif

extern void do_timer(unsigned long ticks);
extern void update_wall_time(void);
