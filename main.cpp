/*! \mainpage Ejercicio Titulo
 * \date 18/09/2023
 * \author Benitti, Mariano Gabriel
 * \section genDesc Descripcion general
 * [Complete aqui con su descripcion]
 *
 * \section desarrollos Observaciones generales
 * [Complete aqui con sus observaciones]
 *
 * \section changelog Registro de cambios
 *
 * |   Fecha    | Descripcion                                    |
 * |:----------:|:-----------------------------------------------|
 * | 01/01/2023 | Creacion del documento                         |
 *
 */



/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "Ticker.h"
#include "stdlib.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/
typedef enum{EST_U=0,EST_N,EST_E,EST_R,NUMBYTES,TOKEN,PAYLOAD,CHECKSUM}e_estCMD;
//____________________________________________________________________________
typedef union{
    struct {
       uint8_t b0:1;//se usa para saber si hay bytes leidos
       uint8_t b1:1;
       uint8_t b2:1;
       uint8_t b3:1;
       uint8_t b4:1;
       uint8_t b5:1;
       uint8_t b6:1;
       uint8_t b7:1;
    }bit;
    uint8_t byte;
}band;
//______________________________________________________________________________
typedef volatile struct __attribute__((packed,aligned(1))){
uint8_t iRL;//indice de lectura
uint8_t iRE;//indice de escritura
e_estCMD estDecode;//estado del decodificado del comando
uint8_t nBytes;//numero de bytes de datos
uint8_t iDatos;//inicio de los datos en el buffer
uint8_t idCMD;//id del comando leido
uint8_t *bufL;//puntero al buffer con los datos leidos
uint8_t checksum;//checksum del comando
uint8_t iChecksum;//indice del checksum
uint8_t tamBuffer;//tamaño que limita el buffer debe ser de 2 a la n para que funcione correctamente la compuerta and de reinicio
}s_LDatos;//estructura de lectura

typedef volatile struct __attribute__((packed,aligned(1))){
uint8_t iTL;//indice de escritura en el buffer
uint8_t iTE;//indice de lectura para transmitir los datos
uint8_t checksum;//checksum del comando
uint8_t *bufE;//puntero al buffer de escritura
uint8_t tamBuffer;//tamaño del buffer de escritura, debe ser de 2 a la n
}s_EDatos;//estructura de escritura
/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/
#define ISNEWBYTE banderas.bit.b0
/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);
BusIn BOTONES(PA_7,PA_6,PA_5,PA_4);
BusOut LEDS(PB_6,PB_7,PB_14,PB_15);
RawSerial PC(PA_9,PA_10);
/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/
/** \fn OnRxByte
 * \brief 
 *  Verifica si hay algo para leer en el buffer de entrada, si hay algo para leer lo recibe y 
 * lo pasa al buffer de memoria para poder analizarlo.
 * */
void OnRxByte();
/** \fn DecodeCMD
 * \brief 
 *  Esta funcion revisa si se ingresa un comando, detecta el header y una vez detectado
 *  prepara la estructura que se utilizara para analizarlo posteriormente en otra funcion
 * */
void DecodeCMD(s_LDatos *datosCMD);

/** \fn ExecuteCMD
 * \brief 
 *  Esta funcion ejecuta el comando con los datos ingresados en la estructura
 *  una vez determinado el comando por su id procede a ejecutarlo
 * */
void ExecuteCMD(s_LDatos *datosCMD);

void TickerGen();

void DecodeCMD();
/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
Ticker tickerGen;
band banderas;//generamos una variable de banderas
//generamos el buffer de Lectura
uint8_t bufferL[64];
s_LDatos datosLec;

//generamos el buffer de Escritura
uint8_t bufferE[64];
s_EDatos datosEsc;
/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/
void OnRxByte(){
    while (PC.readable()){
      datosLec.bufL[datosLec.iRE]=PC.getc();
       datosLec.iRE++;
       if(datosLec.iRE>=datosLec.tamBuffer){
        datosLec.iRE=0;
       }
        ISNEWBYTE=1;
    }
}

void TickerGen(){
    LED = !LED;
}

void DecodeCMD(){
    unsigned char indiceE;
    indiceE=datosLec.iRE;//guardamos la posicion del buffer de escritura hasta donde esta actualmente
    while(datosLec.iRL!=indiceE){
        switch (datosLec.estDecode){
        case EST_U:
                 if(datosLec.bufL[datosLec.iRL]=='U'){
                    datosLec.estDecode=EST_N;
                    
                    }
                    
            break;
        case EST_N:if(datosLec.bufL[datosLec.iRL]=='N'){
                    datosLec.estDecode=EST_E;
                    
                    }else{
                        if(datosLec.iRL>0){
                            datosLec.iRL--;
                        }else{
                            datosLec.iRL=datosLec.tamBuffer-1;
                        }
                         datosLec.estDecode=EST_U;
                    }
            break;
        case EST_E:if(datosLec.bufL[datosLec.iRL]=='E'){
                    datosLec.estDecode=EST_R;
                    
                    }else{
                        if(datosLec.iRL>0){
                            datosLec.iRL--;
                        }else{
                            datosLec.iRL=datosLec.tamBuffer-1;
                        }
                         datosLec.estDecode=EST_U;
                    }
            break;
        case EST_R:if(datosLec.bufL[datosLec.iRL]=='R'){
                    datosLec.estDecode=NUMBYTES;
                    
                    }else{
                        if(datosLec.iRL>0){
                            datosLec.iRL--;
                        }else{
                            datosLec.iRL=datosLec.tamBuffer-1;
                        }
                         datosLec.estDecode=EST_U;
                    }
            break;
        case NUMBYTES:
                        
                        PC.putc('G');
                            
                        datosLec.estDecode=EST_U;
                        
                        if(datosLec.iRL>0){
                            datosLec.iRL--;
                        }else{
                            datosLec.iRL=datosLec.tamBuffer-1;
                        }
                        
            break;
        default:datosLec.estDecode=EST_U;
            break;
        }
        datosLec.iRL++;
        if(datosLec.iRL>=datosLec.tamBuffer){
            datosLec.iRL=0;
        }
    }
}
/* END Function prototypes user code ------------------------------------------*/

int main()
{
/* Local variables -----------------------------------------------------------*/

/* END Local variables -------------------------------------------------------*/


/* User code -----------------------------------------------------------------*/
    //ENLAZAMOS EL TICKER
    tickerGen.attach(&TickerGen,200*0.001);//seleccionamos al ticker un intervalo de 10 ms
    //ENLAZAMOS EL PUERTO SERIE
    PC.baud(115200);//ASIGNAMOS LA VELOCIDAD DE COMUNICACION
    PC.attach(&OnRxByte,SerialBase::IrqType::RxIrq);//ENLAZAMOS LA FUNCION CON EL METODO DE LECTURA
    //INICIALIZAMOS BUFFER DE ENTRADA
    datosLec.iRL=0;
    datosLec.iRE=0;
    datosLec.nBytes=0;
    datosLec.iDatos=0;
    datosLec.checksum=0;
    datosLec.estDecode=EST_U;
    datosLec.tamBuffer=64;
    datosLec.bufL=bufferL;
    datosLec.iChecksum=0;
    //INICIALIZAOS EL BUFFER DE SALIDA
    datosEsc.iTL=0;
    datosEsc.iTE=0;
    datosEsc.checksum=0;
    datosEsc.bufE=bufferE;
    datosEsc.tamBuffer=64;
    while(1){
    	
    	if (datosLec.iRE!=datosLec.iRL)
        {
            //PC.putc(datosLec.bufL[datosLec.iRL]);
            //datosLec.iRL++;
            //if(datosLec.iRL>=datosLec.tamBuffer){
              //  datosLec.iRL=0;
            //}
            DecodeCMD();
        }
        
           
        

    }

/* END User code -------------------------------------------------------------*/
}


