/*
 * (C) Copyright 2014 Du Huanpeng <u74147@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <mach/debug_ll.h>


//UART4
#define UART4_BASEESS  0x80010000

#define HW_UART4_CTRL0          (UART4_BASEESS + 0x00)
#define HW_UART4_CTRL1          (UART4_BASEESS + 0x10)
#define HW_UART4_CTRL2          (UART4_BASEESS + 0x20)
#define HW_UART4_LINECTRL       (UART4_BASEESS + 0x30)
#define HW_UART4_INTR           (UART4_BASEESS + 0x40)
#define HW_UART4_DATA           (UART4_BASEESS + 0x50)
#define HW_UART4_STAT           (UART4_BASEESS + 0x60)
#define HW_UART4_DEBUG          (UART4_BASEESS + 0x70)
#define HW_UART4_ILPR           (UART4_BASEESS + 0x80)
#define HW_UART4_RS485CTRL      (UART4_BASEESS + 0x90)
#define HW_UART4_RS485ADRMATCH  (UART4_BASEESS + 0xa0)
#define HW_UART4_RS485DLY       (UART4_BASEESS + 0xb0)
#define HW_UART4_AUTOBAUD       (UART4_BASEESS + 0xc0)
#define HW_UART4_CTRL3          (UART4_BASEESS + 0xd0)

static int as92_serial_setbaudrate(struct console_device *cdev, int baudrate)
{

	return 0;
}

static int as92_serial_init_port(struct console_device *cdev)
{

	return 0;
}

static void as92_serial_putc(struct console_device *cdev, char c)
{
	PUTC_LL(c);
}

static int as92_serial_getc(struct console_device *cdev)
{
        while ( ((readl(HW_UART4_STAT)) & 0x01000000) !=0 );
        return readb(HW_UART4_DATA);
}

static int as92_serial_tstc(struct console_device *cdev)
{

        return ((readl(HW_UART4_STAT) & 0x01000000) != 0x01000000);


}

static int as92_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	
	if(!cdev){
		printf("%s: NO MEM\n", __func__);
		return -ENOMEM;
	}
	cdev->dev = dev;
	cdev->tstc = as92_serial_tstc;
	cdev->putc = as92_serial_putc;
	cdev->getc = as92_serial_getc;
	cdev->setbrg = as92_serial_setbaudrate;

	as92_serial_init_port(cdev);

	console_register(cdev);
//
	as92_serial_putc(NULL, 'C');
	as92_serial_putc(NULL, 'O');
	as92_serial_putc(NULL, 'N');
//
	return 0;
}

static struct driver_d as92_serial_driver = {
        .name  = "as92_serial",
        .probe = as92_serial_probe,
};
console_platform_driver(as92_serial_driver);
