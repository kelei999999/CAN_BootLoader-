/*
 * BootLoader.h
 *
 *  Created on: 2019年7月16日
 *      Author: USB_WL
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_
#include "Typedef.h"
/*
 * 特别注意：
 * 本BootLoader程序支持每行最大64个字节，在生成HEX文件时，务必注意每一行的字节数不能超过64；
 * 本bootLoader程序占据Flash的前16KB，应用程序的向量偏移地址必须从默认值上加上0x4000;
 * 本bootLoader程序在跳转到用户程序之前将内核的中断关闭，应用程序在初始化外设后应重新打开内核
 * 中断，否则CPU不会进入任何中断。
 * 本bootLoader程序使用内部RC振荡器作为时钟，其精准度可能受到设备的温度影响。
 */
/*
 * 必须特别注意，HEX文件每一行的长度必须是8的整数倍，否则将导致错误！！！！！！！！！！！！！！！！！！
 *
 * 以下地址为应用程序指定的空闲flash地址，通常是在用户程序中定义一个无用的const变量并指定的到以下地址。
 * 该地址引根据实际的MCU的地址进行分配
 */
/*
 * 修改计记录：20200611在原来的基础上增加了CAN2也可以作为BootLoader的功能。
 */
#define ENTRY_ADDR_APP		0x08008004
#define ENTRY_ADDR_APP_H	0x0800
#define ENTRY_ADDR_APP_L	0x8004
/*
 * 指定CAN节点用于BootLoader,默认CAN1和CAN2均可用于Bootloader，但是两个CAN不能同时用于Bootloader
 * 改变量的值会根据实际接收到数据的CAN端口自行改变。
 */
extern volatile uint8 CAN_Node;
/*
 * 以下定义了本CPU正常的应用程序复位地址（入口地址），由于BootLoader代码占用了该地址，导致用户程序必须
 * 使用ENTRY_ADDR_APP做入口地址
 */
#define RESET_ADDRESS_H		0x0800
#define RESET_ADDRESS_L		0x0000
/*
 * 以下宏定义定义了本BOOT程序所在的位置，当应用程序中的HEX文件有该段地址的数据，Boot程序将自动忽略写入
 * 从而保证Boot程序本身不会被改写
 */
#define BootProgramStart_H	0x0800
#define BootProgramStart_L	0x0000
#define BootProgramEnd_H	0x0800
#define BootProgramEnd_L	0x8000
#define BootProgramStart	(((uint32)BootProgramStart_H << 16) + BootProgramStart_L)
#define BootProgramEnd		(((uint32)BootProgramEnd_H << 16) + BootProgramEnd_L)
/*
 *
 */
#define ENTRY_APP()			((void(*)(void))*(uint32*)ENTRY_ADDR_APP)()
/*
 * 以下定义为系统的中断关闭命令，BootLoader交出CPU之前必须将所有中断关闭，否则应用程序初始化的时候就会导致
 * 错误中断的发生，从而导致CPU死锁
 * 应根据实际的CPU的指令或服务函数进行定义
 */
#define Disable_ALL_ISR			__disable_irq();
/*
 * 以下宏定义为看门狗清零,应根据实际的CPU的指令或服务函数进行定义
 */
#define ClrWDTM()				(IWDG->KR = 0xAAAA)
/*
 * 禁止中断,应根据实际的CPU的指令或服务函数进行定义
 */
#define Disable_ISR				__disable_irq()
/*
 * 注意！注意！看门狗超时时间必须大于100ms，否则Boot程序将出错
 * 使能中断,应根据实际的CPU的指令或服务函数进行定义
 */
#define Enable_ISR				__enable_irq();

/*
 * BootID主要是用于CAN总线
 */
#ifdef BootID
	#undef BootID
	#define BootID				0xAAAB
#else
	//#define BootID				0xAA9B/*4G通信DTU Boot ID*/
	#define BootID				0xAAAB
#endif
/*
 * 以下定义了通信帧起始标志
 */
#ifdef Enter_key
	#undef Enter_key
	#define Enter_key			0x0d
#else
	#define Enter_key			0x0d
#endif
#ifdef Newline_key
	#undef Newline_key
	#define Newline_key			0x0a
#else
	#define Newline_key			0x0a
#endif
#ifdef Start_key
	#undef Start_key
	#define Start_key			':'
#else
	#define Start_key			':'
#endif
/*
 * 定义成功标志
 */
#ifndef STATUS_SUCCESS
	#define STATUS_SUCCESS		0
#endif
typedef union
{
	struct
	{
		uint8 Erase								:1;/*flash擦除标志位；1：代表已经执行擦除操作；0：未执行擦除操作*/
		uint8 Frame_Start						:1;/*帧开始标志位；1：代表通信已经接收的到了Start_key；0：暂未接到Start_key*/
		uint8 Frame_End							:1;/*帧结束标志位；1：代表通信已经接收的到了Enter_key+Newline_key；0：暂未接到Enter_key+Newline_key*/
		uint8 Pragram_Start						:1;/*编程开始标志位；1：代表开始flash编程开始；0：未开始编程或者编程已结束*/
		uint8 Enter								:1;/*回车标志位；1：代表检测到了回车；0：未检测到回车*/
		uint8 Newline							:1;/*换行标志位；1：代表检测到了换行；0：未检测到换行*/
		uint8 File_Start						:1;/*Boot文件开始*/
		uint8 File_End							:1;/*Boot文件结束*/
		uint8 Communi_ERROR						:1;/*通信错误*/
		uint32 reserve							:25;
	}bits;
	uint32 all;
}bl_sfr;
extern volatile bl_sfr BootLoader_SFR;
extern void BootLoader_Main(void);
extern void CAN0_Handle(uint8 ModuleNode,uint32 ID,uint8 DLC,uint8 *DATA);
extern void Timer1_Handle(uint32 *DATA);
#endif /* BOOTLOADER_H_ */
