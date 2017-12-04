/*
************************************************************************
 ECE 362 - Mini-Project C Source File - Spring 2017
***********************************************************************

 Team ID: < ? >

 Project Name: < ? >

 Team Members:

   - Team/Doc Leader: < ? >      Signature: ______________________

   - Software Leader: < ? >      Signature: ______________________

   - Interface Leader: < ? >     Signature: ______________________

   - Peripheral Leader: < ? >    Signature: ______________________


 Academic Honesty Statement:  In signing above, we hereby certify that we
 are the individuals who created this HC(S)12 source file and that we have
 not copied the work of any other student (past or present) while completing
 it. We understand that if we fail to honor this agreement, we will receive
 a grade of ZERO and be subject to possible disciplinary action.

***********************************************************************

 The objective of this Mini-Project is to .... < ? >


***********************************************************************

 List of project-specific success criteria (functionality that will be
 demonstrated):

 1.

 2.

 3.

 4.

 5.

***********************************************************************

  Date code started: < 12/2/17 >

  Update history (add an entry every time a significant change is made):

  Date: < ? >  Name: < ? >   Update: < ? >

  Date: < ? >  Name: < ? >   Update: < ? >

  Date: < ? >  Name: < ? >   Update: < ? >


***********************************************************************
*/

#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include <mc9s12c32.h>

/* All functions after main should be initialized here */
char inchar(void);
void outchar(char x);
void shiftout(char x);
void lcdwait(void);
void send_byte(char x);
void send_i(char x);
void chgline(char x);
void print_c(char x);
void pmsglcd(char[]);


/* Variable declarations */
int leftpb = 0;    // left pushbutton flag
int rghtpb = 0;    // right pushbutton flag
int runstp = 0;    // run/stop flag��
int prevlpb = 1; //prev state of left pb
int prevrpb = 1; //prev state of right pb
char n = 0;      //temp char to shift numbers into lcd
int pwmout = 0;
int bar = 0;
int m = 0;
int avgout = 0;
unsigned int out;
unsigned int out1;

/* Special ASCII characters */
#define CR 0x0D		// ASCII return�
#define LF 0x0A		// ASCII new line�

/* LCD COMMUNICATION BIT MASKS (note - different than previous labs) */
#define RS 0x10		// RS pin mask (PTT[4])
#define RW 0x20		// R/W pin mask (PTT[5])
#define LCDCLK 0x40	// LCD EN/CLK pin mask (PTT[6])

/* LCD INSTRUCTION CHARACTERS */
#define LCDON 0x0F	// LCD initialization command
#define LCDCLR 0x01	// LCD clear display command
#define TWOLINE 0x38	// LCD 2-line enable command
#define CURMOV 0xFE	// LCD cursor move instruction
#define LINE1 0x80	// LCD line 1 cursor position
#define LINE2 0xC0	// LCD line 2 cursor position


/*
***********************************************************************
 Initializations
***********************************************************************
*/

void  initializations(void) {

/* Set the PLL speed (bus clock = 24 MHz) */
  CLKSEL = CLKSEL & 0x80; //; disengage PLL from system
  PLLCTL = PLLCTL | 0x40; //; turn on PLL
  SYNR = 0x02;            //; set PLL multiplier
  REFDV = 0;              //; set PLL divider
  while (!(CRGFLG & 0x08)){  }
  CLKSEL = CLKSEL | 0x80; //; engage PLL

/* Disable watchdog timer (COPCTL register) */
  COPCTL = 0x40   ; //COP off; RTI and COP stopped in BDM-mode

/* Initialize asynchronous serial port (SCI) for 9600 baud, interrupts off initially */
  SCIBDH =  0x00; //set baud rate to 9600
  SCIBDL =  0x9C; //24,000,000 / 16 / 156 = 9600 (approx)
  SCICR1 =  0x00; //$9C = 156
  SCICR2 =  0x0C; //initialize SCI for program-driven operation
  DDRB   =  0x10; //set PB4 for output mode
  PORTB  =  0x10; //assert DTR pin on COM port

/* Initialize peripherals */
    DDRAD = 0; 		//program port AD for input mode
    ATDDIEN = 0xC0; //program PAD7 and PAD6 pins as digital inputs

    DDRT = 0b00011011; //outputs = PTT 0,1,3,4

    ATDCTL2 = 0x80;
    ATDCTL3 = 0x00;
    ATDCTL4 = 0b10000101;  //8bit res, 16 ATD clock periods, prescaler = 12


    SPICR1 = 0b01010000;
    SPICR2 = 0b00000000;

/* Initialize SPI for baud rate of 6 Mbs */
   SPIBR = 0b00010000; //(0+1)*2^(1+1) = 4 = baud rate divisor

/* Initialize digital I/O port pins */
   DDRT = 0b11111111; //outputs = PTT 2,3,4
   DDRM = 0b11111111; //outputs = PTM 4,5



/* Initialize the LCD
     - pull LCDCLK high (idle)
     - pull R/W' low (write state)
     - turn on LCD (LCDON instruction)
     - enable two-line mode (TWOLINE instruction)
     - clear LCD (LCDCLR instruction)
     - wait for 2ms so that the LCD can wake up
*/
  PTT_PTT4 = 1;   //pull LCDCLK high (connected to PT4)
  PTT_PTT3 = 0;   //pull R/W' low (connected to PT3)
  send_i(LCDON); //turn on LCD
  send_i(TWOLINE); //enable 2-line mode
  send_i(LCDCLR); //clear LCD
  lcdwait();  //wait
 // pmsglcd("test");

/* Initialize interrupts */

	 RTICTL = 0x31;
   CRGINT = 0x80;

   DDRP = 0xFF;
   MODRR = 0x03;
   PWME = 0x03;
   PWMPOL = 0x01;
   PWMCTL = 0x00;
   PWMCAE = 0x00;
   PWMPER0 = 0xFF;
   PWMDTY0 = 0x00;
   PWMPER1 = 0xFF;
   PWMDTY1 = 0x90;
   PWMCLK = 0x02;
   PWMPRCLK = 0x06;
   PWMSCLA = 4;

  /*Initialize TIM here */
  TSCR1 = 0x80;
    TSCR2 = 0x0C;
  TIOS = 0x80;
  TIE = 0x80;
  TC7 = 150;
}




/*
***********************************************************************
Main
***********************************************************************
*/
void main(void) {
  	DisableInterrupts
	initializations();
	EnableInterrupts;

 for(;;) {

/* < start of your main loop > */
  int delayct = 0;
  int i = 0;

   if ( leftpb == 1){
      leftpb = 0;
      //toggle runstp
      if ( runstp == 1 ){
        runstp = 0;
      } else{
        runstp = 1;
      }
   }

   if (runstp == 1){
       //ATD conversion sequence
       

       out = ATDDR0; //output digital signal on PTT0
       out1 = ATDDR1;
       out = out * out1;
       for (i=0; i < 10; i++){

           //check output value's 1st bit and output that bit to PTT3
           if ((out & 0x80) == 0x80){
              PTT_PTT0 = 1;
           }
           else {
              PTT_PTT0 = 0;
           }


           //shift output bits
           out = out << 1;
        }

        out = ATDDR0H; //output digital signal on PTT0
        avgout = out + avgout;
        m++;
        
        if ( m == 10) {
            m = 0;
            out = avgout/10;
            avgout = 0;
            //out = out - (out - 225) * 2;
        
        chgline(LINE1);
        send_i(LCDCLR);
        pmsglcd("out = ");
        n = (out / 100) % 10;
        print_c(n + '0');
        n = (out / 10) % 10;
        print_c(n + '0');
        n = out % 10;
        print_c(n + '0');
        
        chgline(LINE2);
        bar = out/17;
        while(bar > 0) {
          print_c(0xFF);
          bar--;
        }
       }

   }


   } /* loop forever */

}   /* do not leave main */




/*
***********************************************************************   ����  � ������   ��
 RTI interrupt service routine: RTI_ISR
************************************************************************
*/

interrupt 7 void RTI_ISR(void)
{
  	// clear RTI interrupt flagt
  	CRGFLG = CRGFLG | 0x80;

    if ((PORTAD0_PTAD7 == 0) && (prevlpb == 1)){
       leftpb = 1;
    }

    if ((PORTAD0_PTAD6 == 0) && (prevrpb == 1)) {
      rghtpb = 1;
    }

    prevlpb = PORTAD0_PTAD7;
    prevrpb = PORTAD0_PTAD6;

}

/*
***********************************************************************   ����  � ������   ��
  TIM interrupt service routine
***********************************************************************
*/

interrupt 15 void TIM_ISR(void)
{
  	// clear TIM CH 7 interrupt flag
 	TFLG1 = TFLG1 | 0x80;
 	ATDCTL5 = 0x10; //output 10-bit data
       while(ATDSTAT0 != 0x80) { //check conversion complete
       }
   asm {
    	ldaa	ATDDR0		
	    ldab	ATDDR1	
	    mul		
	    adca	#0		
	    staa	PWMDTY0	
  }


}

/*
***********************************************************************   ����  � ������   ��
  SCI interrupt service routine
***********************************************************************
*/

interrupt 20 void SCI_ISR(void)
{



}


/*
***********************************************************************
  shiftout: Transmits the character x to external shift
            register using the SPI.  It should shift MSB first.

            MISO = PM[4]
            SCK  = PM[5]
***********************************************************************
*/

void shiftout(char x)

{
  int i = 0;
  // read the SPTEF bit, continue if bit is 1
  // write data to SPI data register
  // wait for 30 cycles for SPI data to shift out
  while (SPISR_SPTEF != 1){
  }

   SPIDR = x;
   for(i=0;i < 30; i++){
   }

}

/*
***********************************************************************
  lcdwait: Delay for approx 2 ms
***********************************************************************
*/

void lcdwait()
{
   unsigned int j;
   for(j=0; j < 48000; j++){
   }
}

/*
***********************************************************************
  send_byte: writes character x to the LCD
***********************************************************************
*/

void send_byte(char x)
{
     // shift out character
     // pulse LCD clock line low->high->low
     // wait 2 ms for LCD to process data
     shiftout(x);
     PTT_PTT4 = 0;
     PTT_PTT4 = 1;
     PTT_PTT4 = 0;
     lcdwait();

}

/*
***********************************************************************
  send_i: Sends instruction byte x to LCD
***********************************************************************
*/

void send_i(char x)
{
        // set the register select line low (instruction data)
        // send byte
        PTT_PTT2 = 0;
        send_byte(x);
}

/*
***********************************************************************
  chgline: Move LCD cursor to position x
  NOTE: Cursor positions are encoded in the LINE1/LINE2 variables
***********************************************************************
*/

void chgline(char x)
{
    send_i(CURMOV);
    send_i(x);
}

/*
***********************************************************************
  print_c: Print (single) character x on LCD
***********************************************************************
*/

void print_c(char x)
{
    PTT_PTT2 = 1;
    send_byte(x);
}

/*
***********************************************************************
  pmsglcd: print character string str[] on LCD
***********************************************************************
*/

void pmsglcd(char str[])
{
   int k = 0;
   while(str[k] != 0){
     print_c(str[k]);
     k++;
   }
}


/*
***********************************************************************
 Character I/O Library Routines for 9S12C32
***********************************************************************
 Name:         inchar
 Description:  inputs ASCII character from SCI serial port and returns it
 Example:      char ch1 = inchar();
***********************************************************************
*/

char inchar(void) {
  /* receives character from the terminal channel */
        while (!(SCISR1 & 0x20)); /* wait for input */
    return SCIDRL;
}

/*
***********************************************************************
 Name:         outchar    (use only for DEBUGGING purposes)
 Description:  outputs ASCII character x to SCI serial port
 Example:      outchar('x');
***********************************************************************
*/

void outchar(char x) {
  /* sends a character to the terminal channel */
    while (!(SCISR1 & 0x80));  /* wait for output buffer empty */
    SCIDRL = x;
}
