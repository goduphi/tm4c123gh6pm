// Infrared Signal Decoder Library
// Sarker Nadir Afridi Azmi
// Contains functions that help in emit a NEC transmission protocol IR signal

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40MHz

// Hardware configuration:
// PWM module:
// PMW Module 0, Generator 0, Output 0 (M0PWM0) (PB6)
// Timer module:
// Timer 2, Periodic mode, Interrupt enabled

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "nec_ir_decoder.h"

uint8_t sampleVals[] = {0, 0, 0, 1, 1,
                        0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1};

uint32_t sampleWaitTimes[4] = {154000, 60000, 71250, 22720};

uint8_t index = 0;
bool isInvalidBit = false;

int8_t zeroOneCounter = 0;
uint8_t dataBitCounter = 0;
uint8_t dataByte = 0;
bool complement = false;
uint8_t complementDataByte = 0;

uint8_t isEndBit = 1;

// Initialize Hardware
void initIrDecoder(void)
{
    // Enable clocks for the timer
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    // Enable clocks for Port B
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1 | SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);

#ifdef DEBUG
    GPIO_PORTF_DIR_R |= 4 | 8 | 2;
    GPIO_PORTF_DR2R_R |= 4 | 8 | 2;
    GPIO_PORTF_DEN_R |= 4 | 8 | 2;
#endif

    // Make PB0 an input and PB2 an output
    GPIO_PORTB_DIR_R |= GPO_LOGIC_ANALYZER_MASK;
    GPIO_PORTB_DIR_R &= ~GPI_IR_SENSOR_MASK;
    GPIO_PORTB_DR2R_R |= GPO_LOGIC_ANALYZER_MASK;
    GPIO_PORTB_DEN_R |= GPI_IR_SENSOR_MASK | GPO_LOGIC_ANALYZER_MASK;

    GPIO_PORTB_DATA_R &= ~GPO_LOGIC_ANALYZER_MASK;

    // Configure the interrupt for the GPIO pin
    GPIO_PORTB_IM_R &= ~GPI_IR_SENSOR_MASK;            // Turn off interrupts for PB0
    GPIO_PORTB_IS_R &= ~GPI_IR_SENSOR_MASK;            // Configure the pin to detect edges
    GPIO_PORTB_IBE_R &= ~GPI_IR_SENSOR_MASK;           // Let the Interrupt Event register interrupt generation
    GPIO_PORTB_ICR_R |= GPI_IR_SENSOR_MASK;            // Clear the interrupt as per the documentation
    GPIO_PORTB_IEV_R &= ~GPI_IR_SENSOR_MASK;           // Detect falling edges
    NVIC_EN0_R |= (1 << (INT_GPIOB - 16));              // Enable the master interrupt control

    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                    // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;              // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;             // configure for periodic mode (count down)
    NVIC_EN0_R |= (1 << (INT_TIMER1A - 16));            // Enable the master interrupt control

    GPIO_PORTB_IM_R |= GPI_IR_SENSOR_MASK;             // Enable the interrupt
}

void edgeDetectIsr(void)
{
    GPIO_PORTB_ICR_R |= GPI_IR_SENSOR_MASK;
    GPIO_PORTB_IM_R &= ~GPI_IR_SENSOR_MASK;
    TIMER1_TAILR_R = 90000;
    TIMER1_IMR_R |= TIMER_IMR_TATOIM;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;

    // Reset everything for the next button input
    index = 0;
    isInvalidBit = false;

    zeroOneCounter = 0;
    dataBitCounter = 0;
    dataByte = 0;
    complement = false;
    complementDataByte = 0;

    isEndBit = 1;
}

// This function takes an unsigned 8 bit number, and prints it to a terminal
// This is meant for debugging
void printUB10Number(uint8_t x)
{
    int lastDigit = x % 10;
    x /= 10;
    int secondDigit = x % 10;
    x /= 10;
    int firstDigit = x % 10;
    putcUart0('0' + firstDigit);
    putcUart0('0' + secondDigit);
    putcUart0('0' + lastDigit);
}

void log(const char* message, uint8_t code)
{
    putcUart0('\n');
    putsUart0(message);
    printUB10Number(code);
    putcUart0('\n');
}

void irSignalSamplingIsr(void)
{
    if((index == 103 && !isEndBit) || isInvalidBit)
    {
#ifdef DEBUG
        log("Button: ", dataByte);
        log("Complement: ", complementDataByte);
        log("Index: ", index);
        // Do stuff with the button code
        if(!(dataByte & complementDataByte))
        {
            switch(dataByte)
            {
            case 48:
                GPIO_PORTF_DATA_R ^= 4;
                break;
            case 24:
                GPIO_PORTF_DATA_R ^= 2;
                break;
            case 122:
                GPIO_PORTF_DATA_R ^= 8;
                break;
            }
        }
#endif

        // clear interrupt flag
        TIMER1_ICR_R = TIMER_ICR_TATOCINT;
        // Turn off Timer 1
        TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
        // Turn off interrupts
        TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;
        // Turn on GPI interrupt
        GPIO_PORTB_IM_R |= GPI_IR_SENSOR_MASK;
        return;
    }

#ifdef DEBUG
    putcUart0('0' + IR_INPUT);
    putcUart0(' ');
    GPO_LOGIC_ANALYZER ^= 1;
#endif
    if(index <= 52 && IR_INPUT^sampleVals[index])
    {
        isInvalidBit = true;
    }

    if(index >= 2 && index <= 5)
    {
        TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
        TIMER1_TAV_R = 0;
        TIMER1_TAILR_R = sampleWaitTimes[index - 2];
        TIMER1_CTL_R |= TIMER_CTL_TAEN;
    }
    // Sample the data
    if(index >= 53 && index <= 102)
    {
        if(!(IR_INPUT | 0))
            zeroOneCounter++;
        else if(IR_INPUT & 1)
            zeroOneCounter--;

        if(zeroOneCounter == 0)
        {
            if(index > 100)
                isEndBit = 0;
            else if(complement)
                complementDataByte <<= 1;
            else
                dataByte <<= 1;
            dataBitCounter++;
        }
        else if(zeroOneCounter == -2)
        {
            zeroOneCounter = 0;
            if(complement)
                complementDataByte |= 1;
            else
                dataByte |= 1;
        }

        if(dataBitCounter == 8)
        {
            complement = true;
        }
    }
    index++;
    // clear interrupt flag
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;
}
