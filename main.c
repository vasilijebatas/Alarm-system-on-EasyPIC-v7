/*
 * File:   main.c
 * Author: Q
 *
 * Created on January 23, 2022, 8:18 PM
 */


#include "xc.h"
#include "tajmeri.h"
#include "driverGLCD.h"
#include "adc.h"
#include "slike.h"
#include <stdio.h>
#include <stdlib.h>
#include<p30fxxxx.h>

#define DRIVE_A PORTCbits.RC13
#define DRIVE_B PORTCbits.RC14

_FOSC(CSW_FSCM_OFF & XT_PLL4);//instruction takt je isti kao i kristal
_FWDT(WDT_OFF);
_FGS(CODE_PROT_OFF);

//PROMENLJIVE
//**********************************************************************************
enum state {init, ready, brojac, aktivan, sirena, vrata, z_vrata };
enum state stanje = init;

unsigned int sirovi0,sirovi1,sirovi2,sirovi3,stoperica, i;
unsigned int X, Y,x_vrednost, y_vrednost;
 
const unsigned int AD_Xmin =220;
const unsigned int AD_Xmax =3642;
const unsigned int AD_Ymin =520;
const unsigned int AD_Ymax =3450;

unsigned int broj,broj1,broj2,temp0,temp1; 
unsigned int n,temp; 
unsigned char tempRX;
//**********************************************************************************



//TOUCH SCREEN
//**********************************************************************************
void ConfigureTSPins(void)
{

	TRISCbits.TRISC13=0;
    TRISCbits.TRISC14=0;

}

void Touch_Panel (void)
{
// vode horizontalni tranzistori
	DRIVE_A = 1;  
	DRIVE_B = 0;
    
     LATCbits.LATC13=1;
     LATCbits.LATC14=0;

	Delay_ms(50); //cekamo jedno vreme da se odradi AD konverzija
				
	// ocitavamo x	
	x_vrednost = temp0;//temp0 je vrednost koji nam daje AD konvertor na BOTTOM pinu		

	// vode vertikalni tranzistori
     LATCbits.LATC13=0;
     LATCbits.LATC14=1;
	DRIVE_A = 0;  
	DRIVE_B = 1;

	Delay_ms(50); //cekamo jedno vreme da se odradi AD konverzija
	
	// ocitavamo y	
	y_vrednost = temp1;// temp1 je vrednost koji nam daje AD konvertor na LEFT pinu	
	
//Ako želimo da nam X i Y koordinate budu kao rezolucija ekrana 128x64 treba skalirati vrednosti x_vrednost i y_vrednost tako da budu u opsegu od 0-128 odnosno 0-64
//skaliranje x-koordinate

    X=(x_vrednost-161)*0.03629;

//X= ((x_vrednost-AD_Xmin)/(AD_Xmax-AD_Xmin))*128;	
//vrednosti AD_Xmin i AD_Xmax su minimalne i maksimalne vrednosti koje daje AD konvertor za touch panel.


//Skaliranje Y-koordinate
	Y= ((y_vrednost-500)*0.020725);

//	Y= ((y_vrednost-AD_Ymin)/(AD_Ymax-AD_Ymin))*64;
}
void Write_GLCD(unsigned int data)
{
    unsigned char temp;

    temp=data/1000;
    Glcd_PutChar(temp+'0');
    data=data-temp*1000;
    temp=data/100;
    Glcd_PutChar(temp+'0');
    data=data-temp*100;
    temp=data/10;
    Glcd_PutChar(temp+'0');
    data=data-temp*10;
    Glcd_PutChar(data+'0');
}
//**********************************************************************************



//TAJMER
//**********************************************************************************
void Delay_ms (int vreme)//funkcija za kasnjenje u milisekundama
{
	stoperica = 0;
	while(stoperica < vreme);
}


void __attribute__ ((__interrupt__)) _T2Interrupt(void) // svakih 1ms
{

	TMR2 =0;

    stoperica++;//brojac za funkciju Delay_ms

	IFS0bits.T2IF = 0; 
       
}
//**********************************************************************************


//AD KONVERZIJA
//**********************************************************************************
void __attribute__((__interrupt__)) _ADCInterrupt(void) 
{
							

	sirovi0=ADCBUF0;
	sirovi1=ADCBUF1;
    sirovi2=ADCBUF2;
    sirovi3=ADCBUF3;
    									
	temp0=sirovi0;
	temp1=sirovi1;									

    IFS0bits.ADIF = 0;

} 
//**********************************************************************************


//SERIJSKA KOMUNIKACIJA
//**********************************************************************************
void initUART1(void)
{
    U1BRG=0x0040;//ovim odredjujemo baudrate 9600

    U1MODEbits.ALTIO=0;//biramo koje pinove koristimo za komunikaciju osnovne ili alternativne

    IEC0bits.U1RXIE=1;//omogucavamo rx1 interupt

    U1STA&=0xfffc;

    U1MODEbits.UARTEN=1;//ukljucujemo ovaj modul

    U1STAbits.UTXEN=1;//ukljucujemo predaju
}


void __attribute__((__interrupt__)) _U1RXInterrupt(void) 
{
    IFS0bits.U1RXIF = 0;
    //tempRX=U1RXREG;
} 


void WriteUART1(unsigned int data)
{
	  while(!U1STAbits.TRMT);

    if(U1MODEbits.PDSEL == 3)
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}

void RS232_putst(register const char *str)
{
    while((*str)!=0)
    {
        WriteUART1(*str);
        if(*str==13)WriteUART1(10);
        if(*str==10)WriteUART1(13);
        str++;
    }
}
void WriteUART1dec2string(unsigned int data)
{
	unsigned char temp;

	temp=data/1000;
	WriteUART1(temp+'0');
	data=data-temp*1000;
	temp=data/100;
	WriteUART1(temp+'0');
	data=data-temp*100;
	temp=data/10;
	WriteUART1(temp+'0');
	data=data-temp*10;
	WriteUART1(data+'0');
}
//**********************************************************************************

int main(void) {
    
    //inicijalizacija
    ADCinit();
    Init_T2();
    initUART1();
    ADCON1bits.ADON=1;//pocetak Ad konverzije 
    Delay_ms(20);
    ConfigureTSPins();
    Delay_ms(20);
    ConfigureLCDPins();
    Delay_ms(20);
    GLCD_LcdInit();
    Delay_ms(20);
    GLCD_ClrScr();
    
    //pin za PIR senzor
    ADPCFGbits.PCFG7 = 1; //digitalni
    TRISBbits.TRISB7 = 1; //ulaz
    
    //pin za servo motor
    ADPCFGbits.PCFG6 = 1; //digitalni
    TRISBbits.TRISB6 = 0; //izlaz
    
    //pin za buzzer
    TRISFbits.TRISF6 = 0; //izlaz
    
    
    while(1)
    {
        
        switch(stanje)
        {
            case init:
                GLCD_DisplayPicture(aktiviraj); 
                LATFbits.LATF6 = 0;
                stanje = ready;
                break;
                
            case ready:
                if(sirovi2 > 3100)//fotootpornik
                {
                    stanje = brojac;
                }  
              
                
                Touch_Panel ();
                if ((X>1)&&(X<128)&& (Y>1)&&(Y<64))
                {
                    stanje = brojac;
                }
                break;
                
            case brojac:

                GLCD_DisplayPicture(tri); 
                Delay_ms(1000);
                GLCD_DisplayPicture(dva);
                Delay_ms(1000);
                GLCD_DisplayPicture(jedan);
                Delay_ms(1000);

                stanje = aktivan;
                break;
                
            case aktivan:
                GLCD_DisplayPicture(aktiviran);
        
                if(sirovi3 > 1000)//senzor za dim
                {   
                    int j;
                    j = 0;
                    GLCD_DisplayPicture(dim);
                    RS232_putst("KOLICINA DIMA (max 4095): ");
                    WriteUART1(13);
                    
                    while(j < 4)
                    {
                        
                        WriteUART1dec2string(sirovi3);
                        WriteUART1(13);
                        Delay_ms(1000);
                        j++;
                    }
                    stanje = vrata;
                }
                else if(PORTBbits.RB7 == 1)//senzor pokreta
                 {
                    GLCD_DisplayPicture(pokret);
                    Delay_ms(2000);
                    stanje = sirena;
                 }
                break;
                
            case vrata:
                    
                    for(i = 0; i<10; i++)
                    {
                        LATBbits.LATB6 = 1;
                        Delay_ms(2);
                        LATBbits.LATB6 = 0;
                        Delay_ms(18);
                    }
                    stanje = z_vrata;
                break;
            case z_vrata:
                    GLCD_DisplayPicture(zatvori_vrata);
                    Touch_Panel ();
                    if ((X>1)&&(X<128)&& (Y>1)&&(Y<64))
                    {
                        for(i = 0; i<10; i++)
                        {
                            LATBbits.LATB6 = 1;
                            Delay_ms(1);
                            LATBbits.LATB6 = 0;
                            Delay_ms(19);
                        }
                        stanje = init;
                    }
                break;
                
            case sirena:
                GLCD_DisplayPicture(ugasi_sirenu);
                LATFbits.LATF6 = 1;
                Touch_Panel ();
                if ((X>1)&&(X<128)&& (Y>1)&&(Y<64))
                    {
                        stanje = init;
                    }
                
                break;
               
                
        
        }

    }
    
    
    
    return 0;
}
