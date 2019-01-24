# Kernel-Module-Numbers-Generator [![GPL Licence](https://badges.frapsoft.com/os/gpl/gpl.png?v=103)](https://opensource.org/licenses/GPL-3.0/)
  <p align="center">
  <span>Idioma:</span> 
  <a href="">Español</a> |
  <a href="https://github.com/Zildj1an/Kernel-Module-Numbers-Generator">English</a> 
</p>

👨‍🔧 Éste módulo del kernel une muchos conceptos interesantes sobre el desarrollo de módulos del kernel de Android y Linux, en sólo 2 entradas /proc.
Evidentemente no incluye librerías de la biblioteca estándar de C.

Es una implementación SMP-Safe de un módulo del kernel que genera números aleatoriamente tanto pares como impares.

Particularmente interesante en cuanto a código ya que:
* Maneja una lista doblemente enlazada de Linux con nodo fantasma (lee más en <a href="https://github.com/Zildj1an/Linux-Linked-Lists">mi repo</a>)
* Emplea interrupciones del kernel para llenar un buffer circular del kernel (intervalos configurables)
* Emplea semáforos y mutexes del kernel especiales (spin-locks) requeridos por las interrupciones de CPU en el código.
* Usa work-queues privadas para planificar el trabajo de CPU (en este caso, vaciar las listas enlazadas)

También puedes configurar y checkear los parámetros auxiliares del módulo en la entrada /proc/modconfig 

## Uso
Compilar, cargar el módulo, acceder por cat y echo a /proc/modtimer2. El primer proceso en abrir y escribir en el archivo recibe los pares aleatorios, los impares el segundo.

```bash

    $ make # make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

    $ sudo insmod modtimer.ko

    $ echo max_random 4 > /proc/modconfig  o  $ cat modconfig

    ...

    $ cat /proc/modtimer
    
    ...


```
Puedes comprobar que funcióno con <i> $sudo lsmod | head </i> y leyendo <i>$sudo dmesg</i>.

## Cargando el módulo en Android

Éste módulo fue desarrollado para Debian, pero puede usarse facilmente en Android.
Simplemente usa <a href="https://github.com/Zildj1an/Kernel-Module-Numbers-Generator/blob/master/Makefile_Android">Makefile</a> para recompilar.
Ojo: Ese Makefile usa un kernel Android x-86 <a href= "https://www.android.com/versions/oreo-8-0/"> versión oreo</a> compilado.

