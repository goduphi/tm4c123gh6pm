// Wait functions
// Jason Losh - waitMicrosecond
// Sarker Nadir Afridi Azmi - wait10Seconds

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef WAIT_H_
#define WAIT_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void waitMicrosecond(uint32_t us);
void wait10Seconds();

#endif
