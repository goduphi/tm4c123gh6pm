/*
 * Sarker Nadir Afridi Azmi
 */

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "common_terminal_interface.h"
#include "nec_ir_decoder.h"
#include "ir_emitter.h"

void initHw(void)
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    SYSCTL_GPIOHBCTL_R = 0;
}

int main()
{
    initHw();
    initIrDecoder();
    initIrEmitter();
    initUart0();

    // Endless loop
    while(true)
    {
        USER_DATA data;
        data.fieldCount = 0;
        putsUart0("Command: ");
        getsUart0(&data);
        parseField(&data);

        if(isCommand(&data, "play", 2))
        {
            int32_t arg1ptr = getFieldInteger(&data, 1);
            int32_t arg2ptr = getFieldInteger(&data, 2);

            playCommand(getInteger(&data, arg1ptr), getInteger(&data, arg2ptr));
        }
        else
            putsUart0("Invalid command.");
        putcUart0('\n');
    }
}
