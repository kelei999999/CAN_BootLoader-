/*
 * BootLoader.h
 *
 *  Created on: 2019��7��16��
 *      Author: USB_WL
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_
#include "Typedef.h"
/*
 * �ر�ע�⣺
 * ��BootLoader����֧��ÿ�����64���ֽڣ�������HEX�ļ�ʱ�����ע��ÿһ�е��ֽ������ܳ���64��
 * ��bootLoader����ռ��Flash��ǰ16KB��Ӧ�ó��������ƫ�Ƶ�ַ�����Ĭ��ֵ�ϼ���0x4000;
 * ��bootLoader��������ת���û�����֮ǰ���ں˵��жϹرգ�Ӧ�ó����ڳ�ʼ�������Ӧ���´��ں�
 * �жϣ�����CPU��������κ��жϡ�
 * ��bootLoader����ʹ���ڲ�RC������Ϊʱ�ӣ��侫׼�ȿ����ܵ��豸���¶�Ӱ�졣
 */
/*
 * �����ر�ע�⣬HEX�ļ�ÿһ�еĳ��ȱ�����8�������������򽫵��´��󣡣���������������������������������
 *
 * ���µ�ַΪӦ�ó���ָ���Ŀ���flash��ַ��ͨ�������û������ж���һ�����õ�const������ָ���ĵ����µ�ַ��
 * �õ�ַ������ʵ�ʵ�MCU�ĵ�ַ���з���
 */
/*
 * �޸ļƼ�¼��20200611��ԭ���Ļ�����������CAN2Ҳ������ΪBootLoader�Ĺ��ܡ�
 */
#define ENTRY_ADDR_APP		0x08008004
#define ENTRY_ADDR_APP_H	0x0800
#define ENTRY_ADDR_APP_L	0x8004
/*
 * ָ��CAN�ڵ�����BootLoader,Ĭ��CAN1��CAN2��������Bootloader����������CAN����ͬʱ����Bootloader
 * �ı�����ֵ�����ʵ�ʽ��յ����ݵ�CAN�˿����иı䡣
 */
extern volatile uint8 CAN_Node;
/*
 * ���¶����˱�CPU������Ӧ�ó���λ��ַ����ڵ�ַ��������BootLoader����ռ���˸õ�ַ�������û��������
 * ʹ��ENTRY_ADDR_APP����ڵ�ַ
 */
#define RESET_ADDRESS_H		0x0800
#define RESET_ADDRESS_L		0x0000
/*
 * ���º궨�嶨���˱�BOOT�������ڵ�λ�ã���Ӧ�ó����е�HEX�ļ��иöε�ַ�����ݣ�Boot�����Զ�����д��
 * �Ӷ���֤Boot�������ᱻ��д
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
 * ���¶���Ϊϵͳ���жϹر����BootLoader����CPU֮ǰ���뽫�����жϹرգ�����Ӧ�ó����ʼ����ʱ��ͻᵼ��
 * �����жϵķ������Ӷ�����CPU����
 * Ӧ����ʵ�ʵ�CPU��ָ�����������ж���
 */
#define Disable_ALL_ISR			__disable_irq();
/*
 * ���º궨��Ϊ���Ź�����,Ӧ����ʵ�ʵ�CPU��ָ�����������ж���
 */
#define ClrWDTM()				(IWDG->KR = 0xAAAA)
/*
 * ��ֹ�ж�,Ӧ����ʵ�ʵ�CPU��ָ�����������ж���
 */
#define Disable_ISR				__disable_irq()
/*
 * ע�⣡ע�⣡���Ź���ʱʱ��������100ms������Boot���򽫳���
 * ʹ���ж�,Ӧ����ʵ�ʵ�CPU��ָ�����������ж���
 */
#define Enable_ISR				__enable_irq();

/*
 * BootID��Ҫ������CAN����
 */
#ifdef BootID
	#undef BootID
	#define BootID				0xAAAB
#else
	//#define BootID				0xAA9B/*4Gͨ��DTU Boot ID*/
	#define BootID				0xAAAB
#endif
/*
 * ���¶�����ͨ��֡��ʼ��־
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
 * ����ɹ���־
 */
#ifndef STATUS_SUCCESS
	#define STATUS_SUCCESS		0
#endif
typedef union
{
	struct
	{
		uint8 Erase								:1;/*flash������־λ��1�������Ѿ�ִ�в���������0��δִ�в�������*/
		uint8 Frame_Start						:1;/*֡��ʼ��־λ��1������ͨ���Ѿ����յĵ���Start_key��0����δ�ӵ�Start_key*/
		uint8 Frame_End							:1;/*֡������־λ��1������ͨ���Ѿ����յĵ���Enter_key+Newline_key��0����δ�ӵ�Enter_key+Newline_key*/
		uint8 Pragram_Start						:1;/*��̿�ʼ��־λ��1������ʼflash��̿�ʼ��0��δ��ʼ��̻��߱���ѽ���*/
		uint8 Enter								:1;/*�س���־λ��1�������⵽�˻س���0��δ��⵽�س�*/
		uint8 Newline							:1;/*���б�־λ��1�������⵽�˻��У�0��δ��⵽����*/
		uint8 File_Start						:1;/*Boot�ļ���ʼ*/
		uint8 File_End							:1;/*Boot�ļ�����*/
		uint8 Communi_ERROR						:1;/*ͨ�Ŵ���*/
		uint32 reserve							:25;
	}bits;
	uint32 all;
}bl_sfr;
extern volatile bl_sfr BootLoader_SFR;
extern void BootLoader_Main(void);
extern void CAN0_Handle(uint8 ModuleNode,uint32 ID,uint8 DLC,uint8 *DATA);
extern void Timer1_Handle(uint32 *DATA);
#endif /* BOOTLOADER_H_ */
