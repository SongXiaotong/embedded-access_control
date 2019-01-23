
#define MAIN_Fosc		22118400L	//������ʱ��

#include	"STC15Fxxxx.H"
//========================================================================
// ����: u16	Get_ADC10bitResult(u8 channel)
// ����: ��ѯ����һ��ADC���.
// ����: channel: ѡ��Ҫת����ADC.
// ����: 10λADC���.
// �汾: V1.0, 2012-10-22
//========================================================================
 u16	Get_ADC10bitResult(u8 channel)	//channel = 0~7
{
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xe0) | 0x08 | channel; 	//start the ADC
	NOP(4);

	while((ADC_CONTR & 0x10) == 0)	;	//wait for ADC finish
	ADC_CONTR &= ~0x10;		//���ADC������־
	return	(((u16)ADC_RES << 2) | (ADC_RESL & 3));
}


//	MF52E 10K at 25, B = 3950, ADC = 12 bits
u16 code temp_table[]={
		509,	//;-15	0
		533,	//;-14	1
		558,	//;-13	2
		583,	//;-12	3
		610,	//;-11	4
		638,	//;-10	5
		667,	//;-9	6
		696,	//;-8	7
		727,	//;-7	8
		758,	//;-6	9
		791,	//;-5	10
		824,	//;-4	11
		858,	//;-3	12
		893,	//;-2	13
		929,	//;-1	14
		965,	//;0	15
		1003,	//;1	16
		1041,	//;2	17
		1080,	//;3	18
		1119,	//;4	19
		1160,	//;5	20
		1201,	//;6	21
		1243,	//;7	22
		1285,	//;8	23
		1328,	//;9	24
		1371,	//;10	25
		1414,	//;11	26
		1459,	//;12	27
		1503,	//;13	28
		1548,	//;14	29
		1593,	//;15	30
		1638,	//;16	31
		1684,	//;17	32
		1730,	//;18	33
		1775,	//;19	34
		1821,	//;20	35
		1867,	//;21	36
		1912,	//;22	37
		1958,	//;23	38
		2003,	//;24	39
		2048,	//;25	40
		2093,	//;26	41
		2137,	//;27	42
		2182,	//;28	43
		2225,	//;29	44
		2269,	//;30	45
		2312,	//;31	46
		2354,	//;32	47
		2397,	//;33	48
		2438,	//;34	49
		2479,	//;35	50
		2519,	//;36	51
		2559,	//;37	52
		2598,	//;38	53
		2637,	//;39	54
		2675,	//;40	55
		2712,	//;41	56
		2748,	//;42	57
		2784,	//;43	58
		2819,	//;44	59
		2853,	//;45	60
		2887,	//;46	61
		2920,	//;47	62
		2952,	//;48	63
		2984,	//;49	64
		3014,	//;50	65

};

/********************  �����¶� ***********************************************/
// ������: 0��Ӧ-15.0��, 150��Ӧ0��, ���650��Ӧ50��. 
// Ϊ��ͨ��, ADC����Ϊ12bit��ADCֵ.
// ��·�������㷨���: Coody
/**********************************************/

#define		D_SCALE		10		//����Ŵ���, �Ŵ�10�����Ǳ���һλС��
u16	get_temperature(u16 adc)
{
	u16	code *p;
	u16	i;
	u8	j,k,min,max;
	
	adc = 4096 - adc;	//Rt�ӵ�
	p = temp_table;
	if(adc < p[0])		return (0xfffe);
	if(adc > p[65])	return (0xffff);
	
	min = 0;		//-15��
	max = 65;		//50��

	for(j=0; j<5; j++)	//�Էֲ��
	{
		k = min / 2 + max / 2;
		if(adc <= p[k])	max = k;
		else			min = k;
	}
		 if(adc == p[min])	i = min * D_SCALE;
	else if(adc == p[max])	i = max * D_SCALE;
	else	// min < temp < max
	{
		while(min <= max)
		{
			min++;
			if(adc == p[min])	{i = min * D_SCALE;	break;}
			else if(adc < p[min])
			{
				min--;
				i = p[min];	//min
				j = (adc - i) * D_SCALE / (p[min+1] - i);
				i = min;
				i *= D_SCALE;
				i += j;
				break;
			}
		}
	}
	return i;
}
