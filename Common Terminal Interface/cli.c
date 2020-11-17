// Serial Example
// Sarker Nadir Afridi Azmi

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "common_terminal_interface.h"

// Bitband aliases
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

// PortF masks
#define GREEN_LED_MASK 8
#define RED_LED_MASK 2

#define DEBUG

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
	// Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable clocks
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

    // Configure LED pins
    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK;  // bits 1 and 3 are outputs
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | RED_LED_MASK;  // enable LEDs
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
	// Initialize hardware
	initHw();
	initUart0();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

	USER_DATA info;
	info.fieldCount = 0;

	// Get the string from the user
	getsUart0(&info);

	// Echo back what I have written to myself using the TTY interface
	// Only for testing purposes
#ifdef DEBUG
	putsUart0(info.buffer);
	putcUart0('\n');
#endif

	// Parse the fields
	parseField(&info);
	// Echo back the parsed field information (type and field)
#ifdef DEBUG
	uint8_t i;
	for(i = 0; i < info.fieldCount; i++)
	{
	    putcUart0(info.fieldType[i]);
	    putcUart0('\t');
	    putsUart0(&info.buffer[info.fieldPosition[i]]);
	    putcUart0('\n');
	}
	putcUart0(info.fieldCount);
#endif

	// Command Evaluation
	bool valid = false;
	// set add, data
	// add and data are integers
	if(isCommand(&info, "set", 2))
	{
	    int32_t add = getFieldInteger(&info, 1);
	    int32_t data = getFieldInteger(&info, 2);
	    valid = true;
	    // TO DO: Configure something
#ifdef DEBUG
	    putsUart0("Requested integers: ");
	    // Convert both the integers into printable ascii characters
	    putcUart0((char)(add + 48));
	    putcUart0('\t');
	    putcUart0((char)(data + 48));
	    putcUart0('\n');
#endif
	}

	// alert ON | OFF
	// alert ON or OFF are the expected commands
	if(isCommand(&info, "alert", 1))
	{
	    char* str = getFieldString(&info, 1);
#ifdef DEBUG
	    putsUart0("String argument: ");
	    putsUart0(str);
	    putcUart0('\n');
#endif
	    if(stringCompare(str, "ON"))
	        RED_LED = 1;
	    else if(stringCompare(str, "OFF"))
	        GREEN_LED = 1;
	    valid = true;
	}

	// Process other commands here

	// Look for error
	if(!valid)
	    putsUart0("Invalid command\n");

	// Endless loop
	while(true);
}
