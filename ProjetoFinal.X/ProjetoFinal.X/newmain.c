#include <xc.h>
#include <stdio.h>
#include "nxlcd.h" 

// Configurações dos bits de configuração
#pragma config FOSC = HS        // Oscilador HS
#pragma config WDT = OFF        // Watchdog Timer desligado
#pragma config LVP = OFF        // Desabilita o ICSP de baixa voltagem
#pragma config PBADEN = OFF     // Porta B configurada como I/O digital ao resetar

#define _XTAL_FREQ 20000000 // Frequência do oscilador

// Definição das portas
#define POT_UMIDADE PORTAbits.RA0
#define POT_PH PORTAbits.RA1
#define POT_TEMP PORTAbits.RA2
#define RELAY_UMIDADE PORTCbits.RC0
#define RELAY_PH PORTCbits.RC1
#define RELAY_TEMP PORTCbits.RC2

int umidade, ph, temp;
int limite_UR = 50, limite_Tc = 20, limite_Ph = 7;

int LEADC(int channel) {
    ADCON0bits.CHS = channel; 
    __delay_ms(2); 
    ADCON0bits.GO = 1; 
    while (ADCON0bits.GO); 
    return (ADRESH << 8) + ADRESL; 
}

void controleAgua(int UR) {
    if (UR < limite_UR){
        RELAY_UMIDADE = 1;      
    } else {
        RELAY_UMIDADE = 0;
    }
}

void controlePh(int Ph){
    if (Ph > limite_Ph) {
        RELAY_PH = 1;       
    } else {
        RELAY_PH = 0;
    }
} 

void controleTc(int Tc){
    if (Tc > limite_Tc) {
        RELAY_TEMP = 1;        
    } else {
        RELAY_TEMP = 0;
    }
}


char Keypad() {
    
    PORTB = 0b11110111;
    if (!PORTBbits.RB4) return '1';
    if (!PORTBbits.RB5) return '4';
    if (!PORTBbits.RB6) return '7';
    if (!PORTBbits.RB7) return '*';
    
    PORTB = 0b11111011;
    if (!PORTBbits.RB4) return '2';
    if (!PORTBbits.RB5) return '5';
    if (!PORTBbits.RB6) return '8';
    if (!PORTBbits.RB7) return '0';
    
    PORTB = 0b11111101;
    if (!PORTBbits.RB4) return '3';
    if (!PORTBbits.RB5) return '6';
    if (!PORTBbits.RB6) return '9';
    if (!PORTBbits.RB7) return '#';
    
    PORTB = 0b11111110;
    if (!PORTBbits.RB4) return 'A';
    if (!PORTBbits.RB5) return 'B';
    if (!PORTBbits.RB6) return 'C';
    if (!PORTBbits.RB7) return 'D';
    
    return '\0';
}

int readNumber() {
    char key = '\0';
    int num = 0;
    //char valorStr[16];
   // sprintf(valorStr, "%d", num);
    
    while (1) {  // Mudança na condição de loop para um loop infinito que quebra internamente
        key = Keypad();  // Atualize key dentro do loop
        if (key == '#') {
            break;  // Sai do loop se o caractere for '#'
        }
        if (key >= '0' && key <= '9') {
            num = num * 10 + (key - '0');
        }
       // WriteCmdXLCD(0xC0);
       // putsXLCD(valorStr);
        __delay_ms(200); // Debounce delay
    }
    return num;
}


int limitante() {
    char op;
    int valor;
   
    WriteCmdXLCD(0x01);
    WriteCmdXLCD(0x80);
    putsXLCD("Carregando");
    __delay_ms(1000);
    WriteCmdXLCD(0x01);
 
    while(1){
        WriteCmdXLCD(0x80);  
        putsXLCD("A-UR B-Ph C-Tc");
        op = Keypad();
        if (op == 'A' || op == 'B' || op == 'C') {
            WriteCmdXLCD(0xC0);
            putsXLCD("Valor:");
            valor = readNumber();
            if (op == 'A') limite_UR = valor;
            if (op == 'B') limite_Ph = valor;
            if (op == 'C') limite_Tc = valor;
            putsXLCD("Atualizado");
            __delay_ms(2000);
            break;
        }
        if(op == '*') break;
        __delay_ms(100);
    }
    return NULL;
}


//Principal 

void main(void) {
    
    TRISA = 0xFF; // Porta A como entrada
    TRISC = 0x00; // Porta C como saída
    
    ADCON0bits.ADON = 1; // Liga o módulo ADC
    ADCON1 = 0x0D; 
    ADCON2 = 0xA9;
    
    TRISB = 0b11110000; 
    
    OpenXLCD(FOUR_BIT & LINES_5X7); 
	WriteCmdXLCD(0x01);  	 	
	__delay_ms(10);
    
    
    while(1){
        
    WriteCmdXLCD(0x01);  	 	
    umidade = LEADC(3); // Lê e converte o valor do ADC do canal 0 (umidade)
    temp = LEADC(1); // Lê e converte o valor do ADC do canal 1 (temperatura)
    ph = LEADC(2); // Lê e converte o valor do ADC do canal 2 (pH)
    
    umidade = (umidade*100)/1023;
    ph = (ph*14)/1023; 
    temp = (temp*30)/1023;     
    __delay_ms(2);
    
    // Buffers para armazenar as strings formatadas
    char umidadeStr[16], phStr[16], tempStr[16];

    // Formata os valores das variáveis em strings
    sprintf(umidadeStr, "%d", umidade);
    sprintf(phStr, "%d", ph);
    sprintf(tempStr, "%d", temp);

    WriteCmdXLCD(0x80);  
    putsXLCD ("UR:");
    WriteCmdXLCD(0xC0);  
    putsXLCD(umidadeStr);
    WriteCmdXLCD(0xC3);
    putsXLCD("%");
    
    WriteCmdXLCD(0x86);  
    putsXLCD ("Tc:");
    WriteCmdXLCD(0xC6); 
    putsXLCD(tempStr);    
    WriteCmdXLCD(0xC8);
    putsXLCD("C");

    WriteCmdXLCD(0x8C);  
    putsXLCD ("Ph:");
    WriteCmdXLCD(0xCC); 
    putsXLCD(phStr);   
    WriteCmdXLCD(0xCF); 
    
    controleAgua(umidade);
    controlePh(ph);
    controleTc(temp);
    
    if(Keypad() == '*') limitante();

    __delay_ms(500);
    }
}
