#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/kfifo.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/random.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Carlos Bilbao");
MODULE_DESCRIPTION("SMP-Safe Linux kernel module - random nums generator");

#define CBUF_SIZE		32 			/* 8 integers */
#define KBUF_SIZE	    	256
#define MODULE_NAME		"modtimer"

//*-------------------- RESOURCES --------------------*//

/* Upper Layer resources */

static struct proc_dir_entry *proc_modconfig;
static struct proc_dir_entry *proc_modtimer;
DEFINE_SPINLOCK(lock_cons); /* Only one reader */
struct semaphore sem_cons; /* Sempahore for the consumer */
static int cons_waiting = 0;

/* Top-Half resources (interruption context) */

static int timer_period_ms 	=  100;
static int max_random 		=  300;
static int emergency_threshold 	=  5;  /* When reached, activates differed work */
struct timer_list my_timer; /* kernel timer */

struct kfifo cbuffer;
DEFINE_SPINLOCK(lock_buff);
static int elemsBuff = 0;

/* Bottom-Half resources (kernel thread)*/

static struct workqueue_struct* my_wq; /* Workqueue descriptor */
struct work_struct my_work; /* Work descriptor */
static int flagWork = 0; /* 1 if work yet to be done */

struct list_item {
	unsigned int data;
	struct list_head links;
};

struct semaphore sem_list;
struct list_head even_ghost_node;
struct list_head odd_ghost_node;

//*-------------------- FUNCTIONS --------------------*//

int modtim_init(void);
void modtim_exit(void);

/* UPPER LAYER FUNCTIONS
   For user interaction (/proc entries) */

ssize_t modconfig_write(struct file *f,const char *b, size_t s, loff_t *l);
ssize_t modconfig_read(struct file *f, char __user *b, size_t l, loff_t *o);
ssize_t modtimer_read(struct file *f, char __user *b, size_t l, loff_t *o);
void getElement(struct list_head* list, char members[]);
int modtimer_open(struct inode *i, struct file *f);
int modtimer_release(struct inode *i, struct file *f);

/* TOP-HALF FUNCTIONS
   Kernel timer, manages the circular buffer */

void fire_timer(unsigned long data);

/* BOTTOM-HALF FUNCTIONS
   Workqueue, manages the linked list */
void copy_items_into_list(struct work_struct *work);

void removeList(struct list_head* ghost_node);
