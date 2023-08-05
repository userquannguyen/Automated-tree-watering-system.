#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "uartstdio.h" 


int temp;
int humidity;
uint32_t g_ui32SysClock;

void UARTInit(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    //SU DUNG UART0
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0|GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE|UART_CONFIG_PAR_NONE));
    //IntEnable(INT_UART0);
    //UARTIntEnable(UART0_BASE, UART_INT_RX|UART_INT_RT);
    //IntMasterEnable();
    UARTStdioConfig(0,115200,SysCtlClockGet());
    UARTprintf("START \n");
}

inline long WaitUntilPinState(int clockMhz, long pinbase, long pin, long timerbase, long timer, int previousstate, int usec)
{
// Debounce really not needed unless you have huge noise, but just in case (the processor is fast enough)
#define DEBOUNCEMATCHES 5
    long timermax;
    long timerval;
    int matches = 0;
    TimerDisable(timerbase, timer);
    TimerConfigure(timerbase, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_ONE_SHOT);
    TimerLoadSet(timerbase, timer, usec*clockMhz);
    timermax = TimerValueGet(timerbase, timer);
    timerval = timermax;
    TimerEnable(timerbase, timer);

        while(matches < DEBOUNCEMATCHES){ /* Wait for DHT to pull down */
        if( GPIOPinRead(pinbase, pin) > 0 != previousstate > 0) {
           matches++;
        } else {
            matches = 0;
        }
        timerval = TimerValueGet(timerbase, timer);
        if(!timerval) timerval = 1; /* make sure we never return 0 by mistake */
        if (timerval==timermax){
            return 0;
        }
    }
    return timerval;
}



//Read the DHT11 temperature and humidity sensor value//

int ReadDHT()
{
    // bit buffers & timeout
    char bitcount;
    char byte;
    char bits[5] = {0,0,0,0,0};
    unsigned int bitints[6];
    long timerval;
    int clockMhz;

#define MY_PIN_PERIPH       SYSCTL_PERIPH_GPIOF
#define MY_PIN_PORTBASE     GPIO_PORTF_BASE
#define MY_PINNR            GPIO_PIN_1
#define MY_TIMER_PERIPH     SYSCTL_PERIPH_TIMER0
#define MY_TIMERBASE        TIMER0_BASE
#define MY_TIMER            TIMER_B

    clockMhz = SysCtlClockGet() / 1000000;

    /* Configure hardware counter to check how long we are looping */
    SysCtlPeripheralEnable(MY_TIMER_PERIPH);
    TimerConfigure(MY_TIMERBASE, TIMER_CFG_PERIODIC);


    // request reading
    SysCtlPeripheralEnable(MY_PIN_PERIPH);

    GPIOPinTypeGPIOOutput(MY_PIN_PORTBASE, MY_PINNR);
    GPIOPinWrite(MY_PIN_PORTBASE, MY_PINNR, 0x0);

    // #loops = loops_per_us*time_in_us = time in us * loops/us = time_in_us *(clockfreq/10000000)
    SysCtlDelay(10000*clockMhz); // -> 30 ms
    /////// SET PIN HIGH (putting it to input will do)
    GPIOPinTypeGPIOInput(MY_PIN_PORTBASE, MY_PINNR);

    SysCtlDelay(10*clockMhz);

    /* Wait for DHT pull down */
    if(!WaitUntilPinState(clockMhz,MY_PIN_PORTBASE,MY_PINNR,MY_TIMERBASE,MY_TIMER,1,50)) return 0;

    /* Wait for dummy up */
    if(!WaitUntilPinState(clockMhz,MY_PIN_PORTBASE,MY_PINNR,MY_TIMERBASE,MY_TIMER,0,90)) return 0;

    /* Wait for dummy down */
    if(!WaitUntilPinState(clockMhz,MY_PIN_PORTBASE,MY_PINNR,MY_TIMERBASE,MY_TIMER,1,90)) return 0;

    // start receiving 40 bits
    char i;
    bitcount = 7;
    byte = 0;
    for (i=0; i < 40; i++)
    {
        /* Wait for pull up */
        if(!WaitUntilPinState(clockMhz,MY_PIN_PORTBASE,MY_PINNR,MY_TIMERBASE,MY_TIMER,0,60)) return 0;

        /* Wait for pull down and count */
        timerval = WaitUntilPinState(clockMhz,MY_PIN_PORTBASE,MY_PINNR,MY_TIMERBASE,MY_TIMER,1,90);

        if(!timerval) return 0;

        if (timerval < 50*clockMhz) bits[byte] |= (1 << bitcount);
        if (bitcount == 0)
        {
            bitcount = 7;
            byte++;
        }else{
            bitcount--;
        }
    }

    // checksum

    /* Little Endian */
    bitints[0] = ((unsigned int) bits[0]  & (0x000000FF));
    bitints[1] = ((unsigned int) bits[1]  & (0x000000FF));
    bitints[2] = ((unsigned int) bits[2]  & (0x000000FF));
    bitints[3] = ((unsigned int) bits[3]  & (0x000000FF));
    bitints[4] = ((unsigned int) bits[4]  & (0x000000FF));
    if(((bitints[0] + bitints[1] + bitints[2] + bitints[3]) & (0x000000FF)) == bitints[4]){
        temp = bitints[2];
        humidity =  bitints[0];
        return 1;
    }else{
        return 0;
    }

}



int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);

    UARTInit();

    UARTprintf("Its runing at : %d\n",g_ui32SysClock);


    while(1)
    {
        //
        // Turn on D1. LED 1
        //
        SysCtlDelay(SysCtlClockGet()/3);
        if(ReadDHT() == 0)
            {
             UARTprintf("Erro!\n");
            }
        else
        {
            UARTprintf("Temp %d !\n",temp);
            UARTprintf("Hum %d !\n",humidity);

        }

    }
}

