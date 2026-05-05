/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是LS2K0300 开源库的一部分
*
* LS2K0300 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main
* 公司名称          成都逐飞科技有限公司
* 适用平台          LS2K0300
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者           备注
* 2025-02-27        大W            first version
********************************************************************************************************************/

#include "zf_common_headfile.h"







void cleanup()
{
    printf("程序异常退出，执行清理操作\n");

    pwm_set_duty(PWM1, 0);   
    pwm_set_duty(PWM2, 0);
    pwm_set_duty(PWM3, 0);   
}

void sigint_handler(int signum) 
{
    printf("收到Ctrl+C，程序即将退出\n");
    cleanup();
    exit(0);
}

// *************************** 例程硬件连接说明 ***************************
//  久久派与主板使用54pin排线连接，再将久久派插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  久久派与主板使用54pin排线连接，再将久久派插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  久久派与主板使用54pin排线连接，再将久久派插到主板上面，确保插到底核心板与主板插座间没有缝隙即可
//  使用本历程，就需要使用我们逐飞科技提供的内核。
// 
// *************************** 例程使用步骤说明 ***************************
// 1.根据硬件连接说明连接好模块，使用电源供电(下载器供电会导致模块电压不足)
//
// 2.查看电脑所连接的wifi，记录IP地址
//
// 3.在下方的代码区域中修改宏定义，SERVER_IP为电脑的IP地址，PORT为端口号
//
// 5.下载例程到久久派中
//
// 6.打开逐飞助手，设置为TCP，设置端口，选择合适的本机地址后点击连接
//
//
// *************************** 例程测试说明 ***************************
// 1.当本机设备成功连接到目标后（例如电脑使用逐飞助手上位机进入TCP模式 然后本机连接到电脑的 IP 与端口）
//
// 2.打开逐飞助手软件的图像显示，就可以看到灰度图像
// 
// 如果发现现象与说明严重不符 请参照本文件最下方 例程常见问题说明 进行排查
//
// **************************** 代码区域 ****************************


#define SERVER_IP "10.254.15.206"
#define PORT 8086


// **************************** 代码区域 ****************************

//0：不包含边界信息  
//1：包含三条边线信息，边线信息只包含横轴坐标，纵轴坐标由图像高度得到，意味着每个边界在一行中只会有一个点
//2：包含三条边线信息，边界信息只含有纵轴坐标，横轴坐标由图像宽度得到，意味着每个边界在一列中只会有一个点，一般来说很少有这样的使用需求
//3：包含三条边线信息，边界信息含有横纵轴坐标，意味着你可以指定每个点的横纵坐标，边线的数量也可以大于或者小于图像的高度，通常来说边线数量大于图像的高度，一般是搜线算法能找出回弯的情况
//4：没有图像信息，仅包含三条边线信息，边线信息只包含横轴坐标，纵轴坐标由图像高度得到，意味着每个边界在一行中只会有一个点，这样的方式可以极大的降低传输的数据量
#define INCLUDE_BOUNDARY_TYPE   0



// 边界的点数量远大于图像高度，便于保存回弯的情况
#define BOUNDARY_NUM            (UVC_HEIGHT * 4 / 2)

// 只有X边界
uint8 xy_x1_boundary[BOUNDARY_NUM], xy_x2_boundary[BOUNDARY_NUM], xy_x3_boundary[BOUNDARY_NUM];

// 只有Y边界
uint8 xy_y1_boundary[BOUNDARY_NUM], xy_y2_boundary[BOUNDARY_NUM], xy_y3_boundary[BOUNDARY_NUM];

// X Y边界都是单独指定的
uint8 x1_boundary[UVC_HEIGHT], x2_boundary[UVC_HEIGHT], x3_boundary[UVC_HEIGHT];
uint8 y1_boundary[UVC_WIDTH], y2_boundary[UVC_WIDTH], y3_boundary[UVC_WIDTH];


uint8 image_copy[UVC_HEIGHT][UVC_WIDTH];

int main() 
{



 signal(SIGINT, sigint_handler);
    
    Init_all();
    Data_Settings(); 
 



#if(1 == INCLUDE_BOUNDARY_TYPE || 2 == INCLUDE_BOUNDARY_TYPE || 4 == INCLUDE_BOUNDARY_TYPE)
    int32 i = 0;
#elif(3 == INCLUDE_BOUNDARY_TYPE)
int32 i = 0;
    int32 j = 0;
#endif

    // 初始化TCP客户端,需要先打开TCP服务器,这才不会卡主。
    // 初始化TCP客户端,需要先打开TCP服务器,这才不会卡主。
    // 初始化TCP客户端,需要先打开TCP服务器,这才不会卡主。
    if(tcp_client_init(SERVER_IP, PORT) == 0)
    {
        printf("tcp_client ok\r\n");
    }
    else
    {
        printf("tcp_client error\r\n");
        return -1;
    }

    // 逐飞助手初始化 设置回调函数
    seekfree_assistant_interface_init(tcp_client_send_data, tcp_client_read_data);

#if(0 == INCLUDE_BOUNDARY_TYPE)
    // 发送摄像头图像图像信息(仅包含原始图像信息)
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_copy[0], UVC_WIDTH, UVC_HEIGHT);


#elif(1 == INCLUDE_BOUNDARY_TYPE)
    // 发送摄像头图像图像信息(并且包含三条边界信息，边界信息只含有横轴坐标，纵轴坐标由图像高度得到，意味着每个边界在一行中只会有一个点)
    // 对边界数组写入数据
    for(i = 0; i < UVC_HEIGHT; i++)
    {
        x1_boundary[i] = 50 - (50 - 20) * i / UVC_HEIGHT;
        x2_boundary[i] = UVC_WIDTH / 2;
        x3_boundary[i] = 70 + (148 - 70) * i / UVC_HEIGHT;
    }
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_copy[0], UVC_WIDTH, UVC_HEIGHT);
    seekfree_assistant_camera_boundary_config(X_BOUNDARY, UVC_HEIGHT, x1_boundary, x2_boundary, x3_boundary, NULL, NULL ,NULL);


#elif(2 == INCLUDE_BOUNDARY_TYPE)
    // 发送摄像头图像图像信息(并且包含三条边界信息，边界信息只含有纵轴坐标，横轴坐标由图像宽度得到，意味着每个边界在一列中只会有一个点)
    // 通常很少有这样的使用需求
    // 对边界数组写入数据
    for(i = 0; i < UVC_WIDTH; i++)
    {
        y1_boundary[i] = 50 - (50 - 20) * i / UVC_HEIGHT;
        y2_boundary[i] = UVC_WIDTH / 2;
        y3_boundary[i] = 78 + (78 - 58) * i / UVC_HEIGHT;
    }
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_copy[0], UVC_WIDTH, UVC_HEIGHT);
    seekfree_assistant_camera_boundary_config(Y_BOUNDARY, UVC_WIDTH, NULL, NULL ,NULL, y1_boundary, y2_boundary, y3_boundary);


#elif(3 == INCLUDE_BOUNDARY_TYPE)
    // 发送摄像头图像图像信息(并且包含三条边界信息，边界信息含有横纵轴坐标)
    // 这样的方式可以实现对于有回弯的边界显示
    j = 0;
    for(i = UVC_HEIGHT - 1; i >= UVC_HEIGHT / 2; i--)
    {
        // 直线部分
        xy_x1_boundary[j] = 34;
        xy_y1_boundary[j] = i;
        
        xy_x2_boundary[j] = 47;
        xy_y2_boundary[j] = i;
        
        xy_x3_boundary[j] = 60;
        xy_y3_boundary[j] = i;
        j++;
    }

    for(i = UVC_HEIGHT / 2 - 1; i >= 0; i--)
    {
        // 直线连接弯道部分
        xy_x1_boundary[j] = 34 + (UVC_HEIGHT / 2 - i) * (UVC_WIDTH / 2 - 34) / (UVC_HEIGHT / 2);
        xy_y1_boundary[j] = i;
        
        xy_x2_boundary[j] = 47 + (UVC_HEIGHT / 2 - i) * (UVC_WIDTH / 2 - 47) / (UVC_HEIGHT / 2);
        xy_y2_boundary[j] = 15 + i * 3 / 4;
        
        xy_x3_boundary[j] = 60 + (UVC_HEIGHT / 2 - i) * (UVC_WIDTH / 2 - 60) / (UVC_HEIGHT / 2);
        xy_y3_boundary[j] = 30 + i / 2;
        j++;
    }

    for(i = 0; i < UVC_HEIGHT / 2; i++)
    {
        // 回弯部分
        xy_x1_boundary[j] = UVC_WIDTH / 2 + i * (138 - UVC_WIDTH / 2) / (UVC_HEIGHT / 2);
        xy_y1_boundary[j] = i;
        
        xy_x2_boundary[j] = UVC_WIDTH / 2 + i * (133 - UVC_WIDTH / 2) / (UVC_HEIGHT / 2);
        xy_y2_boundary[j] = 15 + i * 3 / 4;
        
        xy_x3_boundary[j] = UVC_WIDTH / 2 + i * (128 - UVC_WIDTH / 2) / (UVC_HEIGHT / 2);
        xy_y3_boundary[j] = 30 + i / 2;
        j++;
    }
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_copy[0], UVC_WIDTH, UVC_HEIGHT);
    seekfree_assistant_camera_boundary_config(XY_BOUNDARY, BOUNDARY_NUM, xy_x1_boundary, xy_x2_boundary, xy_x3_boundary, xy_y1_boundary, xy_y2_boundary, xy_y3_boundary);


#elif(4 == INCLUDE_BOUNDARY_TYPE)
    // 发送摄像头图像图像信息(并且包含三条边界信息，边界信息只含有横轴坐标，纵轴坐标由图像高度得到，意味着每个边界在一行中只会有一个点)
    // 对边界数组写入数据
    for(i = 0; i < UVC_HEIGHT; i++)
    {
        x1_boundary[i] = 70 - (70 - 20) * i / UVC_HEIGHT;
        x2_boundary[i] = UVC_WIDTH / 2;
        x3_boundary[i] = 80 + (159 - 80) * i / UVC_HEIGHT;
    }
    seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, NULL, UVC_WIDTH, UVC_HEIGHT);
    seekfree_assistant_camera_boundary_config(X_BOUNDARY, UVC_HEIGHT, x1_boundary, x2_boundary, x3_boundary, NULL, NULL ,NULL);


#endif

    atexit(cleanup);
    signal(SIGINT, sigint_handler);
    // 初始化UVC摄像头
    if(uvc_camera_init("/dev/video0") < 0)
    {
        return -1;
    }

    while (true) 
    {
        // 阻塞式等待，图像刷新
        if(wait_image_refresh() < 0)
        {
            // 摄像头未采集到图像，这里需要关闭电机，关闭电调等。
            exit(0);
        }

       ImageProcess();
       Motor_Argument();
        //Encoder_Test1();
       printf("ImageStatus.Left_Line: %d  ImageStatus.Right_Line: %d  ImageStatus.WhiteLine: %d  ImageFlag.image_element_rings_flag: %d  ImageStatus.Road_type  %d  SystemData.Det_True %d\n", 
       ImageStatus.Left_Line, 
       ImageStatus.Right_Line, 
       ImageStatus.WhiteLine,
       ImageFlag.image_element_rings_flag,
       ImageStatus.Road_type,
       ImageStatus.Det_True);
        
     //  printf("ImageStatus.Det_True: %d \n", ImageStatus.Det_True);
      //  pwm_set_duty(PWM3, 5080);

   uint8_t* display_buf = (uint8_t*)image_copy[0]; 

        // 2. 遍历底层的二值化数组 Pixle，写入图传缓冲区
// ==============================================================
        // 1. 将 80x60 的二值化图像放大 2 倍，填入 160x120 的图传数组中
        // ==============================================================
        for(int y = 0; y < 60; y++) 
        {
            for(int x = 0; x < 80; x++) 
            {
                // 获取当前像素的颜色 (黑=0，白=255)
                uint8_t color = (Pixle[y][x] == 0) ? 0 : 255;
                
                // 将 1 个像素放大成 2x2 的 4 个像素，填进 image_copy
                int map_y = y * 2;
                int map_x = x * 2;
                image_copy[map_y][map_x]         = color; // 左上
                image_copy[map_y][map_x + 1]     = color; // 右上
                image_copy[map_y + 1][map_x]     = color; // 左下
                image_copy[map_y + 1][map_x + 1] = color; // 右下
            }
        }

        // ==============================================================
        // 2. 叠加计算出的左右边界和中线 (坐标也需要乘 2 映射)
        // ==============================================================
        for(int y = 0; y < 60; y++) 
        {
            // 坐标映射：底层坐标 * 2
            int left_x  = ImageDeal[y].close_LeftBorder * 2;
            int right_x = ImageDeal[y].close_RightBorder * 2;
            int mid_x   = ImageDeal[y].Center * 2;
            
            int map_y = y * 2; // 对应的图传 Y 坐标

            // 画左边界线 (用 128 灰色)
            if(left_x >= 0 && left_x < 160)  
            {
                image_copy[map_y][left_x] = 128;  
                image_copy[map_y + 1][left_x] = 128;  // 加粗一点
                if(left_x + 1 < 160) {
                    image_copy[map_y][left_x + 1] = 128;
                    image_copy[map_y + 1][left_x + 1] = 128;
                }
            }
            
            // 画右边界线
            if(right_x >= 0 && right_x < 160) 
            {
                image_copy[map_y][right_x] = 128; 
                image_copy[map_y + 1][right_x] = 128;
                if(right_x - 1 >= 0) {
                    image_copy[map_y][right_x - 1] = 128;
                    image_copy[map_y + 1][right_x - 1] = 128;
                }
            }
            
            // 画中线 (画得最粗最明显)
            if(mid_x >= 0 && mid_x < 160) 
            {
                image_copy[map_y][mid_x] = 128;
                image_copy[map_y + 1][mid_x] = 128;
                if(mid_x + 1 < 160) {
                    image_copy[map_y][mid_x + 1] = 128;
                    image_copy[map_y + 1][mid_x + 1] = 128;
                }
                if(mid_x - 1 >= 0) {
                    image_copy[map_y][mid_x - 1] = 128;
                    image_copy[map_y + 1][mid_x - 1] = 128;
                }
            }
        }

        seekfree_assistant_camera_send();

        seekfree_assistant_camera_send();
    }


    return 0;
}

// **************************** 代码区域 ****************************

// *************************** 例程常见问题说明 ***************************
// 遇到问题时请按照以下问题检查列表检查
// 
// 问题1：终端提示未找到xxx文件
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//      使用本历程，就需要使用我们逐飞科技提供的内核，否则提示xxx文件找不到
//
// 问题2：逐飞助手的UDP或TCP一直连接不上
//      需要将久久派连接上网络
//      查看IP地址和端口号是否正确