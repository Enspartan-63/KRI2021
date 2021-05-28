/*============================================================================================
							    PROJECT BW 2016 Ver.1.0
								  BW I2C LCD LIBRARY
==============================================================================================*/

#include "bw_i2c_lcd.h"

uint8_t LCDMask = 0x00;
#define nible_hi(byte)     (byte>>4)
#define nible_lo(byte)     (byte & 0x0f)

uint8_t I2C_ADDRESS_W = (LCD_ADDRESS << 1) | 0x00;
uint8_t I2C_ADDRESS_R = (LCD_ADDRESS << 1) | 0x01;

#define PCF_RS  0x01
#define PCF_RW  0x02
#define PCF_EN  0x04
#define PCF_BACKLIGHT 0x08
#define RSMODE_CMD  0
#define RSMODE_DATA 1
#define true	1
#define false   0

#define LOW 1
int I2C2_DATA[2];
int Counter_Clear_LCD;

/* func      : 	void BW_I2C_LCD_Init (void)
 * brief     : 	Character LCD I2C BUS Initialization
 * param     :	N/A
 * retval    : 	N/A
 * Ver       : 1
 * written By: Ryan (Thursday, February 11st 2016)
 * Revised By: Reyhan (Wednesday, September 18th 2018)
 * Notes	 : Set the initialization to match HD44780
 * 			   device protocol
 * 			   Added all working function to use the LCD properly
 *
 */

void BW_I2C_LCD_Init(void)
{
    //LCD I2C INIT

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

	/*  GPIO Setup  */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Connect PXx to I2C_SCL*/
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
    /* Connect PXx to I2C_SDA*/
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

    I2C_DeInit(I2C2);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_Cmd(I2C2, ENABLE);
    I2C_Init(I2C2, &I2C_InitStructure);
}

int16_t I2C2_ReadByte_LCD(uint8_t ADDRESS, uint8_t REGISTER)
{
	  int16_t RetVal=0;
	  uint32_t TimeOut=I2C2_TIMEOUT;
//	  uint8_t Value,counter;

	  //Start Sequences
	  I2C_GenerateSTART(I2C2, ENABLE);

	  TimeOut= I2C2_TIMEOUT;
	  while(!I2C_GetFlagStatus(I2C2,I2C_FLAG_SB))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-1));
	  }

	  // ACK disable
	  I2C_AcknowledgeConfig(I2C2, DISABLE);

	  // Slave Address Send (write)
	  I2C_Send7bitAddress(I2C2, ADDRESS, I2C_Direction_Transmitter);

	  TimeOut= I2C2_TIMEOUT;
	  while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-2));
	  }

	  // Clear ADDR Flag
	  I2C2->SR2;

	  TimeOut=I2C2_TIMEOUT;
	  while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-3));
	  }

	  // SEND REGISTER ADDRESS
	  I2C_SendData(I2C2, REGISTER);

	  TimeOut=I2C2_TIMEOUT;
	  while ((!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF)))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-4));
	  }

	  // Start Sequence
	  I2C_GenerateSTART(I2C2, ENABLE);

	  TimeOut=I2C2_TIMEOUT;
	  while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-5));
	  }

	  // Slave Address Send (read(CMPS11_ADDRESS, CMPS11_BEARING_8BIT);
	  I2C_Send7bitAddress(I2C2, ADDRESS, I2C_Direction_Receiver);

	  TimeOut=I2C2_TIMEOUT;
	  while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-6));
	  }

	  // Clear ADDR Flag
	  I2C2->SR2;

	  while (!I2C_GetFlagStatus(I2C2, I2C_FLAG_RXNE))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-7));
	  }

	  // Stop Sequences
	  I2C_GenerateSTOP(I2C2, ENABLE);

	  // Read data
	  I2C2_DATA[0]=I2C_ReceiveData(I2C2);

	  // ACK enable
	  I2C_AcknowledgeConfig(I2C2, ENABLE);

	  // Everything's... Alright~ ;) *I hope
	  RetVal=0;

	  return(RetVal);
}

int16_t I2C2_WriteByte_LCD(uint8_t ADDRESS, uint8_t VALUE)
{
	  int16_t RetVal=0;
	  uint32_t TimeOut=I2C2_TIMEOUT;

	  //Start Sequences
	  I2C_GenerateSTART(I2C2, ENABLE);

	  TimeOut=I2C2_TIMEOUT;
	  while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_SB))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-1));
	  }

//	  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));

	  // Slave Address Send (write)
	  I2C_Send7bitAddress(I2C2,(ADDRESS << 1),I2C_Direction_Transmitter);

	  TimeOut=I2C2_TIMEOUT;
	  while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-2));
	  }

//	  while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	  // Clear ADDR Flag
	  I2C2->SR2;

	  TimeOut=I2C2_TIMEOUT;
	  while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-3));
	  }

	  // Value Send
	  I2C_SendData(I2C2, VALUE);

	  TimeOut=I2C2_TIMEOUT;
	  while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE))
	  {
		  if(TimeOut!=0) TimeOut--;
		  else return (I2C2_TimeOut_LCD(-4));
	  }

	  // Fungsi Dimatikan Karena protokol PCF8574 hanya butuh address dan Data kemudian stop
//	  //Send Data
//	  I2C_SendData(I2C2, VALUE);
//
//	  TimeOut=I2C2_TIMEOUT;
//
//	  while ((!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF)))
//	  {
//		  if(TimeOut!=0) TimeOut--;
//		  else return (I2C2_TimeOut_LCD(-5));
//	  }

	  // Stop Sequences
	  I2C_GenerateSTOP(I2C2, ENABLE);

	  // Everything's... Alright~ ;) *I hope
	  RetVal=0;

	  return (RetVal);
}

int16_t I2C2_TimeOut_LCD(int16_t VALUE)
{
	int16_t RetVal= VALUE;

//	Stop and reset
	I2C_GenerateSTOP(I2C2,ENABLE);
	I2C_SoftwareResetCmd(I2C2,ENABLE);
	I2C_SoftwareResetCmd(I2C2, DISABLE);
	I2C_SoftwareResetCmd(I2C2, DISABLE);

	// I2C De-Init
	I2C_DeInit(I2C2);

//	I2C Re-Init
	BW_I2C_LCD_Init();

    return(RetVal);
}

// START OF PORTED CODE

/* brief     : 	I2C LCD USING PCF8547
 * param     :	N/A
 * retval    : 	N/A
 * Ver       : 1.1
 * written By: Reyhan (Thursday, Sep 20th 2018)
 * Revised By: Reyhan (Friday, September 21st 2018)
 * Notes	 :
 * 1. Writes working code for I2C LCD
 * 2. Revised the code regarding speed of toggling the pins on PCF8547
 * 3. Adding some delay on few functions
 * 4. Some code cleanups
 *
 */

void PCF8574_WriteI2C(uint8_t halfByte, uint8_t mode, uint8_t enable)
{
	uint8_t i2cData = halfByte << 4;
	if (mode > 0) i2cData |= PCF_RS;
	// PCF_RW Gak Dipakai
	if (enable > 0) i2cData |= PCF_EN;
	if (backlight > 0) i2cData |= PCF_BACKLIGHT;

	I2C2_WriteByte_LCD(LCD_ADDRESS, i2cData);
}

void PCF8574_Send_Nibble(uint8_t halfByte, uint8_t mode)
{
	  PCF8574_WriteI2C(halfByte, mode, true);
	  delay_us(1);    // enable pulse must be >450ns
	  PCF8574_WriteI2C(halfByte, mode, false);
//	  delay_us(50);
}

void PCF8574_Send(uint8_t value, uint8_t mode)
{
	uint8_t valueLo = value    & 0x0F;
	uint8_t valueHi = value>>4 & 0x0F;

	PCF8574_Send_Nibble(valueHi, mode);
	PCF8574_Send_Nibble(valueLo, mode);
}

void PCF8574_Command(uint8_t value)
{
	PCF8574_Send(value, RSMODE_CMD);
}

void PCF8574_Write(uint8_t value)
{
	PCF8574_Send(value, RSMODE_DATA);
}

void PCF8574_LCD_putch(const uint8_t chr)
{
	PCF8574_Write(chr);
	delay_us(7);	// Butuh delay karena char error kalau terlalu cepat
}

void PCF8574_LCD_putstr(const char* str)
{
	while(*str)
	{
		PCF8574_LCD_putch(*str++);
	}
}

void LiquidCrystal_PCF8574(uint8_t addr) // Backlight harus 1 untuk menyalakan
{
	Addr = addr;
	backlight = 1;
}

void PCF8574_begin_LCD(uint8_t cols, uint8_t lines, uint8_t dotsize)
{
	// cols ignored !
	numlines = lines;

	displayfunction = 0;

	if (lines > 1)
	{
		displayfunction |= LCD_2LINE;
	}

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1))
	{
	    displayfunction |= LCD_5x10DOTS;
	}

	BW_I2C_LCD_Init();
	LiquidCrystal_PCF8574(0x27);


	// initializing the display
	PCF8574_WriteI2C(0x00, LOW, false);
	delay_us(50000);

//	put the LCD into 4 bit mode according to the hitachi HD44780 datasheet figure 26, pg 47
	PCF8574_Send_Nibble(0x03, RSMODE_CMD);
	delay_us(4500);
	PCF8574_Send_Nibble(0x03, RSMODE_CMD);
	delay_us(4500);
	PCF8574_Send_Nibble(0x03, RSMODE_CMD);
	delay_us(150);
	// finally, set to 4-bit interface
	PCF8574_Send_Nibble(0x02, RSMODE_CMD);

	// finally, set # lines, font size, etc.
	PCF8574_Command(LCD_FUNCTIONSET | displayfunction);

	// turn the display on with no cursor or blinking default
	displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	PCF8574_LCD_DisplayOn();

	// clear it off
	PCF8574_LCD_Clear();

	// Initialize to default text direction (for romance languages)
	displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// set the entry mode
	PCF8574_Command(LCD_ENTRYMODESET | displaymode);
}

void PCF8574_createChar(uint8_t location, uint8_t charmap[])
{
	location &= 0x7; // Lokasi hanya 0-7
	PCF8574_Command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++)
	{
		PCF8574_Write(charmap[i]);
	}
}

void PCF8574_LCD_BlinkCursor(void)
{
	 displaycontrol |= LCD_BLINKON;
	 PCF8574_Command(LCD_DISPLAYCONTROL | displaycontrol);
}

void PCF8574_LCD_noBlinkCursor(void)
{
	 displaycontrol &= ~LCD_BLINKON;
	 PCF8574_Command(LCD_DISPLAYCONTROL | displaycontrol);
}

void PCF8574_LCD_CursorOn(void)
{
	 displaycontrol |= LCD_CURSORON;
	 PCF8574_Command(LCD_DISPLAYCONTROL | displaycontrol);
}

void PCF8574_LCD_CursorOff(void)
{
	 displaycontrol &= ~LCD_CURSORON;
	 PCF8574_Command(LCD_DISPLAYCONTROL | displaycontrol);
}

void PCF8574_LCD_DisplayOn(void)
{
	 displaycontrol |= LCD_DISPLAYON;
	 PCF8574_Command(LCD_DISPLAYCONTROL | displaycontrol);
}

void PCF8574_LCD_DisplayOff(void)
{
	 displaycontrol &= ~LCD_DISPLAYON;
	 PCF8574_Command(LCD_DISPLAYCONTROL | displaycontrol);
}

void PCF8574_LCD_gotoxy(uint8_t col, uint8_t row)
{
	delay_us(6);
//	int row_offsets[] = { 0x00, 0x40, 0x10, 0x50   }; // Ini offset khusus 16x4 LCD
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54   }; // Ini offset khusus 20x4 LCD
	if ( row >= numlines )
	{
		row = numlines-1;
	}
	PCF8574_Command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void PCF8574_LCD_Home(void)
{
    PCF8574_Command(LCD_RETURNHOME);
    delay_us(4200);
}

void PCF8574_LCD_Clear(void)
{
	PCF8574_Command(LCD_CLEARDISPLAY);
	delay_us(1200);
}
void ENSPARTAN_LCD_Init(void)
{
	PCF8574_begin_LCD(20,4,0); // Inisialisasi 4x20 dotsize default
	delay_ms(200);
	PCF8574_LCD_CursorOff();
}
void ENSPARTAN_LCD_CLEAR()
{
	PCF8574_LCD_Clear();
}
void lcd_gotoxy(uint8_t col, uint8_t row)
{
	PCF8574_LCD_gotoxy(col, row);
}

void lcd_putstr(const char* str)
{
	while(*str)
	{
		PCF8574_LCD_putch(*str++);
	}
}

void Counter_LCD_Clear(void){
	Counter_Clear_LCD++;
	if (Counter_Clear_LCD >= 6) Counter_Clear_LCD = 0;
}

// END OF PORTED LIBRARY

// START OF MY OWN METHOD, UNFIXED, PARTIALLY WORKING

//void PCF8574_Write_Nible(const uint8_t nible)
//{
//	uint8_t LCDMask_Temp = LCDMask;
//	I2C2_WriteByte_LCD(LCD_ADDRESS, EN_ON);
//
//	if ((nible & 0b00000001) == 0b00000001) LCDMask_Temp = LCDMask_Temp | D4_ON;
//	else                LCDMask_Temp = LCDMask_Temp | D4_OFF;
//	if ((nible & 0b00000010) == 0b00000010) LCDMask = LCDMask_Temp | D5_ON;
//	else                LCDMask_Temp = LCDMask_Temp | D5_OFF;
//	if ((nible & 0b00000100) == 0b00000100)	LCDMask = LCDMask_Temp | D6_ON;
//	else                LCDMask_Temp = LCDMask_Temp | D6_OFF;
//	if ((nible & 0b00001000) == 0b00001000) LCDMask = LCDMask_Temp | D7_ON;
//	else                LCDMask_Temp = LCDMask_Temp | D7_OFF;
//
//	// Send Nible with EN ON
//	I2C2_WriteByte_LCD(LCD_ADDRESS, (LCDMask_Temp | EN_ON));
//
//	delay_us(100);
//
//	// Send Nible with EN OFF
//	I2C2_WriteByte_LCD(LCD_ADDRESS, (LCDMask_Temp & 0xFB));
//}
//
//// Sementara Gak Guna
////void Initial_Command_Write(uint8_t Value)
////{
////	uint8_t cmd;
////	cmd = I2C_ADDRESS << 1;
////	cmd = cmd | 0x00;
////	I2C2_WriteByte_LCD(cmd, Value);
////}
////
////void Initial_Command_Read(void)
////{
////	uint8_t cmd;
////	cmd = I2C_ADDRESS << 1;
////	cmd = cmd | 0x01;
////	I2C2_WriteByte_LCD(I2C_ADDRESS, cmd);
////}
//
//void PCF8574_WriteByte(const uint8_t byte)
//{
//	PCF8574_Write_Nible(nible_hi(byte));
//	PCF8574_Write_Nible(nible_lo(byte));
//	delay_us(70);
//}
//
//void PCF8574_WriteDataByte(const uint8_t byte)
//{
//	LCDMask = 0x00;
//	//RS On untuk kirim data
//	I2C2_WriteByte_LCD(LCD_ADDRESS, (LCDMask | RS_ON));
//	PCF8574_WriteByte(byte);
//}
//
//void I2C_LCD_Putchar(const char chr)
//{
//	PCF8574_WriteDataByte(chr);
//}
//
//void I2C_LCD_Putstr(const char *str)
//{
//	while(*str)
//	{
//		I2C_LCD_Putchar(*str++);
//	}
//}
//
//void I2C_LCD_WriteCMD_Byte(const uint8_t cmd)
//{
//	LCDMask = 0x00;
//	// RS OFF Untuk Write Command
//	I2C2_WriteByte_LCD(LCD_ADDRESS, (LCDMask | RS_OFF));
//
//	PCF8574_WriteByte(cmd);
//
//	if(cmd==I2C_DISPLAY_CLEAR || cmd==I2C_CURSOR_HOME)delay_us(2100); //1000-2000 masih error characternya
//}
//
//#define lcd_row_addr(row) (row%2? (row>>1)*col_num+0x40:
//                                   (row>>1)*col_num)
//
//void I2C_gotoxy(uint8_t col, uint8_t row)
//{
//	I2C_LCD_WriteCMD_Byte(I2C_DDRAM_ADDR_SET |
//	                  lcd_row_addr(row) | col );
//}
//
//void I2C_Display_Clear(void)
//{
//	I2C_LCD_WriteCMD_Byte(I2C_DISPLAY_CLEAR);
//}
//
//void I2C_LCD_Home(void)
//{
//	I2C_LCD_WriteCMD_Byte(I2C_CURSOR_HOME);
//	delay_us(199);
//}
//
//void I2C_LCD_WriteCMD_Nible(const uint8_t cmd)
//{
//	LCDMask = 0x00;
//	// RS OFF Untuk Write Command
//	I2C2_WriteByte_LCD(LCD_ADDRESS, LCDMask & RS_OFF);
//
//	PCF8574_Write_Nible(cmd);
//}
//
//// END OF MY OWN METHOD /////////////////////////////////////////
