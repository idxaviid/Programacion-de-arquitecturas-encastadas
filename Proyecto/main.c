//Xavi Cano
//Claudio Cárcamo


#include <msp430x54xA.h>
#include <stdio.h>
#include "hal_lcd.h"
#include "libreria.h"
char saludo[17]= "PROYECTO";
char cadena[17];
char borrado[] = "                 ";
unsigned char contraste   = 0x64;
unsigned char iluminacion  = 30;
unsigned char linea=1;
char estado=0;
long int i;
long int adelante;
int flag_GI = 0;
int flag_GD = 0;
int pared = 0;
int obstaculo = 0;
long int derecha;
long int izquierda;

/**************************************************************************
 * BORRAR LINEA
 *
 * Datos de entrada: Linea, indica la linea a borrar
 *
 * Sin datos de salida
 *
 **************************************************************************/

void borrar(unsigned char Linea)
{
	halLcdPrintLine(borrado, Linea, OVERWRITE_TEXT); //incluimos una linea en blanco
}


/**************************************************************************
 * ESCRIBIR LINEA
 *
 * Datos de entrada: Linea, indica la linea a escribir
 * 					 String, cadena de caracteres que vamos a introducir
 *
 * Sin datos de salida
 *
 **************************************************************************/

void escribir(char String[], unsigned char Linea)

{
	/* Superponemos la nueva palabra introducida, haya o no algo. */
	halLcdPrintLine(String, Linea, OVERWRITE_TEXT); 
}

/**************************************************************************
 * INICIALIZACIÓN DE LOS BOTONES.
 * Configuramos botones y leds:
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 **************************************************************************/

void init_botons(void)
{
  /* Configuramos botones y leds: */

  P1DIR |= 0x03;	//Puertos P1.0 y P1.1 como salidas (Leds)
  P1OUT |= 0x00;	//Inicializamos puerto P1.0 a 1,
  P1OUT &= 0xFD;	// y P1.1 a 0, para leds en alternancia


  P2DIR &= ~0xC0;	//Puertos P2.6 y P2.7 como entradas (botones S1 y S2)
  P2SEL &= ~0xC0;	//Puertos P2.6 y P2.7 como I/O digitales,
  P2REN |= 0xC0;	//con resistencia activada
  P2OUT |= 0xC0;	// de pull-up
  P2IE |= 0xC0; 	//Interrupciones activadas en P2.6 y P2.7,
  P2IES &= ~0xC0;	// con transicion L->H


  /* Configuramos el joystick: */
  P2DIR &= ~0x3E;	//Puertos P2.1 a P2.5 como entradas (joystick)
  P2SEL &= ~0x3E;	//Puertos P2.1 y P2.5 como I/O digitales,
  P2REN |= 0x3E;	//con resistencia activada
  P2OUT |= 0x3E;	// de pull-up
  P2IE |= 0x3E; 	//Interrupciones activadas en P2.1 a P2.5,
  P2IES &= ~0x3E;	// con transicion L->H

}

/*****************************************************************************
 * CONFIGURACIÓN DEL PUERTO 4, PARA ILUMINAR LOS LEDS. A REALIZAR POR EL ALUMNO
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 ****************************************************************************/

void config_P4_LEDS (void)
{
	/* Puertos P4.0 a P4.7 configuradas como entradas/salidas. Ponemos a 0 todos los bits */
	P4SEL &= 0x00; 
	/* Puertos P4.0 a P4.7 configurados como salidas. Ponemos a 1 todos los bits */
	P4DIR |= 0xFF; 
	/* Inicializamos P4.0 a P4.7 apagados. Ponemos a 0 todos los bits, para apagar los leds */
	P4OUT &= 0x00; 
}

/**************************************************************************
 * INICIALIZACIÓN DE PANTALLA.
 * Inicializamos la pantallita LCD:
 *
 * Sin datos de entrada
 *
 * Sin datos de salida
 *
 **************************************************************************/

void init_LCD(void)
{

  /* Programa interno para iniciar la pantalla */
  halLcdInit();  
  /* Inicio de Iluminación posterior de la pantalla */
  halLcdBackLightInit(); 
  /* Determinación de la Iluminación posterior de la pantalla */
  halLcdSetBackLight(iluminacion); 
  /* Establecimiento del contraste */
  halLcdSetContrast(contraste); 
  /* Limpiar (borrar) la pantalla */
  halLcdClearScreen();  
}



void main(void)
		/* Paramos el watchdog timer */
		WDTCTL = WDTPW+WDTHOLD;       	
		/* Iniciamos los botones y Leds. */
	  	init_botons();
	  	/* Iniciamos la UCS */
	  	init_UCS();
	  	/* Iniciamos la UART */
	  	 init_LCD();
	  	/* Activamos la salida de los leds P4 */
	  	Init_UART();
	  	init_CONST();					// Inicializamos variables
	  	config_TIMER_A1();				// Configuramos el TimerA1
	  	/* Activamos las interrupciones a nivel global del chip */
	    _enable_interrupt();
	    //config_P4_LEDS();
	    /* Escribimos saludo en la primera linea */
	    escribir(saludo,linea); 
	    /* Aumentamos el valor de linea y con ello pasamos a la linea siguiente */			
	  	linea++;
	  	do
  	        {
	  			derecha = get_right_sensor_data();
				adelante = get_center_sensor_data();
				izquierda = get_left_sensor_data();
	  			escribir(borrado, 3);
				sprintf(cadena,"DERECHA: %d", derecha); 	// Guardamos en cadena lo siguiente frase: estado "valor del estado"
				escribir(cadena,3);
				escribir(borrado, 4);
				sprintf(cadena,"CENTRO: %d", adelante); 	// Guardamos en cadena lo siguiente frase: estado "valor del estado"
				escribir(cadena,4);
				escribir(borrado, 5);
				sprintf(cadena,"IZQUIERDA: %d", izquierda); 	// Guardamos en cadena lo siguiente frase: estado "valor del estado"
				escribir(cadena,5);
				escribir(borrado, 6);
				if(estado == 1){
					//si hay un obstaculo,y estamos en movimiento, giramos a la derecha y creamos un obstaculo
					if(adelante == 255 && flag_GD == 0 && flag_GI == 0){
						girar_derecha();
						obstaculo = 1;
					}
					//si el sensor no capta obstaculos delanteros, y estabamos girando despues de encontrar un obstaculo
					//movemos el sensor hacia delante y quitamos el obstaculo creado
					else if(adelante == 0 && obstaculo == 1){
						obstaculo = 0;
						mover_adelante();
					}
					//sin no se cumplen ninguna de las otras condiciones y no hay obstaculo comprobamos la pared derecha
					else if(obstaculo == 0){
						//si el rango del sensor va entre 50 y 100 el robor debe avanzar
						if(izquierda <= 100 && izquierda >= 20 ){
							mover_adelante();
							flag_GD = 0;
							flag_GI = 0;
						}
						// si el sensor se esta acercando a la pared y el no robot no esta girando a la derecha flag_GD=1
						//giramos a la derecha
						else if(izquierda >100  && flag_GI == 0){
							girar_derecha();
							flag_GD =1;
						}
						// si el robot ya habia encontrado una pared pared = 1, se aleja de la pared y no esta girando a la izquierda flag_GI =1
						//giramos a la izquierda
						else if(izquierda <20 && izquierda !=0 && flag_GD == 0){
							 girar_izquierda();
							 flag_GI =1;
						}
					}
	  			}
	  			if(estado == 2){
	  						//si hay un obstaculo,y estamos en movimiento, giramos a la izquierda y creamos un obstaculo
	  						if(adelante == 255 && flag_GD == 0 && flag_GI == 0){
	  							girar_izquierda();
	  							obstaculo = 1;
	  						}
	  						//si el sensor no capta obstaculos delanteros, y estabamos girando despues de encontrar un obstaculo
	  						//movemos el sensor hacia delante y quitamos el obstaculo creado
	  						else if(adelante == 0 && obstaculo == 1){
	  							obstaculo = 0;
	  							mover_adelante();
	  						}
	  						//sin no se cumplen ninguna de las otras condiciones y no hay obstaculo comprobamos la pared derecha
	  						else if(obstaculo == 0){
	  							//si el rango del sensor va entre 50 y 100 el robor debe avanzar
	  							if(derecha <= 100 && derecha >= 20 ){
	  								mover_adelante();
	  								flag_GD = 0;
	  								flag_GI = 0;
	  							}
	  							// si el sensor se esta acercando a la pared y el no robot no esta girando a la derecha flag_GD=1
	  							//giramos a la izquierda
	  							else if(derecha >100  && flag_GD == 0){
	  								girar_izquierda();
	  								flag_GI =1;
	  							}
	  							// si el robot ya habia encontrado una pared pared = 1, se aleja de la pared y no esta girando a la izquierda flag_GI =1
	  							//giramos a la derecha
	  							else if(derecha <20 && flag_GI == 0){
	  								 girar_derecha();
	  								 flag_GD =1;
	  							}
	  						}
				}
  	        }while(1);
}


/**************************************************************************
 * MINIPROGRAMA DE LOS BOTONES:
 * Mediante este programa, se detectará que botón se ha pulsado
 *
 * Sin Datos de entrada
 *
 * Sin datos de salida
 *
 * Actualiza el valor estado
 *
 **************************************************************************/

/* Interrupción de los botones. Actualiza el valor de la variable global estado. */
#pragma vector=PORT2_VECTOR  
__interrupt void Port2_ISR(void)
{
	/* Interrupciones botones S1 y S2 (P2.6 y P2.7) desactivadas */
	P2IE &= 0xC0; 	
	/* Interrupciones joystick (2.1-2.5) desactivadas */
	P2IE &= 0x3E;   

	switch(P2IFG){

		/* Boton S1 */
		case 0x40: 
			estado = 1; 
			break;

		/* Boton S2 */
		case 0x80: 
			estado = 2;
			break;

		/* Izquierda */
		case 0x02: 
			estado = 3;
			break;

		/* Derecha */
		case 0x04: 
			estado = 4;
			break;

		/* Centro */
		case 0x08: 
			estado = 5;
			break;

		/* Arriba */
		case 0x10: 
			estado = 6;
			break;

		/* Abajo */

		case 0x20: 
			estado = 7;
			break;
	}



	/***********************************************
   	 * HASTA AQUI BLOQUE CASE
   	 ***********************************************/

   	/* Limpiamos todas las interrupciones */ 
	P2IFG = 0;	
	/* Interrupciones botones S1 y S2 (P2.6 y P2.7) reactivadas */
	P2IE |= 0xC0; 	
	/* Interrupciones joystick (2.1-2.5) reactivadas */
	P2IE |= 0x3E;  
 return;
}
