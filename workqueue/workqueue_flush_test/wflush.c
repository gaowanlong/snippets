#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/random.h>

struct cookie_struct {
	struct list_head head;
	unsigned long cookie;
};

static DEFINE_MUTEX(cookie_lock);
static unsigned long test_cookie;
static LIST_HEAD(cookie_head);

static unsigned long last_cookie(void)
{
	unsigned long cookie;

	mutex_lock(&cookie_lock);
	cookie = test_cookie - 1; /* c->cookie = test_cookie++; */
	mutex_unlock(&cookie_lock);

	return cookie;
}

static unsigned long tip_cookie(void)
{
	unsigned long cookie;

	mutex_lock(&cookie_lock);
	if (list_empty(&cookie_head))
		cookie = test_cookie;
	else
		cookie = list_first_entry(&cookie_head, struct cookie_struct, head)->cookie;
	mutex_unlock(&cookie_lock);

	return cookie;
}

struct test_work {
	struct work_struct w;
	struct cookie_struct c;
};

static void add_test_work(struct test_work *t)
{
	struct cookie_struct *c = &t->c;

	mutex_lock(&cookie_lock);
	c->cookie = test_cookie++;
	BUG_ON(!list_empty(&c->head));
	list_add_tail(&c->head, &cookie_head);
	schedule_work(&t->w);
	mutex_unlock(&cookie_lock);

	udelay(1+random32()%50);
}

static void test_work_fn(struct work_struct *w)
{
	struct test_work *t = container_of(w, struct test_work, w);

	mutex_lock(&cookie_lock);
	list_del_init(&t->c.head);
	mutex_unlock(&cookie_lock);

	udelay(1+random32()%50);
}

static int test_thread(void *arg)
{
	unsigned long lcookie, tcookie;
	struct test_work t[10];
	int i;

	for (i = 0; i < ARRAY_SIZE(t); i++) {
		INIT_WORK_ONSTACK(&t[i].w, test_work_fn);
		INIT_LIST_HEAD(&t[i].c.head);
	}

	do {
		for (i = 0; i < ARRAY_SIZE(t); i++) {
			add_test_work(t+i);
		}

		lcookie = last_cookie();
		flush_scheduled_work();
		tcookie = tip_cookie();
		BUG_ON((long)(tcookie - lcookie) <= 0);
	} while (!kthread_should_stop());
	return 0;
}

static struct task_struct *test_tasks[100];

static int test_init(void)
{
	int i;

	printk(KERN_ERR "wflush test start\n");
	for (i = 0; i < ARRAY_SIZE(test_tasks); i++) {
		test_tasks[i] = kthread_run(test_thread, NULL, "wflush");
		if (!test_tasks[i]) {
			printk(KERN_ERR "wflush create fail %d\n", i);
			break;
		}
	}

	return 0;
}
module_init(test_init);

static void test_exit(void)
{
	int i;

	printk(KERN_ERR "wflush test stopping\n");
	for (i = 0; i < ARRAY_SIZE(test_tasks); i++) {
		if (test_tasks[i])
			kthread_stop(test_tasks[i]);
	}
	printk(KERN_ERR "wflush test stopped\n");
}
module_exit(test_exit);

MODULE_LICENSE("GPL");
