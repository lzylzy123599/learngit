#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#define argb8888_to_rgb565(color)   ({ \
            unsigned int temp = (color); \
            ((temp & 0xF80000UL) >> 8) | \
            ((temp & 0xFC00UL) >> 5) | \
            ((temp & 0xF8UL) >> 3); \
            })

static int width;                   //LCD X分辨率
static int height;                      //LCD Y分辨率
static unsigned short *screen_base = NULL;      //映射后的显存基地址

static void lcd_fill(unsigned int start_x, unsigned int end_x,
            unsigned int start_y, unsigned int end_y,
            unsigned int color)
{
    unsigned short rgb565_color = argb8888_to_rgb565(color);//得到RGB565颜色值
    unsigned long temp;
    unsigned int x;

    /* 对传入参数的校验 */
    if (end_x >= width)
        end_x = width - 1;
    if (end_y >= height)
        end_y = height - 1;

    /* 填充颜色 */
    temp = start_y * width; //定位到起点行首
    for ( ; start_y <= end_y; start_y++, temp+=width) {

        for (x = start_x; x <= end_x; x++)
            screen_base[temp + x] = rgb565_color;
    }
}

static void lcd_draw_line(unsigned int x, unsigned int y, int dir,
            unsigned int length, unsigned int color)
{
    unsigned short rgb565_color = argb8888_to_rgb565(color);//得到RGB565颜色值
    unsigned int end;
    unsigned long temp;

    /* 对传入参数的校验 */
    if (x >= width)
        x = width - 1;
    if (y >= height)
        y = height - 1;

    /* 填充颜色 */
    temp = y * width + x;//定位到起点
    if (dir) {  //水平线
        end = x + length - 1;
        if (end >= width)
            end = width - 1;

        for ( ; x <= end; x++, temp++)
            screen_base[temp] = rgb565_color;
    }
    else {  //垂直线
        end = y + length - 1;
        if (end >= height)
            end = height - 1;

        for ( ; y <= end; y++, temp += width)
            screen_base[temp] = rgb565_color;
    }
}

int main(int argc, char *argv[])
{
    struct fb_fix_screeninfo fb_fix;
    struct fb_var_screeninfo fb_var;
    unsigned int screen_size;
    int fd;

    /* 打开framebuffer设备 */
    if (0 > (fd = open("/dev/fb0", O_RDWR))) {
        perror("open error");
        exit(EXIT_FAILURE);
    }

    /* 获取参数信息 */
    ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
    ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix);

    screen_size = fb_fix.line_length * fb_var.yres;
    width = fb_var.xres;
    height = fb_var.yres;

    /* 将显示缓冲区映射到进程地址空间 */
    screen_base = mmap(NULL, screen_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == (void *)screen_base) {
        perror("mmap error");
        close(fd);
        exit(EXIT_FAILURE);
    }
    lcd_fill(0, width, 0, height, 0xFFCFDA); //红色方块
    /* 画线: 十字交叉线 */
    lcd_draw_line(0, height * 0.5, 1, width, 0xFFFFFF);//白色线
    lcd_draw_line(width * 0.5, 0, 0, height, 0xFFFFFF);//白色线

    /* 退出 */
    munmap(screen_base, screen_size);  //取消映射
    close(fd);  //关闭文件
    exit(EXIT_SUCCESS);    //退出进程
}