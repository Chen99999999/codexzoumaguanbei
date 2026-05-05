#include "camera_app.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

static cv::VideoCapture g_cap;
static int g_fbfd = -1;
static fb_var_screeninfo g_vinfo{};
static fb_fix_screeninfo g_finfo{};
static unsigned char* g_fbp = nullptr;
static long g_screensize = 0;

static inline unsigned short bgr888_to_rgb565(unsigned char b, unsigned char g, unsigned char r)
{
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

bool camera_init(int index, int width, int height)
{
    std::cout << "camera_init start" << std::endl;

    g_cap.open(index);
    if (!g_cap.isOpened())
    {
        std::cout << "camera open failed!" << std::endl;
        return false;
    }

    g_cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    g_cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    g_cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    int actual_w = (int)g_cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int actual_h = (int)g_cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    std::cout << "camera opened: " << actual_w << "x" << actual_h << std::endl;

    g_fbfd = open("/dev/fb0", O_RDWR);
    if (g_fbfd < 0)
    {
        perror("open /dev/fb0 failed");
        return false;
    }

    if (ioctl(g_fbfd, FBIOGET_FSCREENINFO, &g_finfo) < 0)
    {
        perror("FBIOGET_FSCREENINFO failed");
        return false;
    }

    if (ioctl(g_fbfd, FBIOGET_VSCREENINFO, &g_vinfo) < 0)
    {
        perror("FBIOGET_VSCREENINFO failed");
        return false;
    }

    std::cout << "fb0: " << g_vinfo.xres << "x" << g_vinfo.yres
              << " bpp=" << g_vinfo.bits_per_pixel
              << " line_length=" << g_finfo.line_length << std::endl;

    g_screensize = g_finfo.smem_len;
    g_fbp = (unsigned char*)mmap(0, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fbfd, 0);
    if (g_fbp == MAP_FAILED)
    {
        perror("mmap failed");
        g_fbp = nullptr;
        return false;
    }

    return true;
}

bool camera_read(cv::Mat& frame)
{
    if (!g_cap.isOpened())
        return false;

    return g_cap.read(frame) && !frame.empty();
}

bool camera_show(const cv::Mat& frame)
{
    if (frame.empty() || g_fbp == nullptr)
        return false;

    cv::Mat rotated;
    cv::rotate(frame, rotated, cv::ROTATE_90_CLOCKWISE);

    if (rotated.cols != (int)g_vinfo.xres || rotated.rows != (int)g_vinfo.yres)
    {
        cv::resize(rotated, rotated, cv::Size(g_vinfo.xres, g_vinfo.yres));
    }

    for (int y = 0; y < (int)g_vinfo.yres; ++y)
    {
        unsigned short* row = (unsigned short*)(g_fbp + y * g_finfo.line_length);
        const cv::Vec3b* src_row = rotated.ptr<cv::Vec3b>(y);

        for (int x = 0; x < (int)g_vinfo.xres; ++x)
        {
            const cv::Vec3b& pix = src_row[x];
            row[x] = bgr888_to_rgb565(pix[0], pix[1], pix[2]);
        }
    }

    return true;
}

bool camera_save(const cv::Mat& frame, const std::string& path)
{
    if (frame.empty())
        return false;

    return cv::imwrite(path, frame);
}

void camera_release()
{
    if (g_fbp)
    {
        munmap(g_fbp, g_screensize);
        g_fbp = nullptr;
    }

    if (g_fbfd >= 0)
    {
        close(g_fbfd);
        g_fbfd = -1;
    }

    if (g_cap.isOpened())
    {
        g_cap.release();
    }
}