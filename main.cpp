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
typedef enum{DERECHA=0,IZQUIERDA,ARRIBA,ABAJO}e_Muros;
typedef enum{XPARAM=0x00,YPARAM=0x01,VELPARAM=0x02,ANGPARAM=0x03}e_Parametros;
typedef enum{ALIVEID=0xF0,ACKID=0xF0,FIRMWAREID=0XF1,LEDSID=0x10,BUTTONSID=0x12,CONTROLID=0x17,WALLID=0x18}e_IDSCMD;
//_____________________________________________________________________________
typedef enum{UP,FALLING,RISSING,DOWN}e_estadoB;
typedef union{
uint8_t uint8[4];
int8_t int8[4];
uint16_t uint16[2];
int16_t int16[2];
uint32_t uint32;
int32_t int32;
}u_ConvDatos;

typedef struct __attribute__((packed,aligned(1))){
e_estadoB e_estadoBoton;
uint8_t presion;//indica si el boton se presiono
uint32_t Tpresion;//indica el tiempo que se presiono
}boton;
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
#define ISNEWCMD banderas.bit.b1


/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);
BusIn BOTONES(PA_7,PA_6,PA_5,PA_4);
BusOut LEDS(PB_6,PB_7,PB_14,PB_15);
RawSerial PC(PA_9,PA_10);
/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/
/** \fn ComprobarBotones
 * \brief 
 *  Esta funcion comprueba el estado de los 4 botones que estan dentro del bus de entrada
 *  una vez comprobado el estado dentro de la estructura global de los botones establece el estado del boton,
 *  si fue presionado y cuanto tiempo estubo presionado.
 * */
void ComprobarBotones();
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
void DecodeCMD();

/** \fn ExecuteCMD
 * \brief 
 *  Esta funcion ejecuta el comando con los datos ingresados en la estructura
 *  una vez determinado el comando por su id procede a ejecutarlo
 * \param[in]: datosCMD es la estructura donde estan todos los datos leidos
 * */
void ExecuteCMD(s_LDatos *datosCMD);

/** \fn ColocarHeader
 * \brief 
 *  Esta funcion carga en el buffer de escritura el proximo comando a enviar a la pc
 * 
 * \param[in]: datosE es la estructura del buffer de escritura
 * \param[in]: ID es el id del comando que se preparara para enviar
 * \param[in]: nbytes es el numero de bytes de datos del comando es decir el length
 * */
void ColocarHeader(s_EDatos *datosE,uint8_t ID,uint8_t nBytes);

/** \fn ColocarPayload
 * \brief 
 *  Esta funcion carga en el buffer de escritura el proximo comando a enviar a la pc
 * 
 * \param[in]: datosE es la estructura del buffer de escritura
 * \param[in]: ID es el id del comando que se preparara para enviar
 * \param[in]: string es la cadena de datos sin el id
 * \param[in]: nDatos es la cantidad de datos de la cadena
 * */
void ColocarPayload(s_EDatos *datosE,uint8_t *string,uint8_t nDatos);

void CambiarLeds(uint8_t LedstoAct,uint8_t LedsValue);

void WallCommand(uint8_t wall);

void ButtonsState();

void ControlLanzamiento(uint8_t IdParam,int16_t value);

void TickerGen();


/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/
//CONTADORES DE TIEMPO
uint8_t tdeLectura=0;
uint8_t tdeHeartBeat=0;
uint8_t tdeApagarLed[4]={0,0,0,0};
uint8_t tdeReiniBoton=0;
uint8_t tdeControlBotones=0;
//variable para comvertir datos
u_ConvDatos convTipos;
//ticker
Ticker tickerGen;
band banderas;//generamos una variable de banderas
//generamos el buffer de Lectura
uint8_t bufferL[64];
s_LDatos datosLec;

//generamos el buffer de Escritura
uint8_t bufferE[64];
s_EDatos datosEsc;

//generamos los 4 botones a evaluar
boton botones[4];//tiene los datos de cada boton en particular

/* END Global variables ------------------------------------------------------*/


/* Function prototypes user code ----------------------------------------------*/
void ComprobarBotones(){
    uint8_t i;
    if(tdeReiniBoton==0){
            tdeReiniBoton=4;
            for(i=0;i<4;i++){
                switch(botones[i].e_estadoBoton){
                    case UP:
                            if(BOTONES & (1<<i)){
                                botones[i].e_estadoBoton=FALLING;
                            }
                        break;
                    case FALLING:
                            if(BOTONES & (1<<i)){
                                botones[i].e_estadoBoton=DOWN;
                                botones[i].presion=1;
                                botones[i].Tpresion=0;//reiniciamos el contador de tiempo
                            }
                        break;
                    case DOWN:  botones[i].Tpresion+=4;//cada vez que ingresa se le suma 4 indicando que pasaron 40 ms mas
                                if((BOTONES & (1<<i))==0){
                                     botones[i].e_estadoBoton=RISSING;
                                 }
                                
                        break;    
                    case RISSING:if((BOTONES & (1<<i))==0){
                                    botones[i].e_estadoBoton=UP;
                                    botones[i].Tpresion=0;//reiniciamos el contador de tiempo
                                    }
                        break;
                    default:botones[i].e_estadoBoton=UP;
                        break;
                }

            }
        }
}

void OnRxByte(){
    while (PC.readable()){
      datosLec.bufL[datosLec.iRE]=PC.getc();
       datosLec.iRE++;
       if(datosLec.iRE>=datosLec.tamBuffer){
        datosLec.iRE=0;
       }
    }
}

void TickerGen(){
    uint8_t i;

    if(tdeHeartBeat){
        tdeHeartBeat--;
        
    }else{
        LED = !LED;
        tdeHeartBeat=20;
    }
    if(tdeLectura){
        tdeLectura--;
    }
    if(tdeReiniBoton){
        tdeReiniBoton--;
    }
    if(tdeControlBotones){
        tdeControlBotones--;
    }
    for(i=0;i<4;i++){
        if(tdeApagarLed[i]){
            tdeApagarLed[i]--;
            if(tdeApagarLed[i]==0){
                CambiarLeds(1<<i,0);//apagamos el led que estaba prendido
            }
        }
    }
    

}

void DecodeCMD(){
    unsigned char indiceE;
    indiceE=datosLec.iRE;//guardamos la posicion del buffer de escritura hasta donde esta actualmente
   if(tdeLectura){
    tdeLectura=7;
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
        case NUMBYTES: datosLec.nBytes=datosLec.bufL[datosLec.iRL];
                       datosLec.estDecode=TOKEN;
                       
            break;
        case TOKEN:if(datosLec.bufL[datosLec.iRL]==':'){
                    datosLec.estDecode=PAYLOAD;
                    }else{
                        if(datosLec.iRL>0){
                            datosLec.iRL--;
                        }else{
                            datosLec.iRL=datosLec.tamBuffer-1;
                        }
                         datosLec.estDecode=EST_U;
                    }
            break;
        case PAYLOAD: 
                      datosLec.idCMD=datosLec.bufL[datosLec.iRL];//guardo la id del comando
                      datosLec.iDatos=datosLec.iRL;//guardo la posicion de los datos
                      datosLec.checksum='U'^'N'^'E'^'R'^datosLec.nBytes^':'^datosLec.idCMD;//inicializamos el checksum
                      datosLec.iChecksum=datosLec.iDatos+datosLec.nBytes-1;//guardo la posicion del checksum esperada
                      datosLec.estDecode=CHECKSUM;
            break;
        case CHECKSUM:
                        if(datosLec.iRL!=datosLec.iChecksum){
                            datosLec.checksum=datosLec.checksum^datosLec.bufL[datosLec.iRL];
                        }else{
                            if(datosLec.bufL[datosLec.iRL]==datosLec.checksum){
                                ISNEWCMD=1;
                            }else{
                                if(datosLec.iRL>0){//VOLVEMOS A VERIFICAR SI ES UNA U
                                    datosLec.iRL--;
                                }else{
                                    datosLec.iRL=datosLec.tamBuffer-1;
                                }
                            }
                            datosLec.estDecode=EST_U;
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
   }else{
    tdeLectura=4;//si paso demasiado tiempo entre la anterior recepcion
    datosLec.estDecode=EST_U;
   }
}

void ExecuteCMD(s_LDatos *datosCMD){
    uint8_t str[35];
    switch (datosCMD->idCMD){
    case ALIVEID://alive
               ColocarHeader(&datosEsc,ACKID,3);
               str[0]=0x0D;
               ColocarPayload(&datosEsc,str,1);
        break;
    case FIRMWAREID://firmware
               ColocarHeader(&datosEsc,FIRMWAREID,3);
               str[0]='A';
               ColocarPayload(&datosEsc,str,1);
        break;
    case LEDSID:CambiarLeds(datosLec.bufL[datosLec.iDatos+1],datosLec.bufL[datosLec.iDatos+2]);
        break;
    case WALLID:WallCommand(datosLec.bufL[datosLec.iDatos+1]);
        break;
    case BUTTONSID:ButtonsState();
        break;
    default:   //COMANDO NO CONOCIDO
               ColocarHeader(&datosEsc,datosCMD->idCMD,3);
               str[0]=0xFF;
               ColocarPayload(&datosEsc,str,1);
        break;
    }
}

void ColocarHeader(s_EDatos *datosE,uint8_t ID,uint8_t nBytes){
    datosE->bufE[datosE->iTE]='U';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]='N';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]='E';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]='R';
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]=nBytes;
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->bufE[datosE->iTE]=':';
    datosE->iTE++;
   if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
     datosE->bufE[datosE->iTE]=ID;
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
        datosE->iTE=0;
    }
    datosE->checksum='U'^'N'^'E'^'R'^nBytes^':'^ID;//inicializamos el checksum
}


void ColocarPayload(s_EDatos *datosE,uint8_t *string,uint8_t nDatos){
    uint8_t i;
    for(i=0;i<nDatos;i++){
        datosE->checksum=datosE->checksum^string[i];
        datosE->bufE[datosE->iTE]=string[i];
        datosE->iTE++;
        if(datosE->iTE>=datosE->tamBuffer){
            datosE->iTE=0;
        }
    }
    datosE->bufE[datosE->iTE]=datosE->checksum;
    datosE->iTE++;
    if(datosE->iTE>=datosE->tamBuffer){
            datosE->iTE=0;
        }
}

void CambiarLeds(uint8_t LedstoAct,uint8_t LedsValue){
     uint8_t i;
     for(i=0;i<4;i++){
        if(LedstoAct & 1<<i){//verificamos si hay que actualizar ese led
           
            if(LedsValue & 1<<i){//verificamos si hay que encenderlo o apagarlo
                LEDS=LEDS & ~(1<<i);//encendemos el led
            }else{
                LEDS=LEDS | 1 <<i;//apagamos el led
            }
        }
    }
}

void WallCommand(uint8_t wall){
    
    uint8_t led=0;
    switch(wall){
        case DERECHA:led=3;//seleccionamos el led a encender
            break;
        case IZQUIERDA:led=0;//seleccionamos el led a encender
            break;
        case ARRIBA:led=1;//seleccionamos el led a encender
            break;
        case ABAJO:led=2;//seleccionamos el led a encender
            break;
    }
    LEDS=LEDS & ~(1<<led);//encendemos el led del muro chocado
    tdeApagarLed[led]=5;
}

void ButtonsState(){
    uint8_t estadobotones=0;
    ColocarHeader(&datosEsc,BUTTONSID,3);
    estadobotones=BOTONES;
    ColocarPayload(&datosEsc,&estadobotones,1);//lo puedo pasar como una string aunque no sea un vector porque lo paso como puntero y solo cargo un elemento
}

void ControlLanzamiento(uint8_t IdParam,int16_t value){
    uint8_t str[3];
    ColocarHeader(&datosEsc,CONTROLID,5);
    str[0]=IdParam;
    convTipos.int16[0]=value;
    str[1]=convTipos.uint8[0];
    str[2]=convTipos.uint8[1];
    ColocarPayload(&datosEsc,str,3);
}

void ControlBotones(){
   
        //AUMENTA LA VELOCIDAD DE SALIDA
        if(botones[0].presion && botones[2].presion){
            ControlLanzamiento(VELPARAM,1);
            botones[0].presion=0;
            botones[2].presion=0;
        }
        
        //DISMINUYE LA VELOCIDAD DE SALIDA
        if(botones[1].presion && botones[2].presion){
            ControlLanzamiento(VELPARAM,-1);
            botones[1].presion=0;
            botones[2].presion=0;
        }

        //AUMENTA EL ANGULO DE SALIDA
        if(botones[0].presion && botones[3].presion){
            ControlLanzamiento(ANGPARAM,1);
            botones[0].presion=0;
            botones[3].presion=0;
        }
        //DISMINUYE EL ANGULO DE SALIDA
        if(botones[1].presion && botones[3].presion){
            ControlLanzamiento(ANGPARAM,-1);
            botones[1].presion=0;
            botones[3].presion=0;
        }

        

        if(tdeControlBotones==0){
        tdeControlBotones=15;
        //DEZPLAZAMOS LA PELOTA HACIA ARRIBA
        if(botones[0].presion){
            ControlLanzamiento(YPARAM,1);
            botones[0].presion=0;
        }
        //DEZPLAZAMOS LA PELOTA HACIA ABAJO
        if(botones[1].presion){
            ControlLanzamiento(YPARAM,-1);
            botones[1].presion=0;
        }
        //DEZPLAZAMOS LA PELOTA HACIA LA IZQUIERDA
        if(botones[2].presion){
            ControlLanzamiento(XPARAM,-1);
            botones[2].presion=0;
        }
        //DEZPLAZAMOS LA PELOTA HACIA LA IZQUIERDA
        if(botones[3].presion){
            ControlLanzamiento(XPARAM,1);
            botones[3].presion=0;
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
    tickerGen.attach_us(&TickerGen,10*1000);//seleccionamos al ticker un intervalo de 1000 ms en microseg
    //ENLAZAMOS EL PUERTO SERIE
    PC.baud(115200);//ASIGNAMOS LA VELOCIDAD DE COMUNICACION
    PC.attach(&OnRxByte,SerialBase::IrqType::RxIrq);//ENLAZAMOS LA FUNCION CON EL METODO DE LECTURA
    
    LEDS=0xF;
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
    	
        
        ComprobarBotones();
        ControlBotones();
        


    	if (datosLec.iRE!=datosLec.iRL){
            DecodeCMD();
        }
        if(ISNEWCMD){
            ExecuteCMD(&datosLec);
            ISNEWCMD=0;
        }
        
        if(datosEsc.iTE!=datosEsc.iTL){//si hay para escribir se manda a la pc
            if(PC.writeable()){
                PC.putc(datosEsc.bufE[datosEsc.iTL]);
                datosEsc.iTL++;
                if(datosEsc.iTL>=datosEsc.tamBuffer){
                    datosEsc.iTL=0;
                }

            }
        }
           
        

    }

/* END User code -------------------------------------------------------------*/
}


