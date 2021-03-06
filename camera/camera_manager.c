/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <camera/camera_manager.h>

/*
 * Ioctl Commands
 */
#define IOCTL_READ_REG            0
#define IOCTL_WRITE_REG           1
#define IOCTL_READ_EEPROM         2
#define IOCTL_WRITE_EEPROM        3
#define IOCTL_SET_ADDR            4
#define IOCTL_SET_CLK             5

#define IOCTL_SET_IMG_FORMAT      8  //arg type: enum imgformat
#define IOCTL_SET_TIMING_PARAM    9  //arg type: timing_param_t
#define IOCTL_SET_IMG_PARAM      10  //arg type: img_param_t
#define IOCTL_GET_FRAME          11
#define IOCTL_GET_FRAME_BLOCK    12

/*
 * Struct
 */
#define SIZE   5
struct reg_msg {
    uint32_t write_size;
    uint32_t read_size;
    uint8_t reg_buf[SIZE];
};

/*
 * Macro
 */
#define DEFAULT_CIM_DEV     "/dev/cim0"
#define DEFAULT_SENSOR_DEV  "/dev/sensor"

#define LOG_TAG  "camera"

/*
 * Variables
 */
static int cim_fd    = -1;
static int sensor_fd = -1;

/*
 * Functions
 */
static int32_t set_img_param(struct camera_img_param *img) {
    assert_die_if(!img, "Error: img pointer cannot to 'NULL'\n");

    if (ioctl(cim_fd, IOCTL_SET_IMG_PARAM, (unsigned long)img) < 0) {
        LOGE("ioctl: failed to set img param: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t set_timing_param(struct camera_timing_param *timing) {
    assert_die_if(!timing, "Error: timing pointer cannot to 'NULL'\n");

    if (ioctl(cim_fd, IOCTL_SET_TIMING_PARAM, (unsigned long)timing) < 0) {
        LOGE("ioctl: ailed to set timing param: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t sensor_setup_addr(int32_t chip_addr) {
    assert_die_if(!(chip_addr > 0), "Error: chip addr must greater than 0\n");

    if (ioctl(sensor_fd, IOCTL_SET_ADDR, (unsigned long)&chip_addr) < 0) {
        LOGE("ioctl: failed to set chip addr: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static uint8_t sensor_read_reg(uint32_t regaddr) {
    struct reg_msg msg;

#if (SENSOR_ADDR_LENGTH == 8)
    msg.write_size = 1;
    msg.read_size  = 1;
    msg.reg_buf[0] = (regaddr & 0xff);
#elif (SENSOR_ADDR_LENGTH == 16)
    msg.write_size = 2;
    msg.read_size  = 1;
    msg.reg_buf[0] = (regaddr >> 8) & 0xff;
    msg.reg_buf[1] = (regaddr & 0xff);
#endif

    if (ioctl(sensor_fd, IOCTL_READ_REG, (void *)&msg) < 0) {
        LOGE("ioctl: failed to read reg: 0x%x\n", regaddr);
        return -1;
    }

    return msg.reg_buf[0];
}

static int32_t sensor_write_reg(uint32_t regaddr, uint8_t regval) {
    struct reg_msg msg;

#if (SENSOR_ADDR_LENGTH == 8)
    msg.write_size = 2;
    msg.read_size  = 0;
    msg.reg_buf[0] = (regaddr & 0xff);
    msg.reg_buf[1] = regval;
#elif (SENSOR_ADDR_LENGTH == 16)
    msg.write_size = 3;
    msg.read_size  = 0;
    msg.reg_buf[0] = (regaddr >> 8) & 0xff;
    msg.reg_buf[1] = (regaddr & 0xff);
    msg.reg_buf[2] = regval;
#endif

    if (ioctl(sensor_fd, IOCTL_WRITE_REG, (unsigned long *)&msg) < 0) {
        LOGE("ioctl: failed to write reg: 0x%x\n", regaddr);
        return -1;
    }

    return 0;
}

static int32_t sensor_setup_regs(const struct camera_regval_list *vals) {
    while(1) {
        if ((vals->regaddr == ADDR_END) && \
            (vals->regval == VAL_END)) {
            LOGI("Set sensor registers finished!\n");
            break;
        }
        sensor_write_reg(vals->regaddr, vals->regval);
        usleep(SENSOR_SET_REG_DELAY_US);
#if 0
        uint8_t retval = sensor_read_reg(vals->regaddr);
        if (retval != vals->regval) {
            LOGE("Set sensor {0x%02x, 0x%02x} failed, retval: 0x%02x\n", vals->regaddr, vals->regval, retval);
        }
#endif
        vals++;
    }

    return 0;
}

static int32_t camera_read(uint8_t *yuvbuf, uint32_t size) {

    assert_die_if(!yuvbuf, "Error: framebuf pointer cannot to 'NULL'\n");
    return read(cim_fd, yuvbuf, size);
}

static int32_t camera_init(void) {
    struct camera_timing_param timing;

    timing.mclk_freq = 24000000;
    timing.pclk_active_level  = 0;
    timing.hsync_active_level = 1;
    timing.vsync_active_level = 1;

    cim_fd = open(DEFAULT_CIM_DEV, O_RDWR);
    assert_die_if(cim_fd < 0, "Failed to open cim dev: %s\n", \
                  strerror(errno));

    sensor_fd = open(DEFAULT_SENSOR_DEV, O_RDWR);
    assert_die_if(sensor_fd < 0, "Failed to open sensor dev: %s\n", \
                  strerror(errno));

    set_timing_param(&timing);

    return 0;
}

static void camera_deinit(void) {
    close(cim_fd);
    close(sensor_fd);
    cim_fd    = -1;
    sensor_fd = -1;
}

struct camera_manager camera_manager = {
    .camera_init       = camera_init,
    .camera_deinit     = camera_deinit,
    .camera_read       = camera_read,
    .set_img_param     = set_img_param,
    .set_timing_param  = set_timing_param,
    .sensor_setup_addr = sensor_setup_addr,
    .sensor_setup_regs = sensor_setup_regs,
    .sensor_write_reg  = sensor_write_reg,
    .sensor_read_reg   = sensor_read_reg,
};

struct camera_manager *get_camera_manager(void) {
    return &camera_manager;
}
