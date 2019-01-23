#define MAIN_Fosc		22118400L	//定义主时钟

#include	"STC15Fxxxx.H"
#define DIS_DOT		0x20
#define DIS_BLACK	0x10
#define DIS_		0x11
extern u8 	LED8[8];		//显示缓冲
extern u8	month,day,hour,minute,second;	//RTC变量
extern u16 year;
extern u8  curr_input;
extern u8  key[];
/**********************************************/
void	Display(u8 curr_show);
void	DisplayTime(void);
void	DisplayDate(void);
void	DisplayTemp(void);
void	DisplayKey(void);
void	DisplayNewKey(void);
void	reKey(void);
u16		get_temperature(u16 adc);
u16		Get_ADC10bitResult(u8 channel);	//channel = 0~7


void Display(u8 curr_show){
	
	if(curr_show == 3){
		DisplayKey();
	}
	else if(curr_show == 4){
		DisplayNewKey();
	}
	else{
		reKey();
		if(curr_show == 0){
			DisplayTime();
		}
		else if(curr_show == 1){
			DisplayDate();
		}
		else if(curr_show == 2){
			DisplayTemp();
		}
	}
	
}

/********************** 显示时钟函数 ************************/
void	DisplayTime(void)
{
	if(hour >= 10)	LED8[0] = hour / 10;
	else			LED8[0] = DIS_BLACK;
	LED8[1] = hour % 10;
	LED8[2] = DIS_;
	LED8[3] = minute / 10;
	LED8[4] = minute % 10;
	LED8[5] = DIS_;
	LED8[6] = second / 10;
	LED8[7] = second % 10;	

}

void	DisplayDate(void)
{
	LED8[0] = year / 1000;
	LED8[1] = year % 1000 / 100;
	LED8[2] = year % 100 / 10;
	LED8[3] = year % 10;
	LED8[4] = month / 10;
	LED8[5] = month % 10;
	LED8[6] = day / 10;
	LED8[7] = day % 10;
}

void	DisplayTemp(void)
{
	u8 i;
	u16 j;

	j = Get_ADC10bitResult(2);	//参数0~7,查询方式做一次ADC, 返回值就是结果, == 1024 为错误
				
	if(j < 1024)
	{
		LED8[0] = j / 1000;		//显示ADC值
		LED8[1] = (j % 1000) / 100;
		LED8[2] = (j % 100) / 10;
		LED8[3] = j % 10;
		if(LED8[0] == 0)	LED8[0] = DIS_BLACK;
	}
	else	//错误
	{
		for(i=0; i<4; i++)	LED8[i] = DIS_;
	}

	j = Get_ADC10bitResult(3);	//参数0~7,查询方式做一次ADC, 返回值就是结果, == 1024 为错误
	j += Get_ADC10bitResult(3);
	j += Get_ADC10bitResult(3);
	j += Get_ADC10bitResult(3);

	if(j < 1024*4)
	{
		j =	get_temperature(j);	//计算温度值

		if(j >= 150)	F0 = 0,	j -= 150;		//温度 >= 0度
		else			F0 = 1,	j  = 150 - j;	//温度 <  0度
		LED8[4] = j / 1000;		//显示温度值
		LED8[5] = (j % 1000) / 100;
		LED8[6] = (j % 100) / 10 + DIS_DOT;
		LED8[7] = j % 10;
		if(LED8[4] == 0)	LED8[4] = DIS_BLACK;

	}
}

void DisplayKey(void){
	u8 i;
	LED8[0] = DIS_BLACK;
	LED8[1] = DIS_BLACK;
	LED8[2] = DIS_BLACK;
	LED8[3] = DIS_BLACK;
	
	for(i = 4; i < curr_input + 4; ++i) LED8[i] = key[i-4];
	for(i = curr_input + 4; i < 8; ++i) LED8[i] = DIS_;
}

void	DisplayNewKey(void){
   	u8 i;
	for(i = 4; i < curr_input; ++i) LED8[i] = key[i];
	for(i = curr_input; i < 8; ++i) LED8[i] = DIS_;
}