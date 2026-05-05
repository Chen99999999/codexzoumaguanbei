#pragma once
#include <opencv2/opencv.hpp>
#include <string>

bool camera_init(int index = 0, int width = 320, int height = 240);
bool camera_read(cv::Mat& frame);
bool camera_show(const cv::Mat& frame);
bool camera_save(const cv::Mat& frame, const std::string& path);
void camera_release();