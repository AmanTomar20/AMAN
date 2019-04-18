

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#define _XTAL_FREQ 20000000

#define RS RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7

#define SIM900_OK 1
#define SIM900_READY 2
#define SIM900_FAIL 3
#define SIM900_RING 4
#define SIM900_NC 5
#define SIM900_UNLINK 6

inline unsigned char _SIM900_waitResponse(void);
int recv;
char p =1;


void ADC_Initialize()
{
  ADCON0 = 0b10000001; //ADC ON and Fosc/16 is selected
  ADCON1 = 0b11000000; // Internal reference voltage is selected
}

unsigned int ADC_Read(unsigned char channel)
{
  ADCON0 &= 0x11000101; //Clearing the Channel Selection Bits
  ADCON0 |= channel<<3; //Setting the required Bits
  __delay_ms(2); //Acquisition time to charge hold capacitor
  GO_nDONE = 1; //Initializes A/D Conversion
  while(GO_nDONE); //Wait for A/D Conversion to complete
  return ((ADRESH<<8)+ADRESL); //Returns Result
}

//LCD Functions Developed by Circuit Digest.
void Lcd_SetBit(char data_bit) //Based on the Hex value Set the Bits of the Data Lines
{
    if(data_bit& 1) 
        D4 = 1;
    else
        D4 = 0;

    if(data_bit& 2)
        D5 = 1;
    else
        D5 = 0;

    if(data_bit& 4)
        D6 = 1;
    else
        D6 = 0;

    if(data_bit& 8) 
        D7 = 1;
    else
        D7 = 0;
}

void Lcd_Cmd(char a)
{
    RS = 0;           
    Lcd_SetBit(a); //Incoming Hex value
    EN  = 1;         
        __delay_ms(4);
        EN  = 0;
        __delay_ms(4);
}

Lcd_Clear()
{
    Lcd_Cmd(0); //Clear the LCD
    Lcd_Cmd(1); //Move the curser to first position
}

void Lcd_Set_Cursor(char a, char b)
{
    char temp,z,y;
    if(a== 1)
    {
      temp = 0x80 + b - 1; //80H is used to move the curser
        z = temp>>4; //Lower 8-bits
        y = temp & 0x0F; //Upper 8-bits
        Lcd_Cmd(z); //Set Row
        Lcd_Cmd(y); //Set Column
    }
    else if(a== 2)
    {
        temp = 0xC0 + b - 1;
        z = temp>>4; //Lower 8-bits
        y = temp & 0x0F; //Upper 8-bits
        Lcd_Cmd(z); //Set Row
        Lcd_Cmd(y); //Set Column
    }
}

void Lcd_Start()
{
  Lcd_SetBit(0x00);
  for(int i=1065244; i<=0; i--)  NOP();  
  Lcd_Cmd(0x03);
    __delay_ms(5);
  Lcd_Cmd(0x03);
    __delay_ms(11);
  Lcd_Cmd(0x03); 
  Lcd_Cmd(0x02); //02H is used for Return home -> Clears the RAM and initializes the LCD
  Lcd_Cmd(0x02); //02H is used for Return home -> Clears the RAM and initializes the LCD
  Lcd_Cmd(0x08); //Select Row 1
  Lcd_Cmd(0x00); //Clear Row 1 Display
  Lcd_Cmd(0x0C); //Select Row 2
  Lcd_Cmd(0x00); //Clear Row 2 Display
  Lcd_Cmd(0x06);
}

void Lcd_Print_Char(char data)  //Send 8-bits through 4-bit mode
{
   char Lower_Nibble,Upper_Nibble;
   Lower_Nibble = data&0x0F;
   Upper_Nibble = data&0xF0;
   RS = 1;             // => RS = 1
   Lcd_SetBit(Upper_Nibble>>4);             //Send upper half by shifting by 4
   EN = 1;
   for(int i=2130483; i<=0; i--)  NOP(); 
   EN = 0;
   Lcd_SetBit(Lower_Nibble); //Send Lower half
   EN = 1;
   for(int i=2130483; i<=0; i--)  NOP();
   EN = 0;
   for(int i=2130483; i<=0; i--)  NOP();
}

void Lcd_Print_String(char *a)
{
    int i;
    for(i=0;a[i]!='\0';i++)
       Lcd_Print_Char(a[i]);  //Split the string using pointers and call the Char function 
}

void Door(char a)
{
            RD1=0;
            Lcd_Clear();
            Lcd_Set_Cursor(1,4);
            Lcd_Print_String("DOOR ");
            Lcd_Print_Char(a);
            Lcd_Set_Cursor(2,1);
            Lcd_Print_String("NOT CLOSED");
            __delay_ms(2000);
}


//***Initialize UART for SIM900**//
void Initialize_SIM900(void)
{
    //****Setting I/O pins for UART****//
    TRISC6 = 0; // TX Pin set as output
    TRISC7 = 1; // RX Pin set as input
    //________I/O pins set __________//
    
    /**Initialize SPBRG register for required 
    baud rate and set BRGH for fast baud_rate**/
    SPBRG = 129; //SIM900 operates at 9600 Baud rate so 129
    BRGH  = 1;  // for high baud_rate
    //_________End of baud_rate setting_________//
    
    //****Enable Asynchronous serial port*******//
    SYNC  = 0;    // Asynchronous
    SPEN  = 1;    // Enable serial port pins
    //_____Asynchronous serial port enabled_______//
    //**Lets prepare for transmission & reception**//
    TXEN  = 1;    // enable transmission
    CREN  = 1;    // enable reception
    //__UART module up and ready for transmission and reception__//
    
    //**Select 8-bit mode**//  
    TX9   = 0;    // 8-bit reception selected
    RX9   = 0;    // 8-bit reception mode selected
    //__8-bit mode selected__//     
}
//________UART module Initialized__________//
 
 
//**Function to send one byte of date to UART**//
void _SIM900_putch(char bt)  
{
    while(!TXIF);  // hold the program till TX buffer is free
    TXREG = bt; //Load the transmitter buffer with the received value
}
//_____________End of function________________//
 
 
//**Function to get one byte of date from UART**//
char _SIM900_getch()   
{
    if(OERR) // check for Error 
    {
        CREN = 0; //If error -> Reset 
        CREN = 1; //If error -> Reset 
    }
    
    while(!RCIF);  // hold the program till RX buffer is free
    
    return RCREG; //receive the value and send it to main function
}
//_____________End of function________________//
 
 
//**Function to convert string to byte**//
void SIM900_send_string(char* st_pt)
{
    while(*st_pt) //if there is a char
        _SIM900_putch(*st_pt++); //process it as a byte data
}
//___________End of function______________//
//**End of modified Codes**//
 
void _SIM900_print(unsigned const char *ptr) {
    while (*ptr != 0) {
        _SIM900_putch(*ptr++);
    }
}
 
int SIM900_isStarted(void) {
    _SIM900_print("AT\r\n");
    return (_SIM900_waitResponse() == SIM900_OK);
}
 
int SIM900_isReady(void) {
    _SIM900_print("AT+CPIN?\r\n");
    return (_SIM900_waitResponse() == SIM900_READY);
}
 
inline unsigned char _SIM900_waitResponse(void) {
    unsigned char so_far[6] = {0,0,0,0,0,0};
    unsigned const char lengths[6] = {2,12,5,4,6,6};
    unsigned const char* strings[6] = {"OK", "+CPIN: READY", "ERROR", "RING", "NO CARRIER", "Unlink"};
    unsigned const char responses[6] = {SIM900_OK, SIM900_READY, SIM900_FAIL, SIM900_RING, SIM900_NC, SIM900_UNLINK};
    unsigned char received;
    unsigned char response;
    char continue_loop = 1;
    while (continue_loop) {
        received = _SIM900_getch();
        for (unsigned char i = 0; i < 6; i++) {
            if (strings[i][so_far[i]] == received) {
                so_far[i]++;
                if (so_far[i] == lengths[i]) {
                    response = responses[i];
                    continue_loop = 0;
                }
            } else {
                so_far[i] = 0;
            }
        }
    }
    return response;
}
 

void main()
{
    int a,b,c,d,e,f,g,h,adc; //just variables
    int i = 0; //the 4-digit value that is to be displayed
    int flag =0; //for creating delay
    int f2=0;
    int f3=0;
    //*****I/O Configuration****//
    //TRISC=0X00;
    //PORTC=0X00;
    TRISD=0x00;
    PORTD=0X00;
    TRISB=0XFF;

    OPTION_REG=0b01011110;


    //***End of I/O configuration**///

    ADC_Initialize();
    Initialize_SIM900();
    #define _XTAL_FREQ 20000000

    Lcd_Start();

    Lcd_Clear();
    Lcd_Set_Cursor(1,1);
    Lcd_Print_String("PRESS AUTOSTART");
    Lcd_Set_Cursor(2,1);
    Lcd_Print_String("TO START");


    while(1)
    {
        if(RB0==0 || f3==1)
        {
            __delay_ms(100);
            if(RB0==0 || f3==1)
            {
                Lcd_Clear();
                Lcd_Set_Cursor(1,1);
                Lcd_Print_String("ALCOHOL ");
                Lcd_Set_Cursor(2,1);
                Lcd_Print_String("LEVEL: ");
                f2=1;
                flag=51;
                f3=0;
                RD1=1;
            }

        }
        if(RB5==0)
        {
            __delay_ms(100);
            if(RB5==0)
            {
                RD1=0;
                Lcd_Clear();
                Lcd_Set_Cursor(4,1);
                Lcd_Print_String("WEAR");
                Lcd_Set_Cursor(2,1);
                Lcd_Print_String("SEATBELT");
                __delay_ms(2000);
                if(RB5!=0)
                    f3=1;
            }

        }
        if(RB1==0)
        {
            __delay_ms(100);
            if(RB1==0)
            {
                Door('1');
                if(RB2!=0)
                    f3=1;
            }

        }
        if(RB2==0)
        {
            __delay_ms(100);
            if(RB2==0)
            {
                Door('2');
                if(RB2!=0)
                    f3=1;
            }

        }
        if(RB3==0)
        {
            __delay_ms(100);
            if(RB3==0)
            {
                Door('3');
                if(RB2!=0)
                    f3=1;
            }

        }
        if(RB4==0)
        {
            __delay_ms(100);
            if(RB4==0)
            {
                Door('4');
                if(RB2!=0)
                    f3=1;
            }

        }
        if(flag>50)
        {
            if(f2==1)
            {
                Lcd_Set_Cursor(2,8);
            // Lcd_Print_Char(g+'0');
             Lcd_Print_Char(e+'0');

             Lcd_Print_Char(c+'0');

            // Lcd_Print_Char(a+'0');
             Lcd_Print_Char('%');
                adc = (ADC_Read(4));
                i = adc*0.488281*2;
                flag=0; //only if flag is hundred "i" will get the ADC value
            }

        }


        if(i>700)
        {
            RD1=0;    
            Lcd_Set_Cursor(1,1);
            Lcd_Print_String("SIM900 & PIC");

               /*Check if the SIM900 communication is successful*/
            do
            {
            Lcd_Set_Cursor(2,1);
            Lcd_Print_String("Module not found");
            }while (!SIM900_isStarted()); //wait till the GSM to send back "OK"
            Lcd_Set_Cursor(2,1);
            Lcd_Print_String("Module Detected ");
            __delay_ms(1500);


               /*Check if the SIM card is detected*/
             do
            {
            Lcd_Set_Cursor(2,1);
            Lcd_Print_String("SIM not found   ");
            }while (!SIM900_isReady()); //wait till the GSM to send back "+CPIN: READY"
            Lcd_Set_Cursor(2,1);
            Lcd_Print_String("SIM Detected    ");
            __delay_ms(1500);

           Lcd_Clear();


             /*Place a Phone Call*/
             do
            {
            _SIM900_print("ATD8698566828;\r\n");  //Here we are placing a call to number 93643XXXXX
            Lcd_Set_Cursor(1,1);
            Lcd_Print_String("Placing Call....");
            }while (_SIM900_waitResponse() != SIM900_OK); //wait till the ESP send back "OK"
            Lcd_Set_Cursor(1,1);
            Lcd_Print_String("Call Placed....");
            __delay_ms(1500);

            while(1)
            {
                if (_SIM900_waitResponse() == SIM900_RING) //Check if there is an incoming call
                {
                  Lcd_Set_Cursor(2,1);
                  Lcd_Print_String("Incoming Call!!.");  
                }       
            }
        }
        flag++;

          //***Splitting "i" into four digits***//  
        a=i%10;//4th digit is saved here
        b=i/10;
        c=b%10;//3rd digit is saved here
        d=b/10;
        e=d%10; //2nd digit is saved here
        f=d/10;
        g=f%10; //1st digit is saved here
        h=f/10;
        //***End of splitting***//



    }
}
