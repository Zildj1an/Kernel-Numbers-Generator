# Kernel-Module-Numbers-Generator [![GPL Licence](https://badges.frapsoft.com/os/gpl/gpl.png?v=103)](https://opensource.org/licenses/GPL-3.0/)

👨‍🔧 This kernel module merges many interesting concepts regarding Linux kernel modules development in only two /proc entries.

It`s a SMP-Safe implementation of a Linux kernel module - random digits generator (even or odd).

Particularly interesting in terms of coding since:

* It manages a kernel doubly-linked list with ghost node (read more about them at  <a href="https://github.com/Zildj1an/Linux-Linked-Lists">my repo</a>)
* It employs kernel interruptions to fill a kernel circular buffer (configurable intervals)
* It uses semaphores and special kernel mutexes called spin-locks which are required for a code with CPU interruptions.
* It uses private work-queues to schedule CPU work (in this case, empty the linked list)

You can also configure and check the module parameters with the auxiliar entry /proc/modconfig

## Usage
Compile, load the module, access via cat and echo to /proc/modtimer2. The first process to open and write from the file will receive even numbers randomly generated, while the second one will be feed with odd digits.


```bash

    $ make # make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

    $ sudo insmod modtimer.ko

    $ echo max_random 4 > /proc/modconfig  or  $ cat modconfig

    ...

    $ cat /proc/modtimer
    
    ...


```
