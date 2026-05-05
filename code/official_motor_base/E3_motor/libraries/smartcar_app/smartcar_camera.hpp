#pragma once

#include "zf_common_headfile.hpp"

// ============================================================
// smartcar_camera.hpp
// UVC 摄像头封装
//
// 第一版目标：
// 1. 初始化 UVC 摄像头
// 2. 等待新图像帧
// 3. 获取灰度图 / RGB565 图像指针
// 4. 给后续循迹算法提供原始图像
// ============================================================

bool camera_init(void);
bool camera_update(void);

uint8* camera_get_gray(void);
uint16* camera_get_rgb565(void);

int camera_get_width(void);
int camera_get_height(void);

void camera_print_status(void);


// 保存当前灰度图为 PGM 文件
// path 示例：/home/root/capture/gray_0001.pgm
bool camera_save_gray_pgm(const char *path);
