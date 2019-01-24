#define MAIN_Fosc		22118400L	//定义主时钟

#include	"STC15Fxxxx.H"



/***********************************************************/

#define DIS_DOT		0x20
#define DIS_BLACK	0x10
#define DIS_		0x11


/****************************** 用户定义宏 ***********************************/


#define	Timer0_Reload	(65536UL -(MAIN_Fosc / 1000))		//Timer 0 中断频率, 1000次/秒

/*****************************************************************************/




/*************	本地常量声明	**************/
u8 code t_display[]={						//标准字库
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1

u8 code T_COM[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};		//位码
u8 day_num[]={31,28,31,30,31,30,31,31,30,31,30,31};

/*************	IO口定义	**************/
sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock


/*************	本地变量声明	**************/

u8 	LED8[8];		//显示缓冲

u8	display_index;	//显示位索引
bit	B_1ms;			//1ms标志

u8  IO_KeyState, IO_KeyState1, IO_KeyHoldCnt;	//行列键盘变量
u8	KeyHoldCnt;	//键按下计时
u8	KeyCode;	//给用户使用的键码, 1~16有效
u8	cnt50ms;
u8	try,able;

u8	month,day,hour,minute,second;	//RTC变量
u8  curr_input;
u8  key[] = {0, 0, 0, 0, 0, 0, 0, 0};
u8 	standard[] = {1, 2, 3, 4};
u8  curr_show;  // 0显示时间，1显示日期，2显示温度
u8  open;
u16 year;
u16	msecond, opentime, trytime;


/*************	本地函数声明	**************/
void	CalculateAdcKey(u16 adc);
void	Display(u8 curr_show);
void	IO_KeyScan(void);	//50ms call
void	WriteNbyte(u8 addr, u8 *p, u8 number);
void	ReadNbyte( u8 addr, u8 *p, u8 number);
void	DisplayTime(void);
void	DisplayDate(void);
void	DisplayTemp(void);
void	DisplayKey(void);
void	DisplayNewKey(void);
void	reKey(void);
void	changeDate(void);
void	changing(u8 KeyCode, u8 curr_show);
void	ReadRTC(void);
void	WriteRTC(void);
void	alert(u8 _code);
void	delay_ms(u8 ms);
u16		get_temperature(u16 adc);
u16		Get_ADC10bitResult(u8 channel);	//channel = 0~7
										   
/****************  外部函数声明和外部变量声明 *****************/


/**********************************************/
void main(void)
{
	u8	i;

	P0M1 = 0;	P0M0 = 0;	//设置为准双向口
	P1M1 = 0;	P1M0 = 0;	//设置为准双向口
	P2M1 = 0;	P2M0 = 0;	//设置为准双向口
	P3M1 = 0;	P3M0 = 0;	//设置为准双向口
	P4M1 = 0;	P4M0 = 0;	//设置为准双向口
	P5M1 = 0;	P5M0 = 0;	//设置为准双向口
	P6M1 = 0;	P6M0 = 0;	//设置为准双向口
	P7M1 = 0;	P7M0 = 0;	//设置为准双向口
	
	P1ASF = 0x0C;		//P1.2 P1.3做ADC
	ADC_CONTR = 0xE0;	//90T, ADC power on
	
	display_index = 0;
	curr_show = 0;
	curr_input = 0;
	opentime = 0;
	trytime = 0;
	AUXR = 0x80;	//Timer0 set as 1T, 16 bits timer auto-reload, 
	TH0 = (u8)(Timer0_Reload / 256);
	TL0 = (u8)(Timer0_Reload % 256);
	ET0 = 1;	//Timer0 interrupt enable
	TR0 = 1;	//Tiner0 run
	EA = 1;		//打开总中断
	
	for(i=0; i<8; i++)	LED8[i] = 0x10;	//上电消隐

	
	ReadRTC();
	F0 = 0;
	month = 1;
	year = 2019;
	day = 1;
	if(second >= 60)	F0 = 1;	//错误
	if(minute >= 60)	F0 = 1;	//错误
	if(hour   >= 24)	F0 = 1;	//错误
	if(F0)	//有错误, 默认12:00:00
	{
		second = 0;
		minute = 0;
		hour  = 12;
		WriteRTC();
	}

	Display(curr_show);
	open = 0;
	try = 0;
	able = 1;

	KeyHoldCnt = 0;	//键按下计时
	KeyCode = 0;	//给用户使用的键码, 1~16有效

	IO_KeyState = 0;
	IO_KeyState1 = 0;
	IO_KeyHoldCnt = 0;
	cnt50ms = 0;
	
	while(1)
	{
		if(B_1ms)	//1ms到
		{
			if(open == 1) opentime++;
			else opentime = 0;
			if(opentime >= 2000) reKey();
			if(opentime >= 9000) alert(2);
			B_1ms = 0;
			if(try > 0 && try < 5) trytime++;
			if(trytime >= 20000){
				  try = 0;
				  trytime = 0;
			} 
			if(++msecond >= 1000)	//1秒到
			{
				msecond = 0;
				changeDate();
				ReadRTC();
				Display(curr_show);
			}

			if(++cnt50ms >= 50)		//50ms扫描一次行列键盘
			{
				cnt50ms = 0;
				IO_KeyScan();
			}
			
			if(KeyCode != 0)		//有键按下
			{
				if(KeyCode == 27)	curr_show = 0; // 时钟模式
				if(KeyCode == 28)	curr_show = 1; // 日期模式
				if(KeyCode == 29)	curr_show = 2; // 温度模式
				if(KeyCode == 30 && curr_show != 3 && able == 1){   // 输入密码模式
					curr_show = 3; 
					curr_input = 0;
				}	
				if((KeyCode == 31 && curr_show != 4 && curr_show != 3 && able == 1) || (KeyCode == 31 && curr_show == 3 && curr_input == 0 && able == 1)){
					curr_show = 4; 
				   	curr_input = 0;
				}
		 
				if(KeyCode >= 17 && KeyCode <= 26 && curr_show == 3 && curr_input < 4 && able == 1)	//输入密码
				{
					LED8[curr_input+4] = KeyCode - 17;
					key[curr_input] = KeyCode - 17;
					curr_input++;
				}
				if(KeyCode >= 17 && KeyCode <= 26 && curr_show == 4 && curr_input < 8 && able == 1)	//修改密码
				{
					LED8[curr_input] = KeyCode - 17;
					key[curr_input] = KeyCode - 17;
					curr_input++;
				}
				else if(KeyCode >= 17 && KeyCode <= 20 && curr_show != 3){	// 时间和日期的调整
					changing(KeyCode,curr_show);
				}
				if(KeyCode == 30 && curr_show == 3 && curr_input == 4 && able == 1){   // 输入密码模式，检测
				   	if(standard[0] == key[0] && standard[1] == key[1]  && standard[2] == key[2] && standard[3] == key[3]){
						P17 = 0;
						P16 = 0;
						P47 = 0;
						P46 = 0;
						open = 1;
						try = 0;
					}
					else{
						alert(1);
						if(++try >= 5){
							alert(4);
						    able = 0;
						}
						curr_input = 0;
					}
				}
				if(KeyCode == 31 && curr_show == 4 && curr_input == 8 && able == 1){   // 修改密码模式，检测
				   	if(standard[0] == key[0] && standard[1] == key[1]  && standard[2] == key[2] && standard[3] == key[3]){
						try = 0;
						reKey();
						if(key[4] == 0 && key[5] == 0 && key[6] == 0 && key[7] == 0){
							alert(3);
						}
						else{
							P17 = 0; P16 = 0;P47 = 0;P46 = 0;
							standard[0] = key[4]; 
							standard[1] = key[5]; 
							standard[2] = key[6]; 
							standard[3] = key[7];
							delay_ms(1500);							 
							curr_show = 3;	 
						}	
					}
					
					else{
						alert(1);
						if(++try >= 5){
							alert(4);
							able = 0;
						}
						curr_input = 0;
					}
					
				}					
				if(KeyCode == 32){	  // 撤销+关门+门锁复位
					
					if(curr_show == 3 && curr_input > 0 && able == 1){
						key[curr_input-1] = 0;
						LED8[curr_input+3] = DIS_;
						curr_input--;
					}
					else if(curr_show == 4 && curr_input > 0 && able == 1) {
						key[curr_input-1] = 0;
						LED8[curr_input-1] = DIS_;
						curr_input--;
					}
					else{
						open = 0;
						reKey();
						curr_show = 0;
					}
	
				}  

				

				Display(curr_show);
				KeyCode = 0;
	  
			}

		}
	}
} 
/******************************/
void	changeDate(void){
	if(hour == 0 && minute == 0 && second == 0)
		changing(19,1);
}
void	changing(u8 KeyCode, u8 curr_show){
	if(curr_show == 0){
		if(KeyCode == 17){
			if(hour == 23){
				hour = 0;
				changing(19, 1);
			}
			else{
			   hour++;
			}
		}
		else if(KeyCode == 18){
			if(hour == 0){
				hour = 23;
				changing(20, 1);
			}
			else{
			   hour--;
			}
		}
		else if (KeyCode == 19){
			if(minute == 59){
				minute = 0;
				changing(17, 0);
			}
			else minute++;
			second = 0;
		}
		else if (KeyCode == 20){
			if(minute == 0){
				minute = 59;
				changing(18, 0);
			}
			else minute--;
			second = 0;
		}
		WriteRTC();
	}
	else if(curr_show == 1){
		if(KeyCode == 17){
			if(month == 12){
				year++;
				month = 1;
			}
			else month++;
		}
		else if(KeyCode == 18){
			if(month == 1){
				year--;
				month = 12;
			}
			else{
				month--;
			}
		}
		else if(KeyCode == 19){
		  	if(day == day_num[month-1]){
				day = 1;
				changing(17, 1);
			}
			else{
				++day;
			}
		}
		else if(KeyCode == 20){
			if(day == 1){
				changing(18, 1);
				day = day_num[month-1];
			}
			else{
				day--;
			}
		}
	}
}

void alert(u8 _code){
	if(_code == 1){
		P17 = 0;
		P16 = 1;
		P47 = 1;
		P46 = 1;
	}
	if(_code == 2){
		P17 = 1;
		P16 = 0;
		P47 = 1;
		P46 = 1;
	}
	if(_code == 3){
		P17 = 1;
		P16 = 1;
		P47 = 0;
		P46 = 1;
	}
	if(_code == 4){
		while(1)
		{
			P17 = 0;
			delay_ms(250);
			delay_ms(250);
			P17 = 1;
			P16 = 0;
			delay_ms(250);
			delay_ms(250);
			P16 = 1;
			P47 = 0;
			delay_ms(250);
			delay_ms(250);
			P47 = 1;
			P46 = 0;
			delay_ms(250);
			delay_ms(250);
			P46 = 1;
		}
	}
}

void reKey(){
	P17 = 1;	
	P16 = 1;
	P47 = 1;
	P46 = 1; 
	key[0] = 0;
	key[1] = 0;
	key[2] = 0;
	key[3] = 0;
	key[4] = 0;
	key[5] = 0;
	key[6] = 0;
	key[7] = 0;
	curr_input = 0;
}




/**************** 向HC595发送一个字节函数 ******************/
void Send_595(u8 dat)
{		
	u8	i;
	for(i=0; i<8; i++)
	{
		dat <<= 1;
		P_HC595_SER   = CY;
		P_HC595_SRCLK = 1;
		P_HC595_SRCLK = 0;
	}
}

/********************** 显示扫描函数 ************************/
void DisplayScan(void)
{	
	Send_595(~T_COM[display_index]);				//输出位码
	Send_595(t_display[LED8[display_index]]);	//输出段码

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							//锁存输出数据
	if(++display_index >= 8)	display_index = 0;	//8位结束回0
}


/********************** Timer0 1ms中断函数 ************************/
void timer0 (void) interrupt TIMER0_VECTOR
{
	DisplayScan();	//1ms扫描显示一位
	B_1ms = 1;		//1ms标志
}

void  delay_ms(u8 ms)
{
     u16 i;
	 do{
	      i = MAIN_Fosc / 13000;
		  while(--i)	;   //14T per loop
     }while(--ms);
}

