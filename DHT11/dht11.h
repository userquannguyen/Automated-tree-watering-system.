#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"


#define DHT_PORT_PERIPH SYSCTL_PERIPH_GPIOF
#define DHT_PORT GPIO_PORTF_BASE
#define DHT_PIN GPIO_PIN_1

#define DHT_TIM_PERIPH  SYSCTL_PERIPH_TIMER0
#define DHT_TIM_BASE    TIMER0_BASE
#define DHT_TIM         TIMER_A

#define MCU_CLOCK SysCtlClockGet()
#define DHT_WAIT_18ms ((MCU_CLOCK*18)/3000)
#define DHT_WAIT_20us ((MCU_CLOCK*2)/300000)

#define DHT_TIMEOUT ((MCU_CLOCK*9)/100000) // 90us
#define DHT_TIME_BIT_1 ((MCU_CLOCK*7)/100000) // 70us
