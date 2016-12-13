/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic QRcode SDK Project
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

#include <types.h>

#define UART_MAX_CHANNELS   5

struct uart_manager {
    /*************************************************
    Function: uart_init
    Description: 串口初始化
    Input:
        devname: 串口设备名称
            例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
        baudrate: 波特率 单位:bis per second
            波特率取值范围1200~3000000
        date_bits: 数据位宽
        stop_bits: 停止位宽
        parity_bits: 奇偶校验位
            可选设置UART_PARITY_NONE, UART_PARITY_ODD, UART_PARITY_EVEN, UART_PARITY_MARK, UART_PARITY_SPACE
            分别对应无校验、奇校验、偶校验、校验位总为1、校验位总为0
    Output: 无
    Return: 0: 成功， -1: 失败
    Others: 内部最大可以支持5个uart通道[配置宏UART_MAX_CHANNELS], 每个通道在使用前必须优先调用uart_init
                 默认流控不开启
    *************************************************/
    int32_t (*init)(char* devname, uint32_t baudrate, uint8_t date_bits, uint8_t parity,
        uint8_t stop_bits);
    /*************************************************
    Function: uart_deinit
    Description: 串口释放
    Input:
        devname: 串口设备名称
            例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
    Output: 无
    Return: 0: 成功， -1: 失败
    Others: 对应uart_init, 在不再使用uart时调用
    *************************************************/
    int32_t (*deinit)(char* devname);
    /*************************************************
    Function: uart_flow_control
    Description: 串口流控
    Input:
        devname: 串口设备名称
            例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
        flow_ctl: 流控选项
        UART_FLOWCONTROL_NONE: 无流控
        UART_FLOWCONTROL_XONXOFF: 软件流控使用XON/XOFF字符
        UART_FLOWCONTROL_RTSCTS: 硬件刘空使用RTS/CTS信号
        UART_FLOWCONTROL_DTRDSR: 硬件流控使用DTR/DSR信号
    Output: 无
    Return: 0: 成功， -1: 失败
    Others: 对应uart_init, 在不再使用uart时调用
    *************************************************/
    int32_t (*flow_control)(char* devname, uint8_t flow_ctl);
    /*************************************************
    Function: uart_write
    Description: 串口写，在超时时间内写入数据到指定串口
    Input:
        devname: 串口设备名称
            例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
        buf: 写数据缓冲区
        count: 写数据长度，单位:bytes
        timeout_ms: 写超时，单位:ms
    Output: 无
    Return: 正数或0: 成功写入的字节数
    Others: 在使用之前要先调用uart_init
    *************************************************/
    uint32_t (*write)(char* devname, const void* buf, uint32_t count,
        uint32_t timeout_ms);
    /*************************************************
    Function: uart_read
    Description: 串口读，在超时时间内从指定串口中读出数据
    Input:
        devname: 串口设备名称
            例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
        buf: 读数据缓冲区
        count: 读数据长度，单位:bytes
        timeout_ms: 读超时，单位:ms
    Output: 无
    Return: 正数或0: 成功读取的字节数
    Others: 在使用之前要先调用uart_init
    *************************************************/
    uint32_t (*read)(char* devname, void* buf, uint32_t count, uint32_t timeout_ms);
};

/*************************************************
Function: get_uart_manager
Description: 获取uart_mananger句柄
Input:  无
Output: 无
Return: 返回uart_manager结构体指针
Others: 通过该结构体指针访问内部提供的方法
*************************************************/
struct uart_manager* get_uart_manager(void);

/** Parity settings. */
enum uart_parity {
    /** Special value to indicate setting should be left alone. */
    UART_PARITY_INVALID = -1,
    /** No parity. */
    UART_PARITY_NONE = 0,
    /** Odd parity. */
    UART_PARITY_ODD = 1,
    /** Even parity. */
    UART_PARITY_EVEN = 2,
    /** Mark parity. */
    UART_PARITY_MARK = 3,
    /** Space parity. */
    UART_PARITY_SPACE = 4,
};

/** FLowcontrol settings. */
enum uart_flowcontrol {
    /** No flow control. */
    UART_FLOWCONTROL_NONE = 0,
    /** Software flow control using XON/XOFF characters. */
    UART_FLOWCONTROL_XONXOFF = 1,
    /** Hardware flow control using RTS/CTS signals. */
    UART_FLOWCONTROL_RTSCTS = 2,
    /** Hardware flow control using DTR/DSR signals. */
    UART_FLOWCONTROL_DTRDSR = 3,
};