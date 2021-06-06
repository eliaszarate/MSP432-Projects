/*
 * Elias Zarate
 *
 * Project: Keypad Input
 *
 * A state machine is designed to accept keypad inputs from
 * with hardware debouncing. The input values are then
 * show cased on the four-digit LED display.
 *
 */

#include "msp.h"
#include <stdio.h>

// Function prototypes
void display(void);
int keypad(int digit);
void process_input(int selector);
void de_bounce(void);
void delay(void);

// Global variables
int count;
int key = -1;
int value= 10;
int state;

int digit_array[] = {0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,
                     0x00,0x18,0x08,0x03,0x46,0x21,0x06,0x0E};      // LED hex value (0-9,A-F)
int sequence_array[4] = {0x40,0x40,0x40,0x40};                      // Array to be updated and displayed
int pin_out[] = {0x1C,0x2C,0x34,0x38};                              // {8.5,8.4,8.3,8.2}

int table[4][4] = {{1,2,3,10},{4,5,6,11},
                   {7,8,9,12},{14,0,15,13}};                         // Table for mapping input values


void main(void){

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;             // stop watchdog timer

    P8->DIR |= BIT5| BIT4| BIT3| BIT2;                      // OUTPUT DIRECTION
    P4->DIR |= BIT0| BIT1| BIT2| BIT3 |BIT4| BIT5| BIT6;    // OUTPUT DIRECTION
    P9->DIR &= ~(BIT3| BIT2| BIT1| BIT0);                   // INPUT DIRECTION

    P9->OUT |= BIT0| BIT1| BIT2| BIT3;                      // OUTPUT HIGH

    state = 0;      // Initialize state

    while(1){
        display();
        de_bounce();
    } // Loop forever
}

void de_bounce(void){

    switch(state)
     {
     case 0: // Idle
         if (key != -1){
             state  = 1;
         }
         break;
     case 1: // Key-press debounce
         if(count > 100){
             state = 2;
         }
         else if(count < 100){
             state = 0;             // still waiting
         }
         break;
     case 2: // Process input
         process_input(value);      // update sequence_array[]
         state = 3;                 // Go to state 3 regardless
         break;
     case 3: // key_release_debounce
         count = 0;
         state = 0;
         break;
     }
}

int keypad(int digit)
{
    // pass through the row of P8.x selection
    int row = digit;
    int column;

    if(P9->IN & BIT3) {
        column = 0;
        count++;                      // Get after every iteration
        value = table[row][column];   // Obtain value from table
    }
    else if(P9->IN & BIT2) { // press 2
        column = 1;
        count++;
        value = table[row][column];
    }
    else if(P9->IN & BIT1){
        column = 2;
        count++;
        value = table[row][column];
    }
    else if(P9->IN & BIT0){
        column = 3;
        count++;
        value = table[row][column];
    }
    return value;
}

void display(void){
    // For every i iterator we need to check the input of the bit
    int i;
    for(i = 0; i < 4; i++){
        P8->OUT = pin_out[i];           // Go through turning LEDS one at a time
        keypad(i);
        key = keypad(i);                // Return key for next state
        P4->OUT = sequence_array[i];
        delay();
    }
}

// Function to update global sequence_array
void process_input(int selector)
{
    int selection = digit_array[selector];        // pass through the row of P8.x selection

    int i;
    for(i = 3; i > 0; i--){
        sequence_array[i] = sequence_array[i-1];  // shift array to the right
    }
    sequence_array[0] = selection;                // add the selection to the first element of array
}

void delay(void){
    int i;
    for(i=400; i>0; i--){
     // A busy wait
    }
}

/*
 * EOF
 */
