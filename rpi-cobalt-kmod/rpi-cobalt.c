/*
 * rpi-cobalt.ko -	This is a kernel module for the raspberry pi to run a 
 *					Cobalt RaQ front panel status LED set.  
 *
 *	@Author: Jason Williams <uberlinuxguy@gmail.com>
 *
 *	Portions, examples, and some general ideas for this kernel module come 
 *	from the following:
 *
 *	- http://www.opensourceforu.com/2011/04/kernel-debugging-using-kprobe-and-jprobe/
 *	- http://sysprogs.com/VisualKernel/tutorials/raspberry/leddriver/ledblink.c
 *  - http://www.makelinux.net/books/lkd2/ch07lev1sec4
 *
 *
 */

// TODO: Clean up includes, some of these may not be needed.
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

// TODO: make the led pins a variable instead of a define
// and allow them to be set by module args
// Note the comments here about why true is off for certain LEDs.
#define hdd_led 22 // switched ground, true is off
#define txrx_led 8 // switched ground, true is off 
#define col_led 23 // switched ground, true is off
#define link_led 24 // switched ground, true is off
#define speed_led 25 // switched ground, true is off
#define web_led 17 // switched positive, true is on


// GPIO Registers struct
struct GpioRegisters
{
	uint32_t GPFSEL[6];
	uint32_t Reserved1;
	uint32_t GPSET[2];
	uint32_t Reserved2;
	uint32_t GPCLR[2];
};

struct GpioRegisters *s_pGpioRegisters;

// used to save a pointer to eth0.
static struct net_device *eth0_dev;

// save stats in between calls to the timer.
// used for comparing packet counts to blink the
// traffic and collision lights.
static unsigned long net_packets;
static unsigned long net_collisions;

// set up for the work queue to run that updates the link light
static void ngs_work_handler(struct work_struct *w);
static struct workqueue_struct *ngs_wq = 0;
static DECLARE_DELAYED_WORK(ngs_work, ngs_work_handler);

// the timer struct for our led update timer
static struct timer_list s_LedRstTimer;

// set the timer to run every 10 micro(?) seconds
static int s_LedRstPeriod = 10;


/*! SetGPIOFunction - Used to set the GPIO's direction
 *
 * \param GPIO the BOARD GPIO id
 * \param functionCode sets either INPUT(0) or OUTPUT(0b001)
 *
 */
static void SetGPIOFunction(int GPIO, int functionCode)
{

	int registerIndex = GPIO / 10;
	int bit = (GPIO % 10) * 3;

	unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
	unsigned mask = 0b111 << bit;

	s_pGpioRegisters->GPFSEL[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
}

/*! SetGPIOOutputValue - Used to set the output value of a GPIO pin
 *
 * \param GPIO the BOARD GPIO id
 * \param outputValue Set the pin to On(1) or Off(0)
 *
 */

static void SetGPIOOutputValue(int GPIO, bool outputValue)
{

	if (outputValue)
		s_pGpioRegisters->GPSET[GPIO / 32] = (1 << (GPIO % 32));
	else
		s_pGpioRegisters->GPCLR[GPIO / 32] = (1 << (GPIO % 32));
}


/*! ngs_work_handler - work queue callback 
 *
 * \param w the work struct to use.
 *
 */
static void ngs_work_handler(struct work_struct *w) {


	struct rtnl_link_stats64 *netif_stats;

	int ret = 0;
	
	// check for no eth0 device.
	if(eth0_dev != NULL){
		// check the link, get it from operstate
		if(eth0_dev->operstate == IF_OPER_UP ){
			// link is up.
			SetGPIOOutputValue(link_led, 0);
			// if we have an eth0, allocate some memory for the net stats
			netif_stats = (struct rtnl_link_stats64*)kzalloc(sizeof(struct rtnl_link_stats64),GFP_KERNEL);
			if(netif_stats == NULL ) {
				printk(KERN_ERR "rpi-cobalt: memory allocation error.");
			} else {
				// TODO: Need a check here that eth0_dev didn't go away.
				// This is hardware on the board, but who knows what could 
				// happen.
				dev_get_stats(eth0_dev, netif_stats);
				// if there are stats for eth0, process them
				if(netif_stats->rx_packets != 0){
					// if the sum of the rx and tx packets changes since the last run
					// turn the led on
					unsigned long tmp_packets = netif_stats->rx_packets + netif_stats->tx_packets;
					if(tmp_packets != net_packets) {
						SetGPIOOutputValue(txrx_led, 0);
						net_packets = tmp_packets;
					} else {
						// turn the txrx led off, no traffic moved since
						// the last run.
						SetGPIOOutputValue(txrx_led, 1);
					}

					// collision light works the same as the tx/rx light
					if(netif_stats->collisions != net_collisions) {
						SetGPIOOutputValue(col_led, 0);
						net_collisions = netif_stats->collisions;
					} else {
						SetGPIOOutputValue(col_led, 1);
					}
				}
			}
		} else {
			SetGPIOOutputValue(link_led, 1);
			SetGPIOOutputValue(txrx_led, 1);
			SetGPIOOutputValue(col_led, 1);
		}
		
		// set the lock for the rtnl layer
		if (rtnl_trylock()){
			// make sure the device is runing, 
			// has ethtool_ops and has a 
			// get_settings function set
			if (netif_running(eth0_dev) &&
					eth0_dev->ethtool_ops &&
					eth0_dev->ethtool_ops->get_settings) {
			
				// get the interface settings via ethtool
				struct ethtool_cmd cmd = { ETHTOOL_GSET };
				if (!eth0_dev->ethtool_ops->get_settings(eth0_dev, &cmd))
					ret = ethtool_cmd_speed(&cmd);
				
				// check the link speed and set the 100M led on
				// if the speed is over 100
				if(ret >= 100 ){
					SetGPIOOutputValue(speed_led, 0);
				}else{
					SetGPIOOutputValue(speed_led, 1);
				}

			}
		}
		// release the rtnl lock
		rtnl_unlock();
	}
	
}

/*! LedRstHandler - the timer callback for led processing
 *
 * \param unused an unused long int that needs to be part of the timer handler
 *
 *
 */
static void LedRstHandler(unsigned long unused)
{
	
	
	// turn off the hdd led.  It could have been turned on by the kprobe below.
    SetGPIOOutputValue(hdd_led, 1);
	
	// check for no eth0 device.
	if(eth0_dev != NULL){
		// check the link, get it from operstate
		if(eth0_dev->operstate == IF_OPER_UP ){
			// add the work queue handler call to the work queue.	
			if(ngs_wq) 
				queue_delayed_work(ngs_wq, &ngs_work, 0); 
		}
	}

	// re-add the timer so that we run again
    mod_timer(&s_LedRstTimer, jiffies + msecs_to_jiffies(s_LedRstPeriod));
}

// kprobe structure for the probe into the block system
static struct kprobe kp;

/*! handler_pre - precall handler for the kprobe
 *
 */
int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	// turn the hdd led on if the probe hits, because 
	// that means a block transaction is in flight somewhere
	SetGPIOOutputValue(hdd_led, 0);
	return 0;
}


/* fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk("fault_handler: p->addr=0x%p, trap #%dn",
				p->addr, trapnr);
	// Return 0 because we don't handle the fault. 
	return 0;
}

/*! rpi_init_module - one of the init routines, sets up the kprobe.
 *
 *
 */

int rpi_init_module(void)
{
	int ret;

	// set the handlers on the kprobe.
	kp.pre_handler = handler_pre;
	kp.post_handler = NULL;
	kp.fault_handler = handler_fault;

	// attaching to blk_run_queue will allow the disk light to flash
	// regardless of which disk has activity.
	kp.addr = (kprobe_opcode_t*) kallsyms_lookup_name("__blk_run_queue");

	// if we didn't find the sym we are looking for, 
	// don't try to setup the probe.
	if (!kp.addr) {
		printk("Couldn't find %s to plant kprobe\n", "__blk_run_queue");
		return -1;
	}

	// attempt the probe register
	if ((ret = register_kprobe(&kp) < 0)) {
		printk("register_kprobe failed, returned %d\n", ret);
		return -1;
	}

	return 0;
}

/*! rpi_cleanup_module - unregisters the kprobe.
 *
 */
void rpi_cleanup_module(void)
{
		unregister_kprobe(&kp);
}

/*! rpicobaltinit - the actual module init/entry
 *
 */

static int __init rpicobaltinit(void)
{
	int result = 0;
	struct net_device *dev;

	// setup all the leds as output
	s_pGpioRegisters = (struct GpioRegisters *)__io_address(GPIO_BASE);
	SetGPIOFunction(hdd_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(txrx_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(col_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(link_led, 0b001);	//Configure the pin as output
	SetGPIOFunction(speed_led, 0b001);	//Configure the pin as output

	// turn them all off.
	SetGPIOOutputValue(hdd_led, 1);	
	SetGPIOOutputValue(txrx_led, 1);	
	SetGPIOOutputValue(col_led, 1);	
	SetGPIOOutputValue(link_led, 1);	
	SetGPIOOutputValue(speed_led, 1);	
	SetGPIOOutputValue(web_led, 0);	


	// now look for eth0 device
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


	// set up the work queue for "lazy" led updates.
	if(!ngs_wq)
		ngs_wq = create_singlethread_workqueue("ngs_read");
	if(ngs_wq)
		queue_delayed_work(ngs_wq, &ngs_work, 0);
	
	// set up the timer, should be next to last.
	setup_timer(&s_LedRstTimer, LedRstHandler, 0);
    result = mod_timer(&s_LedRstTimer, jiffies + msecs_to_jiffies(s_LedRstPeriod));
    BUG_ON(result < 0);

	// init the kprobe, should be last.
	rpi_init_module();
	printk(KERN_INFO "rpi-cobalt: Module Loaded\n");
	return 0;
}

/*! rpicobaltexit - the actual module unload/exit
 *
 */
static void __exit rpicobaltexit(void)
{
	// unregister the kprobe, should be first.
	rpi_cleanup_module();

	// remove the timer, should be second
	del_timer(&s_LedRstTimer);

	// clear the work queue, should be third.
	if(ngs_wq){
		cancel_delayed_work_sync(&ngs_work);
		destroy_workqueue(ngs_wq);
	}

	// flip all the LEDs back to input.
	SetGPIOFunction(hdd_led, 0);	//Configure the pin as input
	SetGPIOFunction(txrx_led, 0);	//Configure the pin as input
	SetGPIOFunction(col_led, 0);	//Configure the pin as input
	SetGPIOFunction(link_led, 0);	//Configure the pin as input
	SetGPIOFunction(speed_led, 0);	//Configure the pin as input

	printk(KERN_INFO "rpi-cobalt: Module Unloaded\n");
}

// module stuff.
module_init(rpicobaltinit);
module_exit(rpicobaltexit);
MODULE_LICENSE("GPL");
