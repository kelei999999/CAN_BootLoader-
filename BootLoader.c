/*
 * BootLoader.c
 *
 *  Created on: 2019年7月16日
 *      Author: USB_WL
 */
#include "BootLoader.h"
//-----------------------------------------------------Flash操作函数必须的变量或者函数
extern FLASH_EraseInitTypeDef Flash;
extern uint32_t Flash_state;
extern FDCAN_TxHeaderTypeDef pTxHeader_Can1;
extern FDCAN_RxHeaderTypeDef pRxHeader_Can1;
extern uint32 BootID_X;
//-----------------------------------------------------BootLoader使用的变量或者函数
static int8 StrToHex(uint8 ascii,uint8 *hex);
static uint8 CHKSUM (uint8* data,uint16 length);
void CAN0_Handle(uint8 ModuleNode,uint32 ID,uint8 DLC,uint8 *DATA);
void Timer1_Handle(uint32 *DATA);
int8 Flash_Verify(uint32 Flash_Address,uint64 *Program_DATA,uint32 Length);
volatile bl_sfr BootLoader_SFR;
volatile uint8 CAN_Node = 1;
uint8 RAW_HEX_DATA[256] = {0};
uint16 RAW_HEX_Piont = 0;/*本次收到的有效字符个数*/
uint8 HEX_DATA[128] = {0};
uint8 CANS[8] = {0};
uint16 HighFlash_AdressX = 0;/*FLASH高地址*/
uint16 HighFlash_Adress = 0;/*FLASH高地址*/
uint16 LowFlash_Adress = 0;/*FLASH低地址*/
uint32 Program_Length = 0;/*要写入flash字节的长度*/
uint64 *Program_DATA;
uint64 *Program_Address;
void BootLoader_Main(void)
{
	uint16 Status = 0;
	uint32 Program_address;
	uint32 Program_Address;
	uint32  Length = 0;         // Number of 16-bit values to be programmed
	uint16 i;
	uint32 j;
	uint32 temp;
	uint16 Byte_Count = 0;/*本次有效字节数*/
	BootLoader_SFR.all = 0;
	ClrWDTM();
	//------------------------------------------------------------------------
	CAN_Init(1);
	CAN_Init(2);
	//------------------------------------------------------------------------开中断
	Enable_ISR;/*开中断*/
	while(1)
	{
		ClrWDTM();
		//--------------------------------------------------------------------
		if(1 == BootLoader_SFR.bits.Communi_ERROR)/*通信帧错误*/
		{
			while(1)
			{
				CANS[0] = 'E';
				CANS[1] = 'R';
				CANS[2] = 'R';
				CANS[3] = 'O';
				CANS[4] = 'R';
				CANS[5] = '0';
				CANS[6] = '0';
				CANS[7] = '7';
				CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
				for(j = 0;j < 0xffff;j ++);
				if(0 == BootLoader_SFR.bits.File_Start)
				{
					Disable_ALL_ISR;
					ClrWDTM();
					ENTRY_APP();/*跳转到用户程序入口地址*/
				}
			}
		}
		//--------------------------------------------------------------------
		BootStart:
		if(1 == BootLoader_SFR.bits.Frame_End)/*接收到了1行数据，接下来解析解析*/
		{
			/*第一步，将单一数字合并*/
			Byte_Count = (RAW_HEX_Piont >> 1);
			for(i = 0;i < Byte_Count;i ++)
			{
				HEX_DATA[i] = (RAW_HEX_DATA[i * 2] << 4) + RAW_HEX_DATA[i * 2 + 1];
			}
			/*第二步，验证校验和*/
			if(HEX_DATA[Byte_Count - 1] == CHKSUM(HEX_DATA,(Byte_Count - 1)))/*校验通过*/
			{
				/*第三步，判断本帧是数据帧还是命令帧*/
				if(0 == HEX_DATA[3])/*本帧是数据帧*/
				{
					if(1 == BootLoader_SFR.bits.File_Start)/*如果在此之前收到了扩展地址命令，则认为本次写入是正确的*/
					{
						Program_DATA = (uint64*)&HEX_DATA[4];/*进一步将单字节数组转成8字节数组，方便写入flash*/
						Length = HEX_DATA[0];
						HighFlash_Adress = HighFlash_AdressX;
						LowFlash_Adress = (HEX_DATA[1] << 8) + HEX_DATA[2];
						/*此处做了以下特殊处理，应该要编程的地址已经到了BootLoader程序的flash空间了，所以不能对此编程*/
						temp = ((uint32)HighFlash_Adress << 16) + LowFlash_Adress + Length;
						if((temp >= BootProgramStart)&&(temp <= BootProgramEnd))
						{
							BootLoader_SFR.bits.Frame_End = 0;
							CANS[0] = 'E';
							CANS[1] = 'R';
							CANS[2] = 'R';
							CANS[3] = 'O';
							CANS[4] = 'R';
							CANS[5] = '0';
							CANS[6] = '0';
							CANS[7] = '8';
							CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
							for(j = 0;j < 0xffff;j ++);
							Disable_ALL_ISR;
							ClrWDTM();
							ENTRY_APP();/*跳转到用户程序入口地址*/
						}
						else
						{
							BootLoader_SFR.bits.Frame_End = 0;
							BootLoader_SFR.bits.Pragram_Start = 1;
						}
					}
					else/*尚未收到扩展地址命令即有要写入flash的数据，本BootLoader认为其不正确*/
					{
						CANS[0] = 'E';
						CANS[1] = 'R';
						CANS[2] = 'R';
						CANS[3] = 'O';
						CANS[4] = 'R';
						CANS[5] = '0';
						CANS[6] = '0';
						CANS[7] = '8';
						CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
						for(j = 0;j < 0xffff;j ++);
						Disable_ALL_ISR;
						ClrWDTM();
						ENTRY_APP();/*跳转到用户程序入口地址*/
					}
				}
				else if(4 == HEX_DATA[3])/*本帧是命令帧，设置扩展地址*/
				{
					HighFlash_AdressX = (HEX_DATA[4] << 8) + HEX_DATA[5];
					BootLoader_SFR.bits.File_Start = 0;
					BootLoader_SFR.bits.Frame_End = 0;
					BootLoader_SFR.bits.File_Start = 1;/*正式开始bootloader*/
				}
				else if(1 == HEX_DATA[3])/*本帧是命令帧，代表文件结束*/
				{
					/*结束Bootloader程序，向用户应用程序移交CPU使用权*/
					BootLoader_SFR.bits.Frame_End = 0;
					BootLoader_SFR.bits.File_Start = 0;
					BootLoader_SFR.bits.Erase = 0;
					Disable_ALL_ISR;
					ClrWDTM();
					ENTRY_APP();/*跳转到用户程序入口地址*/
				}
				else if(5 == HEX_DATA[3])/*本帧是命令帧，开始线性地址记录。通常用于指明程序的入口地址*/
				{
					BootLoader_SFR.bits.Frame_End = 0;
					/*不做任何操作*/
				}
				else
				{
					/*通信错误，返回错误代码，命令错误*/
					BootLoader_SFR.bits.Frame_End = 0;
					while(1)
					{
						CANS[0] = 'E';
						CANS[1] = 'R';
						CANS[2] = 'R';
						CANS[3] = 'O';
						CANS[4] = 'R';
						CANS[5] = '0';
						CANS[6] = '0';
						CANS[7] = '5';
						CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
						for(j = 0;j < 0x1ffff;j ++);
					}
				}
			}
			else
			{
				/*返回错误代码，校验和错误*/
				BootLoader_SFR.bits.Frame_End = 0;
				while(1)
				{
					CANS[0] = 'E';
					CANS[1] = 'R';
					CANS[2] = 'R';
					CANS[3] = 'O';
					CANS[4] = 'R';
					CANS[5] = '0';
					CANS[6] = '0';
					CANS[7] = '4';
					CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
					for(j = 0;j < 0x1ffff;j ++);
					if(0 == BootLoader_SFR.bits.File_Start)
					{
						Disable_ALL_ISR;
						ClrWDTM();
						ENTRY_APP();/*跳转到用户程序入口地址*/
					}
				}
			}
		}
		//-------------------------------------------------------------------启动编程flash写入
		if(1 == BootLoader_SFR.bits.Pragram_Start)
		{
			Disable_ISR;/*在擦写flash之前关闭所有中断*/
			if(0 == BootLoader_SFR.bits.Erase)/*flash尚未擦除，执行flash擦除*/
			{
				CANS[0] = 'E';
				CANS[1] = 'R';
				CANS[2] = 'A';
				CANS[3] = 'S';
				CANS[4] = 'I';
				CANS[5] = 'N';
				CANS[6] = 'G';
				CANS[7] = 0;
				CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
				ClrWDTM();
				//-----------------------------------------------------------
				Flash.Banks = FLASH_BANK_1;/*擦除第一个BANK*/
				Flash.Page = 8;/*从第8页开始擦除，前面0~7页存储着boot loader程序*/
				Flash.NbPages = 128 - Flash.Page;/**/
				Flash.TypeErase = 0;/*执行页擦除*/
				HAL_FLASH_Unlock();
				Status = HAL_FLASHEx_Erase(&Flash, &Flash_state);
				Flash.Banks = FLASH_BANK_2;/*擦除第二个BANK*/
				Flash.Page = 0;/*从第8页开始擦除，前面0~7页存储着boot loader程序*/
				Flash.NbPages = 128 - Flash.Page;/**/
				Flash.TypeErase = 0;/*执行页擦除*/
				Status = HAL_FLASHEx_Erase(&Flash, &Flash_state);
				HAL_FLASH_Lock();
				if(Status != STATUS_SUCCESS)
				{
					/*擦除错误*/
					while(1)
					{
						CANS[0] = 'E';
						CANS[1] = 'R';
						CANS[2] = 'R';
						CANS[3] = 'O';
						CANS[4] = 'R';
						CANS[5] = '0';
						CANS[6] = '0';
						CANS[7] = '1';
						CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
						for(j = 0;j < 0x1ffff;j ++);
					}
				}
				CANS[0] = 'E';
				CANS[1] = 'R';
				CANS[2] = 'A';
				CANS[3] = 'S';
				CANS[4] = 'E';
				CANS[5] = 'D';
				CANS[6] = 0;
				CANS[7] = 0;
				CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
				BootLoader_SFR.bits.Erase = 1;
			}
			/*flash已经擦除，开始写入*/
			ClrWDTM();
			Program_Address = (((uint32)HighFlash_Adress << 16) + LowFlash_Adress);
			Program_address = Program_Address;
			Program_Length = Length >> 3;
			HAL_FLASH_Unlock();
			for(i = 0;i < Program_Length;i ++)
			{
				Status = HAL_FLASH_Program(0,Program_address,Program_DATA[i]);
				Program_address += 8;
			}
			HAL_FLASH_Lock();
			if(Status != STATUS_SUCCESS)
			{
				/*编程错误*/
				while(1)
				{
					CANS[0] = 'E';
					CANS[1] = 'R';
					CANS[2] = 'R';
					CANS[3] = 'O';
					CANS[4] = 'R';
					CANS[5] = '0';
					CANS[6] = '0';
					CANS[7] = '2';
					CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
					for(j = 0;j < 0x1ffff;j ++);
				}
			}
			// Verify the values programmed.  The Program step itself does a verify
			// as it goes.  This verify is a 2nd verification that can be done.
			ClrWDTM();
			Status = Flash_Verify(Program_Address,Program_DATA,Program_Length);
			if(Status != STATUS_SUCCESS)
			{
				/*校对错误*/
				while(1)
				{
					CANS[0] = 'E';
					CANS[1] = 'R';
					CANS[2] = 'R';
					CANS[3] = 'O';
					CANS[4] = 'R';
					CANS[5] = '0';
					CANS[6] = '0';
					CANS[7] = '3';
					CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
					for(j = 0;j < 0x1ffff;j ++);
				}
			}
			BootLoader_SFR.bits.Pragram_Start = 0;/*写入完成*/
			Enable_ISR;/*在擦写flash之后开启所有中断*/
		}
		//-------------------------------------------------------------------已经进入Boot程序，但是还没有接受收到任何字节，boot程序将每隔100ms发送一个回车换行符给上位机
		if((0 == BootLoader_SFR.bits.File_Start)&&(0 == BootLoader_SFR.bits.Frame_Start)&&(0 == BootLoader_SFR.bits.Frame_End))/*尚未接到第一个帧，通常第一个帧是设置扩展地址*/
		{
			CANS[0] = 'S';
			CANS[1] = 0x0D;
			CANS[2] = 0x0A;
			CANS[3] = 0;
			CANS[4] = 0;
			CANS[5] = 0;
			CANS[6] = 0;
			CANS[7] = 0;
			CAN_Sent(1,BootID_X,1,0,8,CANS);
			CAN_Sent(2,BootID_X,1,0,8,CANS);
			//---------------------------------------------------延时50mS,在50ms中如果接收到了Boot起始字符
			for(j = 0;j < 0x8fff;j ++)
			{
				if((1 == BootLoader_SFR.bits.Frame_Start)||(1 == BootLoader_SFR.bits.Frame_End))/*当收到开始字符后，停止延时*/
					goto BootStart;
			}
			Disable_ALL_ISR;
			/*50mS内没有收到上位机的任何信息，则跳转到用户程序*/
			//while(1);
			ClrWDTM();
			ENTRY_APP();/*跳转到用户程序入口地址*/
		}
		else/*已经接收到了第一个帧*/
		{
			/*上次编程已经完毕,又尚未接到新的有效字符，下位机主动发送“继续Boot”的请求*/
			if((0 == BootLoader_SFR.bits.Pragram_Start)&&(0 == BootLoader_SFR.bits.Frame_Start)&&(0 == BootLoader_SFR.bits.Frame_End))
			{
				CANS[0] = 'C';
				CANS[1] = 0x0D;
				CANS[2] = 0x0A;
				CANS[3] = 0;
				CANS[4] = 0;
				CANS[5] = 0;
				CANS[6] = 0;
				CANS[7] = 0;
				CAN_Sent(CAN_Node,BootID_X,1,0,8,CANS);
				ClrWDTM();
				for(j = 0;j < 0x1ffff;j ++)/*在延时过程中，接收到了有效字符则跳出延时，等待接收数据完毕*/
				{
					if((1 == BootLoader_SFR.bits.Frame_Start)||(1 == BootLoader_SFR.bits.Frame_End))/*当收到开始字符后，停止延时*/
						break;
				}
			}
		}
	}
}
//****************************************************************************
// @函数名		void TIMER1_Handle(void)
//----------------------------------------------------------------------------
// @描述			定时器1节点接收中断向量函数
//
//----------------------------------------------------------------------------
// @输入			uint32 ID接收数据的ID号，识别码，标准帧为11位，扩展帧为29位
//				uint8 DLC接收数据的ID号的长度，0~8个字节长度可选
//				uint8 *DATA接收到的数据
//----------------------------------------------------------------------------
// @输出			接收到数并解析后，得到的参数值
//
//----------------------------------------------------------------------------
// @返回值		void
//
//----------------------------------------------------------------------------
// @日期			2017年4月21日
//
//****************************************************************************
void Timer1_Handle(uint32 *DATA)
{
	static uint16 Frame_end_delay = 0;
	static uint32 File_end_delay = 0;
	if((1 == BootLoader_SFR.bits.Frame_Start)&&(0 == BootLoader_SFR.bits.Frame_End))
	{
		Frame_end_delay ++;
		if(Frame_end_delay > 1000)
		{
			BootLoader_SFR.bits.Frame_Start = 0;
			BootLoader_SFR.bits.Communi_ERROR = 1;/*通信错误，终止Bootloader*/
		}
	}
	else
	{
		Frame_end_delay = 0;
	}
	if(1 == BootLoader_SFR.bits.File_Start)
	{
		File_end_delay ++;
		if(File_end_delay > 240000)
		{
			File_end_delay = 0;
			BootLoader_SFR.bits.File_Start = 0;/*Boot超时*/
			BootLoader_SFR.bits.Communi_ERROR = 1;/*通信错误，终止Bootloader*/
		}
	}
	else
	{
		File_end_delay = 0;
	}
}
//****************************************************************************
// @函数名		void OS_CAN0_Handle(uint32 ID,uint8 DLC,uint8 *DATA)
//----------------------------------------------------------------------------
// @描述			CAN0节点接收中断向量函数
//
//----------------------------------------------------------------------------
// @输入			uint32 ID接收数据的ID号，识别码，标准帧为11位，扩展帧为29位
//				uint8 DLC接收数据的ID号的长度，0~8个字节长度可选
//				uint8 *DATA接收到的数据
//----------------------------------------------------------------------------
// @输出			接收到数并解析后，得到的参数值
//
//----------------------------------------------------------------------------
// @返回值		void
//
//----------------------------------------------------------------------------
// @日期			2017年4月21日
//
//****************************************************************************
void CAN0_Handle(uint8 ModuleNode,uint32 ID,uint8 DLC,uint8 *DATA)
{
	uint8 *P = DATA;
	uint8 i;
	if(ID == BootID_X)//万能协议
	{
		CAN_Node = ModuleNode;
		if(0 == BootLoader_SFR.bits.Frame_Start)/*在通信帧未建立之前，需要在通信数据里面搜索起始标识“:”*/
		{
			for(i = 0;i < DLC;i ++)
			{
				if(Start_key == DATA[0])//接收到的数据中第一个字节为“:”，认为通信帧开始
				{
					BootLoader_SFR.bits.Frame_Start = 1;/*找到起始标识*/
					RAW_HEX_Piont = 0;
					P = &DATA[1];/*剔除冒号，方便下一步处理*/
					DLC -= 1;/*剔除冒号后，总的字节数减少了1个字节*/
					break;
				}
			}
		}
		if(1 == BootLoader_SFR.bits.Frame_Start)/*找到“：”后进行下一步处理*/
		{
			//-----------------------------------------------------------------
			for(i = 0;i < DLC; i ++)
			{
				if(Enter_key == P[i])
				{
					BootLoader_SFR.bits.Enter = 1;
				}
				else if(Newline_key == P[i])
				{
					BootLoader_SFR.bits.Newline = 1;
				}
				else
				{
					//-----------------------------------------------------------------数据还原并填充
					if(1 == StrToHex(P[i],&RAW_HEX_DATA[RAW_HEX_Piont]))//数据正确
					{
						RAW_HEX_Piont ++;
					}
				}
				if((1 == BootLoader_SFR.bits.Enter)&&(1 == BootLoader_SFR.bits.Newline))/*找到行尾*/
				{
					BootLoader_SFR.bits.Enter = 0;
					BootLoader_SFR.bits.Newline = 0;
					BootLoader_SFR.bits.Frame_End = 1;/*本行结束*/
					BootLoader_SFR.bits.Frame_Start = 0;/*新行开始*/
					break;
				}
			}
		}
	}
	else
	{

	}
}
//****************************************************************************
// @函数名		int8 StrToHex(uint8 ascii,uint8 *hex)
//----------------------------------------------------------------------------
// @描述			字符串数字转二进制数字
//
//----------------------------------------------------------------------------
// @输入			uint8 ascii：待转换的字符串数字
//				uint8 *DATA：转换完毕的数字值
//----------------------------------------------------------------------------
// @输出			接收到数并解析后，得到的参数值
//
//----------------------------------------------------------------------------
// @返回值		int8：1、ascii是正确的ASCII码，并且转换已完成；0、ascii非正确的ASCII码，转换未成功
//
//----------------------------------------------------------------------------
// @日期			2017年4月21日
//
//****************************************************************************
static int8 StrToHex(uint8 ascii,uint8 *hex)
{
	if((ascii >= 0x30)&&(ascii <= 0x39))
	{
		*hex = ascii - 0x30;
		return 1;
	}
	else if((ascii >= 0x41)&&(ascii <= 0x46))
	{
		*hex = ascii - 0x41 + 0x0a;
		return 1;
	}
	else
		return 0;
}
//****************************************************************************
// @函数名		int8 StrToHex(uint8 ascii,uint8 *hex)
//----------------------------------------------------------------------------
// @描述			字符串数字转二进制数字
//
//----------------------------------------------------------------------------
// @输入			uint8 ascii：待转换的字符串数字
//				uint8 *DATA：转换完毕的数字值
//----------------------------------------------------------------------------
// @输出			接收到数并解析后，得到的参数值
//
//----------------------------------------------------------------------------
// @返回值		int8：1、ascii是正确的ASCII码，并且转换已完成；0、ascii非正确的ASCII码，转换未成功
//
//----------------------------------------------------------------------------
// @日期			2017年4月21日
//
//****************************************************************************
static uint8 CHKSUM (uint8* data,uint16 length)
{
	uint16 i;
	uint8 chk = 0;
	for(i = 0;i < length;i ++)
		chk += data[i];
	return (0x100 - (chk&0xff)) & 0xff;
}

int8 Flash_Verify(uint32 Flash_Address,uint64 *Program_DATA,uint32 Length)
{
	uint64 *Flash_P = (uint64 *)Flash_Address;
	uint32 i;
	for(i = 0;i < Length;i ++)
	{
		if(*Flash_P == Program_DATA[i])
		{
			Flash_P ++;
		}
		else
			return -1;
	}
	return 0;
}
