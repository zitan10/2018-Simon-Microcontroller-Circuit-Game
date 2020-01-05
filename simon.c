#include <XC.h>
#include <stdio.h>
#include <stdlib.h>
 
// Configuration Bits (somehow XC32 takes care of this)
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz)
 
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK

// Defines
#define SYSCLK 40000000L
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

#define GREEN 0
#define BLUE 1
#define YELLOW 2
#define RED 3

 
void wait_1s(); //wait around 1 sec
void off(); //off all leds
 
void UART2Configure(int baud_rate)
{
    // Peripheral Pin Select
    U2RXRbits.U2RXR = 4;    //SET RX to RB8
    RPB9Rbits.RPB9R = 2;    //SET RB9 to TX

    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX
    U2BRG = Baud2BRG(baud_rate); // U2BRG = (FPb / (16*baud)) - 1
    
    U2MODESET = 0x8000;     // enable UART2
}

// Good information about ADC in PIC32 found here:
// http://umassamherstm5.org/tech-tutorials/pic32-tutorials/pic32mx220-tutorials/adc
void ADCConf(void)
{
    AD1CON1CLR = 0x8000;    // disable ADC before configuration
    AD1CON1 = 0x00E0;       // internal counter ends sampling and starts conversion (auto-convert), manual sample
    AD1CON2 = 0;            // AD1CON2<15:13> set voltage reference to pins AVSS/AVDD
    AD1CON3 = 0x0f01;       // TAD = 4*TPB, acquisition time = 15*TAD 
    AD1CON1SET=0x8000;      // Enable ADC
}

int ADCRead(char analogPIN)
{
    AD1CHS = analogPIN << 16;    // AD1CHS<16:19> controls which analog pin goes to the ADC
 
    AD1CON1bits.SAMP = 1;        // Begin sampling
    while(AD1CON1bits.SAMP);     // wait until acquisition is done
    while(!AD1CON1bits.DONE);    // wait until conversion done
 
    return ADC1BUF0;             // result stored in ADC1BUF0
}

int _mon_getc(int canblock)
{
	char c;
	
    if (canblock)
    {
	    while( !U2STAbits.URXDA); // wait (block) until data available in RX buffer
	    c=U2RXREG;
	    if(c=='\r') c='\n'; // When using PUTTY, pressing <Enter> sends '\r'.  Ctrl-J sends '\n'
		return (int)c;
    }
    else
    {
        if (U2STAbits.URXDA) // if data available in RX buffer
        {
		    c=U2RXREG;
		    if(c=='\r') c='\n';
			return (int)c;
        }
        else
        {
            return -1; // no characters to return
        }
    }
}

void SomeDelay (void)
{
	int j;
	for(j=0; j<50000; j++);
}

//Wrong value enetered
void FlashRed(void){

    int i = 0;
    while (i<80){
        LATBbits.LATB0 = !LATBbits.LATB0;
        LATBbits.LATB10 = !LATBbits.LATB10;
        SomeDelay();
        i++;
    }
    return;
}

void main(void)
{
	volatile unsigned long t=0;
    int adcval;
    float voltage;

	CFGCON = 0;
  
    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
 
    // Configure pins as analog inputs
    ANSELBbits.ANSB3 = 1;   // set RB3 (AN5, pin 7 of DIP28) as analog pin
    TRISBbits.TRISB3 = 1;   // set RB3 as an input
    
//*******************************Initialize LED pins***********************************
    
    //RB6 - pin 6
	TRISBbits.TRISB2 = 0;
	LATBbits.LATB2 = 0;	
	
	//RB5 - pin 14
	TRISBbits.TRISB5 = 0;
	LATBbits.LATB5 = 0;	

    //RB0 - pin 4 
	TRISBbits.TRISB0 = 0;
	LATBbits.LATB0 = 0;	

    //RB1 - pin 5
    TRISBbits.TRISB1 = 0;
	LATBbits.LATB1 = 0;	

    //RB10 - pin21 - buzz buzz
    TRISBbits.TRISB10 = 0;
	LATBbits.LATB10 = 0;

//*******************************Initialize Push button pins*****************************

    ANSELB &= ~(1<<6); // Set RB6 as a digital I/O
    TRISB |= (1<<6);   // configure pin RB6 as input
    CNPUB |= (1<<6);   // Enable pull-up resistor for RB6

    ANSELB &= ~(1<<13); // Set RB13 as a digital I/O
    TRISB |= (1<<13);   // configure pin RB13 as input
    CNPUB |= (1<<13);   // Enable pull-up resistor for RB13

    ANSELB &= ~(1<<14); // Set RB14 as a digital I/O
    TRISB |= (1<<14);   // configure pin RB14 as input
    CNPUB |= (1<<14);   // Enable pull-up resistor for RB14

    ANSELB &= ~(1<<15); // Set RB15 as a digital I/O
    TRISB |= (1<<15);   // configure pin RB15 as input
    CNPUB |= (1<<15);   // Enable pull-up resistor for RB15
		
	INTCONbits.MVEC = 1;
 
    ADCConf(); // Configure ADC
 
    printf("*** PIC32 ADC test ***\r\n");

    int start_count = 0;

	while(start_count<6)
	{
		t++;
		if(t==500000)
		{
        	adcval = ADCRead(5); // note that we call pin AN5 (RB3) by it's analog number
        	voltage=adcval*3.3/1023.0;
            //Random Number
        	int r = ((int)(voltage*100))%4;  

        	printf("AN5=0x%04x, %.3fV, %d\r", adcval, voltage, r);        	
        	fflush(stdout);
			t = 0;
			LATBbits.LATB2 = !LATBbits.LATB2; // Blink LED on RB6
			LATBbits.LATB5 = !LATBbits.LATB5; // Blink LED on RB5
            LATBbits.LATB0 = !LATBbits.LATB0; // Blink LED on RB0
            LATBbits.LATB1 = !LATBbits.LATB1; // Blink LED on RB1
            start_count++;
		}
	}

    //*******************************Start Simon**************************************

    //

    int count = 1;
    int i = 0;
    int RNG_values[100];
    int z = 0;

    wait_1s();

    while(1){

        //Show pattern
        while(i < count){

            adcval = ADCRead(5); // note that we call pin AN5 (RB3) by it's analog number
        	voltage=adcval*3.3/1023.0;
	        RNG_values[i] = ((int)(voltage*100))%4; //Get RNG value
	
            //Which light to flash

            //0 is green
    	    if (RNG_values[i] == 0){
	    	    LATBbits.LATB2 = 1;
	        }	

            //1 is blue
            else if (RNG_values[i] == 1){
                LATBbits.LATB5 = 1;
            }

            //2 is yellow
            else if (RNG_values[i] == 2){
                LATBbits.LATB1 = 1;
            }

            //3 is red
	        else{
		        LATBbits.LATB0 = 1;
    	    }
    	    wait_1s();
    	    off();
            wait_1s();	
            i++;
            z = 0;
        }

        while(z<i){

            while((PORTB&(1<<15)? 1: 0) && (PORTB&(1<<14)? 1: 0) && (PORTB&(1<<13)? 1: 0) && (PORTB&(1<<6)? 1: 0)){

               SomeDelay();
               
            }
            //If blue is selected 
            if(!(PORTB&(1<<15)? 1: 0)){

                while(!(PORTB&(1<<15)? 1: 0)){

                       LATBbits.LATB5 = 1;

                }
                SomeDelay();
                LATBbits.LATB5 = 0; 
                
                if(RNG_values[z] != BLUE){
                    FlashRed();
                    count = 0;
                    i = 0;
                    z = -1;
                }
            }

            //If green is selected 
            else if(!(PORTB&(1<<14)? 1: 0)){

                while(!(PORTB&(1<<14)? 1: 0)){

                    LATBbits.LATB2 = 1;

                }
                SomeDelay();
                LATBbits.LATB2 = 0; 

                if(RNG_values[z] != GREEN){
                    FlashRed();
                    count = 0;
                    i = 0;
                    z = -1;
                }
            }

            //If red is selected
            else if(!(PORTB&(1<<13)? 1: 0)){

                while(!(PORTB&(1<<13)? 1: 0)){

                    LATBbits.LATB0 = 1;

                }
                SomeDelay();
                LATBbits.LATB0 = 0; 

                if(RNG_values[z] != RED){
                    FlashRed();
                    count = 0;
                    i = 0;
                    z = -1;
                }
            }

            //If yellow is selected
            else{

                while(!(PORTB&(1<<6)? 1: 0)){

                    LATBbits.LATB1 = 1;

                }
                SomeDelay();
                LATBbits.LATB1 = 0;

                if(RNG_values[z] != YELLOW){
                    FlashRed();
                    count = 0;
                    i = 0;
                    z = -1;
                }
            }
            z++;
        }
        wait_1s();
        wait_1s();
        wait_1s();
        i=0;
        count++;
    }	
}

//wait around 1 second
void wait_1s(){

	int count = 0;
	
	while(count < 500000*4){
		count++;
	}
	return;
}

//Turn off all LEDs
void off(){

	LATBbits.LATB2 = 0;
	LATBbits.LATB5 = 0;
    LATBbits.LATB0 = 0;
    LATBbits.LATB1 = 0;
	return;
}	

