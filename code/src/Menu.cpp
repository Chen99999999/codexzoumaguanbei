// #include "zf_common_headfile.h"



// void Ips_Menu()
// {
//     //ips200_show_gray_image(0, 0, rgay_image, 80, 60);
//     Key1=gpio_get_level(KEY_1);
//     Key2=gpio_get_level(KEY_2);
//     Key3=gpio_get_level(KEY_3);
//     Key4=gpio_get_level(KEY_4);
//     SWITCH1=gpio_get_level(SWITCH_1);
//     SWITCH2=gpio_get_level(SWITCH_2);

//     if(SWITCH1==0 && SWITCH2==1)
//     {
//         if(Key1==0)	
//         {
            

//             Servo_Kp+=0.12;
//         }

//         if(Key2==0)	
//         {

//             Servo_Kd+=0.12;
//         }

//         if(Key3==0)	
//         {

//             Servo_Kp-=0.12;
//         }
    
//         if(Key4==0)	
//         {
    
//             Servo_Kd-=0.12;
//         }
//     }

//     else if(SWITCH1==0 && SWITCH2==0)
//     {
//         if(Key1==0)	
//         {

//             ImageStatus.TowPoint+=1;
//         }

//         if(Key2==0)	
//         {

//             ImageStatus.TowPoint-=1;
//         }

//         if(Key3==0)	
//         {

//             Speed_Begin+=20;
//         }

//         if(Key4==0)	
//         {

//             Speed_Begin-=20;
//         }
//     }

//     else if(SWITCH1==1 && SWITCH2==1)
//     {
//         if(Key1==0)	
//         {

//             K+=0.02;
//         }

//         if(Key2==0)	
//         {

//             K-=0.02;
//         }

//         if(Key3==0)	
//         {
//             ips200_clear ();
//             ips200_show_string(50, 130, "GO!GO!GO!");
//             gpio_set_level(BEEP, 1);	
//             system_delay_ms(1000);
//             ips200_clear ();
//             system_delay_ms(1000);
//             gpio_set_level(BEEP, 0);
//             Ready_Go=1;
//             ips200_clear();
           
//         }

//     }
    

    

//     ips200_show_string(0,	80, "Error =");		ips200_show_int(6*10, 80, (ImageStatus.Det_True-39),4);
//     ips200_show_string(0,	100, "Servo_Kp =");		ips200_show_float(10*10, 100, Servo_Kp,2,3);
//     ips200_show_string(0,	120, "Servo_Kd =");		ips200_show_float(10*10, 120, Servo_Kd,2,3);    
       
//     ips200_show_string(0,	140, "TowPoint =");		ips200_show_int(10*10, 140, ImageStatus.TowPoint,2);   
//     ips200_show_string(0,	160, "Speed_Begin =");		ips200_show_int(10*10, 160, Speed_Begin,4);

//     ips200_show_string(0,	180, "Diff_SpeedL_expect =");		ips200_show_int(16*10, 180, Diff_SpeedL_expect,4);    
//     ips200_show_string(0,	200, "Diff_SpeedR_expect =");		ips200_show_int(16*10, 200, Diff_SpeedR_expect,4);

//     ips200_show_string(0,	220, "Servo_angle =");		ips200_show_float(10*10, 220, Servo_angle,3,3);   
//     ips200_show_string(0,	240, "Dif =");		ips200_show_float(6*10, 240, Dif,2,3);

//     ips200_show_string(0,	260, "K =");		ips200_show_float(6*10, 260, K,3,3);

// }   



// void Bin_show()
// {
//    for (int i = 0; i < LCDH; ++i)
//     {
//         for (int j = 0; j < LCDW; ++j)
//         {
//             // Ϊ�����ն�����ʾ�������������ò�ͬ�ַ���ʾ 0 �� 255
//             if (Pixle[i][j] == 0)
//             {
//                 std::cout << " ";
//             }
//             else
//             {
//                 std::cout << "*";
//             }
//         }
//         std::cout << std::endl;
//     }


// }
