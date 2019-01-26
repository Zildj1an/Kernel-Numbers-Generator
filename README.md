# Kernel-Module-Numbers-Generator [![GPL Licence](https://badges.frapsoft.com/os/gpl/gpl.png?v=103)](https://opensource.org/licenses/GPL-3.0/)
  <p align="center">
  <span>Language:</span> 
  <a href="https://github.com/Zildj1an/Kernel-Module-Numbers-Generator/blob/master/LEEME.md">Español</a> |
  <a href="https://github.com/Zildj1an/Kernel-Module-Numbers-Generator">English</a> 
</p>

👨‍🔧 This kernel module merges many interesting concepts regarding Linux and Android kernel modules development in only two /proc entries.Needless to say, it does not include any standard C library.

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
You can check it worked with <i> $sudo lsmod | head </i>and reading <i>$sudo dmesg</i> 

## Loading module in Android's Kernel

The module was developed for Debian, but the can be easily used in Android.
You just need to use the <a href="https://github.com/Zildj1an/Kernel-Module-Numbers-Generator/blob/master/Makefile_Android">Makefile</a> for re-compiling. Please notice that the Makefile employs a compiled Android-x86 kernel <a href= "https://www.android.com/versions/oreo-8-0/"> oreo version </a>

## License
This project is licensed under the GNU-GPL License - see the LICENSE.md file for details
