/* 
 * File:   SLAVE2.c
 * Author: diego
 *
 * Created on 12 de mayo de 2022, 07:20 PM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>
/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 4000000      // Frecuencia de Oscilador
#define FLAG_SPI 0xFFF
#define IN_MIN 0                // Valor minimo de entrada del potenciometro
#define IN_MAX 255              // Valor máximo de entrada del potenciometro
#define OUT_MIN 0               // Valor minimo de ancho de pulso de señal PWM
#define OUT_MAX 804             // Valor máximo de ancho de pulso de señal PWM



/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
unsigned short CCPR = 0;        // Variable para almacenar ancho de pulso al hacer la interpolación lineal
uint8_t ULTIMO = 0;
/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);
unsigned short map(uint8_t val, uint8_t in_min, uint8_t in_max, unsigned short out_min, unsigned short out_max);
/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    
    if (PIR1bits.SSPIF){
        
        ULTIMO = SSPBUF;                                            //Se carga el valor de buffer en ULTIMO
        if (ULTIMO != FLAG_SPI){                                    // Se revisa si es señal de comunicación o información
        CCPR = map(ULTIMO, IN_MIN, IN_MAX, OUT_MIN, OUT_MAX);       // Se genera el ancho de pulso respecto a los valores en ULTIMO
        CCPR1L = (uint8_t)(CCPR>>2);                                // 8 bits mas significativos en CPR1L
        CCP1CONbits.DC1B = CCPR & 0b11;                             // comunicación de datos con CCP1
        }
        PIR1bits.SSPIF = 0;                                         // Limpieza de bandera de SPI
    }
    
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main(void) {
    setup();
    while(1){
        
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
void setup(void){
    //Configuración Entradas / Salidas
    ANSEL = 0;                       // AN0 entrada analógica
    ANSELH = 0;                      // I/O digitales
    TRISA = 0b00100000;              // SS y RA0 como entradas       
    PORTA = 0;                       // Limpieza PORTA
    
    
    // Configuración Oscilador
    OSCCONbits.IRCF = 0b110;        // 4MHz
    OSCCONbits.SCS = 1;             // Oscilador interno
    
   
    // Configuración PWM
    TRISCbits.TRISC2 = 1;           // Deshabilitamos salida de CCP1
    PR2 = 200;                      // periodo de 2ms
    
    // Configuración CCP1
    CCP1CON = 0;                    // Apagamos CCP1
    CCP1CONbits.P1M = 0;            // Modo single output
    CCP1CONbits.CCP1M = 0b1100;     // PWM
    CCPR1L = 125>>2;
    CCP1CONbits.DC1B = 125 & 0b11;  // 25% duty cycle
    
    // Configuración TMR2
    PIR1bits.TMR2IF = 0;           // Limpieza de Bandera
    T2CONbits.T2CKPS = 0b11;       // prescaler 1:16
    T2CONbits.TMR2ON = 1;          // ON TMR2
    while(!PIR1bits.TMR2IF);       // Esperar un ciclo del TMR2
    PIR1bits.TMR2IF = 0;           // Limpieza de Bandera

    //Configuración SPI (Slave)
    TRISC = 0b00011000;         // -> SDI y SCK entradas, SD0 como salida
    PORTC = 0;
    // SSPCON <5:0>
    SSPCONbits.SSPM = 0b0100;   // -> SPI Esclavo, SS entrada o salida
    SSPCONbits.CKP = 0;         // -> Reloj inactivo en 0
    SSPCONbits.SSPEN = 1;       // -> Habilitamos pines de SPI
    // SSPSTAT<7:6>
    SSPSTATbits.CKE = 1;        // -> Dato enviado cada flanco de subida
    SSPSTATbits.SMP = 0;        // -> Dato al final del pulso de reloj (Slave))

    // Configuraciones de interrupciones
    PIR1bits.SSPIF = 0;         // Limpieza bandera SPI
    PIE1bits.SSPIE = 1;         // Habilitación interrupciones SPI
    INTCONbits.GIE = 1;         // Habilitación interrupciones globales
    INTCONbits.PEIE = 1;        // Habilitación interrupciones de perifericos
    
}

/* Función para hacer la interpolación lineal del valor de la entrada analógica */

unsigned short map(uint8_t x, uint8_t x0, uint8_t x1, 
            unsigned short y0, unsigned short y1){
    return (unsigned short)(y0+((float)(y1-y0)/(x1-x0))*(x-x0));
}
