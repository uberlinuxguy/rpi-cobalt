
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/platform.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/export.h>



/*kprobe_example.c*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>

#include <linux/device.h>
#include <linux/err.h>

#include <linux/netdevice.h>
#include <linux/rtnetlink.h>

#include <linux/kthread.h>

MODULE_LICENSE("GPL");
#define hdd_led 22
#define txrx_led 8
#define col_led 23
#define link_led 24
#define speed_led 25
#define web_led = 17

struct GpioRegisters
{
	uint32_t GPFSEL[6];
	uint32_t Reserved1;
	uint32_t GPSET[2];
	uint32_t Reserved2;
	uint32_t GPCLR[2];
};

struct GpioRegisters *s_pGpioRegisters;
static struct net_device *eth0_dev;

static unsigned long net_packets;
static unsigned long net_collisions;
//static struct task_struct *net_get_speed_thread;
static void ngs_work_handler(struct work_struct *w);

static struct workqueue_struct *ngs_wq = 0;
static DECLARE_DELAYED_WORK(ngs_work, ngs_work_handler);

static void SetGPIOFunction(int GPIO, int functionCode)
{
	int registerIndex = GPIO / 10;
	int bit = (GPIO % 10) * 3;

	unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
	unsigned mask = 0b111 << bit;
	printk("Changing function of GPIO%d from %x to %x\n", GPIO, (oldValue >> bit) & 0b111, functionCode);
	s_pGpioRegisters->GPFSEL[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
}

static void SetGPIOOutputValue(int GPIO, bool outputValue)
{
	if (outputValue)
		s_pGpioRegisters->GPSET[GPIO / 32] = (1 << (GPIO % 32));
	else
		s_pGpioRegisters->GPCLR[GPIO / 32] = (1 << (GPIO % 32));
}

static DECLARE_WAIT_QUEUE_HEAD(net_wq);
static int net_rd_flag;
static int no_net = 0;

//int net_getspeed_thread_fn(void *data){
static void ngs_work_handler(struct work_struct *w) {
	int ret = 0;

		if(eth0_dev != NULL){
			if (rtnl_trylock()){
				if (netif_running(eth0_dev) &&
						eth0_dev->ethtool_ops &&
						eth0_dev->ethtool_ops->get_settings) {
				
					struct ethtool_cmd cmd = { ETHTOOL_GSET };
					
					if (!eth0_dev->ethtool_ops->get_settings(eth0_dev, &cmd))
						ret = ethtool_cmd_speed(&cmd);
					
					if(ret >= 100 ){
						SetGPIOOutputValue(speed_led, 0);
					}else{
						SetGPIOOutputValue(speed_led, 1);
					}
				}else {
					 if(!no_net) {
						printk(KERN_INFO "no lock");
						no_net=1;
					}
				}

			rtnl_unlock();
			} else {
				if(!no_net) {
					printk(KERN_INFO "no lock");
					no_net=1;
				}
			}
		} else {
			if(!no_net) {
				printk(KERN_INFO "no net");
				no_net=1;
			}
		}
		net_rd_flag=0;
	
//	return 0;
}


static struct timer_list s_LedRstTimer;
static int s_LedRstPeriod = 10;
static void LedRstHandler(unsigned long unused)
{
	struct rtnl_link_stats64 *netif_stats;
    SetGPIOOutputValue(hdd_led, 1);

	if(eth0_dev != NULL){
		netif_stats = (struct rtnl_link_stats64*)kzalloc(sizeof(struct rtnl_link_stats64),GFP_KERNEL);
		if(netif_stats == NULL ) {
			printk(KERN_ERR "rpi-cobalt: memory allocation error.");
		} else {
			// TODO: Need a check here that eth0_dev didn't go away.
			// This is hardware on the board, but who knows what could 
			// happen.
			dev_get_stats(eth0_dev, netif_stats);
			if(netif_stats->rx_packets != 0){
				unsigned long tmp_packets = netif_stats->rx_packets + netif_stats->tx_packets;
				if(tmp_packets != net_packets) {
					SetGPIOOutputValue(txrx_led, 0);
					net_packets = tmp_packets;
				} else {
					SetGPIOOutputValue(txrx_led, 1);
				}
				if(netif_stats->collisions != net_collisions) {
					SetGPIOOutputValue(col_led, 0);
					net_collisions = netif_stats->collisions;
				} else {
					SetGPIOOutputValue(col_led, 1);
				}
			}
		}
		
		// check the link
		if(eth0_dev->operstate == IF_OPER_UP ){
			// link is up.
			SetGPIOOutputValue(link_led, 0);
		} else {
			SetGPIOOutputValue(link_led, 1);
		}

		// wake up the non-interrupt thread to update the link speed led.
//		net_rd_flag=1;
//		wake_up_process(net_get_speed_thread);
//		net_rd_flag = 1;
//		wake_up_interruptible(&net_wq);
		if(ngs_wq) 
			queue_delayed_work(ngs_wq, &ngs_work, net_rd_flag); 

	} else {
		if(!no_net){
			printk(KERN_ERR "No network dev.");
			no_net=1;
		}
	}

    mod_timer(&s_LedRstTimer, jiffies + msecs_to_jiffies(s_LedRstPeriod));
}




/*For each probe you need to allocate a kprobe structure*/
static struct kprobe kp;
//static struct kprobe kp2;

/*kprobe pre_handler: called just before the probed instruction is executed*/
int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
//	printk("pre_handler: p->addr=0x%p, eip=%lx, eflags=0x%lx\n",
//			p->addr, regs->eip, regs->eflags);
//	dump_stack();
	//printk("pre hit");
	
	SetGPIOOutputValue(hdd_led, 0);
	return 0;
}

/*kprobe post_handler: called after the probed instruction is executed*/
void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
//	printk("post_handler: p->addr=0x%p, eflags=0x%lx\n",
//			p->addr, regs->eflags);
//	SetGPIOOutputValue(hdd_led, 1);
}


/* fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk("fault_handler: p->addr=0x%p, trap #%dn",
				p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}

int rpi_init_module(void)
{
	int ret;
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;
//	kp.addr = (kprobe_opcode_t*) kallsyms_lookup_name("scsi_dispatch_cmd");
	kp.addr = (kprobe_opcode_t*) kallsyms_lookup_name("__blk_run_queue");
	/* register the kprobe now */
	if (!kp.addr) {
		printk("Couldn't find %s to plant kprobe\n", "scsi_dispatch_cmd");
		return -1;
	}
	if ((ret = register_kprobe(&kp) < 0)) {
		printk("register_kprobe failed, returned %d\n", ret);
		return -1;
	}
/*
	kp2.pre_handler = NULL;
	kp2.post_handler = handler_post;
	kp2.fault_handler = handler_fault;
	kp2.addr = (kprobe_opcode_t*) kallsyms_lookup_name("__blk_run_queue");
	if(!kp2.addr) { 
		printk("Couldn't find %s to plant kprobe\n", "__blk_run_queue");
		return -1;
	}
	if ((ret = register_kprobe(&kp2) < 0)){
		printk("register_kprobe failed, returned %d\n", ret);
		return -1;
	}*/
	printk("kprobe registered\n");



	return 0;
}

void rpi_cleanup_module(void)
{
		unregister_kprobe(&kp);
		//unregister_kprobe(&kp2);
		printk("kprobe unregistered\n");
}

static int __init rpicobaltinit(void)
{
	int result = 0;
	struct net_device *dev;

	s_pGpioRegisters = (struct GpioRegisters *)__io_address(GPIO_BASE);
	SetGPIOFunction(hdd_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(txrx_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(col_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(link_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(speed_led, 0b001);	//Configure the pin as output

	SetGPIOOutputValue(hdd_led, 1);	
	SetGPIOOutputValue(txrx_led, 1);	
	SetGPIOOutputValue(col_led, 1);	
	SetGPIOOutputValue(link_led, 1);	
	SetGPIOOutputValue(speed_led, 1);	


	eth0_dev = NULL;
	read_lock(&dev_base_lock);
	dev = first_net_device(&init_net);
	while (dev) {
		if(!strncmp(dev->name, "eth0", 4)) {
			// match, save the pointer.
			eth0_dev = dev;
			printk(KERN_INFO "rpi-cobalt: found [%s]\n", dev->name);
			break;
		}
	    dev = next_net_device(dev);
	}
	read_unlock(&dev_base_lock);

/*	net_get_speed_thread = kthread_run(net_getspeed_thread_fn, NULL, "ngs_thread");
	if(net_get_speed_thread) {
		net_rd_flag=1;
		wake_up_interruptible(&net_wq);
	}*/

	net_rd_flag=0;

	if(!ngs_wq)
		ngs_wq = create_singlethread_workqueue("ngs_read");
	if(ngs_wq)
		queue_delayed_work(ngs_wq, &ngs_work, net_rd_flag);
	
	setup_deferrable_timer_on_stack(&s_LedRstTimer, LedRstHandler, 0);
    result = mod_timer(&s_LedRstTimer, jiffies + msecs_to_jiffies(s_LedRstPeriod));
    BUG_ON(result < 0);


	rpi_init_module();

	return 0;
}

static void __exit rpicobaltexit(void)
{
	rpi_cleanup_module();

	del_timer(&s_LedRstTimer);
	if(ngs_wq){
		cancel_delayed_work_sync(&ngs_work);
		destroy_workqueue(ngs_wq);
	}

//	kthread_stop(net_get_speed_thread);
	SetGPIOFunction(hdd_led, 0);	//Configure the pin as input
	SetGPIOFunction(txrx_led, 0);	//Configure the pin as input
	SetGPIOFunction(col_led, 0);	//Configure the pin as input
	SetGPIOFunction(link_led, 0);	//Configure the pin as input
	SetGPIOFunction(speed_led, 0);	//Configure the pin as input

}

module_init(rpicobaltinit);
module_exit(rpicobaltexit);
