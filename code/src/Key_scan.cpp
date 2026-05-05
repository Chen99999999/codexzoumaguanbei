// #include "zf_common_headfile.h"


// // uint8 Ready_Go=0;
// // uint8 mode =0;
// uint8 Key1,Key2,Key3,Key4;
// uint8 SWITCH1,SWITCH2;
// uint8_t Key_Read()
// {
//     Key1=gpio_get_level(KEY_1);
//     Key2=gpio_get_level(KEY_2);
//     Key3=gpio_get_level(KEY_3);
//     Key4=gpio_get_level(KEY_4);

//     SWITCH1=gpio_get_level(SWITCH_1);
//     SWITCH2=gpio_get_level(SWITCH_2);

//     if(Key1==0)	
// 	{		
//         system_delay_ms(2);
//         if(Key1==0)	
// 	    {
//             return 1;
//         }	
// 	}

// 	if(Key2==0)	
// 	{
// 		system_delay_ms(2);
// 		if(Key2==0);
// 		{
// 			return 2;
// 		}
// 	}

// 	if(Key3==0)	
// 	{
// 		system_delay_ms(2);
// 		if(Key3==0);
// 		{
// 			return 3;
// 		}
// 	}
	
// 	if(Key4==0)	
// 	{
// 		system_delay_ms(2);
// 		while(Key4==0);
// 		{
// 			system_delay_ms(2);
// 			return 4;
// 		}
// 	}	
   
// 	return 0;
// }

// void Change_argument(uint8 idex)
// {
//     switch (idex)
//     {
//         case 1:
//                 Speed_P_l += 0.1;
//                 break;
//         case 2:
//                 Speed_P_l -= 0.1;
//                 break;
//         case 3:
//                 Speed_I_l += 1;
//                 break;
//         case 4:
//                 Speed_I_l -= 1;
//                 break;
//     }
        
// }

// // //----------------��һ���˵���������
// // uint8 Menu_First()
// // {	

// // 	uint8 i=0;
// // 	uint8 Cursor=1;
// // 	uint8 Key_Down=0;

	
// // 	Key_Down=Key_Read();
	
// // 	if(Ready_Go==0)//------------�����˵���ʾ
// // 	{
// // 		Ips_Menu();
// // 	}
// // 	else if(Ready_Go==1)	ips200_clear ();
	
// // 	if(Key_Down==1)
// // 	{
// // 		ips200_clear();
// // 		ips200_show_string(50, 130, "Image");
// // 		ips200_show_string(50, 160, "Control");	
// // 		i=2;		
// // 	}
// // 	if(Key_Down==2)//���������£�����׼�����ܣ�
// // 	{
// // 		ips200_clear ();
// // 		ips200_show_string(50, 130, "GO!GO!GO!");
// // 		beep=1;		
// // 		system_delay_ms(500);
// // 		ips200_clear ();
// // 		system_delay_ms(500);
// // 		beep=0;
// // 		Ready_Go=1;
// // 	}
// // 	while(i==2)//�����һ���˵�ѡ��
// // 	{
// // 		Key_Down=Key_Read();	
// // 		if(Key_Down==1)
// // 		{		
// // 			Cursor++;		
// // 			if(Cursor==3)	Cursor=1;
// // 		}
// // 		if(Key_Down==2)
// // 		{		
// // 			ips200_clear ();
// // 			return Cursor;
// // 		}

// // 		switch(Cursor)
// // 		{
// // 			case 1:
// // 				{
// // 					ips200_show_string(35, 130, "->");
// // 					ips200_show_string(35, 160, "  ");				
// // 				}	
// // 			break;
			
// // 			case 2:
// // 				{
// // 					ips200_show_string(35, 130, "  ");
// // 					ips200_show_string(35, 160, "->");
// // 				}		
// // 			break;
			
// // 		}
// // }
	
// // }

// // //-------------ͼ��˵���������
// // void Menu_Image()
// // {
	
	
// // 	uint8 Key_Down=0;
// // 	uint8 Cursor=1;
// // 	uint8 i=0;

	
// // 	while(1)
// // 	{	
// // 		//ͼ��˵���ʾĿ¼		
				
// // 		// ips200_show_string(30, 250, "Ts1=");		ips200_show_float(6*10, 250,Ts1,2,3);
// // 		// ips200_show_string(30, 270, "Ts2=");		ips200_show_float(6*10, 270,Ts2,2,3);
		
			
// // 		Key_Down=Key_Read();
		
// // 		if(Key_Down==1)
// // 		{		
// // 			ips200_clear ();
// // 			return ;
// // 		}
		
// // 		if(Key_Down==2)
// // 		{
// // 			Cursor++;
// // 			if(Cursor==3)	Cursor=1;
// // 		}
		
// // 		// if(Key_Down==3)
// // 		// {
// // 		// 	if(Cursor==1)	Ts1+=0.007;				
// // 		// 	else if(Cursor==2)	Ts2+=0.007;			
// // 		// }
		
// // 		// if(Key_Down==4)
// // 		// {
// // 		// 	if(Cursor==1)	Ts1-=0.012;		
// // 		// 	else if(Cursor==2)	Ts2-=0.012;
// // 		// }
		
		
		
		
// // 		switch(Cursor)
// // 		{
// // 		case 1:
// // 			{
// // 				ips200_show_string(12, 250, "->");
// // 				ips200_show_string(12, 270, "  ");								
// // 			}	
// // 		break;
		
// // 		case 2:
// // 			{
// // 				ips200_show_string(12, 250, "  ");
// // 				ips200_show_string(12, 270, "->");
// // 			}		
// // 		break;			
// // 		}
// // 	}
	

// // }

// // //-------------���Ʋ˵���������
// // void Menu_Control()
// // {
// // 	uint8 Key_Down=0;
// // 	uint8 Cursor=1;
// // 	//����˵���ʾĿ¼

// // 	while(1)
// // 	{
		
// // 	ips200_show_string(30, 40, "Speed_Begin =");		ips200_show_int(13*10, 40,Speed_Begin,4);
// // 	ips200_show_string(30, 60, "Diff_Kp =");				ips200_show_float(15*10, 60, Diff_Kp,2,3);
// // 	ips200_show_string(30, 80, "Diff_Kd =");				ips200_show_float(15*10, 80, Diff_Kd,2,3);	
// // 	ips200_show_string(30, 100, "Servo_Kp =");				ips200_show_float(10*10, 100, Servo_Kp,2,3);
// // 	ips200_show_string(30, 120, "Servo_Kd =");				ips200_show_float(10*10, 120, Servo_Kd,2,5);
// // 	ips200_show_string(30, 140, "mode =");				ips200_show_int(10*10, 140, mode,2);
	
		
// // 		Key_Down=Key_Read();
// // 		if(mode==3)	mode=0;
// // 		if(Key_Down==1)
// // 		{		
// // 			ips200_clear();
// // 			return ;
// // 		}
		
// // 		if(Key_Down==2)
// // 		{
// // 			Cursor++;
// // 			if(Cursor==7)	Cursor=1;
// // 		}
		
// // 		if(Key_Down==3)
// // 		{
// // 			if(Cursor==1)	Speed_Begin+=5;

// // 			else if(Cursor==2)	Diff_Kp+=0.212;

// // 			else if(Cursor==3)	Diff_Kd+=1.012;
	
// // 			else if(Cursor==4)	Servo_Kp+=0.12;
			
// // 			else if(Cursor==5)	Servo_Kd+=0.12;
			
// // 			else if(Cursor==6)	mode+=1;
				
// // 		}
		
// // 		if(Key_Down==4)
// // 		{
// // 			if(Cursor==1)	Speed_Begin-=5;

// // 			else if(Cursor==2)	Diff_Kp-=0.212;

// // 			else if(Cursor==3)	Diff_Kd-=1.012;

// // 			else if(Cursor==4)	Servo_Kd-=0.12;
			
// // 			else if(Cursor==5)	Servo_Kd-=0.12;
			
// // 			else if(Cursor==6)	mode-=1;
// // 		}
		
// // 		switch(Cursor)
// // 		{
// // 		case 1:
// // 			{
// // 				ips200_show_string(12, 40, "->");
// // 				ips200_show_string(12, 60, "  ");
// // 				ips200_show_string(12, 80, "  ");
// // 				ips200_show_string(12, 100, "  ");
// // 				ips200_show_string(12, 120, "  ");
// // 				ips200_show_string(12, 140, "  ");
// // 			}	
// // 			break;
		
// // 		case 2:
// // 			{
// // 				ips200_show_string(12, 40, "  ");
// // 				ips200_show_string(12, 60, "->");
// // 				ips200_show_string(12, 80, "  ");
// // 				ips200_show_string(12, 100, "  ");
// // 				ips200_show_string(12, 120, "  ");
// // 				ips200_show_string(12, 140, "  ");
// // 			}		
// // 			break;

// // 		case 3:
// // 			{
// // 				ips200_show_string(12, 40, "  ");
// // 				ips200_show_string(12, 60, "  ");
// // 				ips200_show_string(12, 80, "->");
// // 				ips200_show_string(12, 100, "  ");
// // 				ips200_show_string(12, 120, "  ");
// // 				ips200_show_string(12, 140, "  ");
// // 			}	
// // 			break;
			
// // 		case 4:
// // 			{
// // 				ips200_show_string(12, 40, "  ");
// // 				ips200_show_string(12, 60, "  ");
// // 				ips200_show_string(12, 80, "  ");
// // 				ips200_show_string(12, 100, "->");
// // 				ips200_show_string(12, 120, "  ");
// // 				ips200_show_string(12, 140, "  ");
		
// // 			}
// // 		break;
			
// // 		case 5:
// // 		{
// // 				ips200_show_string(12, 40, "  ");
// // 				ips200_show_string(12, 60, "  ");
// // 				ips200_show_string(12, 80, "  ");
// // 				ips200_show_string(12, 100, "  ");
// // 				ips200_show_string(12, 120, "->");
// // 				ips200_show_string(12, 140, "  ");
// // 		}
// // 		break;
		
// // 		case 6:
// // 		{
// // 				ips200_show_string(12, 40, "  ");
// // 				ips200_show_string(12, 60, "  ");
// // 				ips200_show_string(12, 80, "  ");
// // 				ips200_show_string(12, 100, "  ");
// // 				ips200_show_string(12, 120, "  ");
// // 				ips200_show_string(12, 140, "->");
// // 		}
// // 		break;
			
	
			
// // 		}
		
// // 	}
	
// // }

// // //------------�˵��ܴ�������
// // void Menu_Proce()
// // {
// // 	uint8 Menu_Flag=0;
// // 	Menu_Flag=Menu_First();
// // 	if(Menu_Flag==1)	
// // 	{
// // 		Menu_Image();
		
// // 	}
// // 	if(Menu_Flag==2)	
// // 	{
// // 		Menu_Control();
// // 	}

	
// // }


