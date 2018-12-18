# Kernel-Module-Numbers-Generator

[EN] 

This code merges many interesting concepts regarding Linux kernel modules development in a single /proc entry.

It`s a SMP-Safe implementation of a random digits Linux kernel module generator (even or odd).

Particularly interesting in terms of coding since:

1. It manages a kernel doubly-linked list with ghost node.
2. It employs kernel interruptions to fill a kernel circular buffer (configurable intervals)
3. It uses sempahores and special kernel mutexes called spin-locks which are required for a code with CPU interruptions.
4. It uses private work-queues to schedule CPU work (in this case, empty the linked list)

You can also configure and check the module parameters with the auxiliar entry /proc/modconfig

Usage: Compile, load the module, access via cat and echo to /proc/modtimer2. The first process to open and write from the file will receive even numbers randomly generated, while the second one will be feed with odd digits.

  $ make

  $ sudo insmod modtimer.ko

  $ echo max_random 4 > /proc/modconfig  or  $ cat modconfig

  ...

  $ cat /proc/modtimer2

  ...
