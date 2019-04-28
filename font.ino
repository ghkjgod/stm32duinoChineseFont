#include <SPI.h>
#include <U8g2lib.h>

unsigned char name0[8] = {0xC4, 0xE3, 0xBA, 0xC3, 0xCA, 0xC0, 0xBD, 0xE7};
unsigned char name1[12] = {0x48, 0x45, 0x4C, 0x4C, 0x4F, 0x2C, 0x57, 0x4F, 0x52, 0x4C, 0x44, 0x21};

static const int spiClk = 1000000;

U8G2_ST7567_JLX12864_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/PA5, /* data=*/PA7, /* cs=*/PA2, /* dc=*/PA3, /* reset=*/PA4);

SPIClass SPI_2(2);

#define CS PB12 
#define SO PB14 
#define SI PB15 
#define CL PB13 

long getAddr(uint16_t str)
{
  byte hb = (byte)((str & 0xff00) >> 8);
  byte lb = (byte)(str & 0x00ff);
  // Serial.print("HB ");
  // Serial.print(hb, HEX);
  // Serial.print(" LB ");
  // Serial.println(lb, HEX);
  long Address;
  if (hb > 0xA0 && lb > 0xA0) //is GB2312
  {
    long BaseAdd = 0;
    if (hb == 0xA9 && lb >= 0xA1)
    {
      Address = (282 + (lb - 0xA1)); //8位机在此出现了溢出，所以分三步
      Address = Address * 32;
      Address += BaseAdd;
    }
    else if (hb >= 0xA1 && hb <= 0xA3 && lb >= 0xA1)
    {
      Address = ((hb - 0xA1) * 94 + (lb - 0xA1));
      Address = Address * 32;
      Address += BaseAdd;
    }
    else if (hb >= 0xB0 && hb <= 0xF7 && lb >= 0xA1)
    {
      Address = ((hb - 0xB0) * 94 + (lb - 0xA1) + 846);
      Address = Address * 32;
      Address += BaseAdd;
    }
  }
  else
  { //is ASCII
    long BaseAdd = 0x03b7c0;
    if (lb >= 0x20 && lb <= 0x7E)
    {
      Address = (lb - 0x20) * 16 + BaseAdd;
    }
  }
  return Address;
}

byte revBit(byte input)
{
  byte output = 0;
  for (int i = 0; i < 8; i++)
  {
    if (input & (1 << i))
      output |= 1 << (7 - i);
  }
  return output;
}

void fetchBitmap16(long address, byte *p)
{
  byte hb = (byte)((address & 0x00ff0000) >> 16);
  byte mb = (byte)((address & 0x0000ff00) >> 8);
  byte lb = (byte)(address & 0x000000ff);
  digitalWrite(CS, LOW); //选通字库
  SPI_2.transfer(0x0b);  //移入命令字
  SPI_2.transfer(hb);    //移入地址次高位
  SPI_2.transfer(mb);    //移入地址次低位
  SPI_2.transfer(lb);    //移入地址最低位
  SPI_2.transfer(0x8d);  //移入dummy byte
  for (int i = 0; i < 16; i++)
  {
    byte buf = SPI_2.transfer(0x00); //读出16bytes
    p[i] = revBit(buf);
  }
  digitalWrite(CS, HIGH); //反选通字库
}

void fetchBitmap32(long address, byte *p)
{
  byte hb = (byte)((address & 0x00ff0000) >> 16);
  byte mb = (byte)((address & 0x0000ff00) >> 8);
  byte lb = (byte)(address & 0x000000ff);
  digitalWrite(CS, LOW); //选通字库
  SPI_2.transfer(0x0b);  //移入命令字
  SPI_2.transfer(hb);    //移入地址次高位
  SPI_2.transfer(mb);    //移入地址次低位
  SPI_2.transfer(lb);    //移入地址最低位
  SPI_2.transfer(0x8d);  //移入dummy byte
  for (int i = 0; i < 32; i++)
  {
    byte buf = SPI_2.transfer(0x00); //读出32bytes
    p[i] = revBit(buf);
  }
  digitalWrite(CS, HIGH); //反选通字库
}

void drawV8P(int x, int y, byte b)
{
  if ((b & 0x80) == 0x80)
    u8g2.drawPixel(x, y);
  if ((b & 0x40) == 0x40)
    u8g2.drawPixel(x, y + 1);
  if ((b & 0x20) == 0x20)
    u8g2.drawPixel(x, y + 2);
  if ((b & 0x10) == 0x10)
    u8g2.drawPixel(x, y + 3);
  if ((b & 0x08) == 0x08)
    u8g2.drawPixel(x, y + 4);
  if ((b & 0x04) == 0x04)
    u8g2.drawPixel(x, y + 5);
  if ((b & 0x02) == 0x02)
    u8g2.drawPixel(x, y + 6);
  if ((b & 0x01) == 0x01)
    u8g2.drawPixel(x, y + 7);
}

void draw8x16(int x, int y, byte *b)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    drawV8P(x + i, y, b[i]);
  }
  for (i = 0; i < 8; i++)
  {
    drawV8P(x + i, y + 8, b[i + 8]);
  }
}

void draw16x16(int x, int y, byte *b)
{
  int i;
  for (i = 0; i < 16; i++)
  {
    drawV8P(x + i, y, b[i]);
  }
  for (i = 0; i < 16; i++)
  {
    drawV8P(x + i, y + 8, b[i + 16]);
  }
}

void displayStr(int x, int y, int len, unsigned char *c)
{
  int i;
  for (i = 0; i < len; i++)
  {
    if (c[i] > 0xA0) //is GB2312
    {
      int code = (int)(c[i] * 256) + (int)c[i + 1];
      byte bmp[32];
      fetchBitmap32(getAddr(code), bmp);
      for (int p = 0; p < 32; p++) {
        Serial.print(bmp[p], HEX);
      }
      Serial.println();
      draw16x16(x, y, bmp);
      x += 16;
      i++;
    }
    else
    { //is ASCII
      int code = (int)c[i];
      byte bmp[16];
      fetchBitmap16(getAddr(code), bmp);
      for (int p = 0; p < 16; p++) {
        Serial.print(bmp[p], HEX);
      }
      Serial.println();
      draw8x16(x, y, bmp);
      x += 8;
    }
  }
}

void displayStrC(int row, int len, unsigned char *c)
{
  int x = 64 - ((len * 8) / 2);
  displayStr(x, row, len, c);
}
/*===================================================================
  以上函数由arduinoCN坛友NoComment提供表示感谢
  这是他的github:https://github.com/XAS-712/gt20l16s1y
  ===================================================================*/
void setup(void)
{
  Serial.begin(115200);

  u8g2.begin();
  u8g2.setContrast(0x2B * 2); //Set Contrast

  u8g2.clearBuffer();                 // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.drawStr(0, 10, "font");        // write something to the internal memory
  u8g2.sendBuffer();                  // transfer internal memory to the display
  
  SPI_2.begin();
  SPI_2.setBitOrder(MSBFIRST); // Set the SPI_2 bit order
  SPI_2.setDataMode(SPI_MODE0); //Set the  SPI_2 data mode 0
  SPI_2.setClockDivider(SPI_CLOCK_DIV16);  // Use a different speed to SPI 1
  pinMode(CS, OUTPUT);
  
  delay(1000);
  Serial.println("start");
}

void loop(void)
{
  // picture loop

  u8g2.firstPage();
  do
  {
    displayStr(0, 0, 8, name0);   //你好世界
    displayStr(0, 16, 12, name1); //HELLO,WORLD!
  } while (u8g2.nextPage());
  delay(1000);
}
