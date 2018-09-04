
/*---------------------------------------------------------------------*/
/* --- STC MCU International Limited ----------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.GXWMCU.com --------------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了宏晶科技的资料及程序   */
/*---------------------------------------------------------------------*/


/*************	功能说明	**************

本文件为STC15xxx系列的端口初始化程序,用户几乎可以不修改这个程序.


******************************************/

#include	"delay.h"



//========================================================================
// 函数: void  delayXms(unsigned char ms)
// 描述: 延时函数。
// 参数: ms,要延时的ms数, 这里只支持1~255ms. 自动适应主时钟.
// 返回: none.
// 版本: VER1.0
// 日期: 2013-4-1
// 备注: 
//========================================================================
void  delayXms(uint8 ms)
{
     unsigned int i;
	 while(ms--)
	 {
	      i = MAIN_Fosc / 13000;
		  while(--i)	;   //14T per loop
     };
}

void delayX10us(uint8 n)
{
	data uint8 i;

//#if MAIN_Fosc <=12000000L || MAIN_Fosc >=11059200L
#     define C_INLOOP()  i=8; while(--i) {_nop_(); _nop_(); _nop_(); _nop_();} // 10us -DEC(3/12)-MOV(1/12)-SJMP(3/12) -MOV(3/12)-JZ(3/12)-[DEC(3/12)+MOV(1/12)+SJMP(3/12) +5/12]*9
// #     define C_INLOOP()  i=5; while(--i) {_nop_(); _nop_(); _nop_(); _nop_(); } // 10us -DEC(3/12)-MOV(1/12)-SJMP(3/12) -MOV(3/12)-JZ(3/12)-[DEC(3/12)+MOV(1/12)+SJMP(3/12) +5/12]*9
#     define C_OUTLOOP() i=8; while(--i) {_nop_(); _nop_(); _nop_();} // 10us - MOV(3/12) -LCALL(6/12) -RET(4/12) -MOV(3/12) -[7/12 +3/12]*10
//#endif // 12M

/*
#if MAIN_Fosc ==24000000L
#     define C_INLOOP()  i=10; while(--i) {_nop_(); _nop_(); _nop_(); _nop_(); }
#     define C_OUTLOOP() i=11; while(--i) {_nop_(); _nop_(); _nop_();}
#endif
*/
	while(--n) // MOV DPTR,#i?041;(24c,3c) MOV A,#0FFH;(12c,2c) MOV B,A;(12,1) LCALL ?C?IILDX;(24,6) ORL A,B;(12,2)	JZ ?C0003;(24,3)
	{ C_INLOOP(); }

	C_OUTLOOP();
}

void delayIOSet()
{
	data uint8 i=4;
	while(--i); // it normally took 4 cycle to get stable when change the io pin
}


