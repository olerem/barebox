Watchdog support
===============================

Warnings and design consideration
---------------------------------

Watchdog is a last line of defense on misbehaving system. Thus, proper hardware
and watchdog design should be made, to be able to reduce impact of filed system
in the field. In best case, the bootloader should not touch it at all. No
watchdog feeding should be done, until application critical software was
started.

In case the bootloader is responsible for watchdog activation, the system can
be considered as failed by design. Following threads can affect the system
which are mostly addressable by properly designed watchdog and watchdog
strategy:
- Software based miss-configurations or bugs prevent the system from starting.
- Glitches caused by under-voltage, not proper power-on sequence or noisy power 
  supply.
- Physical damages caused by humidity, vibration or temperature.
- Temperature based misbehavior of the system. For example clock is not running 
  or running with wrong frequency.
- Chemical reaction. For example some clock crystals will stop to work in 
  contact with Helium. For example: 
  https://ifixit.org/blog/11986/iphones-are-allergic-to-helium/
- Filed storage prevent the booting. NAND, SD, SSD, HDD, SPI-Flash all of this
  some day stop to work.

In all this cases, bootloader won't be able to start and properly designed
watchdog may take some action. For example: to recover the system by resting
it, or power off to reduce the damage.

Barebox watchdog functionality
------------------------------

Nevertheless, in some cases we have no more influence on hardware design or
you are developer and need to be able to feed watchdog to disable it from
bootloader. For this scenarios barebox is providing the watchdog framework with 
following functionality and at least CONFIG_WATCHDOG should be enabled:

Polling
-------
Watchdog polling/feeding. It allows to feed watchdog and keep it running on one 
side and not resetting the system on other side. It is need on hardware with 
short time watchdogs. For example Atheros ar9331 watchdog has maximal timeout of 
7 seconds. So it may reset even on netboot.
Or it can be used on system where watchdog is already running and can't be 
disabled. For imx2 watchdog.
This functionally can be seen as thread, since in error case barebox will 
continue to feed watchdog even if it is not desired. So, depending on your need
CONFIG_WATCHDOG_POLLER can be enabled or disabled on compile time. Even if 
barebox was build with watchdog poling support, it is not enable by default. To 
start polling from command line run:

.. code-block:: sh

  wdog0.autoping=1

Poller interval is not configurable and it is running at 500ms rate.
The watchdog timeout is configured by default to the maximal supported value by hardware.
To change timeout used by poller, run:

.. code-block:: sh

  wdog0.timeout_cur=7

To read current watchdog configuration, run:

.. code-block:: sh

  devinfo wdog0

The output may looks as follow:

.. code-block:: sh

  barebox@DPTechnics DPT-Module:/ devinfo wdog0
  Parameters:
    autoping: 1 (type: bool)
    timeout_cur: 7 (type: uint32)
    timeout_max: 10 (type: uint32)

To make this changes persistent between reboots:

.. code-block:: sh

  nv dev.wdog0.autoping=1
  nv dev.wdog0.timeout_cur=7

Boot watchdog timeout
---------------------
With this functionality barebox may start watchdog or update timeout of running 
one, just before kicking the boot image. It can be configured temporary:

.. code-block:: sh

  global boot.watchdog_timeout=10

or persistent:

.. code-block:: sh

  nv boot.watchdog_timeout=10

On a system with multiple watchdogs, only the first one (wdog0) is affected by 
boot.watchdog_timeout parameter.

