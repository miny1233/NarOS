#include <type.h>
#include <device/tty.h>
#include <device/keyboard.h>
#include <nar/printk.h>
#include <nar/debug.h>
#include <nar/interrupt.h>
#include <nar/task.h>
#include <nar/pipe.h>
#include <nar/panic.h>
#include <nar/mem.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>

void child()
{

}

int init()
{
    tty_init();  //最早初始化 printk依赖
    interrupt_init();
    memory_init();
    task_init();  //任务调度程序
    pipe_init();

    // 外围设备
    interrupt_hardler_register(0x21,keyboard_handler);
    set_interrupt_mask(1,1); //启动键盘中断

    //task_create(child); //创建任务

    return 0; //初始化完毕，初始化程序变idle程序
}


void LoadVideo()
{
    float A = 0, B = 0;
    float i, j;
    int k;
    float z[1761];
    char b[1761];
    // printf("\x1b[2J");
    int out_count = 0;
    for(;;) {
        out_count++;
        memset(b,32,1760);
        memset(z,0,7040);
        for(j=0; j < 6.28; j += 0.07) {
            for(i=0; i < 6.28; i += 0.02) {
                float c = sin(i);
                float d = cos(j);
                float e = sin(A);
                float f = sin(j);
                float g = cos(A);
                float h = d + 2;
                float D = 1 / (c * h * e + f * g + 5);
                float l = cos(i);
                float m = cos(B);
                float n = sin(B);
                float t = c * h * g - f * e;
                int x = 40 + 30 * D * (l * h * m - t * n);
                int y= 12 + 15 * D * (l * h * n + t * m);
                int o = x + 80 * y;
                int N = 8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n);
                if(22 > y && y > 0 && x > 0 && 80 > x && D > z[o]) {
                    z[o] = D;
                    b[o] = ".,-~:;=|*#&@"[N > 0 ? N : 0];
                }
            }
        }
        tty_clear();
        // 计算完成开始输出显示
        for(k = 0; k < 1761; k++) {
            printk("%c",k % 80 ? b[k] : 10);
            A += 0.00004f;
            B += 0.00002f;
        }
        static int dot_num = 1;
        printk("\t\t\t  System is Loading Now ");
        if(!(out_count%=5))dot_num = ++dot_num > 3 ? 1 : dot_num;
        for(int dot=0;dot<dot_num;dot++)printk(".");
    }
}

