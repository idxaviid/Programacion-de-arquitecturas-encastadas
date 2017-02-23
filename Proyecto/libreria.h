/*
 * libreria.h
 *
 *  Created on: 02/05/2016
 *      Author: Xavi Cano, Claudio Cárcamo
 */

#ifndef LIBRERIA_H_
#define LIBRERIA_H_

typedef unsigned char byte;

#define TXD0_READY (UCA0IFG & UCTXIFG)
#define RXD0_READY (UCA0IFG & UCRXIFG)

/* Estructura que devuelve RxPacket */
typedef struct RxReturn {	//Estructura que devuelve RxPacket
	byte StatusPacket[32];	//Array de datos
	int TimeOut;			//Comprobador de tiempo de respuesta

} RxReturn;


/* Cabeceras de funciones */
void init_UCS(void);
void Init_UART(void);
void init_Timer(void);
void init_CONST();
void config_TIMER_A1();
void Sentit_Dades_Rx(void);
void Sentit_Dades_Tx(void);
void TxUAC0(byte bTxdData);
byte TxPacket(byte bID, byte bParameterLength, byte bInstruction);
void mover_atras();
void mover_adelante();
void girar_derecha();
void girar_izquierda();
void detener();
int get_center_sensor_data();
int get_right_sensor_data();
int get_left_sensor_data();
#endif /* LIBRERIA_H_ */
