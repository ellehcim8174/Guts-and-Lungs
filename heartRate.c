//  C8051F380/C8051F381 with LCD in 4-bit interface mode
//  adapted from Jesus Calvino-Fraga
//  ~C51~

#include <C8051f38x.h>
#include <stdio.h>

#define SYSCLK      48000000L  // SYSCLK frequency in Hz
#define BAUDRATE      115200L  // Baud rate of UART in bps

#define LCD_RS P2_2
#define LCD_RW P2_1 // Not used in this code
#define LCD_E  P2_0
#define LCD_D4 P1_3
#define LCD_D5 P1_2
#define LCD_D6 P1_1
#define LCD_D7 P1_0
#define CHARS_PER_LINE 16
#define B2 P2_5
#define B1 P2_3
#define B3 P2_7

unsigned char overflow_count;

void PORT_Init (void)
{
	P0MDOUT |= 0x10; // Enable UART TX as push-pull output
	XBR0=0b_0000_0001; // Enable UART on P0.4(TX) and P0.5(RX)                    
	XBR1=0b_0101_0000; // Enable crossbar.  Enable T0 input.
	XBR2=0b_0000_0000;
}

void SYSCLK_Init (void)
{
	// CLKSEL&=0b_1111_1000; // Not needed because CLKSEL==0 after reset
#if (SYSCLK == 12000000L)
	//CLKSEL|=0b_0000_0000;  // SYSCLK derived from the Internal High-Frequency Oscillator / 4 
#elif (SYSCLK == 24000000L)
	CLKSEL|=0b_0000_0010; // SYSCLK derived from the Internal High-Frequency Oscillator / 2.
#elif (SYSCLK == 48000000L)
	CLKSEL|=0b_0000_0011; // SYSCLK derived from the Internal High-Frequency Oscillator / 1.
#else
	#error SYSCLK must be either 12000000L, 24000000L, or 48000000L
#endif
	OSCICN |= 0x03;   // Configure internal oscillator for its maximum frequency
	RSTSRC  = 0x04;   // Enable missing clock detector
}
 
void UART0_Init (void)
{
	SCON0 = 0x10;
   
#if (SYSCLK/BAUDRATE/2L/256L < 1)
	TH1 = 0x10000-((SYSCLK/BAUDRATE)/2L);
	CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
	CKCON |=  0x08;
#elif (SYSCLK/BAUDRATE/2L/256L < 4)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/4L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 01                  
	CKCON |=  0x01;
#elif (SYSCLK/BAUDRATE/2L/256L < 12)
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/12L);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 00
#else
	TH1 = 0x10000-(SYSCLK/BAUDRATE/2/48);
	CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 10
	CKCON |=  0x02;
#endif
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit autoreload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
}

void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	TR0=0; // Stop Timer/Counter 0
}

char _c51_external_startup (void)
{
	PCA0MD&=(~0x40) ;    // DISABLE WDT: clear Watchdog Enable bit
	VDM0CN=0x80; // enable VDD monitor
	RSTSRC=0x02|0x04; // Enable reset on missing clock detector and VDD

	// CLKSEL&=0b_1111_1000; // Not needed because CLKSEL==0 after reset
	#if (SYSCLK == 12000000L)
		//CLKSEL|=0b_0000_0000;  // SYSCLK derived from the Internal High-Frequency Oscillator / 4 
	#elif (SYSCLK == 24000000L)
		CLKSEL|=0b_0000_0010; // SYSCLK derived from the Internal High-Frequency Oscillator / 2.
	#elif (SYSCLK == 48000000L)
		CLKSEL|=0b_0000_0011; // SYSCLK derived from the Internal High-Frequency Oscillator / 1.
	#else
		#error SYSCLK must be either 12000000L, 24000000L, or 48000000L
	#endif
	OSCICN |= 0x03; // Configure internal oscillator for its maximum frequency
	
	P0MDOUT|=0x10; // Enable Uart TX as push-pull output
	P1MDOUT|=0b_0000_1111; // LCD's D4 to D7 are connected to P1.3 to P1.0
	P2MDOUT|=0b_0000_0111; // P2.2 is LCD's RS, P2.1 is LCD's RW, P2.0 is LCD's E
	XBR0=0x01; // Enable UART on P0.4(TX) and P0.5(RX)                     
	XBR1=0x40; // Enable crossbar and weak pull-ups
	
	#if (SYSCLK/BAUDRATE/2L/256L < 1)
		TH1 = 0x10000-((SYSCLK/BAUDRATE)/2L);
		CKCON &= ~0x0B;                  // T1M = 1; SCA1:0 = xx
		CKCON |=  0x08;
	#elif (SYSCLK/BAUDRATE/2L/256L < 4)
		TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/4L);
		CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 01                  
		CKCON |=  0x01;
	#elif (SYSCLK/BAUDRATE/2L/256L < 12)
		TH1 = 0x10000-(SYSCLK/BAUDRATE/2L/12L);
		CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 00
	#else
		TH1 = 0x10000-(SYSCLK/BAUDRATE/2/48);
		CKCON &= ~0x0B; // T1M = 0; SCA1:0 = 10
		CKCON |=  0x02;
	#endif
	
	TL1 = TH1;     // Init timer 1
	TMOD &= 0x0f;  // TMOD: timer 1 in 8-bit autoreload
	TMOD |= 0x20;                       
	TR1 = 1;       // Start timer1
	SCON = 0x52;
	
	return 0;
}

// Uses Timer3 to delay <us> micro-seconds. 
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON:
	CKCON|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN & 0x80));  // Wait for overflow
		TMR3CN &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN = 0 ;                   // Stop Timer3 and clear overflow flag
}

void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) Timer3us(250);
}

void LCD_pulse (void)
{
	LCD_E=1;
	Timer3us(40);
	LCD_E=0;
}

void LCD_byte (unsigned char x)
{
	// The accumulator in the C8051Fxxx is bit addressable!
	ACC=x; //Send high nible
	LCD_D7=ACC_7;
	LCD_D6=ACC_6;
	LCD_D5=ACC_5;
	LCD_D4=ACC_4;
	LCD_pulse();
	Timer3us(40);
	ACC=x; //Send low nible
	LCD_D7=ACC_3;
	LCD_D6=ACC_2;
	LCD_D5=ACC_1;
	LCD_D4=ACC_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS=1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT (void)
{
	LCD_E=0; // Resting state of LCD's enable is zero
	LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, bit clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

void int2char(char *string, unsigned int num, unsigned int size)
{
	while(size > 0 && num != 0){
		string[size-1] = (48) + (num%10);
		size--;
		num = num/10;
	}
	while(size > 0){
		string[size-1] = ' ';
		size--;
	}
}

//REFINE THIS FUNCTION
int whichWorkout(unsigned int workNum)
{
	while(1){
		if(workNum==1)
		{
			LCDprint("1. Endurance", 1, 1);
			LCDprint("B1-yes  B2-no", 2, 1);
			if(!B2)
				workNum = 2;
			else if(!B1) //if B1 is pressed
				return 1;
			else
				workNum = 1;
		}
		else if(workNum==2)
		{
			LCDprint("2. Cardio", 1, 1);
			LCDprint("B1-yes  B2-no", 2, 1);
			if(!B2)
				workNum = 3;
			else if(!B1) //if B1 is pressed
				return 2;
			else
				workNum = 2;
		}
		else
		{
			LCDprint("3. Intervals", 1, 1);
			LCDprint("B1-yes  B2-no", 2, 1);
			if(!B2)
				workNum = 1;
			else if(!B1) //if B1 is pressed
				return 3;
			else
				workNum = 3;
		}
	}
		
	
}

unsigned int askAge(unsigned int age)
{
	char stgNum[3];
	//stgNum = '\0';
	stgNum[2] = '\0';
	LCDprint("Enter age: ", 1,0);
	LCDprint("B1-up  B2-down", 2, 1);
	
	//while B3 is not pressed
	while(B3)
	{
		int2char(stgNum, age, 2);
		LCDprint(stgNum, 2, 0);
		if(!B1)
		{
			while(!B1){
			age++;
			if(age > 99)
				age = 0;
			}
		}
		else if(!B2)
		{
			while(!B2){
			age--;
			if(age == 0)
				age = 99;	
			}
		}
	
	}
	//press B3 to return
	while(!B3);
	return age;
		
		
}

int * calcTargetRate(unsigned int workout, unsigned int age){
	unsigned int maxRate = 220-age;
	int targetRate1;
	int targetRate2;
	int targetArray[2];
	int* a = &targetArray[0];
	
	if (workout==1){
		targetRate1 = (double) 0.6*maxRate;
		targetRate2 = (double) 0.7*maxRate;
		}
	else if (workout==2){
		targetRate1 = (double) 0.7*maxRate;
		targetRate2 = (double) 0.8*maxRate;
		}
		
	else if (workout==3){
		targetRate1 = (double) 0.8*maxRate;
		targetRate2 = 0;
		//&targetArray[0] = targetRate1;
		}
		
	else {
		targetRate1 = 0;
		targetRate2 = 0;
		}
		
	targetArray[0] = targetRate1;
	targetArray[1] = targetRate2;
	return a;
}

void main (void)
{
	float period;
   	float bpm;
   	unsigned int intbpm;
   	char stringbpm[3]; 
   	unsigned int workout;
   	unsigned int age;
   	char stringTargetHeart1[3];
   	char stringTargetHeart2[3];
   	int* targetHeart;
   	int x, y;
   	int now = 0;
	// Configure the LCD
	LCD_4BIT();
	//initialize string
	stringbpm[2] = '\0';
   
	//PCA0MD &= ~0x40; // WDTE = 0 (clear watchdog timer enable)
	PORT_Init();     // Initialize Port I/O
	//SYSCLK_Init ();  // Initialize Oscillator
	//UART0_Init();    // Initialize UART0
	TIMER0_Init();

	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.

	printf ("Period measurement at pin P0.1 using Timer 0.\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);

    while (1)
    {

		while(B3)
		{
			// Reset the counter
			TL0=0; 
			TH0=0;
			TF0=0;
			overflow_count=0;
		
			while(P0_1!=0); // Wait for the signal to be zero
	
			while(P0_1!=1); // Wait for the signal to be one
	
			TR0=1; // Start the timer
		
	    
			while(P0_1!=0) // Wait for the signal to be zero
			{
				if(TF0==1) // Did the 16-bit timer overflow?
				{
					TF0=0;
					overflow_count++;
				}
			}
		
	    
			while(P0_1!=1) // Wait for the signal to be one
			{
				if(TF0==1) // Did the 16-bit timer overflow?
				{
					TF0=0;
					overflow_count++;
				}
			}
		
			TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
			period=(overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);
			// Send the period to the serial port
			printf( "\rf=%fs" , period);
			bpm = 1.0/(period/60.0);
			intbpm = bpm;
			LCDprint("BPM:",1,1);
			int2char(stringbpm, intbpm, 2);
			LCDprint(stringbpm,2,1);
		}
		
    	
		
	
	    while(!B3)
	    {
			//ask which workout they are doing (initialize to 1st) - CHECK THIS FXN
			workout = whichWorkout(1);
			//set age (initialized to 30) - CHECK THIS FXN
			age = askAge(30);
			targetHeart = calcTargetRate(workout, age);
			x = *targetHeart;  //range 1
			y = *(targetHeart+1);  //range 2
			if (&targetHeart[0] == 0){
				//print undefined message to lcd
				LCDprint("UNDEFINED", 1,1);
				
			int2char(stringTargetHeart1, x, 3);
			int2char(stringTargetHeart2, y, 3);
			LCDprint("Trgt Heart Zone:", 1, 1);
			LCDprint("     to    ", 2, 1);
			LCDprint(stringTargetHeart1, 2, 0);
			LCDprint(stringTargetHeart2, 2, 0);
		
		}	
			
	}
}
}



