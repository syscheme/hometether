#ifndef __LCD_TouchPad_H__
#define __LCD_TouchPad_H__

#include "defines.h"

#define RED      0xf800
#define ORANGE   0xffe0
#define GREEN    0x07e0
#define BLUE     0x001f
#define WHITE    0xffff
#define BLACK    0x0000
#define YELLOW   0xFFE0
#define GRAY     0x8888

// -----------------------------
// definiton of  the LCD display
// -----------------------------

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

#define TRANSPARENT  1           //字体显示的时候不带背景颜色
#define NORMAL       0           //字体显示的时候有背景颜色

#define bus	1
#define DATA_L P2
#define DATA_H P0

sbit LCD_CS		= P1^0;
sbit LCD_RS		= P1^1;
sbit LCD_WR		= P1^2;
sbit LCD_RD		= P1^3;
sbit LCD_RST	= P1^4;
sbit LCD_PWM	= P4^2;
sbit LCD_IM0	= P5^2;

void LCD_init(void);
void LCD_fill(uint color, uchar startX, uint startY, uchar width, uint height);
void LCD_printFont(uint x, uint y, uchar* fondCode, uchar fontW, uchar fontH, uint colorFront, uint colorBack, uchar mod);
void LCD_setTextWindow(uchar startX, uint startY, uchar width, uint height, uint colorFront, uint colorBackground, uchar flags);
void LCD_printNum(uint num);
void LCD_printbyte(uchar num);
void LCD_putc(uchar ch);
void LCD_puts(uchar *str);

#define LCD_clear() LCD_fill(BLACK, 0, 0, LCD_WIDTH-1, LCD_HEIGHT-1)

void LCD_showRGB565_0(uint startX, uint startY, uint width, uint height, const uchar *str);
bit LCD_showBMP24b(char* fnPattern, uchar idx, uchar startX, uint startY, uchar width, uint height);
bit LCD_showRGB565(char* fnPattern, uchar idx, uchar startX, uint startY, uchar width, uint height);

extern uchar code AsciiFonts[];
extern bit   bFileInUse;

// --------------------------------------------------
// SPI port
// --------------------------------------------------
#define STC_SPI

sbit SPI_CLK =P1^7;
sbit SPI_DI  =P1^5;  
sbit SPI_DO  =P1^6;
void  SPI_write(uchar x);
uchar SPI_read();

// -----------------------------
// definiton of the touchpad
// -----------------------------
sbit    touch_CS     =P3^3;
sbit    touch_INT    =P3^2;
sbit    touch_BEEP   =P2^3;
#define touch_DCLK   SPI_CLK
#define touch_DIN    SPI_DI
#define touch_DOUT   SPI_DO

#define CHX     0x90    //X+ channel word
#define CHY     0xD0    //Y+ channel word

extern uint T_x, T_y;
void Touch_init(void);
bit  Touch_getXY(uint *x, uint *y);
void Touch_beep();
void Touch_beepAndOff(uint tenMs);

extern int _LeftTopX, _LeftTopY, _TpWidth, _TpHeight;
#define LEFT_TOP_X      3900
#define LEFT_TOP_Y      3900
#define RIGHT_BOTTOM_X  330
#define RIGHT_BOTTOM_Y  430

// -----------------------------
// definiton of the keyboard
// -----------------------------
void KB_drawButton(uint x, uint y, uchar width, uchar height, uchar* fontCode, uchar fontW, uchar fontH, uchar up);
void KB_drawButton2(uchar x, uint y, uchar width, uchar height, char* str, bit up); // the button to upper layer menu

// -----------------------------
// definiton of the Numpad
// -----------------------------
void  Numpad_reset();
uchar Numpad_readKey();
uchar Numpad_xy2key(uint Lcd_x, uint Lcd_y);

#endif //__LCD_TouchPad_H__