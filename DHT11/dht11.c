#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "uartstdio.h"
#include "dht11.h"

volatile int temp;
volatile int humid;


int readDHT()
{
    char buffer[5] = {0,0,0,0,0};
    unsigned int bufferint[5];
//cau hinh 
    SysCtlPeripheralEnable(DHT_PERIPH);
//Yeu cau giao tiep voi DHT11
    GPIOPinTypeGPIOOutput(DHT_PORTBASE,DHT_PIN);
    //keo xuong 30ms, sau do keo len 40us
    GPIOPinWrite(DHT_PORTBASE, DHT_PIN, 0x00);
    SysCtlDelay(wait_30ms); //30ms
    GPIOPinWrite(DHT_PORTBASE, DHT_PIN, 0x02);
    SysCtlDelay(wait_40us);
    //bat dau doc du lieu
    GPIOPinTypeGPIOInput(DHT_PORTBASE, DHT_PIN);
    //truoc do pin van con muc cao (xem datasheet)
    while(GPIOPinRead(DHT_PORTBASE, DHT_PIN) == DHT_PIN );
    //delay 80us low
    while(GPIOPinRead(DHT_PORTBASE, DHT_PIN) == DHT_PIN );
    //delay 80us high
    while(GPIOPinRead(DHT_PORTBASE, DHT_PIN) == DHT_PIN );
    //start receive 40 buffer
    int bytecounter = 0;
    int bitcounter = 0;
    int i;
    for(i = 0; i < 40; ++i)
    {
        if(GPIOPinRead(DHT_PORTBASE, DHT_PIN) == DHT_PIN)
        {
            SysCtlDelay(wait_bit_1);
            if(GPIOPinRead(DHT_PORTBASE, DHT_PIN) == DHT_PIN) 
            {
                buffer[bytecounter] |= (1 << bitcounter);
            }
            if(bitcounter == 0)
            {
                bitcounter = 7;
                bytecounter++;
            }
            else 
                bitcounter--;
        }
    }
    //kiem tra checksum
    bufferint[0] = ((unsigned int) buffer[0]  & (0x000000FF));
    bufferint[1] = ((unsigned int) buffer[1]  & (0x000000FF));
    bufferint[2] = ((unsigned int) buffer[2]  & (0x000000FF));
    bufferint[3] = ((unsigned int) buffer[3]  & (0x000000FF));
    bufferint[4] = ((unsigned int) buffer[4]  & (0x000000FF));
    if(((bufferint[0] + bufferint[1] + bufferint[2] + bufferint[3]) & (0x000000FF)) == bufferint[4]){
        temp =   bufferint[2];
        humid =  bufferint[0];
        return 1;
    }else{
        return 0;
    }

}
