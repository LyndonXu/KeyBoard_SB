#ifndef _KEY_Led_White_TABLE_H_
#define _KEY_Led_White_TABLE_H_
#include "stm32f10x_conf.h"
#include "user_conf.h"

#define LOC(x, y) 		((((x - 1) & 0xFF) << 8) | ((y - 1) & 0xFF))  	/* 高8 位X 的位置，低8 位Y 的位置 */
#define GET_X(loc)		((loc >> 8) & 0xFF)
#define GET_Y(loc)		(loc & 0xFF)
#define GET_XY(loc) 	GET_X(loc), GET_Y(loc)

extern u8 g_u8KeyTable[KEY_Y_CNT][KEY_X_CNT];
enum 
{
	_Key_Record_Record = 1,
	_Key_Record_Pause,
	_Key_Record_Stop,
	
	_Key_Fun_Reserved1,
	_Key_Fun_Reserved2,
	_Key_Fun_Reserved3,		/* 6*/
	
	_Key_Fun_CG1,
	_Key_Fun_CG2,
	_Key_Fun_CG3,
	_Key_Fun_CG4,
	_Key_Fun_CG5,	
	_Key_Fun_CG6,			/* 12 */
	
	_Key_PGM_1,				
	_Key_PGM_2,
	_Key_PGM_3,
	_Key_PGM_4,
	_Key_PGM_5,
	_Key_PGM_6,
	_Key_PGM_7,
	_Key_PGM_8,
	_Key_PGM_9,
	_Key_PGM_10,
	_Key_PGM_11,
	_Key_PGM_12,		/* 24 */


	_Key_PVW_1,
	_Key_PVW_2,
	_Key_PVW_3,
	_Key_PVW_4,
	_Key_PVW_5,
	_Key_PVW_6,
	_Key_PVW_7,
	_Key_PVW_8,
	_Key_PVW_9,
	_Key_PVW_10,
	_Key_PVW_11,
	_Key_PVW_12,		/* 36 */	

	_Key_Cam_1,
	_Key_Cam_2,
	_Key_Cam_3,
	_Key_Cam_4,
	
	_Key_Cam_Ctrl_Tele,
	_Key_Cam_Ctrl_Wide,	/* 42 */

	_Key_Effect_1,
	_Key_Effect_2,
	_Key_Effect_3,
	_Key_Effect_4,
	_Key_Effect_5,
	_Key_Effect_6,		/* 48 */
	
	_Key_Effect_Ctrl_Take,
	_Key_Effect_Ctrl_Cut,		/* 50 */
	
	_Key_Ctrl_Reserved,

};


enum 
{
	_Led_Record_Record = LOC(1, 2),
	_Led_Record_Pause = LOC(3, 2),
	_Led_Record_Stop = LOC(5, 2),
	_Led_Fun_Reserved1 = LOC(7, 2),
	_Led_Fun_Reserved2 = LOC(1, 11),
	_Led_Fun_Reserved3 = LOC(3, 11),
	
	_Led_White_Record_Record = LOC(2, 2),
	_Led_White_Record_Pause = LOC(4, 2),
	_Led_White_Record_Stop = LOC(6, 2),
	_Led_White_Fun_Reserved1 = LOC(8, 2),
	_Led_White_Fun_Reserved2 = LOC(2, 11),
	_Led_White_Fun_Reserved3 = LOC(4, 11),
	
	
	_Led_Fun_CG1 = LOC(1, 3),
	_Led_Fun_CG2 = LOC(3, 3),
	_Led_Fun_CG3 = LOC(5, 3),
	_Led_Fun_CG4 = LOC(7, 3),
	_Led_Fun_CG5 = LOC(1, 12),
	_Led_Fun_CG6 = LOC(3, 12),

	_Led_White_Fun_CG1 = LOC(2, 3),
	_Led_White_Fun_CG2 = LOC(4, 3),
	_Led_White_Fun_CG3 = LOC(6, 3),
	_Led_White_Fun_CG4 = LOC(8, 3),
	_Led_White_Fun_CG5 = LOC(2, 12),
	_Led_White_Fun_CG6 = LOC(4, 12),

	_Led_PGM_1 = LOC(1, 6),
	_Led_PGM_2 = LOC(3, 6),
	_Led_PGM_3 = LOC(5, 6),
	_Led_PGM_4 = LOC(7, 6),
	_Led_PGM_5 = LOC(1, 14),
	_Led_PGM_6 = LOC(3, 14),
	_Led_PGM_7 = LOC(5, 14),
	_Led_PGM_8 = LOC(7, 14),
	_Led_PGM_9 = LOC(1, 4),
	_Led_PGM_10 = LOC(3, 4),
	_Led_PGM_11 = LOC(5, 4),
	_Led_PGM_12 = LOC(7, 4),

	_Led_White_PGM_1 = LOC(2, 6),
	_Led_White_PGM_2 = LOC(4, 6),
	_Led_White_PGM_3 = LOC(6, 6),
	_Led_White_PGM_4 = LOC(8, 6),
	_Led_White_PGM_5 = LOC(2, 14),
	_Led_White_PGM_6 = LOC(4, 14),
	_Led_White_PGM_7 = LOC(6, 14),
	_Led_White_PGM_8 = LOC(8, 14),
	_Led_White_PGM_9 = LOC(2, 4),
	_Led_White_PGM_10 = LOC(4, 4),
	_Led_White_PGM_11 = LOC(6, 4),
	_Led_White_PGM_12 = LOC(8, 4),

	_Led_PVW_1 = LOC(1, 5),
	_Led_PVW_2 = LOC(3, 5),
	_Led_PVW_3 = LOC(5, 5),
	_Led_PVW_4 = LOC(7, 5),
	_Led_PVW_5 = LOC(1, 13),
	_Led_PVW_6 = LOC(3, 13),
	_Led_PVW_7 = LOC(5, 13),
	_Led_PVW_8 = LOC(7, 13),
	_Led_PVW_9 = LOC(1, 1),
	_Led_PVW_10 = LOC(3, 1),
	_Led_PVW_11 = LOC(5, 1),
	_Led_PVW_12 = LOC(7, 1),	

	_Led_White_PVW_1 = LOC(2, 5),
	_Led_White_PVW_2 = LOC(4, 5),
	_Led_White_PVW_3 = LOC(6, 5),
	_Led_White_PVW_4 = LOC(8, 5),
	_Led_White_PVW_5 = LOC(2, 13),
	_Led_White_PVW_6 = LOC(4, 13),
	_Led_White_PVW_7 = LOC(6, 13),
	_Led_White_PVW_8 = LOC(8, 13),
	_Led_White_PVW_9 = LOC(2, 1),
	_Led_White_PVW_10 = LOC(4, 1),
	_Led_White_PVW_11 = LOC(6, 1),
	_Led_White_PVW_12 = LOC(8, 1),	

	_Led_Cam_1 = LOC(1, 8),
	_Led_Cam_2 = LOC(1, 7),
	_Led_Cam_3 = LOC(3, 8),
	_Led_Cam_4 = LOC(3, 7),
	
	_Led_White_Cam_1 = LOC(2, 8),
	_Led_White_Cam_2 = LOC(2, 7),
	_Led_White_Cam_3 = LOC(4, 8),
	_Led_White_Cam_4 = LOC(4, 7),
	
	_Led_Cam_Ctrl_Tele = LOC(5, 8),
	_Led_Cam_Ctrl_Wide = LOC(5, 7),

	_Led_White_Cam_Ctrl_Tele = LOC(6, 8),
	_Led_White_Cam_Ctrl_Wide = LOC(6, 7),

	_Led_Effect_1 = LOC(1, 9),
	_Led_Effect_2 = LOC(1, 10),
	_Led_Effect_3 = LOC(3, 9),
	_Led_Effect_4 = LOC(3, 10),
	_Led_Effect_5 = LOC(5, 9),
	_Led_Effect_6 = LOC(5, 10),

	_Led_Effect_Ctrl_Take = LOC(7, 10),
	_Led_Effect_Ctrl_Cut = LOC(7, 9),
	
	_Led_White_Effect_1 = LOC(2, 9),
	_Led_White_Effect_2 = LOC(2, 10),
	_Led_White_Effect_3 = LOC(4, 9),
	_Led_White_Effect_4 = LOC(4, 10),
	_Led_White_Effect_5 = LOC(6, 9),
	_Led_White_Effect_6 = LOC(6, 10),
	
	_Led_White_Effect_Ctrl_Take = LOC(8, 10),
	_Led_White_Effect_Ctrl_Cut = LOC(8, 9),
	
	_Led_TPush_1 = LOC(9, 16),
	_Led_TPush_2 = LOC(10, 16),
	_Led_TPush_3 = LOC(11, 16),
	_Led_TPush_4 = LOC(12, 16),
	_Led_TPush_5 = LOC(13, 16),
	_Led_TPush_6 = LOC(14, 16),
	_Led_TPush_7 = LOC(15, 16),	
	_Led_TPush_8 = LOC(16, 16),
	_Led_TPush_9 = LOC(9, 15),
	_Led_TPush_10 = LOC(10, 15),
	_Led_TPush_11 = LOC(11, 15),
	_Led_TPush_12 = LOC(12, 15),
};

extern const u16 c_u16LedGlobalDoubleColorArr[8][8]; 

#endif

