/**
 * Elias Zarate
 * Project - Lockbox
 *
 * main.c
 *
 * The goal of this project is to implement a lock box state machine
 * using the previous de-bounce and keypad state machines. The keypad
 * will first display '____'. Users can enter their 4-digit number and
 * press the A button to submit the password. The keypad will display
 * the characters 'LOC_' until the user enters the correct password again.
 * After 3 tries of unsuccessful attempts, the user will remain locked out
 * for over 3 minutes.
 */

#include "msp.h"


void display(void);
void lock_box_fsm(void);

void keypad_fsm(void);
void verify_passcode(void);
int keypad(int digit);
void process_input(int selector);
void delay(void);

// Global Variables
int N, count, cycle, i, x;
int lockcycle, opencycle;
int num_tries = 0;
int key = -1;
int value= -1;
int search = -1;
int verification = -1;

// Initialize states
int state = 0;          // keyboard_fsm
int currentState = 0;   // lockBox_fsm

int digit_array[] = {0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,
                     0x00,0x18,0x08,0x03,0x46,0x21,0xF7,0xF7}; // LED hex value (0-9,A-F)
int waiting_display[] = {0xF7,0xF7,0xF7,0xF7};
int loc_display[] = {0xF7,0x47,0x40,0x46};
int ld_display[] = {0xF7,0xF7,0x47,0x21};       // {15,14,0,12}; '_, _, L, d'
int InNum[4] = {};                              // holds SECRET_PASSWORD
int sequence_array[4] = {0x40,0x40,0x40,0x40};  // Array to be updated and displayed
int hold_tmp[4] = {};                           // User attempt to unlock box
int pin_out[] = {0x1C,0x2C,0x34,0x38};          // {8.5,8.4,8.3,8.2}

int table[4][4] = {{1,2,3,10},{4,5,6,11},
                   {7,8,9,12},{14,0,15,13}};    // Table for mapping input values (10 and 11 are open/lock buttons)

void main(void){

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;           // stop watchdog timer

    P8->DIR |= BIT5| BIT4| BIT3| BIT2;                    // OUTPUT DIRECTION
    P4->DIR |= BIT0| BIT1| BIT2| BIT3 |BIT4| BIT5| BIT6;  // OUTPUT DIRECTION
    P9->DIR &= ~(BIT3| BIT2| BIT1| BIT0);                 // INPUT DIRECTION

    P9->OUT |= BIT0| BIT1| BIT2| BIT3;                    // OUTPUT HIGH

    while(1){
        // Display what is in sequence array
        display();
        lock_box_fsm();
    } // Loop forever
}

void display(void){
    // For every i iterator we need to check the input of the bit
    int i;
    for(i = 0; i < 4; i++){
        P8->OUT = pin_out[i];       // Go through turning LEDS one at a time
        if (search == 1){
            keypad(i);
            key = keypad(i);        // Return key for next state
        }
        P4->OUT = sequence_array[i];
        delay();
    }
}

void lock_box_fsm(void){

    // State machine implementation for lock box
    switch(currentState)
    {
         case 0: // Normal State
             for (x = 0; x < 4 ; x++){
                 sequence_array[x] = 0x40;                   // Resets sequence array, display '0000'
             }
             search = 1;                                     // Begin searching for user input

             if(value == 10){                                // User press 'A'
                 currentState = 1;
                 for (x = 0; x < 4 ; x++){
                     sequence_array[x] = waiting_display[x]; // Display '_ _ _ _'
                 }
             }else{currentState = 0;}
             value = 0;

             break;

         case 1: // Pre-Lock
             do{
                 P1DIR = 0x01;                               // Turn LED on indicating that we are ready for an input
                 keypad_fsm();                               // Take keypad input
                 int i;
                 for(i = 0; i < 100; i++){
                     cycle++;                                // Give user a couple of seconds to input their password
                 }
             }while(cycle < 100000);
             for (i = 0; i < 4; i++){
                 InNum[i] = sequence_array[i];               // Store users password in InNum[] array
             }
             currentState = 2;
             P1DIR = ~(0x01);                                // Turn off LED to indicate no more input
             break;

         case 2: // Locked
             P5DIR |= 0x01;                                  // Turn on LED to indicate locked state
             for(i = 0; i < 4; i++){
                 sequence_array[i] = loc_display[i];
             }
             currentState = 3;
             break;

         case 3: // Take user input
             search = 1;
             keypad_fsm();                                   // Take user input to unlock the key
             if(value == 11){                                // User presses 'B' to submit input
                 for(i = 0; i < 4; i++){
                     hold_tmp[i] = sequence_array[i];        // Hold whatever was displayed
                 }
                 currentState = 4;
                 value = -1;
             }
             break;

          case 4: // Verify the password to turn on selenoid
              verify_passcode();
              if (verification == 1){
                  do{
                      for (x = 0; x < 4 ; x++){
                          sequence_array[x] = InNum[x];
                      }
                      display();
                      P5DIR = ~(0x01);
                      P1DIR ^= 0x01;                         // Toggle pin 1.0
                      for(i = 0; i < 100; i++){
                          opencycle++;
                      }
                  }while(opencycle < 100000);
              }else{
                  num_tries++;
              }
              opencycle = 0;
              currentState = 2;                              // Return to lock state after time period is over
              verification = -1;
              if (num_tries >= 31){
                  do{
                      P5DIR |= 0x01;
                      display();
                      keypad_fsm();                          // Take keypad input
                      for (x = 0; x < 4 ; x++){
                          sequence_array[x] = ld_display[x];
                      }
                      int i;
                      for(i = 0; i < 100; i++){
                          lockcycle++;
                      }
                  }while(lockcycle < 100000);
                  num_tries = 0;
                  currentState = 2;
                  lockcycle = 0;
                  break;

              }
         }
}

int keypad(int digit)
{
// Pass through the row of P8.x selection
    int row = digit;
    int column;

    if(P9->IN & BIT3) {
        column = 0;
        count++;                    // Increase after every iteration
        value = table[row][column]; // Obtain value from table
    }
    else if(P9->IN & BIT2) {
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

void keypad_fsm(void){

// State machine implementation for keypad
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
             count = 0;
         }
         else if(count < 100){
             state = 0;                // Still waiting
         }
         break;
     case 2: // Process input
         process_input(value);         // Update sequence_array[]
         state = 3;                    // Go to state 3 regardless
         break;
     case 3: // key_release_debounce
         count = 0;
         state = 0;
         break;
     }
}

void verify_passcode(void){
// Verify user input to open lock
    int i;
    for(i = 0; i < 4; i++){
        if(InNum[i] != hold_tmp[i]){
        currentState = 3;
        break;
        }
        else{
            verification = 1;
        }
    }
}

void process_input(int selector)
{
// pass through the row of P8.x selection
    int selection = digit_array[selector];

    int i;
    for(i = 3; i > 0; i--){
        sequence_array[i] = sequence_array[i-1];    // Shift array to the right
    }
    sequence_array[0] = selection;                  // Add the selection to the first element of array
}

void delay(void){
    int i;
    for(i=600; i>0; i--){
     // A busy wait
    }
}

/**
 * EOF
 */

