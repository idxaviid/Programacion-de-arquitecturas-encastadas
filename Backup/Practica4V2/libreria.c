/*
 * libreria.c
 *
 *  Created on: 02/05/2016
 *      Author: mat.aules
 */
#include <msp430x54xA.h>
#include "libreria.h"

int Byte_Recibido;		    /* Comprobacion de que hemos recibido byte */
byte DatoLeido_UART;		/* Dato que leemos del modulo */
byte gbpParameter[32];
byte gbpTxBuffer[32];
int timer;
struct RxReturn answer;
int velocidad = 80;
int timerB_contador;

void init_UCS(void)
{
	 /* Inicialització de les freqüències de rellotge del microcontrolador */
	 unsigned long DCORSEL = DCORSEL_7; //DCORSEL_6 selecciona rango de DCO de 7 a 60 MHz
	 unsigned short FLLN = 487; //Parametro N
	 __bis_SR_register(SCG0); // Desactiva el FLL control loop
	 UCSCTL0 = 0x00; // Posa DCOx, MODx al mínim possible
	 UCSCTL1 = DCORSEL; // Seleccionem el rang de DCO 16 MHz
	 UCSCTL2 = FLLN + FLLD_1; //Selecciona el factor multiplicador del DCO
	 UCSCTL3 = 0; //Referència FLL SELREF = XT1, divisor =1;

	 /* Selecció de la font de rellotge: ACLK el Clock extern de 215, SMCLK i MCLK el DCO intern de 16MHz */
	 UCSCTL4 = SELA__XT1CLK | SELS__DCOCLKDIV | SELM__DCOCLKDIV ;
	 UCSCTL5 = DIVA__1 | DIVS__1; //Divisor per SMCLK: f(SMCLK)/1 i ACLK: f(ACLK)/1
	 __bic_SR_register(SCG0); // Enable the FLL control loop
	 P11DIR = 0x07; //Configurem els pins 11.0, 11.1 y 11.2 com sortida
	 P11SEL = 0x07; //ACLK, MCLK y SMCLK; activem els pins de test
}




void Init_UART(void)
{
	 UCA0CTL1 |= UCSWRST; //Fem un reset de la USCI i es desactiva
	 UCA0CTL0 = 0x00;
	 	 	 	 	 //UCSYNC=0 mode asíncron
					 //UCMODEx=0 seleccionem mode UART
					 //UCSPB=0 nomes 1 stop bit
					 //UC7BIT=0 8 bits de dades
					 //UCMSB=0 bit de menys pes primer
					 //UCPAR=x no es fa servir bit de paritat
					 //UCPEN=0 sense bit de paritat
	 UCA0CTL1 |= UCSSEL__SMCLK; //Triem SMCLK (16MHz) com a font del clock BRCLK
	 UCA0BR0 = 1; //Prescaler de BRCLK fixat a 1 (LO byte = 1, ...
	 UCA0BR1 = 0; //... HI byte = 0).
	 UCA0MCTL = UCOS16; // No necessitem modulació (divisió entera), però
	// si oversampling => bit 0 = UCOS16 = 1.
	 // El Baud rate és BRCLK/16=1MBps
	 P3SEL |= 0x30; //I/O funcion: P3.4 = UART0TX, P3.5 = UART0RX
	 P3REN |= 0x20; //amb resistència activada de pull-up l’entrada P3.5
	 P3OUT |= 0x20; //
	 P3DIR |= 0x80; //Port P3.7 com sortida (Data Direction: selector Tx/Rx)
	 P3SEL &= ~0x80; //Port 3.7 com I/O digital
	 P3OUT &= ~0x80; //Inicialitzem el port P3.7 a 0 (Rx)
	 UCA0CTL1 &= ~UCSWRST; //Reactivem la línia de comunicacions sèrie
	 UCA0IE |= UCRXIE; //Això s’ha d’activà només quan fem la recepció
 }



void Sentit_Dades_Rx(void)
{ //Configuració del Half Duplex dels motors: Recepció
	P3OUT &= ~0x80; //El pin P3.7 (DIRECTION_PORT) el posem a 0 (Rx)
}


void Sentit_Dades_Tx(void)
{
	//Configuració del Half Duplex dels motors: Transmissió
	P3OUT |= 0x80; //El pin P3.7 (DIRECTION_PORT) el posem a 1 (Tx)
}


void init_Timer(void){
	timer = 0;
}

/*
 * Funcion que inicializa las variables para poder usar la libreria
 */
void init_VARS(){
	Byte_Recibido = 0;
	init_Timer();
	timerB_contador=0;
}


/* Funció TxUAC0(byte): envia un byte de dades per la UART 0 */
void TxUAC0(byte bTxdData)
{
	while(!TXD0_READY); // Espera a que estigui preparat el buffer de transmissió
	UCA0TXBUF = bTxdData;
}


byte Get_Byte_Leido_UART(){
	while (!RXD0_READY);
	return UCA0RXBUF;
}


int TimeOut(long int time){
	if (timer < time) { 					//Si el tiempo de timer es menor que el tiempo que le pasamos reornamos 0 True
			return 0;
		} else {							//Si es mas grande desactivamos las inerupcione si retornamos 1 False
			//xxx
			//TA1CCTL0 &= ~CCIE;				//Desactivamos interrupcion
			return 1;
		}
}



/*
TxPacket() necessita 3 paràmetres; ID del Dynamixel, Instruction byte, Mida dels paràmetres.
TxPacket() torna la mida del "Return packet" des del Dynamixel.
*/

byte TxPacket(byte bID, byte bParameterLength, byte bInstruction) {
	 byte bCount,bCheckSum,bPacketLength;
	 volatile int i = 50;
	 Sentit_Dades_Tx(); //El pin P3.7 (DIRECTION_PORT) el posem a 1 (Tx)
	 gbpTxBuffer[0] = 0xff; //Primers 2 bytes que indiquen inici de trama FF, FF.
	 gbpTxBuffer[1] = 0xff;
	 gbpTxBuffer[2] = bID; //ID del mòdul al que volem enviar el missatge
	 gbpTxBuffer[3] = bParameterLength+2; //Length(Parameter,Instruction,Checksum)
	 gbpTxBuffer[4] = bInstruction; //Instrucció que enviem al mòdul
	 for(bCount = 0; bCount < bParameterLength; bCount++)//Comencem a generar la trama
	 {
		 gbpTxBuffer[bCount+5] = gbpParameter[bCount];
	 }
	 bCheckSum = 0;
	 bPacketLength = bParameterLength+4+2;
	 for(bCount = 2; bCount < bPacketLength-1; bCount++) //Càlcul del Checksum
	 {
		 bCheckSum += gbpTxBuffer[bCount];
	 }
	 gbpTxBuffer[bCount] = ~bCheckSum; //Escriu el Checksum (complement a 1)
	 for(bCount = 0; bCount < bPacketLength; bCount++) //Aquest bucle és el que envia la trama
	 {
		 TxUAC0(gbpTxBuffer[bCount]);
	 }
	 while((UCA0STAT&UCBUSY)); //Espera fins s’ha transmès el últim byte
	 Sentit_Dades_Rx(); //Posem la línia de dades en Rx perquè el mòdul Dynamixel envia resposta
	return(bPacketLength);
}

struct RxReturn RxPacket(void) {
	struct RxReturn respuesta;
	byte bCount, bLength, bChecksum;
	int t;
	Sentit_Dades_Rx(); 						//Ponemos la linea half duplex en Rx
	for(t = 0; t < 32; t++) respuesta.StatusPacket[t] = 0;
	//TA1CCTL0 = CCIE;						//Desactivamos las interupciones

	for (bCount = 0; bCount < 4; bCount++){ //bRxPacketLength; bCount++)
		init_Timer();								//reset timer
		respuesta.TimeOut = 0;					//reset timeout
		Byte_Recibido = 0; 					//No_se_ha_recibido_Byte();
		while (!Byte_Recibido){		    //Se_ha_recibido_Byte())
			respuesta.TimeOut= TimeOut(5000); //Seteamos el valor del TimeOut
			if (respuesta.TimeOut) break;					 //sale del while
		}
		if (respuesta.TimeOut) break; 						 //sale del for si ha habido Timeout
		//Si no, es que todo ha ido bien, y leemos un dato:
		respuesta.StatusPacket[bCount] = DatoLeido_UART; //Get_Byte_Leido_UART();
	} //fin del for
	bLength = respuesta.StatusPacket[3]+4;
	// Continua llegint la resta de bytes del Status Packet
	if (!respuesta.TimeOut){
		for (bCount = 4; bCount < bLength ; bCount++){ //bRxPacketLength; bCount++)
			init_Timer();								//reset timer
			respuesta.TimeOut = 0;				//reset timeout
			Byte_Recibido = 0; 				//No_se_ha_recibido_Byte();
			while (!Byte_Recibido){		    //Se_ha_recibido_Byte())
				respuesta.TimeOut= TimeOut(5000); //Seteamos el valor del TimeOut
				if (respuesta.TimeOut) break;					 //sale del while
			}
			if (respuesta.TimeOut) break; 						 //sale del for si ha habido Timeout
			//Si no, es que todo ha ido bien, y leemos un dato:
			respuesta.StatusPacket[bCount] = DatoLeido_UART; //Get_Byte_Leido_UART();
		} //fin del for
	}
	if(bLength > 3){ //checking is available.
		if(respuesta.StatusPacket[0] != 0xff || respuesta.StatusPacket[1] != 0xff ) respuesta.HeaderError = 1;
		if(respuesta.StatusPacket[3] != bLength-4) respuesta.HeaderError = 1;
		//Calculem el checksum
		bChecksum = 0;
		for(bCount = 2; bCount < bLength; bCount++) bChecksum += respuesta.StatusPacket[bCount];
	}

	return respuesta;
}

void config_TIMER_A1() {
	//xxx
	TA1CTL = TASSEL_2 + MC_1;  				//SMCLK + modo UP
	TA1CCTL0 = CCIE;		   				//Habilitar interupciones
	TA1CCR0 = 1000;	   						//Interupcion cada 1ms
}



#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) { 		//Interrupcion de recepcion en la uart A0
	UCA0IE &= ~UCRXIE;					    //Interrupciones desactivadas en RX
	DatoLeido_UART = UCA0RXBUF;				//Guardamos el dato leido del buffer
	Byte_Recibido = 1;						//Activamos el flag de Byte_Recibido
	UCA0IE |= UCRXIE; 						//Interrupciones reactivadas en RX
}


#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void){
		timer++;
		timerB_contador++;
}

// datos de sensores
int get_left_sensor_data(){
	gbpParameter[0] = 0x1A;
	gbpParameter[1] = 0x01;

	TxPacket(100,0x02,0x02);
	answer = RxPacket();
	return answer.StatusPacket[5];
}
int get_center_sensor_data(){
	gbpParameter[0] = 0x1B;
	gbpParameter[1] = 0x01;

	TxPacket(100,0x02,0x02);
	answer = RxPacket();
	return answer.StatusPacket[5];
}
int get_right_sensor_data(){
	gbpParameter[0] = 0x1C;
	gbpParameter[1] = 0x01;

	TxPacket(100,0x02,0x02);
	answer = RxPacket();
	return answer.StatusPacket[5];
}
void mover_atras(){
	gbpParameter[0] = 0x20;				//Direccion de memoria donde queremos escribir los parametros
	gbpParameter[1] = velocidad;		//Primer parametro parte baja velocidad
	gbpParameter[2] = 0x01;
	TxPacket(1,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(2,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	gbpParameter[2] = 0x05;
	TxPacket(3,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(4,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
}
void mover_adelante(){
	gbpParameter[0] = 0x20;				//Direccion de memoria donde queremos escribir los parametros
	gbpParameter[1] = velocidad;		//Primer parametro parte baja velocidad
	gbpParameter[2] = 0x05;
	TxPacket(1,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(2,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	gbpParameter[2] = 0x01;
	TxPacket(3,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(4,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
}
void girar_derecha(){
	gbpParameter[0] = 0x20;				//Direccion de memoria donde queremos escribir los parametros
	gbpParameter[1] = velocidad;		//Primer parametro parte baja velocidad
	gbpParameter[2] = 0x01;
	TxPacket(1,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(2,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
}
void girar_izquierda(){
	gbpParameter[0] = 0x20;				//Direccion de memoria donde queremos escribir los parametros
	gbpParameter[1] = velocidad;				//Primer parametro parte baja velocidad
	gbpParameter[2] = 0x05;
	TxPacket(3,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(4,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
}
void detener(){
	gbpParameter[0] = 0x20;				//Direccion de memoria donde queremos escribir los parametros
	gbpParameter[1] = 0x00;				//Primer parametro parte baja velocidad
	gbpParameter[2] = 0x00;
	TxPacket(1,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(2,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(3,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
	TxPacket(4,0x03,0x03);		        //ID,Numero de parametros, R/W
	RxPacket();
}

void subir_velocidad(){
	if(velocidad < 80){
		velocidad = velocidad + 20;
	}
}

void bajar_velocidad(){
	if(velocidad > 0){
		velocidad = velocidad - 20;
	}
}
