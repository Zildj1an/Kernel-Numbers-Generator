#include "modtimer2.h"

///* UPPER LAYER FUNCTIONS For user interaction (/proc entries) */

int *odd; /* To distinguish between consumers */

ssize_t modconfig_write(struct file *f,const char *b, size_t s, loff_t *l){

	char kbuf[KBUF_SIZE];
	int n;

	if(s > KBUF_SIZE) return -EFBIG;
	if (copy_from_user(kbuf, b, s) > 0) return -EINVAL;
	if((*l > 0)) return 0;

	if(sscanf(kbuf, "timer_period_ms %d", &n) == 1) timer_period_ms = n;
	else if (sscanf(kbuf, "emergency_threshold %d", &n) == 1) emergency_threshold = n;
	else if (sscanf(kbuf, "max_random %d", &n) == 1) max_random = n;
	else return -EINVAL;

	(*l) += s;

	return s;
}

ssize_t modconfig_read(struct file *f, char __user *b, size_t l, loff_t *o){

	char kbuf[KBUF_SIZE];
	int read,t,e,m;

	if ((*o) > 0) return 0; //Previously invoked!

	t = timer_period_ms; e = emergency_threshold; m = max_random;
	read = sprintf(kbuf,"timer_period_ms = %i\nemergency_threshold = %i\nmax_random = %i\n",t,e,m);

	if (copy_to_user(b, kbuf, read)) return -EFAULT;
	(*o) += read;

 return read;
}

/* Auxiliar function - return number of elements*/
int print_list(struct list_head* list, char members[]){

	struct list_item* item = NULL;
	struct list_head* cur_node = NULL;
	unsigned int read = 0, num;

	list_for_each(cur_node, list) { /* while cur_node != list*/

		item = list_entry(cur_node, struct list_item, links);
		if(read < KBUF_SIZE){
			num = (item->data);
			read += sprintf(&members[read], "%u\n",num);
		}
		else return -ENOMEM;
	}

	return read;
}

ssize_t modtimer_read(struct file *f, char __user *b, size_t l, loff_t *o){

	char kbuf[KBUF_SIZE];
	int read = 0;
	unsigned long flags;
	struct list_head* node;

	spin_lock_irqsave(&lock_cons, flags);

	/* Depending if it is the even or the odd digits reader */
	if(f->private_data == NULL) node = &even_ghost_node;
	else node = &odd_ghost_node;

	while(list_empty(node)){
		cons_waiting++;
		spin_unlock_irqrestore(&lock_cons, flags);
		if(down_interruptible(&sem_cons)) return -EINTR;
	}

	spin_lock_irqsave(&lock_cons,flags);

	if(down_interruptible(&sem_list)) return -EINTR;

	if((read = print_list(node, kbuf)) < 0) return -ENOMEM;
	removeList(node);

	up(&sem_list);

	spin_unlock_irqrestore(&lock_cons, flags);

	if (copy_to_user(b, kbuf, read)) return -EFAULT;

 return read;
}

int modtimer_open(struct inode *i, struct file *f){

	odd = vmalloc(sizeof(int));

	if(module_refcount(THIS_MODULE) == 0){
		try_module_get(THIS_MODULE); /* Increase module reference counter */
		while(module_refcount(THIS_MODULE) == 1)
		 	if(down_interruptible(&sem_cons)) return -EINTR;
	}
	else {
		f->private_data = odd;
		/* Kernel Timer initialization */
	    	init_timer(&my_timer);
	    	my_timer.data = 0;
		my_timer.function = fire_timer;
		my_timer.expires = jiffies + timer_period_ms;  /* Active now + timer_period_ms */
		add_timer(&my_timer); /* Timer on */

		INIT_WORK(&my_work, copy_items_into_list);
		INIT_LIST_HEAD(&even_ghost_node);
		INIT_LIST_HEAD(&odd_ghost_node);

		if(kfifo_alloc(&cbuffer, CBUF_SIZE, GFP_KERNEL)){
			printk(KERN_INFO "Couldn't create the circular buffer \n");
			return -ENOMEM;
		}
		try_module_get(THIS_MODULE); /* Increase module reference counter */
		up(&sem_cons);
	}

 	return 0;
}

int modtimer_release(struct inode *i, struct file *f){

    module_put(THIS_MODULE); /* Decrease module reference counter */

    if(module_refcount(THIS_MODULE) == 0){
	    del_timer_sync(&my_timer);
	    flush_workqueue(my_wq);
	    kfifo_free(&cbuffer);
	    removeList(&odd_ghost_node);
	    removeList(&even_ghost_node);
	    flagWork = 0;
	    vfree(odd);
    }

 return 0;
}

/* TOP-HALF FUNCTIONS
   Kernel timer, manages the circular buffer */
void fire_timer(unsigned long data){

	unsigned long flags;
	unsigned int ret;

         spin_lock_irqsave(&lock_buff, flags);

	if(elemsBuff < emergency_threshold){
		ret = get_random_int();
		kfifo_in(&cbuffer,&ret,sizeof(int));
		elemsBuff++;
	}

	/* flagWork indicates if the previous enqueued work is done */
	if(elemsBuff == emergency_threshold && !flagWork){
		queue_work(my_wq, &my_work);
		flagWork = 1;
	}

	spin_unlock_irqrestore(&lock_buff, flags);

	mod_timer(&(my_timer), jiffies + timer_period_ms);
}

/* BOTTOM-HALF FUNCTIONS
   Workqueue, manages the linked list */
void copy_items_into_list(struct work_struct *work){

	char kbuf[KBUF_SIZE],*aux;
	struct list_item *new_item = NULL;
	int i, size;
	unsigned long flags;

	spin_lock_irqsave(&lock_buff, flags);

	size = (kfifo_len(&cbuffer) > KBUF_SIZE)? KBUF_SIZE : kfifo_len(&cbuffer);
	size = kfifo_out(&cbuffer,kbuf, size);

	spin_unlock_irqrestore(&lock_buff, flags);

	/* Semaphore to protect the list */
	if(down_interruptible(&sem_list)) printk(KERN_INFO "Error copying items into list\n");

	aux = kbuf;
	for(i = 0; i < elemsBuff; ++i){
		new_item = vmalloc(sizeof(struct list_item));
		new_item->data = *aux;
		aux += sizeof(int);
		new_item->data = new_item->data % max_random; /* MAX_RANDOM */

		if(new_item->data % 2 == 0)
			list_add_tail(&new_item->links,&even_ghost_node);
		else
			list_add_tail(&new_item->links,&odd_ghost_node);
	}

	elemsBuff = 0;

	if(cons_waiting){
		cons_waiting--;
		up(&sem_cons);
	}

	up(&sem_list);

	flagWork = 0;
}

void removeList(struct list_head* ghost_node){

	struct list_head* cur_node = NULL;
         struct list_head* aux 	   = NULL;
	struct list_item* item     = NULL;

	list_for_each_safe(cur_node, aux, ghost_node) {

	        item = list_entry(cur_node, struct list_item, links);
        	list_del(&item->links);
		vfree(item);
	}
}

const struct file_operations fops1 = {
        .read  = modconfig_read,
        .write = modconfig_write,
};

const struct file_operations fops2 = {
        .read    = modtimer_read,
        .open    = modtimer_open,
        .release = modtimer_release,
};

int modtim_init(void){

	proc_modconfig = proc_create("modconfig", 0666, NULL, &fops1); //Create proc entry modconfig
	proc_modtimer = proc_create("modtimer", 0666, NULL, &fops2); //Create proc entry modtimer

	if((proc_modconfig == NULL) || (proc_modtimer == NULL)){
		printk(KERN_INFO "Couldn't create the /proc entries\n");
		goto nomem;
	}
	else {
		sema_init(&sem_cons, 0);
		sema_init(&sem_list,1);
		/* kernel timer and Linked list initialization at modtimer_open
		Circular buffer memory allocated at modtimer_open ,initialized at modtimer_time
		I made this to avoid problems closing cat /proc/modtimer (modtimer_release) and openning it again
		*/

		/* Create workqueue structure */
		my_wq = create_workqueue("my_queue");
		if(!my_wq) goto nomem;
  	    	printk(KERN_INFO "Module %s succesfully charged. \n", MODULE_NAME);
	}

 	return 0;
nomem:
 	return -ENOMEM;
}

void modtim_exit(void){

	while(module_refcount(THIS_MODULE) > 0){} /* Module won't be unloaded while a program is using it */

	kfifo_free(&cbuffer);
	removeList(&even_ghost_node);
	removeList(&odd_ghost_node);
	remove_proc_entry("modconfig", NULL);
	remove_proc_entry("modtimer", NULL);
	flush_workqueue(my_wq); /* Wait for jobs to finish */
	destroy_workqueue(my_wq);
	/* Wait for completion of the timer func (if running) + delete */
  	del_timer_sync(&my_timer);
	printk(KERN_INFO "Module %s disconnected \n", MODULE_NAME);
}

module_init(modtim_init);
module_exit(modtim_exit);
