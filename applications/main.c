/*
 * File      : main.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-07-29     Arda.Fu      first implementation
 */
#include <rtthread.h>
#include <board.h>
#include <dfs_fs.h>
#include <dfs_posix.h>
#include <dfs_ramfs.h>

static rt_uint32_t ramfs_pool[8192];
int main(void)
{
    /* user app entry */
    dfs_mount(RT_NULL,"/","ram",0,dfs_ramfs_create((rt_uint8_t*) ramfs_pool, 8192*4));
    return 0;
}





