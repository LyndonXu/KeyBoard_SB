/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：protocol.c
* 摘要: 协议控制程序
* 版本：0.0.1
* 作者：许龙杰
* 日期：2013年01月25日
*******************************************************************************/
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f10x_conf.h"
#include "user_conf.h"
#include "user_api.h"
#include "io_buf_ctrl.h"
#include "app_port.h"
	
	
#include "key_led.h"
#include "adc_ctrl.h"
#include "code_switch.h"
#include "key_led_ctrl.h"
	
#include "buzzer.h"
	
#include "user_init.h"
#include "user_api.h"

#include "key_led_table.h"

#include "protocol.h"

#include "message.h"
#include "message_2.h"
#include "message_3.h"
#include "message_usb.h"
#include "flash_ctrl.h"
#include "extern_io_ctrl.h"

#include "common.h"

#define APP_VERSOIN_YY		17
#define APP_VERSOIN_MM		11
#define APP_VERSOIN_DD		07
#define APP_VERSOIN_V		01

#define APP_VERSOIN	"YA_SB_KEY_01_171107"

u8 g_u8CamAddr = 0;
bool g_boUsingUART2 = true;
bool g_boIsBroadcastPlay = false;
bool g_boIsDDRPlay = false;
bool g_boIsRockAus = false;
bool g_boIsRockEnable = true;

EmProtocol g_emProtocol = _Protocol_YNA;
bool g_boIsBackgroundLightEnable = true;

const u16 g_u16CamLoc[CAM_ADDR_MAX] = 
{
	0,
};


int32_t CycleMsgInit(StCycleBuf *pCycleBuf, void *pBuf, uint32_t u32Length)
{
	if ((pCycleBuf == NULL) || (pBuf == NULL) || (u32Length == 0))
	{
		return -1;
	}
	memset(pCycleBuf, 0, sizeof(StCycleBuf));
	pCycleBuf->pBuf = pBuf;
	pCycleBuf->u32TotalLength = u32Length;
	
	return 0;
}

void *CycleGetOneMsg(StCycleBuf *pCycleBuf, const char *pData, 
	uint32_t u32DataLength, uint32_t *pLength, int32_t *pProtocolType, int32_t *pErr)
{
	char *pBuf = NULL;
	int32_t s32Err = 0;
	if ((pCycleBuf == NULL) || (pLength == NULL))
	{
		s32Err = -1;
		goto end;
	}
	if (((pCycleBuf->u32TotalLength - pCycleBuf->u32Using) < u32DataLength)
		/*|| (u32DataLength == 0)*/)
	{
		PRINT_MFC("data too long\n");
		s32Err = -1;
	}
	if (u32DataLength != 0)
	{
		if (pData == NULL)
		{
			s32Err = -1;
			goto end;
		}
		else	/* copy data */
		{
			uint32_t u32Tmp = pCycleBuf->u32Write + u32DataLength;
			if (u32Tmp > pCycleBuf->u32TotalLength)
			{
				uint32_t u32CopyLength = pCycleBuf->u32TotalLength - pCycleBuf->u32Write;
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32CopyLength);
				memcpy(pCycleBuf->pBuf, pData + u32CopyLength, u32DataLength - u32CopyLength);
				pCycleBuf->u32Write = u32DataLength - u32CopyLength;
			}
			else
			{
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32DataLength);
				pCycleBuf->u32Write += u32DataLength;
			}
			pCycleBuf->u32Using += u32DataLength;

		}
	}

	do
	{
		uint32_t i;
		bool boIsBreak = false;

		for (i = 0; i < pCycleBuf->u32Using; i++)
		{
			uint32_t u32ReadIndex = i + pCycleBuf->u32Read;
			char c8FirstByte;
			u32ReadIndex %= pCycleBuf->u32TotalLength;
			c8FirstByte = pCycleBuf->pBuf[u32ReadIndex];
			if (c8FirstByte == ((char)0xAA))
			{
				#define YNA_NORMAL_CMD		0
				#define YNA_VARIABLE_CMD	1 /*big than PROTOCOL_YNA_DECODE_LENGTH */
				uint32_t u32MSB = 0;
				uint32_t u32LSB = 0;
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				
				/* check whether it's a variable length command */
				if (s32RemainLength >= PROTOCOL_YNA_DECODE_LENGTH - 1)
				{
					if (pCycleBuf->u32Flag != YNA_NORMAL_CMD)
					{
						u32MSB = ((pCycleBuf->u32Flag >> 8) & 0xFF);
						u32LSB = ((pCycleBuf->u32Flag >> 0) & 0xFF);
					}
					else
					{
						uint32_t u32Start = i + pCycleBuf->u32Read;
						char *pTmp = pCycleBuf->pBuf;
						if ((pTmp[(u32Start + _YNA_Mix) % pCycleBuf->u32TotalLength] == 0x04)
							&& (pTmp[(u32Start + _YNA_Cmd) % pCycleBuf->u32TotalLength] == 0x00))
						{
							u32MSB = pTmp[(u32Start + _YNA_Data2) % pCycleBuf->u32TotalLength];
							u32LSB = pTmp[(u32Start + _YNA_Data3) % pCycleBuf->u32TotalLength];
							if (s32RemainLength >= PROTOCOL_YNA_DECODE_LENGTH)
							{
								uint32_t u32Start = i + pCycleBuf->u32Read;
								uint32_t u32End = PROTOCOL_YNA_DECODE_LENGTH - 1 + i + pCycleBuf->u32Read;
								char c8CheckSum = 0;
								uint32_t j;
								char c8Tmp;
								for (j = u32Start; j < u32End; j++)
								{
									c8CheckSum ^= pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
								}
								c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
								if (c8CheckSum != c8Tmp) /* wrong message */
								{
									PRINT_MFC("get a wrong command: %d\n", u32MSB);
									pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
									pCycleBuf->u32Read += (i + 1);
									pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
									break;
								}
								
								u32MSB &= 0xFF;
								u32LSB &= 0xFF;
								
								pCycleBuf->u32Flag = ((u32MSB << 8) + u32LSB);
							}
						}
					}
				}
				u32MSB &= 0xFF;
				u32LSB &= 0xFF;
				u32MSB = (u32MSB << 8) + u32LSB;
				u32MSB += PROTOCOL_YNA_DECODE_LENGTH;
				PRINT_MFC("the data length is: %d\n", u32MSB);
				if (u32MSB > (pCycleBuf->u32TotalLength / 2))	/* maybe the message is wrong */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
					pCycleBuf->u32Read += (i + 1);
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					pCycleBuf->u32Flag = 0;
				}
				else if (((int32_t)(u32MSB)) <= s32RemainLength) /* good, I may got a message */
				{
					if (u32MSB == PROTOCOL_YNA_DECODE_LENGTH)
					{
						char c8CheckSum = 0, *pBufTmp, c8Tmp;
						uint32_t j, u32Start, u32End;
						uint32_t u32CmdLength = u32MSB;
						pBuf = (char *)malloc(u32CmdLength);
						if (pBuf == NULL)
						{
							s32Err = -1; /* big problem */
							goto end;
						}
						pBufTmp = pBuf;
						u32Start = i + pCycleBuf->u32Read;

						u32End = u32MSB - 1 + i + pCycleBuf->u32Read;
						PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
						for (j = u32Start; j < u32End; j++)
						{
							c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
							c8CheckSum ^= c8Tmp;
							*pBufTmp++ = c8Tmp;
						}
						c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
						if (c8CheckSum == c8Tmp) /* good message */
						{
							boIsBreak = true;
							*pBufTmp = c8Tmp;

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							PRINT_MFC("get a command: %d\n", u32MSB);
							PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_YNA;
							}
						}
						else
						{
							free(pBuf);
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							pCycleBuf->u32Flag = 0;
						}
					}
					else /* variable length */
					{
						uint32_t u32Start, u32End;
						uint32_t u32CmdLength = u32MSB;
						uint16_t u16CRCModBus;
						uint16_t u16CRCBuf;
						pBuf = (char *)malloc(u32CmdLength);
						if (pBuf == NULL)
						{
							s32Err = -1; /* big problem */
							goto end;
						}
						u32Start = (i + pCycleBuf->u32Read) % pCycleBuf->u32TotalLength;
						u32End = (u32MSB + i + pCycleBuf->u32Read) % pCycleBuf->u32TotalLength;
						PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
						if (u32End > u32Start)
						{
							memcpy(pBuf, pCycleBuf->pBuf + u32Start, u32MSB);
						}
						else
						{
							uint32_t u32FirstCopy = pCycleBuf->u32TotalLength - u32Start;
							memcpy(pBuf, pCycleBuf->pBuf + u32Start, u32FirstCopy);
							memcpy(pBuf + u32FirstCopy, pCycleBuf->pBuf, u32MSB - u32FirstCopy);
						}

						pCycleBuf->u32Flag = YNA_NORMAL_CMD;
						
						/* we need not check the head's check sum,
						 * just check the CRC16-MODBUS
						 */
						u16CRCModBus = CRC16((const uint8_t *)pBuf + PROTOCOL_YNA_DECODE_LENGTH, 
							u32MSB - PROTOCOL_YNA_DECODE_LENGTH - 2);
						u16CRCBuf = 0;

						LittleAndBigEndianTransfer((char *)(&u16CRCBuf), pBuf + u32MSB - 2, 2);
						if (u16CRCBuf == u16CRCModBus) /* good message */
						{
							boIsBreak = true;

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							PRINT_MFC("get a command: %d\n", u32MSB);
							PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_YNA;
							}
						}
						else
						{
							free(pBuf);
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						}
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
			else if(c8FirstByte == ((char)0xFA))
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_RQ_LENGTH)
				{
					volatile char c8CheckSum = 0, *pBufTmp, c8Tmp;
					volatile uint32_t j, u32Start, u32End;
					volatile uint32_t u32CmdLength = PROTOCOL_RQ_LENGTH;
					pBuf = (char *)malloc(u32CmdLength);
					if (pBuf == NULL)
					{
						s32Err = -1; /* big problem */
						goto end;
					}
					pBufTmp = pBuf;
					u32Start = i + pCycleBuf->u32Read;

					u32End = PROTOCOL_RQ_LENGTH - 1 + i + pCycleBuf->u32Read;
					PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
					for (j = u32Start; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						*pBufTmp++ = c8Tmp;
					}
					
					for (j = 2; j < PROTOCOL_RQ_LENGTH - 1; j++)
					{
						c8CheckSum += pBuf[j];
					}
					
					
					c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
					if (c8CheckSum == c8Tmp) /* good message */
					{
						boIsBreak = true;
						*pBufTmp = c8Tmp;

						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
						pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						PRINT_MFC("get a command: %d\n", u32MSB);
						PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
						*pLength = u32CmdLength;
						if (pProtocolType != NULL)
						{
							*pProtocolType = _Protocol_RQ;
						}
					}
					else
					{
						free(pBuf);
						pBuf = NULL;
						PRINT_MFC("get a wrong command: %d\n", u32MSB);
						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
						pCycleBuf->u32Read += (i + 1);
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
			
			else if((c8FirstByte & 0xF0) == ((char)0x80))
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_VISCA_MIN_LENGTH)
				{
					u32 j;
					u32 u32Start = i + pCycleBuf->u32Read;
					u32 u32End = pCycleBuf->u32Using + pCycleBuf->u32Read;
					char c8Tmp = 0;
					for (j = u32Start + PROTOCOL_VISCA_MIN_LENGTH - 1; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						if (c8Tmp == (char)0xFF)
						{
							u32End = j;
							break;
						}
					}
					if (c8Tmp == (char)0xFF)
					{
						/* wrong message */
						if ((u32End - u32Start + 1) > PROTOCOL_VISCA_MAX_LENGTH)
						{
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;							
						}
						else
						{
							char *pBufTmp;
							uint32_t u32CmdLength = u32End - u32Start + 1;
							pBuf = (char *)malloc(u32CmdLength);
							if (pBuf == NULL)
							{
								s32Err = -1; /* big problem */
								goto end;
							}	
							pBufTmp = pBuf;
							boIsBreak = true;
							
							PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
							for (j = u32Start; j <= u32End; j++)
							{
								*pBufTmp++ = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
							}
							
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							PRINT_MFC("get a command: %d\n", u32MSB);
							PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_VISCA;
							}

						}
					}
					else
					{
						/* wrong message */
						if ((u32End - u32Start) >= PROTOCOL_VISCA_MAX_LENGTH)
						{
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;							
						}
						else
						{
							pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
							pCycleBuf->u32Read += i;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							boIsBreak = true;						
						}
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
			else if(c8FirstByte == ((char)0xA5))
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_SB_LENGTH)
				{
					volatile char c8CheckSum = 0, *pBufTmp, c8Tmp;
					volatile uint32_t j, u32Start, u32End;
					volatile uint32_t u32CmdLength = PROTOCOL_SB_LENGTH;
					pBuf = (char *)malloc(u32CmdLength);
					if (pBuf == NULL)
					{
						s32Err = -1; /* big problem */
						goto end;
					}
					pBufTmp = pBuf;
					u32Start = i + pCycleBuf->u32Read;

					u32End = PROTOCOL_SB_LENGTH - 1 + i + pCycleBuf->u32Read;
					PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
					for (j = u32Start; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						*pBufTmp++ = c8Tmp;
					}
					
					for (j = 1; j < PROTOCOL_SB_LENGTH - 1; j++)
					{
						c8CheckSum += pBuf[j];
					}
					
					
					c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
					if (c8CheckSum == c8Tmp) /* good message */
					{
						boIsBreak = true;
						*pBufTmp = c8Tmp;

						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
						pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						PRINT_MFC("get a command: %d\n", u32MSB);
						PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
						*pLength = u32CmdLength;
						if (pProtocolType != NULL)
						{
							*pProtocolType = _Protocol_SB;
						}
					}
					else
					{
						free(pBuf);
						pBuf = NULL;
						PRINT_MFC("get a wrong command: %d\n", u32MSB);
						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
						pCycleBuf->u32Read += (i + 1);
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
		}
		if ((i == pCycleBuf->u32Using) && (!boIsBreak))
		{
			PRINT_MFC("cannot find AA, i = %d\n", pCycleBuf->u32Using);
			pCycleBuf->u32Using = 0;
			pCycleBuf->u32Read += i;
			pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
			pCycleBuf->u32Flag = 0;
		}

		if (boIsBreak)
		{
			break;
		}
	} while (((int32_t)pCycleBuf->u32Using) > 0);

	//if (pCycleBuf->u32Write + u32DataLength)

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pBuf;
}
void *YNAMakeAnArrayVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32Count, uint32_t u32Length, uint32_t *pCmdLength)
{
	uint32_t u32CmdLength;
	uint32_t u32DataLength;
	uint32_t u32Tmp;
	uint8_t *pCmd = NULL;
	uint8_t *pVarialbleCmd;
	if (pData == NULL)
	{
		return NULL;
	}
	
	u32DataLength = u32Count * u32Length;
	
	/*  */
	u32CmdLength = PROTOCOL_YNA_DECODE_LENGTH + 6 + u32DataLength + 2;
	pCmd = malloc(u32CmdLength);
	if (pCmd == NULL)
	{
		return NULL;
	}
	memset(pCmd, 0, u32CmdLength);
	pCmd[_YNA_Sync] = 0xAA;
	pCmd[_YNA_Mix] = 0x04;
	pCmd[_YNA_Cmd] = 0x00;
	
	/* total length */
	u32Tmp = u32CmdLength - PROTOCOL_YNA_DECODE_LENGTH;
	LittleAndBigEndianTransfer((char *)pCmd + _YNA_Data2, (const char *)(&u32Tmp), 2);
	
	YNAGetCheckSum(pCmd);
	
	pVarialbleCmd = pCmd + PROTOCOL_YNA_DECODE_LENGTH;
	
	/* command serial */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd, (const char *)(&u16Cmd), 2);

	/* command count */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 2, (const char *)(&u32Count), 2);

	/* Varialble data length */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 4, (const char *)(&u32Length), 2);
	
	/* copy the data */
	memcpy(pVarialbleCmd + 6, pData, u32DataLength);

	/* get the CRC16 of the variable command */
	u32Tmp = CRC16(pVarialbleCmd, 6 + u32DataLength);
	
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 6 + u32DataLength, 
		(const char *)(&u32Tmp), 2);

	if (pCmdLength != NULL)
	{
		*pCmdLength = u32CmdLength;
	}
	
	return pCmd;
}

void *YNAMakeASimpleVarialbleCmd(uint16_t u16Cmd, void *pData, 
	uint32_t u32DataLength, uint32_t *pCmdLength)
{
	return YNAMakeAnArrayVarialbleCmd(u16Cmd, pData, 1, u32DataLength, pCmdLength);
}

void CopyToUart1Message(void *pData, u32 u32Length)
{
	if ((pData != NULL) && (u32Length != 0))
	{
		void *pBuf = malloc(u32Length);
		if (pBuf != NULL)
		{
			memcpy(pBuf, pData, u32Length);
			if (MessageUartWrite(pBuf, true, _IO_Reserved, u32Length) != 0)
			{
				free (pBuf);
			}	
		}
	}

}
void CopyToUart3Message(void *pData, u32 u32Length)
{
	if ((pData != NULL) && (u32Length != 0))
	{
		void *pBuf = malloc(u32Length);
		if (pBuf != NULL)
		{
			memcpy(pBuf, pData, u32Length);
			if (MessageUart3Write(pBuf, true, _IO_Reserved, u32Length) != 0)
			{
				free (pBuf);
			}	
		}
	}

}

void CopyToUartMessage(void *pData, u32 u32Length)
{
	CopyToUart1Message(pData, u32Length);
	//CopyToUart3Message(pData, u32Length);
}
void CopyToUart2Message(void *pData, u32 u32Length)
{
}


u8 u8YNABuf[PROTOCOL_YNA_ENCODE_LENGTH];
u8 u8SBBuf[PROTOCOL_SB_LENGTH];

int32_t BaseCmdProcess(StIOFIFO *pFIFO, const StIOTCB *pIOTCB)
{
	uint8_t *pMsg;
	bool boGetVaildBaseCmd = true;
	if (pFIFO == NULL)
	{
		return -1;
	}
	pMsg = (uint8_t *)pFIFO->pData;
	
	if (pMsg[_YNA_Sync] != 0xAA)
	{
		return -1;
	}
	
	if (pMsg[_YNA_Mix] == 0x0C)
	{
		if (pMsg[_YNA_Cmd] == 0x80)
		{
			uint8_t u8EchoBase[PROTOCOL_YNA_DECODE_LENGTH] = {0};
			uint8_t *pEcho = NULL;
			uint32_t u32EchoLength = 0;
			bool boHasEcho = true;
			bool boNeedCopy = true;
			bool boNeedReset = false;
			u8EchoBase[_YNA_Sync] = 0xAA;
			u8EchoBase[_YNA_Mix] = 0x0C;
			u8EchoBase[_YNA_Cmd] = 0x80;
			u8EchoBase[_YNA_Data1] = 0x01;
			switch(pMsg[_YNA_Data3])
			{
				case 0x01:	/* just echo the same command */
				{
					//SetOptionByte(OPTION_UPGRADE_DATA);
					//boNeedReset = true;
				}
				case 0x02:	/* just echo the same command */
				{
					u8EchoBase[_YNA_Data3] = pMsg[_YNA_Data3];
					pEcho = (uint8_t *)malloc(PROTOCOL_YNA_DECODE_LENGTH);
					if (pEcho == NULL)
					{
						boHasEcho = false;
						break;
					}
					u32EchoLength = PROTOCOL_YNA_DECODE_LENGTH;						
					break;
				}
				case 0x03:	/* return the UUID */
				{
					StUID stUID;
					GetUID(&stUID);
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x8003, 
							&stUID, sizeof(StUID), &u32EchoLength);
					break;
				}
				case 0x05:	/* return the BufLength */
				{
					uint16_t u16BufLength = 0;
					if (pIOTCB != NULL
						 && pIOTCB->pFunGetMsgBufLength != 0)
					{
						u16BufLength = pIOTCB->pFunGetMsgBufLength();
					}
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x8005, 
							&u16BufLength, sizeof(uint16_t), &u32EchoLength);
					break;
				}
				case 0x09:	/* RESET the MCU */
				{
					NVIC_SystemReset();
					boHasEcho = false;
					break;
				}
				case 0x0B:	/* Get the application's CRC32 */
				{
					uint32_t u32CRC32 = ~0;
					u32CRC32 = AppCRC32(~0);
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x800B, 
							&u32CRC32, sizeof(uint32_t), &u32EchoLength);
					break;
				}
				case 0x0C:	/* get the version */
				{
					const char *pVersion = APP_VERSOIN;
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x800C, 
							(void *)pVersion, strlen(pVersion) + 1, &u32EchoLength);
					break;
				}
				
				default:
					boHasEcho = false;
					boGetVaildBaseCmd = false;
					break;
			}
			if (boHasEcho && pEcho != NULL)
			{
				if (boNeedCopy)
				{
					YNAGetCheckSum(u8EchoBase);
					memcpy(pEcho, u8EchoBase, PROTOCOL_YNA_DECODE_LENGTH);
				}
				if (pIOTCB == NULL)
				{
					free(pEcho);
				}
				else if (pIOTCB->pFunMsgWrite == NULL)
				{
					free(pEcho);			
				}
				else if(pIOTCB->pFunMsgWrite(pEcho, true, _IO_Reserved, u32EchoLength) != 0)
				{
					free(pEcho);
				}
			}
			
			/* send all the command in the buffer */
			if (boNeedReset)
			{
				//MessageUartFlush(true); 
				NVIC_SystemReset();
			}
		}
	}
	else if (pMsg[_YNA_Mix] == 0x04)	/* variable command */
	{
		uint32_t u32TotalLength = 0;
		uint32_t u32ReadLength = 0;
		uint8_t *pVariableCmd;
		
		boGetVaildBaseCmd = false;
		
		/* get the total command length */
		LittleAndBigEndianTransfer((char *)(&u32TotalLength), (const char *)pMsg + _YNA_Data2, 2);
		pVariableCmd = pMsg + PROTOCOL_YNA_DECODE_LENGTH;
		u32TotalLength -= 2; /* CRC16 */
		while (u32ReadLength < u32TotalLength)
		{
			uint8_t *pEcho = NULL;
			uint32_t u32EchoLength = 0;
			bool boHasEcho = true;
			uint16_t u16Command = 0, u16Count = 0, u16Length = 0;
			LittleAndBigEndianTransfer((char *)(&u16Command),
				(char *)pVariableCmd, 2);
			LittleAndBigEndianTransfer((char *)(&u16Count),
				(char *)pVariableCmd + 2, 2);
			LittleAndBigEndianTransfer((char *)(&u16Length),
				(char *)pVariableCmd + 4, 2);

			switch (u16Command)
			{
				case 0x800A:
				{
					/* check the crc32 and UUID, and BTEA and check the number */
					int32_t s32Err;
					char *pData = (char *)pVariableCmd + 6;
					StBteaKey stLic;
					StUID stUID;
					uint32_t u32CRC32;
					
					GetUID(&stUID);
					u32CRC32 = AppCRC32(~0);
					GetLic(&stLic, &stUID, u32CRC32, true);
					
					if (memcmp(&stLic, pData, sizeof(StBteaKey)) == 0)
					{
						s32Err = 0;
						
						WriteLic(&stLic, true, 0);
					}
					else
					{
						s32Err = -1;
					}
					pEcho = YNAMakeASimpleVarialbleCmd(0x800A, &s32Err, 4, &u32EchoLength);
					boGetVaildBaseCmd = true;
					break;
				}
				default:
					break;
			}
			
			if (boHasEcho && pEcho != NULL)
			{
				if (pIOTCB == NULL)
				{
					free(pEcho);
				}
				else if (pIOTCB->pFunMsgWrite == NULL)
				{
					free(pEcho);
				}
				else if(pIOTCB->pFunMsgWrite(pEcho, true, _IO_Reserved, u32EchoLength) != 0)
				{
					free(pEcho);
				}
			}
			
			
			u32ReadLength += (6 + (uint32_t)u16Count * u16Length);
			pVariableCmd = pMsg + PROTOCOL_YNA_DECODE_LENGTH + u32ReadLength;
		}
	}
	else
	{
		boGetVaildBaseCmd = false;
	}

	return boGetVaildBaseCmd ? 0: -1;
}


void GlobalStateInit(void)
{
	g_u8CamAddr = 0;
	
	g_boUsingUART2 = true;
	g_boIsPushRodNeedReset = false;
	g_boIsBroadcastPlay = false;
	g_boIsDDRPlay = false;
	g_boIsRockEnable = true;	
}


void ChangeEncodeState(void)
{

}

void YNADecode(u8 *pBuf)
{
	if (g_u32BoolIsEncode)
	{
			
	}
	else
	{
		
	}

}
void YNAEncodeAndGetCheckSum(u8 *pBuf)
{
	if (g_u32BoolIsEncode)
	{
			
	}
	else
	{
		
	}
}


void YNAGetCheckSum(u8 *pBuf)
{
	s32 i, s32End;
	u8 u8Sum = pBuf[0];

	if (g_u32BoolIsEncode)
	{
		s32End = PROTOCOL_YNA_ENCODE_LENGTH - 1;	
	}
	else
	{
		s32End = PROTOCOL_YNA_DECODE_LENGTH - 1;
	}
	for (i = 1; i < s32End; i++)
	{
		u8Sum ^= pBuf[i];
	}
	pBuf[i] = u8Sum;
}

void PelcoDGetCheckSum(u8 *pBuf)
{
	s32 i;
	u8 u8Sum = 0;
	for (i = 1; i < 6; i++)
	{
		u8Sum += pBuf[i];
	}
	pBuf[i] = u8Sum;
}

void SBGetCheckSum(u8 *pBuf)
{
	s32 i;
	u8 u8Sum = 0;
	for (i = 1; i < PROTOCOL_SB_LENGTH - 1; i++)
	{
		u8Sum += pBuf[i];
	}
	pBuf[i] = u8Sum;
}

bool ProtocolSelect(StIOFIFO *pFIFO)
{
	const u8 u8KeyMap[] = 
	{
		_Key_PGM_1, _Key_PGM_2
	};
	const u16 u16LedMap[] = 
	{
		_Led_PGM_1, _Led_PGM_2
	};
	
	u32 u32MsgSentTime;
	u32 u32State = 0;
	StKeyMixIn *pKeyIn;
	StKeyState *pKey;
	
	if (pFIFO == NULL)
	{
		return false;
	}
		
	pKeyIn = pFIFO->pData;
	if (pKeyIn == NULL)
	{
		return false;
	}

	if (pKeyIn->emKeyType != _Key_Board)
	{
		return false;
	}
	
	pKey = &(pKeyIn->unKeyMixIn.stKeyState[0]);
	if (pKey->u8KeyValue != (u8)_Key_PVW_1)
	{
		return false;		
	}


	ChangeAllLedState(false);
	
	u32MsgSentTime = g_u32SysTickCnt;
	while(1)
	{
		if (pKeyIn != NULL)
		{
			pKey = &(pKeyIn->unKeyMixIn.stKeyState[0]);
			if (u32State == 0)
			{
				if (pKey->u8KeyValue == (u8)_Key_PVW_1)
				{
					u32MsgSentTime = g_u32SysTickCnt;
					if (pKey->u8KeyState == KEY_DOWN)
					{
						ChangeAllLedState(false);
						ChangeLedState(GET_XY(_Led_PVW_1), true);
					}
					else if (pKey->u8KeyState == KEY_UP)
					{
						//ChangeLedState(LED(_Led_Switch_Video_Bak5), false);
						u32State = 1;
					}
				}			
			}
			else if (u32State == 1) /* get protocol or bandrate */
			{
				u32 u32Index = ~0;
				u32 i;
				for (i = 0; i < sizeof(u8KeyMap); i++)
				{
					if (pKey->u8KeyValue == u8KeyMap[i])
					{
						u32Index = i;
						break;
					}						
				}
				if (u32Index != ~0)
				{
					u32MsgSentTime = g_u32SysTickCnt;
					if (pKey->u8KeyState == KEY_DOWN)
					{
						ChangeLedState(GET_XY(u16LedMap[u32Index]), true);
					}
					else if (pKey->u8KeyState == KEY_UP)
					{
						ChangeLedState(GET_XY(u16LedMap[u32Index]), false);
						u32State = 2 + u32Index;
						break;
					}
				}
			}
		}
		
		KeyBufGetEnd(pFIFO);
		
		pFIFO = NULL;
		
		if (SysTimeDiff(u32MsgSentTime, g_u32SysTickCnt) > 5000) /* 10S */
		{
			ChangeAllLedState(false);
			return true;
		}

		
		pKeyIn = NULL;
		pFIFO = KeyBufGetBuf();
		if (pFIFO == NULL)
		{
			continue;
		}
		
		pKeyIn = pFIFO->pData;
		if (pKeyIn == NULL)
		{
			KeyBufGetEnd(pFIFO);
			pFIFO = NULL;
			continue;
		}

		if (pKeyIn->emKeyType != _Key_Board)
		{
			pKeyIn = NULL;
			KeyBufGetEnd(pFIFO);
			pFIFO = NULL;
			continue;
		}	
	}
	
	
	KeyBufGetEnd(pFIFO);
	
	
	if (u32State >= 2)
	{
		switch (u32State)
		{
			case 2:
			{
				g_emProtocol = _Protocol_YNA;
				break;
			}
			case 3:
			{
				g_emProtocol = _Protocol_SB;
				break;
			}
			default:
				break;
		}
		
		
		if (WriteSaveData())
		{
			u32MsgSentTime = g_u32SysTickCnt;
			ChangeAllLedState(true);
			while(SysTimeDiff(u32MsgSentTime, g_u32SysTickCnt) < 1500);/* 延时1s */
			ChangeAllLedState(false);
			return true;
		}

		{
			bool boBlink = true;
			u32 u32BlinkCnt = 0;
			while (u32BlinkCnt < 10)
			{
				boBlink = !boBlink;
				ChangeLedState(GET_XY(_Led_PVW_1), boBlink);
				u32MsgSentTime = g_u32SysTickCnt;
				while(SysTimeDiff(u32MsgSentTime, g_u32SysTickCnt) < 100);/* 延时1s */
				u32BlinkCnt++;
			}
		}	
	}
	
	ChangeAllLedState(false);
	return false;
}


bool BackgroundLightEnableChange(StIOFIFO *pFIFO)
{
	const u8 u8KeyMap[] = 
	{
		_Key_PGM_1, _Key_PGM_2
	};
	const u16 u16LedMap[] = 
	{
		_Led_PGM_1, _Led_PGM_2
	};
	
	u32 u32MsgSentTime;
	u32 u32State = 0;
	StKeyMixIn *pKeyIn;
	StKeyState *pKey;
	
	if (pFIFO == NULL)
	{
		return false;
	}
		
	pKeyIn = pFIFO->pData;
	if (pKeyIn == NULL)
	{
		return false;
	}

	if (pKeyIn->emKeyType != _Key_Board)
	{
		return false;
	}
	
	pKey = &(pKeyIn->unKeyMixIn.stKeyState[0]);
	if (pKey->u8KeyValue != (u8)_Key_PVW_2)
	{
		return false;		
	}


	ChangeAllLedState(false);
	
	u32MsgSentTime = g_u32SysTickCnt;
	while(1)
	{
		if (pKeyIn != NULL)
		{
			pKey = &(pKeyIn->unKeyMixIn.stKeyState[0]);
			if (u32State == 0)
			{
				if (pKey->u8KeyValue == (u8)_Key_PVW_2)
				{
					u32MsgSentTime = g_u32SysTickCnt;
					if (pKey->u8KeyState == KEY_DOWN)
					{
						ChangeAllLedState(false);
						ChangeLedState(GET_XY(_Led_PVW_2), true);
					}
					else if (pKey->u8KeyState == KEY_UP)
					{
						u32State = 1;
					}
				}			
			}
			else if (u32State == 1)
			{
				u32 u32Index = ~0;
				u32 i;
				for (i = 0; i < sizeof(u8KeyMap); i++)
				{
					if (pKey->u8KeyValue == u8KeyMap[i])
					{
						u32Index = i;
						break;
					}						
				}
				if (u32Index != ~0)
				{
					u32MsgSentTime = g_u32SysTickCnt;
					if (pKey->u8KeyState == KEY_DOWN)
					{
						ChangeLedState(GET_XY(u16LedMap[u32Index]), true);
					}
					else if (pKey->u8KeyState == KEY_UP)
					{
						ChangeLedState(GET_XY(u16LedMap[u32Index]), false);
						u32State = 2 + u32Index;
						break;
					}
				}
			}
		}
		
		KeyBufGetEnd(pFIFO);
		
		pFIFO = NULL;
		
		if (SysTimeDiff(u32MsgSentTime, g_u32SysTickCnt) > 5000) /* 10S */
		{
			ChangeAllLedState(false);
			return true;
		}

		
		pKeyIn = NULL;
		pFIFO = KeyBufGetBuf();
		if (pFIFO == NULL)
		{
			continue;
		}
		
		pKeyIn = pFIFO->pData;
		if (pKeyIn == NULL)
		{
			KeyBufGetEnd(pFIFO);
			pFIFO = NULL;
			continue;
		}

		if (pKeyIn->emKeyType != _Key_Board)
		{
			pKeyIn = NULL;
			KeyBufGetEnd(pFIFO);
			pFIFO = NULL;
			continue;
		}	
	}
	
	
	KeyBufGetEnd(pFIFO);
	
	
	if (u32State >= 2)
	{
		switch (u32State)
		{
			case 2:
			{
				g_boIsBackgroundLightEnable = true;
				break;
			}
			case 3:
			{
				g_boIsBackgroundLightEnable = false;
				break;
			}
			default:
				break;
		}
		
		
		if (WriteSaveData())
		{
			u32MsgSentTime = g_u32SysTickCnt;
			ChangeAllLedState(true);
			while(SysTimeDiff(u32MsgSentTime, g_u32SysTickCnt) < 1500);/* 延时1s */
			ChangeAllLedState(false);
			return true;
		}

		{
			bool boBlink = true;
			u32 u32BlinkCnt = 0;
			while (u32BlinkCnt < 10)
			{
				boBlink = !boBlink;
				ChangeLedState(GET_XY(_Led_PVW_2), boBlink);
				u32MsgSentTime = g_u32SysTickCnt;
				while(SysTimeDiff(u32MsgSentTime, g_u32SysTickCnt) < 100);/* 延时1s */
				u32BlinkCnt++;
			}
		}	
	}
	
	ChangeAllLedState(false);
	return false;
}


void TallyUartSend(u8 Tally1, u8 Tally2)
{
	u32 i, j;
	u8 u8Buf[4] = {0xFC, 0x00, 0x00, 0x00};

	for (j = 0; j < 2; j++)
	{	
		for(i = 0; i < 4; i++)
		{
			if (((Tally1 >> (4 * j + i)) & 0x01) != 0x00)
			{
				u8Buf[1 + j] |= (1 << (i * 2));
			}
			
			if (((Tally2 >> (4 * j + i)) & 0x01) != 0x00)
			{
				u8Buf[1 + j] |= (1 << (i * 2 + 1));			
			}			
		}	
	}
	
	u8Buf[3] = u8Buf[0] ^ u8Buf[1] ^ u8Buf[2];
	
	CopyToUart3Message(u8Buf, 4);
}
u8 g_u8YNATally[2] = {0, 0};

void SetTallyPGM(u8 u8Index, bool boIsLight, bool boIsClear, bool boIsSend)
{
	if (u8Index > 7)
	{
		return;
	}
	if (boIsClear)
	{
		g_u8YNATally[0] = 0;
	}
	if (boIsLight)
	{
		g_u8YNATally[0] |= (1 << u8Index);
	}
	else 
	{
		g_u8YNATally[0] &= ~(1 << u8Index);					
	}
	if (boIsSend)
	{
		TallyUartSend(g_u8YNATally[0], g_u8YNATally[1]);					
	}
	
}

void SetTallyPVW(u8 u8Index, bool boIsLight, bool boIsClear, bool boIsSend)
{
	if (u8Index > 7)
	{
		return;
	}
	if (boIsClear)
	{
		g_u8YNATally[1] = 0;
	}
	if (boIsLight)
	{
		g_u8YNATally[1] |= (1 << u8Index);
	}
	else 
	{
		g_u8YNATally[1] &= ~(1 << u8Index);					
	}
	if (boIsSend)
	{
		TallyUartSend(g_u8YNATally[0], g_u8YNATally[1]);					
	}
	
}


static bool KeyBoardProcess(StKeyMixIn *pKeyIn)
{
	u32 i;
	for (i = 0; i < pKeyIn->u32Cnt; i++)
	{
		u8 *pBuf;
		StKeyState *pKeyState = pKeyIn->unKeyMixIn.stKeyState + i;
		u8 u8KeyValue;
		if (pKeyState->u8KeyState == KEY_KEEP)
		{
			continue;
		}

		u8KeyValue = pKeyState->u8KeyValue;

		pBuf = u8YNABuf;

		memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

		pBuf[_YNA_Sync] = 0xAA;
		pBuf[_YNA_Addr] = g_u8CamAddr;
		pBuf[_YNA_Mix] = 0x07;
		if (pKeyState->u8KeyState == KEY_UP)
		{
			pBuf[_YNA_Data1] = 0x01;
		}

		/* 处理按键 */
		switch (u8KeyValue)
		{
			case _Key_Record_Record:
			case _Key_Record_Pause:
			case _Key_Record_Stop:
			{
				pBuf[_YNA_Cmd] = 0x47;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Record_Record + 0x02;

				break;
			}
			
			case _Key_Fun_Reserved1:
			case _Key_Fun_Reserved2:
			case _Key_Fun_Reserved3:
			{
				pBuf[_YNA_Cmd] = 0x4C;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Fun_Reserved1 + 0x80;			
				break;
			}
			
			case _Key_Fun_CG1:
			case _Key_Fun_CG2:
			case _Key_Fun_CG3:
			case _Key_Fun_CG4:
			case _Key_Fun_CG5:
			case _Key_Fun_CG6:
			{
				pBuf[_YNA_Cmd] = 0x4C;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Fun_CG1 + 0x90;			
				break;
			}

			case _Key_PGM_1:
			case _Key_PGM_2:
			case _Key_PGM_3:
			case _Key_PGM_4:
			case _Key_PGM_5:
			case _Key_PGM_6:
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_PGM_1 + 0x02;			
				break;
			}
			case _Key_PGM_7:
			case _Key_PGM_8:
			case _Key_PGM_9:
			case _Key_PGM_10:
			case _Key_PGM_11:
			case _Key_PGM_12:
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_PGM_7 + 0x80;			
				break;
			}


			case _Key_PVW_1:
			case _Key_PVW_2:
			case _Key_PVW_3:
			case _Key_PVW_4:
			case _Key_PVW_5:
			case _Key_PVW_6:
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_PVW_1 + 0x08;			
				break;
			}
			case _Key_PVW_7:
			case _Key_PVW_8:
			case _Key_PVW_9:
			case _Key_PVW_10:
			case _Key_PVW_11:
			case _Key_PVW_12:	
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_PVW_7 + 0x90;			
				break;
			}

			case _Key_Cam_1:
			case _Key_Cam_2:
			case _Key_Cam_3:
			case _Key_Cam_4:
			{
				pBuf[_YNA_Cmd] = 0x44;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Cam_1;			
				break;
			}
			
			case _Key_Cam_Ctrl_Tele:
			{

				if (pKeyState->u8KeyState == KEY_DOWN)
				{
					u8 u8Cmd;
					ChangeLedStateWithBackgroundLight(GET_XY(_Led_Cam_Ctrl_Tele), true);
					u8Cmd = u8KeyValue - _Key_Cam_Ctrl_Tele + 1;
					pBuf[_YNA_Cmd] = u8Cmd << 4;
					pBuf[_YNA_Data3] = 0x0F;
				}
				else
				{
					pBuf[_YNA_Data1] = 0x00;

					ChangeLedStateWithBackgroundLight(GET_XY(_Led_Cam_Ctrl_Tele), false);

				}
				break;
			}
			case _Key_Cam_Ctrl_Wide:
			{
				if (pKeyState->u8KeyState == KEY_DOWN)
				{
					u8 u8Cmd;
					ChangeLedStateWithBackgroundLight(GET_XY(_Led_Cam_Ctrl_Wide), true);
					u8Cmd = u8KeyValue - _Key_Cam_Ctrl_Tele + 1;
					pBuf[_YNA_Cmd] = u8Cmd << 4;
					pBuf[_YNA_Data3] = 0x0F;					
				}
				else
				{
					pBuf[_YNA_Data1] = 0x00;
					ChangeLedStateWithBackgroundLight(GET_XY(_Led_Cam_Ctrl_Wide), false);
				}
				break;
			}

			case _Key_Effect_1:
			case _Key_Effect_2:
			case _Key_Effect_3:
			case _Key_Effect_4:
			case _Key_Effect_5:
			case _Key_Effect_6:
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Effect_1 + 0x0E;			
				break;
			}
			
			case _Key_Effect_Ctrl_Take:
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Effect_Ctrl_Take;	
				break;
			}
			case _Key_Effect_Ctrl_Cut:
			{
				pBuf[_YNA_Cmd] = 0x48;
				pBuf[_YNA_Data2] = u8KeyValue - _Key_Effect_Ctrl_Take;	
				ChangeLedStateWithBackgroundLight(GET_XY(_Led_Effect_Ctrl_Cut), !pBuf[_YNA_Data1]);						
				break;
			}

			default:
				continue;
		}

		
		YNAGetCheckSum(pBuf);
		CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);	
	}
	return true;
}

static bool RockProcess(StKeyMixIn *pKeyIn)
{
	u8 *pBuf;
#if SCENCE_MUTUAL
	TurnOffZoomScence();
	PresetNumInit();
#endif
	if (!g_boIsRockEnable)
	{
		return false;
	}

	pBuf = u8YNABuf;

	memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

	pBuf[_YNA_Sync] = 0xAA;
	pBuf[_YNA_Addr] = g_u8CamAddr;
	pBuf[_YNA_Mix] = 0x07;

	pBuf[_YNA_Cmd] = pKeyIn->unKeyMixIn.stRockState.u8RockDir;
	pBuf[_YNA_Data1] = pKeyIn->unKeyMixIn.stRockState.u16RockXValue;
	pBuf[_YNA_Data2] = pKeyIn->unKeyMixIn.stRockState.u16RockYValue;

	pBuf[_YNA_Data3] = pKeyIn->unKeyMixIn.stRockState.u16RockZValue;
	if (g_boIsRockAus)
	{
		pBuf[_YNA_Data1] |= (0x01 << 6);
	}
	YNAGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
	
	if (!g_boUsingUART2)
	{
		return true;
	}
	if (g_emProtocol == _Protocol_PecloD)
	{
		u8 u8Buf[7];
		u8Buf[_PELCOD_Sync] = 0xFF;
		u8Buf[_PELCOD_Addr] = g_u8CamAddr + 1;
		u8Buf[_PELCOD_Cmd1] = 0;
		u8Buf[_PELCOD_Cmd2] = pKeyIn->unKeyMixIn.stRockState.u8RockDir << 1;
		u8Buf[_PELCOD_Data1] = pKeyIn->unKeyMixIn.stRockState.u16RockXValue;
		u8Buf[_PELCOD_Data2] = pKeyIn->unKeyMixIn.stRockState.u16RockYValue;
		PelcoDGetCheckSum(u8Buf);
		CopyToUart2Message(u8Buf, 7);
	}
	else
	{
		u8 u8Buf[16];
		u8 u8Cmd = pKeyIn->unKeyMixIn.stRockState.u8RockDir << 1;
		static bool boViscaNeedSendZoomStopCmd = false;
		static bool boViscaNeedSendDirStopCmd = false;
		static u8 u8Priority = 0;
		
		u8Cmd &= (PELCOD_DOWN | PELCOD_UP | 
					PELCOD_LEFT | PELCOD_RIGHT |
					PELCOD_ZOOM_TELE | PELCOD_ZOOM_WIDE);
		
		u8Buf[0] = 0x80 + g_u8CamAddr + 1;
		if (u8Priority == 0)
		{
			if ((u8Cmd & (PELCOD_DOWN | PELCOD_UP | PELCOD_LEFT | PELCOD_RIGHT)) != 0)
			{
				u8Priority = 1;
			}
			else if ((u8Cmd & (PELCOD_ZOOM_TELE | PELCOD_ZOOM_WIDE)) != 0)
			{
				u8Priority = 2;
			}
		}
		
		if (u8Priority == 1)
		{
			if (boViscaNeedSendDirStopCmd && 
				((u8Cmd & (PELCOD_DOWN | PELCOD_UP | PELCOD_LEFT | PELCOD_RIGHT)) == 0))
			{
				/* 81 01 06 01 18 18 03 03 FF */
				u8Buf[1] = 0x01;
				u8Buf[2] = 0x06;
				u8Buf[3] = 0x01;
				u8Buf[4] = 0x00;
				u8Buf[5] = 0x00;
				u8Buf[6] = 0x03;
				u8Buf[7] = 0x03;
				u8Buf[8] = 0xFF;
				CopyToUart2Message(u8Buf, 9);
				boViscaNeedSendDirStopCmd = false;
				if ((u8Cmd & (PELCOD_ZOOM_WIDE | PELCOD_ZOOM_WIDE)) != 0)
				{
					u8Priority = 2;
				}
				else
				{
					u8Priority = 0;					
				}
			}
			else
			{
				u8Buf[1] = 0x01;
				u8Buf[2] = 0x06;
				u8Buf[3] = 0x01;
				if ((u8Cmd & (PELCOD_LEFT | PELCOD_RIGHT)) != 0)
				{
					u32 u32Tmp = 0x17 * pKeyIn->unKeyMixIn.stRockState.u16RockXValue;
					u32Tmp /= 0x3F;
					u32Tmp %= 0x18;
					u32Tmp += 1;

					u8Buf[4] = u32Tmp;
					if ((u8Cmd & PELCOD_LEFT) != 0)
					{
						u8Buf[6] = 0x01;
					}
					else
					{
						u8Buf[6] = 0x02;
					}

				}
				else
				{
					u8Buf[4] = 0;
					u8Buf[6] = 0x03;
				}
				
				if ((u8Cmd & (PELCOD_UP | PELCOD_DOWN)) != 0)
				{
					u32 u32Tmp = 0x13 * pKeyIn->unKeyMixIn.stRockState.u16RockYValue;
					u32Tmp /= 0x3F;
					u32Tmp %= 0x14;
					u32Tmp += 1;

					u8Buf[5] = u32Tmp;
					if ((u8Cmd & PELCOD_UP) != 0)
					{
						u8Buf[7] = 0x01;
					}
					else
					{
						u8Buf[7] = 0x02;
					}

				}
				else
				{
					u8Buf[5] = 0;
					u8Buf[7] = 0x03;
				}
				u8Buf[8] = 0xFF;
				CopyToUart2Message(u8Buf, 9);	
				boViscaNeedSendDirStopCmd = true;			
			}	
		}
		
		if (u8Priority == 2)
		{
			if (boViscaNeedSendZoomStopCmd && 
					((u8Cmd & (PELCOD_ZOOM_WIDE | PELCOD_ZOOM_TELE)) == 0))
			{
				u8Buf[1] = 0x01;
				u8Buf[2] = 0x04;
				u8Buf[3] = 0x07;
				u8Buf[4] = 0x00;
				u8Buf[5] = 0xFF;
				CopyToUart2Message(u8Buf, 6);
				boViscaNeedSendZoomStopCmd = false;
				u8Priority = 0;
			}
			else if ((u8Cmd & PELCOD_ZOOM_WIDE) == PELCOD_ZOOM_WIDE)
			{
				u32 u32Tmp = 0x05 * pKeyIn->unKeyMixIn.stRockState.u16RockZValue;
				u32Tmp /= 0x0F;
				u32Tmp %= 6;
				u32Tmp += 2;
				u8Buf[1] = 0x01;
				u8Buf[2] = 0x04;
				u8Buf[3] = 0x07;
				u8Buf[4] = u32Tmp + 0x30;
				u8Buf[5] = 0xFF;
				CopyToUart2Message(u8Buf, 6);
				boViscaNeedSendZoomStopCmd = true;

			}
			else
			{
				u32 u32Tmp = 0x05 * pKeyIn->unKeyMixIn.stRockState.u16RockZValue;
				u32Tmp /= 0x0F;
				u32Tmp %= 6;
				u32Tmp += 2;
				u8Buf[1] = 0x01;
				u8Buf[2] = 0x04;
				u8Buf[3] = 0x07;
				u8Buf[4] = 0x20 + u32Tmp;
				u8Buf[5] = 0xFF;
				CopyToUart2Message(u8Buf, 6);			
					boViscaNeedSendZoomStopCmd = true;
			}
			
		}
		
		if (u8Cmd == 0)
		{
			u8Priority = 0;
		}
	}
	return true;
}

void TPushTurnLight(u32 u32RawValue)
{
	const u16 u16Led[] = 
	{
		_Led_TPush_1 ,
		_Led_TPush_2 ,
		_Led_TPush_3 ,
		_Led_TPush_4 ,
		_Led_TPush_5 ,
		_Led_TPush_6 ,
		_Led_TPush_7 ,
		_Led_TPush_8 ,
		_Led_TPush_9 ,
		_Led_TPush_10,
		_Led_TPush_11,
		_Led_TPush_12,
	};
	
	u32 i;
	for (i = 0; i < (sizeof(u16Led) / sizeof(u16)); i++)
	{
		ChangeLedState(GET_XY(u16Led[i]), false);
	}
	
	if (u32RawValue != (~0))
	{
		u32 u32Index = u32RawValue * 
			((sizeof(u16Led) / sizeof(u16))) / 
			PUSH_ROD_MAX_VALUE;
		if (u32Index >= (sizeof(u16Led) / sizeof(u16)))
		{
			u32Index -= 1;
		}
		
		ChangeLedState(GET_XY(u16Led[u32Index]), true);

	}
	
}


static bool PushPodProcess(StKeyMixIn *pKeyIn)
{
	u8 *pBuf;
	pBuf = u8YNABuf;
	if (pBuf == NULL)
	{
		return false;
	}

	memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

	pBuf[_YNA_Sync] = 0xAA;
	pBuf[_YNA_Addr] = g_u8CamAddr;
	pBuf[_YNA_Mix] = 0x07;

	pBuf[_YNA_Cmd] = 0x80;
	
	pBuf[_YNA_Data1] |= 0x80;

	pBuf[_YNA_Data2] = pKeyIn->unKeyMixIn.u32PushRodValue;
	YNAGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
	TPushTurnLight(pKeyIn->unKeyMixIn.u32PushRodValue);
	return true;
}


#if 0
static bool CodeSwitchProcess(StKeyMixIn *pKeyIn)
{
	u8 *pBuf;
	u16 u16Index;

	pBuf = u8YNABuf;
	if (pBuf == NULL)
	{
		return false;
	}

	memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

	pBuf[_YNA_Sync] = 0xAA;
	pBuf[_YNA_Addr] = g_u8CamAddr;
	pBuf[_YNA_Mix] = 0x07;

	u16Index = pKeyIn->unKeyMixIn.stCodeSwitchState.u16Index;
	switch (u16Index)
	{
		case 0x00:
		{
			pBuf[_YNA_Cmd] = 0x49;
			pBuf[_YNA_Data1] |= 0x80;
			break;
		}
		default:
			return false;

	}
	pBuf[_YNA_Data2] = pKeyIn->unKeyMixIn.stCodeSwitchState.u16Cnt;
	YNAGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
	return true;
	
}
#endif
static bool VolumeProcess(StKeyMixIn *pKeyIn)
{
	u8 *pBuf;
	pBuf = u8YNABuf;
	if (pBuf == NULL)
	{
		return false;
	}

	memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

	pBuf[_YNA_Sync] = 0xAA;
	pBuf[_YNA_Addr] = g_u8CamAddr;
	pBuf[_YNA_Mix] = 0x06;

	pBuf[_YNA_Cmd] = 0x80;
	pBuf[_YNA_Data3] = pBuf[_YNA_Data2] = pKeyIn->unKeyMixIn.u32VolumeValue;
	YNAGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
	return true;
}

static PFun_KeyProcess s_KeyProcessArr[_Key_Reserved] = 
{
	PushPodProcess, KeyBoardProcess, RockProcess,
	VolumeProcess, NULL, 
};

/*
BYTE0          XL      摇杆 X 轴低位	X 轴（00-3FF）
BYTE1          XH      摇杆 X 轴高位	
BYTE2          YL      摇杆 Y 轴低位	Y 轴（00-3FF）
BYTE3          YH      摇杆 Y 轴高位	
BYTE4          ZL      摇杆 Z 轴低位	Z 轴（00-3FF）
BYTE5          ZH      摇杆 Z 轴高位
BYTE6        Audio-1   音量 1          （00-64）HEX
BYTE7        Audio-2   音量 2          （00-64）HEX
BYTE8        Audio-3   音量 3          （00-64）HEX
BYTE9        Audio-4   音量 4          （00-64）HEX
BYTE10       T 推杆    推杆            （00-64）HEX
BYTE11       Button1   按钮 1-8        （00-FF）HEX 
BYTE12       Button2   按钮 9-16       （00-FF）HEX 
BYTE13       Button3   按钮 17-24      （00-FF）HEX 
BYTE14       Button4   按钮 25-32      （00-FF）HEX 
BYTE15       Button5   按钮 33-40      （00-FF）HEX 
BYTE16       Button6   按钮 41-48      （00-FF）HEX 
BYTE17       Button7   按钮 49-56      （00-FF）HEX 
BYTE18       Button8   按钮 57-63      （00-FF）HEX

*/

static u8 s_u8AllBackupForSB[20] = {0x00, 0x02, 0x00, 0x02, 0x00, 0x02};
static bool s_boIsDataUpdateForSB = false;

static u8 s_u8BtnBackupForSB[8] = {0};
static u8 s_u8LedBackupForSB[8] = {0};

void SBUpdateLed(void)
{
	u32 i, j;

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			u16 u16Led = c_u16LedGlobalDoubleColorArr[i][j];
			bool boIsLight;
			if (u16Led == 0xFFFF)
			{
				continue;
			}
			
			boIsLight = (((s_u8LedBackupForSB[i] >> j) & 0x01) == 0) ? false : true;
			if (boIsLight)
			{
				ChangeLedStateWithBackgroundLight(GET_XY(u16Led), true);
			}
			else
			{
				ChangeLedStateWithBackgroundLight(GET_XY(u16Led), false);
			}
			
			{/* tally */
				int k = i * 8 + j;
				if (k >= 12 && k < 24)
				{
					SetTallyPGM(k - 12, boIsLight, false, false);
				}
				else if (k >= 24 && k < 36)
				{				
					SetTallyPVW(k - 24, boIsLight, false, false);
				}
				
			}

		}
	}
	TallyUartSend(g_u8YNATally[0], g_u8YNATally[1]);					

}

static bool KeyBoardProcessForSB(StKeyMixIn *pKeyIn)
{
	u32 i;
	u8 *pBuf = u8SBBuf;
	bool boHasChange = false;

	for (i = 0; i < pKeyIn->u32Cnt; i++)
	{
		StKeyState *pKeyState = pKeyIn->unKeyMixIn.stKeyState + i;
		u8 u8KeyValue;
		u8 u8Arr = 0;
		u8 u8Bit = 0;
		if (pKeyState->u8KeyState == KEY_KEEP)
		{
			continue;
		}

		u8KeyValue = pKeyState->u8KeyValue;
		
		u8KeyValue -= 1;
		u8Arr = u8KeyValue >> 3;
		u8Bit = u8KeyValue & 0x07;
		
		if (u8Arr >= 8)
		{
			continue;
		}
		
		boHasChange = true;
		if (pKeyState->u8KeyState == KEY_DOWN)	
		{
			s_u8BtnBackupForSB[u8Arr] |= (1 << u8Bit);
		}
		else
		{
			s_u8BtnBackupForSB[u8Arr] &= (~(1 << u8Bit));		
		}			
	}
	if (boHasChange)
	{
		pBuf[0] = 0xFF;
		pBuf[1] = 0x04;
		memcpy(pBuf + 2, s_u8BtnBackupForSB, 8);
		
		memcpy(s_u8AllBackupForSB + 11, s_u8BtnBackupForSB, 8);
		s_boIsDataUpdateForSB = true;
		
		SBGetCheckSum(pBuf);
		CopyToUartMessage(pBuf, PROTOCOL_SB_LENGTH);	
	}
	
	return true;
}

static bool RockProcessForSB(StKeyMixIn *pKeyIn)
{
	u8 *pBuf = u8SBBuf;
	u16 u16Tmp = 0;
	u8 u8Dir = 0;

	memset(pBuf, 0, PROTOCOL_SB_LENGTH);

	
	u8Dir = pKeyIn->unKeyMixIn.stRockState.u8RockDir;
	u16Tmp = pKeyIn->unKeyMixIn.stRockState.u16RockXValue;
	if ((u8Dir & _YNA_CAM_LEFT) != 0)
	{
		u16Tmp = (u16Tmp << 3) + 0x200 + 0x07;
	}
	else if ((u8Dir & _YNA_CAM_RIGHT) != 0)
	{
		u16Tmp = (u16Tmp << 3) + 0x07;
	}
	else 
	{
		u16Tmp = 0x200;
	}
	
	pBuf[0] = 0xFF;
	pBuf[1] = 0x01;
	
	pBuf[2] = u16Tmp;
	pBuf[3] = u16Tmp >> 8;

	u16Tmp = pKeyIn->unKeyMixIn.stRockState.u16RockYValue;
	if ((u8Dir & _YNA_CAM_UP) != 0)
	{
		u16Tmp = (u16Tmp << 3) + 0x200 + 0x07;
	}
	else if ((u8Dir & _YNA_CAM_DOWN) != 0)
	{
		u16Tmp = (u16Tmp << 3) + 0x07;
	}
	else 
	{
		u16Tmp = 0x200;
	}
	pBuf[4] = u16Tmp >> 8;
	pBuf[5] = u16Tmp;
	
	u16Tmp = 0x200;
	pBuf[6] = u16Tmp >> 8;
	pBuf[7] = u16Tmp;
	
	
	s_u8AllBackupForSB[0] = pBuf[2];
	s_u8AllBackupForSB[1] = pBuf[3];
	s_u8AllBackupForSB[2] = pBuf[5];
	s_u8AllBackupForSB[3] = pBuf[4];
	s_boIsDataUpdateForSB = true;

	
	SBGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_SB_LENGTH);
	
	return true;
}

void SBSendVolumeCmd(u16 u16Value)
{
	u8 *pBuf = u8SBBuf;
	memset(pBuf, 0, PROTOCOL_SB_LENGTH);

	pBuf[0] = 0xFF;
	pBuf[1] = 0x03;
	
	pBuf[2] = u16Value;
	pBuf[3] = u16Value >> 8;
	
	
	s_u8AllBackupForSB[6] = u16Value;
	s_boIsDataUpdateForSB = true;

	
	SBGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_SB_LENGTH);
}
static bool VolumeProcessForSB(StKeyMixIn *pKeyIn)
{
	SBSendVolumeCmd(pKeyIn->unKeyMixIn.u32VolumeValue);
	return true;
}

static bool PushPodProcessForSB(StKeyMixIn *pKeyIn)
{
	u8 *pBuf = u8SBBuf;
	u16 u16Value = pKeyIn->unKeyMixIn.u32PushRodValue;
	memset(pBuf, 0, PROTOCOL_SB_LENGTH);

	pBuf[0] = 0xFF;
	pBuf[1] = 0x02;
	
	u16Value = u16Value * 0x64 / PUSH_ROD_MAX_VALUE;
	
	pBuf[2] = u16Value;
	pBuf[3] = u16Value >> 8;
	
	
	s_u8AllBackupForSB[10] = u16Value;
	s_boIsDataUpdateForSB = true;

	
	SBGetCheckSum(pBuf);
	CopyToUartMessage(pBuf, PROTOCOL_SB_LENGTH);
	
	
	TPushTurnLight(pKeyIn->unKeyMixIn.u32PushRodValue);

	return true;
}

static PFun_KeyProcess s_KeyProcessForSBArr[_Key_Reserved] = 
{
	PushPodProcessForSB, KeyBoardProcessForSB, RockProcessForSB,
	VolumeProcessForSB, NULL, 
};

void FlushHIDMsgForSB()
{
	if (IsUSBDeviceConnect())
	{
		static u32 u32PrevSendTime = 0;
		u8 u8Buf[24] = {1, 0};
		if (s_boIsDataUpdateForSB)
		{
			memcpy(u8Buf + 1, s_u8AllBackupForSB, 19);
			CopyToUSBMessage(u8Buf, 20);

			s_boIsDataUpdateForSB = false;
			u32PrevSendTime = g_u32SysTickCnt;
		}
		else if (SysTimeDiff(u32PrevSendTime, g_u32SysTickCnt) > 10000)
		{
			memcpy(u8Buf + 1, s_u8AllBackupForSB, 19);
			CopyToUSBMessage(u8Buf, 20);
		
			u32PrevSendTime = g_u32SysTickCnt;
		}
	}
}


bool KeyProcess(StIOFIFO *pFIFO)
{
	StKeyMixIn *pKeyIn = pFIFO->pData;
	
	if (pKeyIn->emKeyType >= _Key_Reserved)
	{
		return false;
	}
	
	if (s_KeyProcessArr[pKeyIn->emKeyType] != NULL)
	{
		if (g_emProtocol == _Protocol_YNA)
		{
			return s_KeyProcessArr[pKeyIn->emKeyType](pKeyIn);	
		}
		else
		{
			return s_KeyProcessForSBArr[pKeyIn->emKeyType](pKeyIn);			
		}
	}
	return false;
}

static void ChangeLedArrayState(const u16 *pLed, u16 u16Cnt, bool boIsLight)
{
	u16 i;
	for (i = 0; i < u16Cnt; i++)
	{
		ChangeLedStateWithBackgroundLight(GET_XY(pLed[i]), boIsLight);
	}
}

bool PCEchoProcessYNA(StIOFIFO *pFIFO)
{
	u8 *pMsg;
	u8 u8Cmd;
	bool boIsLight;

	if (pFIFO == NULL)
	{
		return -1;
	}
	pMsg = (u8 *)pFIFO->pData;	
	u8Cmd = pMsg[_YNA_Cmd];
	
	boIsLight = !pMsg[_YNA_Data3];

	if (pMsg[_YNA_Mix] == 0x06)
	{
		return true;
	}

	switch (u8Cmd)
	{
		case 0x44:
		{
			const u16 u16Led[] = 
			{
				_Led_Cam_1,
				_Led_Cam_2,
				_Led_Cam_3,
				_Led_Cam_4,
			};
			if (pMsg[_YNA_Data2] < 4)
			{
				ChangeLedArrayState(u16Led, sizeof(u16Led) / sizeof(u16), false);
				ChangeLedStateWithBackgroundLight(GET_XY(u16Led[pMsg[_YNA_Data2]]), boIsLight);
			}
			break;
		}
		case 0x45:
		{
			break;
		}
		case 0x46:
		{
			break;
		}
		case 0x47:
		{
			const u16 u16Led[] = 
			{
				_Led_Record_Record,
				_Led_Record_Pause,
				_Led_Record_Stop,
			};
			if ((pMsg[_YNA_Data2] <= 5) && (pMsg[_YNA_Data2] >= 2))
			{
				ChangeLedArrayState(u16Led, sizeof(u16Led) / sizeof(u16), false);
				ChangeLedStateWithBackgroundLight(GET_XY(u16Led[pMsg[_YNA_Data2] - 2]), boIsLight);
			}
			break;
		}
		case 0x48:
		{
			const u16 u16LedPGM[] = 
			{
				_Led_PGM_1, _Led_PGM_2, _Led_PGM_3, _Led_PGM_4,
				_Led_PGM_5, _Led_PGM_6, _Led_PGM_7, _Led_PGM_8,
				_Led_PGM_9, _Led_PGM_10, _Led_PGM_11, _Led_PGM_12,
			};
			const u16 u16LedPVW[] = 
			{
				_Led_PVW_1, _Led_PVW_2, _Led_PVW_3, _Led_PVW_4,
				_Led_PVW_5, _Led_PVW_6, _Led_PVW_7, _Led_PVW_8,
				_Led_PVW_9, _Led_PVW_10, _Led_PVW_11, _Led_PVW_12,
			};

			u8 u8Led = pMsg[_YNA_Data2];
			switch (u8Led)
			{
				case 0x00:
				{
					ChangeLedStateWithBackgroundLight(GET_XY(_Led_Effect_Ctrl_Take), boIsLight);						
					break;
				}
				case 0x02: case 0x03: case 0x04: case 0x05:
				case 0x06: case 0x07:
				{
					ChangeLedArrayState(u16LedPGM, sizeof(u16LedPGM) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16LedPGM[pMsg[_YNA_Data2] - 0x02]), boIsLight);
					if (pMsg[_YNA_Data1] == 1)
					{
						SetTallyPGM(pMsg[_YNA_Data2] - 0x02, boIsLight, true, true);
					}
					break;
				}
				case 0x08: case 0x09: case 0x0A: case 0x0B:
				case 0x0C: case 0x0D: 
				{
					ChangeLedArrayState(u16LedPVW, sizeof(u16LedPVW) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16LedPVW[pMsg[_YNA_Data2] - 0x08]), boIsLight);
					
					if (pMsg[_YNA_Data1] == 1)
						SetTallyPVW(pMsg[_YNA_Data2] - 0x08, boIsLight, true, true);
					break;
				}
				case 0x0E: case 0x0F: case 0x10: case 0x11:
				case 0x12: case 0x13:
				{
					const u16 u16Led[] = 
					{
						_Led_Effect_1,
						_Led_Effect_2,
						_Led_Effect_3,
						_Led_Effect_4,
						_Led_Effect_5,
						_Led_Effect_6,
					};
					ChangeLedArrayState(u16Led, sizeof(u16Led) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16Led[pMsg[_YNA_Data2] - 0x0E]), boIsLight);
					break;
				}
				case 0x80: case 0x81: case 0x82: case 0x83:
				case 0x84: case 0x85:
				{
					ChangeLedArrayState(u16LedPGM, sizeof(u16LedPGM) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16LedPGM[pMsg[_YNA_Data2] - 0x80 + 6]), boIsLight);
					if (pMsg[_YNA_Data1] == 1)
						SetTallyPGM(pMsg[_YNA_Data2] - 0x80 + 6, boIsLight, true, true);

					break;
				}
				case 0x90: case 0x91: case 0x92: case 0x93:
				case 0x94: case 0x95:
				{
					ChangeLedArrayState(u16LedPVW, sizeof(u16LedPVW) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16LedPVW[pMsg[_YNA_Data2] - 0x90 + 6]), boIsLight);
					if (pMsg[_YNA_Data1] == 1)
						SetTallyPVW(pMsg[_YNA_Data2] - 0x90 + 6, boIsLight, true, true);
					break;
				}
				default:
					break;
			}
			break;
		}
		case 0x49:
		{
			break;
		}
		case 0x4A:
		{
			break;
		}
		case 0x4B:
		{
			break;
		}
		case 0x4C:
		{
			u8 u8Led = pMsg[_YNA_Data2];
			switch (u8Led)
			{
				case 0x80: case 0x81: case 0x82:
				{
					const u16 u16Led[] = 
					{
						_Led_Fun_Reserved1,
						_Led_Fun_Reserved2,
						_Led_Fun_Reserved3,									
					};
					ChangeLedArrayState(u16Led, sizeof(u16Led) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16Led[u8Led - 0x80]), boIsLight);
					break;
				}
				case 0x90: case 0x91: case 0x92:
				case 0x93: case 0x94: case 0x95:
				{
					const u16 u16Led[] = 
					{
						_Led_Fun_CG1,
						_Led_Fun_CG2,
						_Led_Fun_CG3,
						_Led_Fun_CG4,
						_Led_Fun_CG5,
						_Led_Fun_CG6,
					};
					ChangeLedArrayState(u16Led, sizeof(u16Led) / sizeof(u16), false);
					ChangeLedStateWithBackgroundLight(GET_XY(u16Led[u8Led - 0x90]), boIsLight);
					break;
				}
			}
			break;
		}
		case 0x80:
		{
			break;
		}
		case 0xC0:
		{
			if (pMsg[_YNA_Data2] == 0x01)
			{
				switch (pMsg[_YNA_Data3])
				{
					case 0x00:
					{
						u8 *pBuf = u8YNABuf;
						if (pBuf == NULL)
						{
							return false;
						}

						memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

						pBuf[_YNA_Sync] = 0xAA;
						pBuf[_YNA_Addr] = g_u8CamAddr;
						pBuf[_YNA_Mix] = 0x07;
						pBuf[_YNA_Cmd] = 0xC0;
						pBuf[_YNA_Data2] = 0x01;
						YNAGetCheckSum(pBuf);
						CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
						break;
					}
					case 0x02:
					{
						ChangeAllLedState(false);
						/* maybe we need turn on some light */
						GlobalStateInit();
						break;
					}
					case 0x03:
					{
						u8 *pBuf = u8YNABuf;
						if (pBuf == NULL)
						{
							return false;
						}

						memset(pBuf, 0, PROTOCOL_YNA_ENCODE_LENGTH);

						pBuf[_YNA_Sync] = 0xAA;
						pBuf[_YNA_Addr] = g_u8CamAddr;
						pBuf[_YNA_Mix] = 0x07;
						pBuf[_YNA_Cmd] = 0x80;
						pBuf[_YNA_Data2] = PushRodGetCurValue();
						YNAGetCheckSum(pBuf);
						CopyToUartMessage(pBuf, PROTOCOL_YNA_DECODE_LENGTH);
						break;
					}
				
					default:
						return false;
				}
			}				
			break;
		}
		default :
			return false;
		
	}

	return true;
}

bool PCEchoProcessForSB(StIOFIFO *pFIFO)
{
	u8 *pMsg;
	u8 *pBuf = u8SBBuf;
	u8 u8Cmd;

	if (pFIFO == NULL)
	{
		return false;
	}
	
	memset(pBuf, PROTOCOL_SB_LENGTH, 0);
	
	pMsg = (u8 *)pFIFO->pData;	
	u8Cmd = pMsg[1];
	switch (u8Cmd)
	{
		case 0x07: case 0x08:
		{
			memcpy(s_u8LedBackupForSB, pMsg + 2, 8);
			SBUpdateLed();
			break;
		}
		case 0x09: case 0x0A:
		{
			pBuf[0] = 0xFF;
			pBuf[1] = u8Cmd;
			memcpy(pBuf + 2, s_u8LedBackupForSB, 8);
			SBGetCheckSum(pBuf);
			CopyToUartMessage(pBuf, PROTOCOL_SB_LENGTH);
			break;
		}
		case 0x0D:
		{
			pBuf[0] = 0xFF;
			pBuf[1] = u8Cmd;

			pBuf[2] = APP_VERSOIN_YY,
			pBuf[3] = APP_VERSOIN_MM,
			pBuf[4] = APP_VERSOIN_DD,
			pBuf[5] = APP_VERSOIN_V	,
			SBGetCheckSum(pBuf);
			CopyToUartMessage(pBuf, PROTOCOL_SB_LENGTH);	
			break;
		}
		case 0x0F:
		{
			SBSendVolumeCmd(VolumeGetCurValue());
			break;
		}
		default:
			break;
	}
	return true;
}

bool PCEchoProcessForHIDSB(StIOFIFO *pFIFO)
{
	u8 *pMsg;

	if (pFIFO == NULL)
	{
		return false;
	}
	pMsg = (u8 *)pFIFO->pData;	
	
	pMsg += 1;
	
	if (pMsg[0] == 0xF5 || pMsg[0] == 0xF6)
	{
		memcpy(s_u8LedBackupForSB, pMsg + 1, 8);
		SBUpdateLed();
	}

	return true;
	
}
bool PCEchoProcess(StIOFIFO *pFIFO)
{
	if (pFIFO->u8ProtocolType == _Protocol_YNA)
	{
		return PCEchoProcessYNA(pFIFO);
	}
	else if (pFIFO->u8ProtocolType == _Protocol_SB)
	{
		return PCEchoProcessForSB(pFIFO);
	}
	else if (pFIFO->u8ProtocolType == _Protocol_SB_HID)
	{
		return PCEchoProcessForHIDSB(pFIFO);
	}
	
	return false;
}


void BackgroundLightEnable(bool boIsEnable)
{
	if (g_boIsBackgroundLightEnable)
	{
		u32 i, j;
		for (i = 0; i < 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				u16 u16Led = c_u16LedGlobalDoubleColorArr[i][j];
				if (u16Led == 0xFFFF)
				{
					continue;
				}
				
				
				{
					u16 x = GET_X(u16Led) + 1;
					u16 y = GET_Y(u16Led);
					
					ChangeLedState(x, y, boIsEnable);
				}
			}
		}
	}
}


void ChangeLedStateWithBackgroundLight(u32 x, u32 y, bool boIsLight)
{
	if (g_boIsBackgroundLightEnable)
	{
		if (x != 0xFF)
		{
			ChangeLedState(x + 1, y, !boIsLight);
		}
	}

	ChangeLedState(x, y, boIsLight);
}
