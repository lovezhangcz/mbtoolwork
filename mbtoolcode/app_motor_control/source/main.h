#ifndef __MAIN_H
#define __MAIN_H



extern u8 functionturnflg;


//整个系统使用的是4bit主优先级，子优先级是0bit

#define TIMER1BK_IRQ_PRIORITY    ( (uint32_t)0x00 )   //定时器1中断,还有OCP中断，所以优先级得最高，对应PWM频率
#define TIMER1_IRQ_PRIORITY    ( (uint32_t)0x00 )
#define ADC_IRQ_PRIORITY       ( (uint32_t)0x01 )   //ADC中断，对应PWM频率

#define USART_IRQ_PRIORITY       ( (uint32_t)0x02 )    //串口优先级，先不分串口1、2，也不分RX、TX


#define TAST_IRQ_PRIORITY   ( (uint32_t)0x03 )   //定时器中断，用于1ms的控制线程
#define SysTick_IRQ_PRIORITY   ( (uint32_t)0x04 )   //系统时钟中断，用于detect线程




#endif
