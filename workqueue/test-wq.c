/*
 * This test is collected from Tejun Heo
 */
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/cpu.h>
#include <linux/kthread.h>

#define MAX_WQ_NAME		64
#define MAX_WQS			64
#define MAX_WORKS		64

struct wq_spec {
	int			id;	/* -1 terminates */
	unsigned int		max_active;
	unsigned int		flags;
};

enum action {
	ACT_TERM,			/* end */
	ACT_LOG,			/* const char * */
	ACT_BURN,			/* ulong duration_msecs */
	ACT_SLEEP,			/* ulong duration_msecs */
	ACT_WAKEUP,			/* ulong work_id */
	ACT_REQUEUE,			/* ulong delay_msecs, ulong cpu */
	ACT_FLUSH,			/* ulong work_id */
	ACT_FLUSH_WQ,			/* ulong workqueue_id */
	ACT_CANCEL,			/* ulong work_id */
};

struct work_action {
	enum action		action;	/* ACT_TERM terminates */
	union {
		unsigned long	v;
		const char	*s;
	};
	unsigned long		v1;
};

struct work_spec {
	int			id;		/* -1 terminates */
	int			wq_id;
	int			requeue_cnt;
	unsigned int		cpu;
	unsigned long		initial_delay;	/* msecs */

	const struct work_action *actions;
};

struct test_scenario {
	const struct wq_spec	*wq_spec;
	const struct work_spec	**work_spec;	/* NULL terminated */
};

static const struct wq_spec dfl_wq_spec[] = {
	{
		.id		= 0,
		.max_active	= 32,
		.flags		= 0,
	},
	{
		.id		= 1,
		.max_active	= 32,
		.flags		= 0,
	},
	{
		.id		= 2,
		.max_active	= 32,
		.flags		= WQ_RESCUER,
	},
	{
		.id		= 3,
		.max_active	= 32,
		.flags		= WQ_FREEZABLE,
	},
	{
		.id		= 4,
		.max_active	= 1,
		.flags		= WQ_UNBOUND | WQ_FREEZABLE/* | WQ_DBG*/,
	},
	{
		.id		= 5,
		.max_active	= 32,
		.flags		= WQ_NON_REENTRANT,
	},
	{
		.id		= 6,
		.max_active	= 4,
		.flags		= WQ_HIGHPRI,
	},
	{
		.id		= 7,
		.max_active	= 4,
		.flags		= WQ_CPU_INTENSIVE,
	},
	{
		.id		= 8,
		.max_active	= 4,
		.flags		= WQ_HIGHPRI | WQ_CPU_INTENSIVE,
	},
	{ .id = -1 },
};

/*
 * Scenario 0.  All are on cpu0.  work16 and 17 burn cpus for 10 and
 * 5msecs respectively and requeue themselves.  18 sleeps 2 secs and
 * cancel both.
 */
static const struct work_spec work_spec0[] = {
	{
		.id		= 16,
		.requeue_cnt	= 1024,
		.actions	= (const struct work_action[]) {
			{ ACT_BURN,	{ 10 }},
			{ ACT_REQUEUE,	{ 0 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 17,
		.requeue_cnt	= 1024,
		.actions	= (const struct work_action[]) {
			{ ACT_BURN,	{ 5 }},
			{ ACT_REQUEUE,	{ 0 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 18,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "will sleep 2s and cancel both" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_CANCEL,	{ 16 }},
			{ ACT_CANCEL,	{ 17 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario0 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec0, NULL },
};

/*
 * Scenario 1.  All are on cpu0.  Work 0, 1 and 2 sleep for different
 * intervals but all three will terminate at around 30secs.  3 starts
 * at @28 and 4 at @33 and both sleep for five secs and then
 * terminate.  5 waits for 0, 1, 2 and then flush wq which by the time
 * should have 3 on it.  After 3 completes @32, 5 terminates too.
 * After 4 secs, 4 terminates and all test sequence is done.
 */
static const struct work_spec work_spec1[] = {
	{
		.id		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_BURN,	{ 3 }},	/* to cause sched activation */
			{ ACT_LOG,	{ .s = "will sleep 30s" }},
			{ ACT_SLEEP,	{ 30000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_BURN,	{ 5 }},
			{ ACT_LOG,	{ .s = "will sleep 10s and burn 5msec and repeat 3 times" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_BURN,	{ 5 }},
			{ ACT_LOG,	{ .s = "@10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_BURN,	{ 5 }},
			{ ACT_LOG,	{ .s = "@20s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_BURN,	{ 5 }},
			{ ACT_LOG,	{ .s = "@30s" }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.actions	= (const struct work_action[]) {
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "will sleep 3s and burn 1msec and repeat 10 times" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@3s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@6s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@9s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@12s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@15s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@18s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@21s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@24s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@27s" }},
			{ ACT_SLEEP,	{ 3000 }},
			{ ACT_BURN,	{ 1 }},
			{ ACT_LOG,	{ .s = "@30s" }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.initial_delay	= 29000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "started@28s, will sleep for 5s" }},
			{ ACT_SLEEP,	{ 5000 }},
			{ ACT_TERM },
		}
	},
	{
		.id		= 4,
		.initial_delay	= 33000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "started@33s, will sleep for 5s" }},
			{ ACT_SLEEP,	{ 5000 }},
			{ ACT_TERM },
		}
	},
	{
		.id		= 5,
		.wq_id		= 1,	/* can't flush self */
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flushing 0, 1 and 2" }},
			{ ACT_FLUSH,	{ 0 }},
			{ ACT_FLUSH,	{ 1 }},
			{ ACT_FLUSH,	{ 2 }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario1 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec1, NULL },
};

/*
 * Scenario 2.  Combination of scenario 0 and 1.
 */
static const struct test_scenario scenario2 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec0, work_spec1, NULL },
};

/*
 * Scenario 3.  More complex flushing.
 *
 *               2:burn 2s        3:4s
 *                <---->      <---------->
 *     0:4s                1:4s
 * <---------->      <..----------->
 *    ^               ^
 *    |               |
 *    |               |
 * 4:flush(cpu0)    flush_wq
 * 5:flush(cpu0)    flush
 * 6:flush(cpu1)    flush_wq
 * 7:flush(cpu1)    flush
 */
static const struct work_spec work_spec2[] = {
	{
		.id		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.initial_delay	= 6000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.initial_delay	= 5000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning 2s" }},
			{ ACT_BURN,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.initial_delay	= 9000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},

	{
		.id		= 4,
		.wq_id		= 1,
		.initial_delay	= 1000,
		.actions	= (const struct work_action[]) {
			{ ACT_FLUSH,	{ 0 }},
			{ ACT_SLEEP,	{ 2500 }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.wq_id		= 1,
		.initial_delay	= 1000,
		.actions	= (const struct work_action[]) {
			{ ACT_FLUSH,	{ 0 }},
			{ ACT_SLEEP,	{ 2500 }},
			{ ACT_FLUSH,	{ 1 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 6,
		.wq_id		= 1,
		.cpu		= 1,
		.initial_delay	= 1000,
		.actions	= (const struct work_action[]) {
			{ ACT_FLUSH,	{ 0 }},
			{ ACT_SLEEP,	{ 2500 }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 7,
		.wq_id		= 1,
		.cpu		= 1,
		.initial_delay	= 1000,
		.actions	= (const struct work_action[]) {
			{ ACT_FLUSH,	{ 0 }},
			{ ACT_SLEEP,	{ 2500 }},
			{ ACT_FLUSH,	{ 1 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario3 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec2, NULL },
};

/*
 * Scenario 4.  Mayday!  To be used with MAX_CPU_WORKERS_ORDER reduced
 * to 2.
 */
static const struct work_spec work_spec4[] = {
	{
		.id		= 0,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 5s" }},
			{ ACT_SLEEP,	{ 5000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 5s" }},
			{ ACT_SLEEP,	{ 5000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 5s" }},
			{ ACT_SLEEP,	{ 5000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 5s" }},
			{ ACT_SLEEP,	{ 5000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 4,
		.wq_id		= 2,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.wq_id		= 2,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 6,
		.wq_id		= 2,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{
		.id		= 7,
		.wq_id		= 2,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_REQUEUE,	{ 5000 }, NR_CPUS },
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario4 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec4, NULL },
};

/*
 * Scenario 5.  To test cpu off/onlining.  A bunch of long running
 * tasks on cpu1.  Gets interesting with various other conditions
 * applied together - lowered MAX_CPU_WORKERS_ORDER, induced failure
 * or delay during CPU_DOWN/UP_PREPARE and so on.
 */
static const struct work_spec work_spec5[] = {
	/* runs for 30 secs */
	{
		.id		= 0,
		.cpu		= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.cpu		= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.cpu		= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.cpu		= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},

	/* kicks in @15 and runs for 15 from wq0 */
	{
		.id		= 4,
		.cpu		= 1,
		.initial_delay	= 10000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.cpu		= 1,
		.initial_delay	= 10000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 6,
		.cpu		= 1,
		.initial_delay	= 10000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 7,
		.cpu		= 1,
		.initial_delay	= 10000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
#if 0
	/* kicks in @15 and runs for 15 from wq2 */
	{
		.id		= 8,
		.wq_id		= 2,
		.cpu		= 1,
		.initial_delay	= 15000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 15s" }},
			{ ACT_SLEEP,	{ 15000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 9,
		.wq_id		= 2,
		.cpu		= 1,
		.initial_delay	= 15000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 15s" }},
			{ ACT_SLEEP,	{ 15000 }},
			{ ACT_TERM },
		},
	},

	/* kicks in @30 and runs for 15 */
	{
		.id		= 10,
		.cpu		= 1,
		.initial_delay	= 30000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 15s" }},
			{ ACT_SLEEP,	{ 15000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 11,
		.cpu		= 1,
		.initial_delay	= 30000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 15s" }},
			{ ACT_SLEEP,	{ 15000 }},
			{ ACT_TERM },
		},
	},
#endif
	{ .id = -1 },
};

static const struct test_scenario scenario5 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec5, NULL },
};

/*
 * Scenario 6.  Scenario to test freezeable workqueue.  User should
 * freeze the machine between 0s and 9s.
 *
 * 0,1:sleep 10s
 * <-------->
 *      <--freezing--><--frozen--><-thawed
 *         2,3: sleeps for 10s
 *          <.....................-------->
 *          <.....................-------->
 */
static const struct work_spec work_spec6[] = {
	/* two works which get queued @0s and sleeps for 10s */
	{
		.id		= 0,
		.wq_id		= 3,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.wq_id		= 3,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	/* two works which get queued @9s and sleeps for 10s */
	{
		.id		= 2,
		.wq_id		= 3,
		.initial_delay	= 9000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.wq_id		= 3,
		.initial_delay	= 9000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 10s" }},
			{ ACT_SLEEP,	{ 10000 }},
			{ ACT_TERM },
		},
	},

	{ .id = -1 },
};

static const struct test_scenario scenario6 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec6, NULL },
};

/*
 * Scenario 7.  Scenario to test multi-colored workqueue.
 *
 *   0 1 2 3 4 5 6 7 8 9 0 1 2	: time in seconds
 * 0:  <------>				cpu0
 * 1:    <------>			cpu1
 * 2:      <------>			cpu2
 * 3:        <------>			cpu3
 * 4:          <------>			cpu0
 * 5:            <------>		cpu1
 * 6:              <------>		cpu2
 * 7:                <------>		cpu3
 * Flush workqueues
 * 10:^
 * 11:  ^
 * 12:  ^
 * 13:      ^
 * 14:        ^
 * 15:        ^
 * 16:            ^
 * 17:              ^
 * 18:              ^
 */
static const struct work_spec work_spec7[] = {
	/* 8 works - each sleeps 4sec and starts staggered */
	{
		.id		= 0,
		.wq_id		= 0,
		.cpu		= 0,
		.initial_delay	= 1000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 1s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.wq_id		= 0,
		.cpu		= 1,
		.initial_delay	= 2000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 2s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.wq_id		= 0,
		.cpu		= 2,
		.initial_delay	= 3000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 3s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.wq_id		= 0,
		.cpu		= 3,
		.initial_delay	= 4000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 4s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 4,
		.wq_id		= 0,
		.cpu		= 0,
		.initial_delay	= 5000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 5s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.wq_id		= 0,
		.cpu		= 1,
		.initial_delay	= 6000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 6s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 6,
		.wq_id		= 0,
		.cpu		= 2,
		.initial_delay	= 7000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 7s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 7,
		.wq_id		= 0,
		.cpu		= 3,
		.initial_delay	= 8000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @ 8s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},

	/* 9 workqueue flushers */
	{
		.id		= 10,
		.wq_id		= 1,
		.initial_delay	= 500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 0.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 11,
		.wq_id		= 1,
		.initial_delay	= 1500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 1.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 12,
		.wq_id		= 1,
		.initial_delay	= 1500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 1.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 13,
		.wq_id		= 1,
		.initial_delay	= 3500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 3.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 14,
		.wq_id		= 1,
		.initial_delay	= 4500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 4.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 15,
		.wq_id		= 1,
		.initial_delay	= 4500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 4.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 16,
		.wq_id		= 1,
		.initial_delay	= 6500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 6.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 17,
		.wq_id		= 1,
		.initial_delay	= 7500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 7.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 18,
		.wq_id		= 1,
		.initial_delay	= 7500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "flush_wq @ 7.5s" }},
			{ ACT_FLUSH_WQ,	{ 0 }},
			{ ACT_TERM },
		},
	},

	{ .id = -1 },
};

static const struct test_scenario scenario7 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec7, NULL },
};

/*
 * Scenario 8.  Scenario to test single thread workqueue.  Test with
 * freeze/thaw, suspend/resume at various points.
 *
 * 0@cpu0 <------>
 * 1@cpu1     <...--->
 * 2@cpu2         <...--->
 * 3@cpu3                  <------>
 * 4@cpu0                      <...--->
 * 5@cpu1                              <------>
 */
static const struct work_spec work_spec8[] = {
	{
		.id		= 0,
		.wq_id		= 4,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @0s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.wq_id		= 4,
		.cpu		= 1,
		.initial_delay	= 2000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s @2:4s" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.wq_id		= 4,
		.cpu		= 2,
		.initial_delay	= 4000,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s @4:6s" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.wq_id		= 4,
		.cpu		= 3,
		.initial_delay	= 8500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @8.5s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 4,
		.wq_id		= 4,
		.cpu		= 0,
		.initial_delay	= 10500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s @10.5:12.5s" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.wq_id		= 4,
		.cpu		= 1,
		.initial_delay	= 14500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 4s @14.5s" }},
			{ ACT_SLEEP,	{ 4000 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario8 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec8, NULL },
};

/*
 * Scenario 9.  Scenario to test non-reentrant workqueue.  
 *
 *        0   2   4   6   8   10  11
 * 0@cpu0 <------>
 * 0@cpu0     <...------->			(requeued to cpu0)
 * 1@cpu0 <------>
 * 1@cpu0     <...------->			(requeued to cpu1)
 * 2@cpu1     <------>
 * 2@cpu1         <...------->			(requeued to cpu1)
 * 3@cpu1     <------>
 * 3@cpu1         <...------->			(requeued to cpu0)
 * 4@cpu0                     <-->
 * 4@cpu1                           <-->	(requeued to cpu1)
 */
static const struct work_spec work_spec9[] = {
	{
		.id		= 0,
		.wq_id		= 5,
		.cpu		= 0,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s, step A, target cpu0" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_REQUEUE,	{ 0 }, 0 },
			{ ACT_LOG,	{ .s = "sleeping for 2s, step B" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.wq_id		= 5,
		.cpu		= 0,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s, step A, target cpu1" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_REQUEUE,	{ 0 }, 1 },
			{ ACT_LOG,	{ .s = "sleeping for 2s, step B" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.wq_id		= 5,
		.cpu		= 1,
		.initial_delay	= 2000,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s, step A, target cpu1" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_REQUEUE,	{ 1 }, 1 },
			{ ACT_LOG,	{ .s = "sleeping for 2s, step B" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.wq_id		= 5,
		.cpu		= 1,
		.initial_delay	= 2000,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 2s, step A, target cpu0" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_REQUEUE,	{ 1 }, 0 },
			{ ACT_LOG,	{ .s = "sleeping for 2s, step B" }},
			{ ACT_SLEEP,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 4,
		.wq_id		= 5,
		.cpu		= 0,
		.initial_delay	= 10000,
		.requeue_cnt	= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping for 1s, step A, target cpu1" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_REQUEUE,	{ 2000 }, 1 },
			{ ACT_LOG,	{ .s = "sleeping for 1s, step B" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario9 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec9, NULL },
};

/*
 * Scenario 10.  Scenario to test high priority workqueue.
 *
 *     0  0.5  1  1.5  2  |
 * 0l  <-------~~f4~~     | ->
 * 1l  <.......~~         | ~~...---->
 * 2l  <.......~~         | ~~........---->
 * 3l      <...~~         | ~~.............---->
 * 4h      <---~~         | ->
 * 5h              <---~~ | ~~-->
 * 6h              <---~~ | ~~-->
 * 7h              <---~~ | ~~-->
 */
static const struct work_spec work_spec10[] = {
	{
		.id		= 0,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_FLUSH,	{ 4 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.wq_id		= 0,
		.cpu		= 0,
		.initial_delay	= 500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s from 500ms" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 4,
		.wq_id		= 6,
		.cpu		= 0,
		.requeue_cnt	= 1,
		.initial_delay	= 500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "highpri burning for 2s from 500ms" }},
			{ ACT_BURN,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.wq_id		= 6,
		.cpu		= 0,
		.requeue_cnt	= 1,
		.initial_delay	= 1500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "highpri burning for 2s from 1500ms" }},
			{ ACT_BURN,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 6,
		.wq_id		= 6,
		.cpu		= 0,
		.requeue_cnt	= 1,
		.initial_delay	= 1500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "highpri burning for 2s from 1500ms" }},
			{ ACT_BURN,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 7,
		.wq_id		= 6,
		.cpu		= 0,
		.requeue_cnt	= 1,
		.initial_delay	= 1500,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "highpri burning for 2s from 1500ms" }},
			{ ACT_BURN,	{ 2000 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario10 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec10, NULL },
};

/*
 * Scenario 11.  Scenario to test cpu intensive workqueue.
 *
 *     0      
 * 0l  <---~~--->
 * 1l  <...~~    <------>
 * 2c  <...~~            <--~~-->
 * 3c  <...~~            <--~~-->
 * 4l  <...~~            <--~~-->  
 * 5l  <...~~                ~~..------>
 * 6ch <---~~--->
 */
static const struct work_spec work_spec11[] = {
	{
		.id		= 0,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.wq_id		= 7,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s, cpu intensive" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 3,
		.wq_id		= 7,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s, cpu intensive" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 4,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 5,
		.wq_id		= 0,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 6,
		.wq_id		= 8,
		.cpu		= 0,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning for 1s" }},
			{ ACT_BURN,	{ 1000 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario11 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec11, NULL },
};

static const struct work_spec work_spec12[] = {
	{
		.id		= 0,
		.cpu		= 1,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "burning 10s" }},
			{ ACT_BURN,	{ 10000 }},
			{ ACT_LOG,	{ .s = "done burning, sleeping 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_LOG,	{ .s = "burning 3s" }},
			{ ACT_BURN,	{ 3000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 1,
		.cpu		= 1,
		.initial_delay	= 100,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_LOG,	{ .s = "burning 3s" }},
			{ ACT_BURN,	{ 3000 }},
			{ ACT_TERM },
		},
	},
	{
		.id		= 2,
		.cpu		= 1,
		.initial_delay	= 100,
		.actions	= (const struct work_action[]) {
			{ ACT_LOG,	{ .s = "sleeping 1s" }},
			{ ACT_SLEEP,	{ 1000 }},
			{ ACT_LOG,	{ .s = "burning 3s" }},
			{ ACT_BURN,	{ 3000 }},
			{ ACT_TERM },
		},
	},
	{ .id = -1 },
};

static const struct test_scenario scenario12 = {
	.wq_spec		= dfl_wq_spec,
	.work_spec		=
	(const struct work_spec *[]) { work_spec12, NULL },
};

static const struct test_scenario *scenarios[] = {
	&scenario0, &scenario1, &scenario2, &scenario3, &scenario4, &scenario5,
	&scenario6, &scenario7, &scenario8, &scenario9, &scenario10, &scenario11,
	&scenario12,
};

/*
 * Execute
 */
static struct task_struct *sequencer;
static char wq_names[MAX_WQS][MAX_WQ_NAME];
static struct workqueue_struct *wqs[MAX_WQS];
static struct delayed_work dworks[MAX_WORKS];
#ifdef CONFIG_LOCKDEP
static struct lock_class_key wq_lockdep_keys[MAX_WORKS];
static struct lock_class_key dwork_lockdep_keys[MAX_WORKS];
#endif
static const struct work_spec *work_specs[MAX_WORKS];
static bool sleeping[MAX_WORKS];
static wait_queue_head_t wait_heads[MAX_WORKS];
static int requeue_cnt[MAX_WORKS];

static void test_work_fn(struct work_struct *work)
{
	int id = to_delayed_work(work) - dworks;
	const struct work_spec *spec = work_specs[id];
	const struct work_action *act = spec->actions;
	int rc;

#define pd(lvl, fmt, args...)	\
	printk("w%02d/%04d@%d "lvl": "fmt"\n", id, current->pid, raw_smp_processor_id() , ##args);
#define plog(fmt, args...)	pd("LOG ", fmt , ##args)
#define pinfo(fmt, args...)	pd("INFO", fmt , ##args)
#define perr(fmt, args...)	pd("ERR ", fmt , ##args)
repeat:
	switch (act->action) {
	case ACT_TERM:
		pinfo("TERM");
		return;
	case ACT_LOG:
		plog("%s", act->s);
		break;
	case ACT_BURN:
		mdelay(act->v);
		break;
	case ACT_SLEEP:
		sleeping[id] = true;
		wait_event_timeout(wait_heads[id], !sleeping[id],
				   msecs_to_jiffies(act->v));
		if (!sleeping[id])
			pinfo("somebody woke me up");
		sleeping[id] = false;
		break;
	case ACT_WAKEUP:
		if (act->v < MAX_WORKS && sleeping[act->v]) {
			pinfo("waking %lu up", act->v);
			sleeping[act->v] = false;
			wake_up(&wait_heads[act->v]);
		} else
			perr("trying to wake up non-sleeping work %lu",
			     act->v);
		break;
	case ACT_REQUEUE:
		if (requeue_cnt[id] > 0 || requeue_cnt[id] < 0) {
			int delay = act->v, cpu = act->v1;

			get_online_cpus();

			if (cpu >= nr_cpu_ids || !cpu_online(cpu))
				cpu = raw_smp_processor_id();

			if (delay)
				queue_delayed_work_on(cpu, wqs[spec->wq_id],
						      &dworks[id],
						      msecs_to_jiffies(delay));
			else
				queue_work_on(cpu, wqs[spec->wq_id],
					      &dworks[id].work);

			pinfo("requeued on cpu%d delay=%dmsecs", cpu, delay);
			if (requeue_cnt[id] > 0)
				requeue_cnt[id]--;

			put_online_cpus();
		} else
			pinfo("requeue limit reached");

		break;
	case ACT_FLUSH:
		if (act->v < MAX_WORKS && work_specs[act->v]) {
			pinfo("flushing work %lu", act->v);
			rc = flush_work(&dworks[act->v].work);
			pinfo("flushed work %lu, rc=%d", act->v, rc);
		} else
			perr("trying to flush non-existent work %lu", act->v);
		break;
	case ACT_FLUSH_WQ:
		if (act->v < MAX_WQS && wqs[act->v]) {
			pinfo("flushing workqueue %lu", act->v);
			flush_workqueue(wqs[act->v]);
			pinfo("flushed workqueue %lu", act->v);
		} else
			perr("trying to flush non-existent workqueue %lu",
			     act->v);
		break;
	case ACT_CANCEL:
		if (act->v < MAX_WORKS && work_specs[act->v]) {
			pinfo("canceling work %lu", act->v);
			rc = cancel_delayed_work_sync(&dworks[act->v]);
			pinfo("canceled work %lu, rc=%d", act->v, rc);
		} else
			perr("trying to cancel non-existent work %lu", act->v);
		break;
	}
	act++;
	goto repeat;
}

#define for_each_work_spec(spec, i, scenario)				\
	for (i = 0, spec = scenario->work_spec[i]; spec;		\
	     spec = (spec + 1)->id >= 0 ? spec + 1 : scenario->work_spec[++i])

#ifdef CONFIG_WORKQUEUE_DEBUGFS
#include <linux/seq_file.h>

static bool show_work(struct seq_file *m, struct work_struct *work, bool running)
{
	struct delayed_work *dwork = to_delayed_work(work);

	if (running)
		return false;

	seq_printf(m, "test work %ld%s", dwork - dworks, running ? " R" : "");
	return true;
}
#endif

static int sequencer_thread(void *__scenario)
{
	const struct test_scenario *scenario = __scenario;
	const struct wq_spec *wq_spec;
	const struct work_spec *work_spec;
	int i, id;

	for (wq_spec = scenario->wq_spec; wq_spec->id >= 0; wq_spec++) {
		if (wq_spec->id >= MAX_WQS) {
			printk("ERR : wq id %d too high\n", wq_spec->id);
			goto err;
		}

		if (wqs[wq_spec->id]) {
			printk("ERR : wq %d already occupied\n", wq_spec->id);
			goto err;
		}

		snprintf(wq_names[wq_spec->id], MAX_WQ_NAME, "test-wq-%02d",
			 wq_spec->id);
		wqs[wq_spec->id] = __alloc_workqueue_key(wq_names[wq_spec->id],
						wq_spec->flags,
						wq_spec->max_active,
						&wq_lockdep_keys[wq_spec->id],
						wq_names[wq_spec->id]);
		if (!wqs[wq_spec->id]) {
			printk("ERR : failed create wq %d\n", wq_spec->id);
			goto err;
		}
#ifdef CONFIG_WORKQUEUE_DEBUGFS
		workqueue_set_show_work(wqs[wq_spec->id], show_work);
#endif
	}

	for_each_work_spec(work_spec, i, scenario) {
		struct delayed_work *dwork = &dworks[work_spec->id];
		struct workqueue_struct *wq = wqs[work_spec->wq_id];

		if (work_spec->id >= MAX_WORKS) {
			printk("ERR : work id %d too high\n", work_spec->id);
			goto err;
		}

		if (!wq) {
			printk("ERR : work %d references non-existent wq %d\n",
			       work_spec->id, work_spec->wq_id);
			goto err;
		}

		if (work_specs[work_spec->id]) {
			printk("ERR : work %d already initialized\n",
			       work_spec->id);
			goto err;
		}
		INIT_DELAYED_WORK(dwork, test_work_fn);
#ifdef CONFIG_LOCKDEP
		lockdep_init_map(&dwork->work.lockdep_map, "test-dwork",
				 &dwork_lockdep_keys[work_spec->id], 0);
#endif
		work_specs[work_spec->id] = work_spec;
		init_waitqueue_head(&wait_heads[work_spec->id]);
		requeue_cnt[work_spec->id] = work_spec->requeue_cnt ?: -1;
	}

	for_each_work_spec(work_spec, i, scenario) {
		struct delayed_work *dwork = &dworks[work_spec->id];
		struct workqueue_struct *wq = wqs[work_spec->wq_id];
		int cpu;

		get_online_cpus();

		if (work_spec->cpu < nr_cpu_ids && cpu_online(work_spec->cpu))
			cpu = work_spec->cpu;
		else
			cpu = raw_smp_processor_id();

		if (work_spec->initial_delay)
			queue_delayed_work_on(cpu, wq, dwork,
				msecs_to_jiffies(work_spec->initial_delay));
		else
			queue_work_on(cpu, wq, &dwork->work);

		put_online_cpus();
	}

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);

err:
	for (id = 0; id < MAX_WORKS; id++)
		if (work_specs[id])
			cancel_delayed_work_sync(&dworks[id]);
	for (id = 0; id < MAX_WQS; id++)
		if (wqs[id])
			destroy_workqueue(wqs[id]);

	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}

	return 0;
}

static int scenario_switch = -1;
module_param_named(scenario, scenario_switch, int, 0444);

static int flush_on_down = 0;
module_param_named(flush_on_down, flush_on_down, int, 0444);

static int __cpuinit test_cpu_callback(struct notifier_block *nfb,
				       unsigned long action, void *hcpu)
{
	int id;

	action &= ~CPU_TASKS_FROZEN;

	switch (action) {
	case CPU_DOWN_PREPARE:
		if (flush_on_down) {
			printk("INFO: CPU_DOWN_PREPARE: flushing work items\n");
			for (id = 0; id < MAX_WORKS; id++)
				if (work_specs[id])
					flush_delayed_work_sync(&dworks[id]);
			printk("INFO: CPU_DOWN_PREPARE: flushing done\n");
		}
		break;
	}

	return 0;
}

static struct notifier_block test_cpu_nb __cpuinitdata =
	{ .notifier_call = test_cpu_callback };

static int __init test_init(void)
{
	if (scenario_switch < 0 || scenario_switch >= ARRAY_SIZE(scenarios)) {
		printk("TEST WQ - no such scenario\n");
		return -EINVAL;
	}

	register_cpu_notifier(&test_cpu_nb);

	printk("TEST WQ - executing scenario %d\n", scenario_switch);
	sequencer = kthread_run(sequencer_thread,
				(void *)scenarios[scenario_switch],
				"test-wq-sequencer");
	if (IS_ERR(sequencer)) {
		unregister_cpu_notifier(&test_cpu_nb);
		return PTR_ERR(sequencer);
	}
	return 0;
}

static void __exit test_exit(void)
{
	kthread_stop(sequencer);
	unregister_cpu_notifier(&test_cpu_nb);
}

module_init(test_init);
module_exit(test_exit);
MODULE_LICENSE("GPL");
