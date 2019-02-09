#define MAIN_Fosc		22118400L	//������ʱ��

#include	"STC15Fxxxx.H"




/***********************************************************/

#define DIS_DOT		0x20
#define DIS_BLACK	0x10
#define DIS_		0x11


/****************************** �û������ ***********************************/


#define	Timer0_Reload	(65536UL -(MAIN_Fosc / 1000))		//Timer 0 �ж�Ƶ��, 1000��/��

/*****************************************************************************/




/*************	���س�������	**************/
u8 code t_display[]={						//��׼�ֿ�
//	 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
	0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71,
//black	 -     H    J	 K	  L	   N	o   P	 U     t    G    Q    r   M    y
	0x00,0x40,0x76,0x1E,0x70,0x38,0x37,0x5C,0x73,0x3E,0x78,0x3d,0x67,0x50,0x37,0x6e,
	0xBF,0x86,0xDB,0xCF,0xE6,0xED,0xFD,0x87,0xFF,0xEF,0x46};	//0. 1. 2. 3. 4. 5. 6. 7. 8. 9. -1

u8 code T_COM[]={0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};		//λ��
u8 day_num[]={31,28,31,30,31,30,31,31,30,31,30,31};

#define     Baudrate1           115200L
#define     UART1_BUF_LENGTH    32

/*************	IO�ڶ���	**************/
sbit	P_HC595_SER   = P4^0;	//pin 14	SER		data input
sbit	P_HC595_RCLK  = P5^4;	//pin 12	RCLk	store (latch) clock
sbit	P_HC595_SRCLK = P4^3;	//pin 11	SRCLK	Shift data clock


/*************	���ر�������	**************/

u8 	LED8[8];		//��ʾ����

u8	display_index;	//��ʾλ����
bit	B_1ms;			//1ms��־

u8  IO_KeyState, IO_KeyState1, IO_KeyHoldCnt;	//���м��̱���
u8	KeyHoldCnt;	//�����¼�ʱ
u8	KeyCode;	//���û�ʹ�õļ���, 1~16��Ч
u8	cnt50ms;
u8	try,able;

u8	month,day,hour,minute,second;	//RTC����
u8  curr_input;
u8  key[] = {0, 0, 0, 0, 0, 0, 0, 0};
u8 	standard[] = {1, 2, 3, 4};
u8  curr_show;  // 0��ʾʱ�䣬1��ʾ���ڣ�2��ʾ�¶�
u8  open;
u16 year;
u16	msecond, opentime, trytime;

//��¼��ǰ״̬
u8 curr_state;   //0�����������ر�;1����������������2�����쳣���رգ����������/�쳣�����

u8  TX1_Cnt;    //���ͼ���
u8  RX1_Cnt;    //���ռ���
bit B_TX1_Busy; //����æ��־
u8  idata RX1_Buffer[UART1_BUF_LENGTH]; //���ջ���
u8* Send_String;


/*************	���غ�������	**************/
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
void    UART1_config(u8 brt);   // ѡ������, 2: ʹ��Timer2��������, ����ֵ: ʹ��Timer1��������.
void    PrintString1(u8 *puts);
u8		*my_itoa(u16 n);
										   
/****************  �ⲿ�����������ⲿ�������� *****************/


/**********************************************/
void main(void)
{
	u8	i;

	P0M1 = 0;	P0M0 = 0;	//����Ϊ׼˫���
	P1M1 = 0;	P1M0 = 0;	//����Ϊ׼˫���
	P2M1 = 0;	P2M0 = 0;	//����Ϊ׼˫���
	P3M1 = 0;	P3M0 = 0;	//����Ϊ׼˫���
	P4M1 = 0;	P4M0 = 0;	//����Ϊ׼˫���
	P5M1 = 0;	P5M0 = 0;	//����Ϊ׼˫���
	P6M1 = 0;	P6M0 = 0;	//����Ϊ׼˫���
	P7M1 = 0;	P7M0 = 0;	//����Ϊ׼˫���
	
	P1ASF = 0x0C;		//P1.2 P1.3��ADC
	ADC_CONTR = 0xE0;	//90T, ADC power on
	
	display_index = 0;
	curr_show = 0;
	curr_input = 0;
	curr_state = 0;
	opentime = 0;
	trytime = 0;
	AUXR = 0x80;	//Timer0 set as 1T, 16 bits timer auto-reload, 
	TH0 = (u8)(Timer0_Reload / 256);
	TL0 = (u8)(Timer0_Reload % 256);
	ET0 = 1;	//Timer0 interrupt enable
	TR0 = 1;	//Tiner0 run

	UART1_config(1);    // ѡ������, 2: ʹ��Timer2��������, ����ֵ: ʹ��Timer1��������.
	EA = 1;		//�����ж�
	
	for(i=0; i<8; i++)	LED8[i] = 0x10;	//�ϵ�����

	
	ReadRTC();
	F0 = 0;
	month = 1;
	year = 2019;
	day = 1;
	if(second >= 60)	F0 = 1;	//����
	if(minute >= 60)	F0 = 1;	//����
	if(hour   >= 24)	F0 = 1;	//����
	if(F0)	//�д���, Ĭ��12:00:00
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

	KeyHoldCnt = 0;	//�����¼�ʱ
	KeyCode = 0;	//���û�ʹ�õļ���, 1~16��Ч

	IO_KeyState = 0;
	IO_KeyState1 = 0;
	IO_KeyHoldCnt = 0;
	cnt50ms = 0;
	
	while(1)
	{
		if(B_1ms)	//1ms��
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
			if(++msecond >= 1000)	//1�뵽
			{
				msecond = 0;
				changeDate();
				ReadRTC();
				Display(curr_show);
			}

			if(++cnt50ms >= 50)		//50msɨ��һ�����м���
			{
				cnt50ms = 0;
				IO_KeyScan();
			}
			
			if(KeyCode != 0)		//�м�����
			{
				if(KeyCode == 27)	curr_show = 0; // ʱ��ģʽ
				if(KeyCode == 28)	curr_show = 1; // ����ģʽ
				if(KeyCode == 29)	curr_show = 2; // �¶�ģʽ
				if(KeyCode == 30 && curr_show != 3 && able == 1){   // ��������ģʽ
					curr_show = 3; 
					curr_input = 0;
				}	
				if((KeyCode == 31 && curr_show != 4 && curr_show != 3 && able == 1) || (KeyCode == 31 && curr_show == 3 && curr_input == 0 && able == 1)){
					curr_show = 4; 
				   	curr_input = 0;
				}
		 
				if(KeyCode >= 17 && KeyCode <= 26 && curr_show == 3 && curr_input < 4 && able == 1)	//��������
				{
					LED8[curr_input+4] = KeyCode - 17;
					key[curr_input] = KeyCode - 17;
					curr_input++;
				}
				if(KeyCode >= 17 && KeyCode <= 26 && curr_show == 4 && curr_input < 8 && able == 1)	//�޸�����
				{
					LED8[curr_input] = KeyCode - 17;
					key[curr_input] = KeyCode - 17;
					curr_input++;
				}
				else if(KeyCode >= 17 && KeyCode <= 20 && curr_show != 3){	// ʱ������ڵĵ���
					changing(KeyCode,curr_show);
				}
				if(KeyCode == 30 && curr_show == 3 && curr_input == 4 && able == 1){   // ��������ģʽ�����
				   	if(standard[0] == key[0] && standard[1] == key[1]  && standard[2] == key[2] && standard[3] == key[3]){//������ȷ
						P17 = 0;
						P16 = 0;
						P47 = 0;
						P46 = 0;
						//���ź�Сʱ�����ӡ����ӷ��͵�log������
						Send_String = my_itoa(hour);
					    PrintString1(Send_String);   
						PrintString1("s");   //�ո� �����ڽ��ն���������
						Send_String = my_itoa(minute);
						PrintString1(Send_String);
						PrintString1("s");
						Send_String = my_itoa(second);
						PrintString1(Send_String);
						PrintString1("s");
						open = 1;
						curr_state = 1;
						try = 0;
					}
					else{
						alert(1);
						if(++try >= 5){
							alert(4);
						    able = 0;
							curr_state = 2;
						}
						curr_input = 0;
					}
				}
				if(KeyCode == 31 && curr_show == 4 && curr_input == 8 && able == 1){   // �޸�����ģʽ�����
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
							curr_state = 0;
						}	
					}
					
					else{
						alert(1);
						if(++try >= 5){
							alert(4);
							able = 0;
							curr_state = 2;
						}
						curr_input = 0;
					}
					
				}					
				if(KeyCode == 32){	  // ����+����+������λ
					
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
						curr_state = 0;
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




/**************** ��HC595����һ���ֽں��� ******************/
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

/********************** ��ʾɨ�躯�� ************************/
void DisplayScan(void)
{	
	Send_595(~T_COM[display_index]);				//���λ��
	Send_595(t_display[LED8[display_index]]);	//�������

	P_HC595_RCLK = 1;
	P_HC595_RCLK = 0;							//�����������
	if(++display_index >= 8)	display_index = 0;	//8λ������0
}


/********************** Timer0 1ms�жϺ��� ************************/
void timer0 (void) interrupt TIMER0_VECTOR
{
	DisplayScan();	//1msɨ����ʾһλ
	B_1ms = 1;		//1ms��־
}

void  delay_ms(u8 ms)
{
     u16 i;
	 do{
	      i = MAIN_Fosc / 13000;
		  while(--i)	;   //14T per loop
     }while(--ms);
}

/*********************** �������������ĸ�ͨ�ź��� ****************/
//========================================================================
// ����: SetTimer2Baudraye(u16 dat)
// ����: ����Timer2�������ʷ�������
// ����: dat: Timer2����װֵ.
// ����: none.
// �汾: VER1.0
// ����: 2014-11-28
// ��ע: 
//========================================================================
void    SetTimer2Baudraye(u16 dat)  // ѡ������, 2: ʹ��Timer2��������, ����ֵ: ʹ��Timer1��������.
{
    AUXR &= ~(1<<4);    //Timer stop
    AUXR &= ~(1<<3);    //Timer2 set As Timer
    AUXR |=  (1<<2);    //Timer2 set as 1T mode
    TH2 = dat / 256;
    TL2 = dat % 256;
    IE2  &= ~(1<<2);    //��ֹ�ж�
    AUXR |=  (1<<4);    //Timer run enable
}

//========================================================================
// ����: void   UART1_config(u8 brt)
// ����: UART1��ʼ��������
// ����: brt: ѡ������, 2: ʹ��Timer2��������, ����ֵ: ʹ��Timer1��������.
// ����: none.
// �汾: VER1.0
// ����: 2014-11-28
// ��ע: 
//========================================================================
void    UART1_config(u8 brt)    // ѡ������, 2: ʹ��Timer2��������, ����ֵ: ʹ��Timer1��������.
{
    /*********** ������ʹ�ö�ʱ��2 *****************/
    if(brt == 2)
    {
        AUXR |= 0x01;       //S1 BRT Use Timer2;
        SetTimer2Baudraye(65536UL - (MAIN_Fosc / 4) / Baudrate1);
    }

    /*********** ������ʹ�ö�ʱ��1 *****************/
    else
    {
        TR1 = 0;
        AUXR &= ~0x01;      //S1 BRT Use Timer1;
        AUXR |=  (1<<6);    //Timer1 set as 1T mode
        TMOD &= ~(1<<6);    //Timer1 set As Timer
        TMOD &= ~0x30;      //Timer1_16bitAutoReload;
        TH1 = (u8)((65536UL - (MAIN_Fosc / 4) / Baudrate1) / 256);
        TL1 = (u8)((65536UL - (MAIN_Fosc / 4) / Baudrate1) % 256);
        ET1 = 0;    //��ֹ�ж�
        INT_CLKO &= ~0x02;  //�����ʱ��
        TR1  = 1;
    }
    /*************************************************/

    SCON = (SCON & 0x3f) | 0x40;    //UART1ģʽ, 0x00: ͬ����λ���, 0x40: 8λ����,�ɱ䲨����, 0x80: 9λ����,�̶�������, 0xc0: 9λ����,�ɱ䲨����
//  PS  = 1;    //�����ȼ��ж�
    ES  = 1;    //�����ж�
    REN = 1;    //�������
    P_SW1 &= 0x3f;
    P_SW1 |= 0x80;      //UART1 switch to, 0x00: P3.0 P3.1, 0x40: P3.6 P3.7, 0x80: P1.6 P1.7 (����ʹ���ڲ�ʱ��)
//  PCON2 |=  (1<<4);   //�ڲ���·RXD��TXD, ���м�, ENABLE,DISABLE

    B_TX1_Busy = 0;
    TX1_Cnt = 0;
    RX1_Cnt = 0;
}


void PrintString1(u8 *puts) //ʹ��uart1����һ���ַ���
{
    for (; *puts != 0;  puts++)     //����ֹͣ��0����
    {
        SBUF = *puts;
        B_TX1_Busy = 1;
        while(B_TX1_Busy);
    }
}

//========================================================================
// ����: void UART1_int (void) interrupt UART1_VECTOR
// ����: UART1�жϺ�����
// ����: nine.
// ����: none.
// �汾: VER1.0
// ����: 2014-11-28
// ��ע: 
//========================================================================
void UART1_int (void) interrupt 4
{
    if(RI)
    {
        RI = 0;
        RX1_Buffer[RX1_Cnt] = SBUF;
        if(++RX1_Cnt >= UART1_BUF_LENGTH)   RX1_Cnt = 0;    //�����
    }

    if(TI)
    {
        TI = 0;
        B_TX1_Busy = 0;
    }
}

//��ת�ַ���
u8 *reverse(u8 *s)
{
    u8 temp;
    u8 *p = s;    //pָ��s��ͷ��
    u8 *q = s;    //qָ��s��β��
    while(*q)
        ++q;
    q--;
    
    //�����ƶ�ָ�룬ֱ��p��q����
    while(q > p)
    {
        temp = *p;
        *p++ = *q;
        *q-- = temp;
    }
    return s;
}

/*
 * ���ܣ�����ת��Ϊ�ַ���
 * char s[] �������Ǵ洢������ÿһλ
 */
u8 *my_itoa(u16 n)
{
    int i = 0,isNegative = 0;
    static u8 s[10];      //����Ϊstatic������������ȫ�ֱ���
    if((isNegative = n) < 0) //����Ǹ�������תΪ����
    {
        n = -n;
    }
    do      //�Ӹ�λ��ʼ��Ϊ�ַ���ֱ�����λ�����Ӧ�÷�ת
    {
        s[i++] = n%10 + '0';
        n = n/10;
    }while(n > 0);
    
    if(isNegative < 0)   //����Ǹ��������ϸ���
    {
        s[i++] = '-';
    }
    s[i] = '\0';    //�������ַ���������
    return reverse(s);
}