#include <stdbool.h>
#include "stm32f10x_conf.h"
#include "user_conf.h"
#include "io_buf_ctrl.h"
#include "key_led.h"
#include "key_led_table.h"

u8 g_u8KeyTable[KEY_Y_CNT][KEY_X_CNT] = 
{
	{
		_Key_PVW_9,
		_Key_PVW_10,
		_Key_PVW_11,
		_Key_PVW_12,	

	},	/* 1 */
	{
		_Key_Record_Record,
		_Key_Record_Pause,
		_Key_Record_Stop,
		
		_Key_Fun_Reserved1,
	},	/* 2 */
	{
		_Key_Fun_CG1,
		_Key_Fun_CG2,
		_Key_Fun_CG3,
		_Key_Fun_CG4,
	},	/* 3 */
	{
		_Key_PGM_9,
		_Key_PGM_10,
		_Key_PGM_11,
		_Key_PGM_12,
	},	/* 4 */
	{
		_Key_PVW_1,
		_Key_PVW_2,
		_Key_PVW_3,
		_Key_PVW_4,
	},	/* 5 */
	{
		_Key_PGM_1,
		_Key_PGM_2,
		_Key_PGM_3,
		_Key_PGM_4,
	}, /* 6 */
	{
		_Key_Cam_2,
		_Key_Cam_4,
		_Key_Cam_Ctrl_Wide,
	
	}, /* 7 */
	{
		_Key_Cam_1,
		_Key_Cam_3,
		_Key_Cam_Ctrl_Tele,
	
	}, /* 8 */
	{
		_Key_Effect_1,
		_Key_Effect_3,
		_Key_Effect_5,
		_Key_Effect_Ctrl_Cut,
	}, /* 9 */
	{
		_Key_Effect_2,
		_Key_Effect_4,
		_Key_Effect_6,
		_Key_Effect_Ctrl_Take,
	}, /* 10 */
	{	
		0, 0, 0, 0,
		_Key_Fun_Reserved2,
		_Key_Fun_Reserved3,
	},	/* 11 */
	{
		0, 0, 0, 0,
		_Key_Fun_CG5,
		_Key_Fun_CG6,
	},	/* 12 */
	{
		0, 0, 0, 0,
		_Key_PVW_5,
		_Key_PVW_6,
		_Key_PVW_7,
		_Key_PVW_8,
	},	/* 13 */
	{
		0, 0, 0, 0,
		_Key_PGM_5,
		_Key_PGM_6,
		_Key_PGM_7,
		_Key_PGM_8,
	
	}, /* 14 */

#if 0
	{
		_Key_PVW_9,
		_Key_PVW_10,
		_Key_PVW_11,
		_Key_PVW_12,	

	},	/* 1 */
	{
		_Key_Record_Record,
		_Key_Record_Pause,
		_Key_Record_Stop,
		
		_Key_Fun_Reserved1,
		_Key_Fun_Reserved2,
		_Key_Fun_Reserved3,
	},	/* 2 */
	{
		_Key_Fun_CG1,
		_Key_Fun_CG2,
		_Key_Fun_CG3,
		_Key_Fun_CG4,
		_Key_Fun_CG5,
		_Key_Fun_CG6,
	},	/* 3 */
	{
		_Key_PGM_9,
		_Key_PGM_10,
		_Key_PGM_11,
		_Key_PGM_12,
	},	/* 4 */
	{
		_Key_PVW_1,
		_Key_PVW_2,
		_Key_PVW_3,
		_Key_PVW_4,
		_Key_PVW_5,
		_Key_PVW_6,
		_Key_PVW_7,
		_Key_PVW_8,
	},	/* 5 */
	{
		_Key_PGM_1,
		_Key_PGM_2,
		_Key_PGM_3,
		_Key_PGM_4,
		_Key_PGM_5,
		_Key_PGM_6,
		_Key_PGM_7,
		_Key_PGM_8,
	
	}, /* 6 */
	{
		_Key_Cam_2,
		_Key_Cam_4,
		_Key_Cam_Ctrl_Wide,
	
	}, /* 7 */
	{
		_Key_Cam_1,
		_Key_Cam_3,
		_Key_Cam_Ctrl_Tele,
	
	}, /* 8 */
	{
		_Key_Effect_1,
		_Key_Effect_3,
		_Key_Effect_5,
		_Key_Effect_Ctrl_Cut,
	}, /* 9 */
	{
		_Key_Effect_2,
		_Key_Effect_4,
		_Key_Effect_6,
		_Key_Effect_Ctrl_Take,
	}, /* 10 */
#endif
};

/* dp, g, f, e, d, c, b, a */
const u8 g_u8LED7Code[] = 
{
	0x3F,		// 0
	0x06,		// 1
	0x5B,		// 2
	0x4F,		// 3
	0x66,		// 4
	0x6D,		// 5
	0x7D,		// 6
	0x07,		// 7
	0x7F,		// 8
	0x6F,		// 9
	0x77,		// A
	0x7C,		// B
	0x39,		// C
	0x5E,		// D
	0x79,		// E
	0x71,		// F
	0x40,		// -
};
 

const u16 c_u16LedGlobalDoubleColorArr[8][8] = 
{
	_Led_Record_Record,
	_Led_Record_Pause,
	_Led_Record_Stop,
	
	_Led_Fun_Reserved1,
	_Led_Fun_Reserved2,
	_Led_Fun_Reserved3,		/* 6*/
	
	_Led_Fun_CG1,
	_Led_Fun_CG2,
	_Led_Fun_CG3,
	_Led_Fun_CG4,
	_Led_Fun_CG5,	
	_Led_Fun_CG6,			/* 12 */
	
	_Led_PGM_1,				
	_Led_PGM_2,
	_Led_PGM_3,
	_Led_PGM_4,
	_Led_PGM_5,
	_Led_PGM_6,
	_Led_PGM_7,
	_Led_PGM_8,
	_Led_PGM_9,
	_Led_PGM_10,
	_Led_PGM_11,
	_Led_PGM_12,		/* 24 */


	_Led_PVW_1,
	_Led_PVW_2,
	_Led_PVW_3,
	_Led_PVW_4,
	_Led_PVW_5,
	_Led_PVW_6,
	_Led_PVW_7,
	_Led_PVW_8,
	_Led_PVW_9,
	_Led_PVW_10,
	_Led_PVW_11,
	_Led_PVW_12,		/* 36 */	

	_Led_Cam_1,
	_Led_Cam_2,
	_Led_Cam_3,
	_Led_Cam_4,
	
	_Led_Cam_Ctrl_Tele,
	_Led_Cam_Ctrl_Wide,	/* 42 */

	_Led_Effect_1,
	_Led_Effect_2,
	_Led_Effect_3,
	_Led_Effect_4,
	_Led_Effect_5,
	_Led_Effect_6,		/* 48 */
	
	_Led_Effect_Ctrl_Take,
	_Led_Effect_Ctrl_Cut,		/* 50 */
	
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,  
};

const u16 g_u16CamAddrLoc[CAM_ADDR_MAX] = 
{
	0,
};

