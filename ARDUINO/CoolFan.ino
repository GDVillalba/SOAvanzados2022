//#include  <Wire.h>
#include  <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Habilitacion de debug para la impresion por el puerto serial ...
//----------------------------------------------
#define SERIAL_DEBUG_ENABLED 0

#if SERIAL_DEBUG_ENABLED
  #define DebugPrint(str)\
    {\
       Serial.println(str);\
    }
#else
  #define DebugPrint(str)
#endif

#define DebugPrintEstado(estado,evento)\
    {\
       String est = estado;\
       String evt = evento;\
       String str;\
       str = "-----------------------------------------------------";\
       DebugPrint(str);\
       str = "De Estado: [ " + est + " ] y Evento: [ " + evt + " ] pasa a ->";\
       DebugPrint(str);\
       str = "-----------------------------------------------------";\
       DebugPrint(str);\
    }
#define DebugPrintEstadoActual(estado)\
    {\
       String est = estado;\
       String str;\
       str = "Estado actual: [ " + estado + " ].";\
       DebugPrint(str);\
    }
//----------------------------------------------
// Para pasar el valor del sensor LM35 a grados centigrados
#define pasarAGradosCelsius(lectura) (((((lectura) * 5.0) / 1024)*100))

//----------------------------------------------
// Definicion de pines
#define PIN_SENSOR_TEMPERATURA                  0
#define PIN_PULSADOR_MAS                        2
#define PIN_PULSADOR_MENOS                      3
#define PIN_PULSADOR_VELOCIDAD                  4
#define PIN_PULSADOR_ON_OFF                     5
#define PIN_TRANSISTOR_MOTOR                    6
#define PIN_TRANSISTOR_ON_OFF                   7
#define PIN_RX_BT                              10
#define PIN_TX_BT                              11

#define I2C_ADDR                             0x27
#define I2C_EN                                  2
#define I2C_RW                                  1
#define I2C_RS                                  0
#define I2C_D4                                  4
#define I2C_D5                                  5
#define I2C_D6                                  6
#define I2C_D7                                  7

//----------------------------------------------

//----------------------------------------------
// Constantes 
#define SERIAL_SPEED                         9600
#define BT_SPEED                             9600

#define MAX_TERMOSTATO                         35
#define INIT_TERMOSTATO                        24
#define MIN_TERMOSTATO                         18
#define MAX_VEL_255                           255
#define MIN_VEL_150                           150
#define MAX_VELOCIDAD                           3
#define MIN_VELOCIDAD                           1
#define DELTA_VELOCIDAD     ((MAX_VEL_255 - MIN_VEL_150) / MAX_VELOCIDAD)
#define MOTOR_REPOSO                            0
#define MOTOR_APAGADO                           1
#define MOTOR_ENCENDIDO                         2

//Defines para pasar el estado por bluetooth
#define REPOSO                                '0'
#define ACTIVO                                '1'

#define CANT_SENSORES                           5
#define SENSOR_TEMPERATURA                      0
#define SENSOR_PULSADOR_ON_OFF                  1
#define SENSOR_PULSADOR_VELOCIDAD               2
#define SENSOR_PULSADOR_TERMOSTATO_MENOS        3
#define SENSOR_PULSADOR_TERMOSTATO_MAS          4

#define VALOR_0                                 0
#define COLUMNAS_LCD                           16
#define FILAS_LCD                               2
#define LCD_COLUMNA_0                           0
#define LCD_COLUMNA_8                           8
#define LCD_COLUMNA_13                         13
#define LCD_FILA_0                              0
#define LCD_FILA_1                              1
#define DELTA_TIMEOUT                         100

//Mensajes de bluetooth recibidos
#define BT_CMD_UPDATE_TEMP                    '0'
#define BT_CMD_ON_OFF                         '1'
#define BT_CMD_CAMBIO_VELOCIDAD               '2'
#define BT_CMD_TERM_DOWN                      '3'
#define BT_CMD_TERM_UP                        '4'
#define BT_CMD_INIT                           '5'
#define BT_END_MSG                            '~' //con esto damos a conocer la finalización del String de datos

//Calculo TIMER_FACTOR1024 = ((16000000 * (t/1000) ) / 1024) - 1
#define TIMER_FACTOR1024(t)  (long)(((float)15.625 * t) -1)

//Tiempo en milisegundos para configurar la interrupción del timer 1
#define TIEMPO_MAX_MILIS 1000
// Para configurar en cero al registro TCCR1A
#define CONFIG_TCCR1A_0                0b00000000
// Para configurar el registro TCCR1B, compara con OCR1A y usa factor escala 1024.
#define CONFIG_TCCR1B_OCR1A_1024       0b00001101

//----------------------------------------------
// Definicion de estructuras
//----------------------------------------------
struct st_valores_globales
  {
    int Temperatura;
    int Termostato;
    int Velocidad;
    char Estado;       //Para enviar por bluetooth si esta en reposo o activo
  };

struct stSensor
  {
    int  pin;
    int valor;
  };
  
  struct stMotor
  {
    int pin_velocidad;
    int pin_on_off;
  int estado;
  };

//----------------------------------------------
// Variables Globales
//----------------------------------------------
st_valores_globales value;
stSensor sensores[CANT_SENSORES];
stMotor motor;

char ComandoBTRecibido = 0;      // variable para almacenar caracter recibido por bluetooth
String Respuesta = "";

//declaracion e inicializacion de la variable LiquidCrystal para controlar el display.
LiquidCrystal_I2C lcd(I2C_ADDR, I2C_EN, I2C_RW, I2C_RS, I2C_D4, I2C_D5, I2C_D6, I2C_D7);

//declaracion e inicializacion de variable SoftwareSerial para comunicacion con bluetooth HC-06
SoftwareSerial miBT(PIN_RX_BT , 11);  // PIN_RX_BT recibe de HC-06, PIN_TX_BT envia a HC-06,

//################

//variables para buscar un nuevo evento cada timeout
bool timeout;
long lct;

//----------------------------------------------

enum states          { ST_INIT   , ST_REPOSO   ,  ST_ACTIVO_ON   , ST_ACTIVO_OFF   , ST_ERROR   } current_state;
String states_s [] = {"ST_INIT"  ,"ST_REPOSO"  , "ST_ACTIVO_ON"  ,"ST_ACTIVO_OFF"  ,"ST_ERROR"  };

enum events          { EV_CONT   , EV_UPDATE_TEMP   , EV_ON_OFF   , EV_CAMBIO_VELOCIDAD   , EV_TERM_DOWN   , EV_TERM_UP   , EV_MOTOR_CAMBIAR  , EV_BT_INIT  , EV_UNKNOW  } new_event;
String events_s [] = {"EV_CONT"  ,"EV_UPDATE_TEMP"  ,"EV_ON_OFF"  ,"EV_CAMBIO_VELOCIDAD"  ,"EV_TERM_DOWN"  ,"EV_TERM_UP"  ,"EV_MOTOR_CAMBIAR" ,"EV_BT_INIT" ,"EV_UNKNOW" };

#define MAX_STATES 5
#define MAX_EVENTS 8

typedef void (*transition)();



//----------------------------------------------
//###############################################
//### Funciones del temporizador por hardware ###
//###############################################
//----------------------------------------------

//Inicializacion del temporizador por HW
void inicializar_timer_por_HW()
  {                           
    // El pin OC1A cambia de estado tras la comparación.
    TCCR1A = CONFIG_TCCR1A_0;
    // Usa el registro OCR1A para comparar y factor escala 1024.
    TCCR1B = CONFIG_TCCR1B_OCR1A_1024;
    // Para que la interrupción ocurra al segundo.
    OCR1A = TIMER_FACTOR1024( TIEMPO_MAX_MILIS );
    // Se utilizara la interrupción timer1 por comparación.
    TIMSK1 = bit(OCIE1A);
    // Habilita las interrupciones globales.
    sei();
  }

//Funcion que lanza el temporizador por HW, se ejecuta cada 1 segundo aproximadamente.
ISR(TIMER1_COMPA_vect)
  {
    leerTemperatura();
  }

//----------------------------------------------
//##########################################################
//### Funciones para inicializar componentes y variables ###
//##########################################################
//----------------------------------------------
void do_init()
  {
    //inicia Monitor en serie
    //Serial.begin(SERIAL_SPEED);

    //inicia comunicacion serie entre Arduino y el modulo con velocidad BT_SPEED
    miBT.begin(BT_SPEED);
    
    //inicia pines de pulsadores
    pinMode(PIN_PULSADOR_ON_OFF, INPUT);
    pinMode(PIN_PULSADOR_VELOCIDAD, INPUT);
    pinMode(PIN_PULSADOR_MENOS, INPUT);
    pinMode(PIN_PULSADOR_MAS, INPUT);
    
    //inicia estructura de motor
  motor.pin_velocidad = PIN_TRANSISTOR_MOTOR;
  motor.pin_on_off = PIN_TRANSISTOR_ON_OFF;
  motor.estado = MOTOR_REPOSO;
  
  //inicia pines para controlar el motor
    pinMode(motor.pin_velocidad, OUTPUT);
    pinMode(motor.pin_on_off, OUTPUT);
    
    //inicia valores iniciales
    leerTemperatura();
    value.Velocidad = MIN_VELOCIDAD;
    value.Termostato = INIT_TERMOSTATO;
    value.Estado = REPOSO;
    
    //inicia estructuras de sensores
    sensores[SENSOR_TEMPERATURA].pin    = PIN_SENSOR_TEMPERATURA;
    sensores[SENSOR_TEMPERATURA].valor = value.Temperatura;
    
    sensores[SENSOR_PULSADOR_ON_OFF].pin    = PIN_PULSADOR_ON_OFF;
    sensores[SENSOR_PULSADOR_ON_OFF].valor = VALOR_0;
    
    sensores[SENSOR_PULSADOR_VELOCIDAD].pin    = PIN_PULSADOR_VELOCIDAD;
    sensores[SENSOR_PULSADOR_VELOCIDAD].valor = VALOR_0;
    
    sensores[SENSOR_PULSADOR_TERMOSTATO_MENOS].pin    = PIN_PULSADOR_MENOS;
    sensores[SENSOR_PULSADOR_TERMOSTATO_MENOS].valor = VALOR_0;
    
    sensores[SENSOR_PULSADOR_TERMOSTATO_MAS].pin    = PIN_PULSADOR_MAS;
    sensores[SENSOR_PULSADOR_TERMOSTATO_MAS].valor = VALOR_0;
    
    //escribe estado inicial del motor
    digitalWrite( motor.pin_on_off , LOW);
    analogWrite(motor.pin_velocidad, (value.Velocidad * DELTA_VELOCIDAD) + MIN_VEL_150);
    
    //inicio el display
    lcd.begin(COLUMNAS_LCD, FILAS_LCD);
    //#####
    lcd.setBacklightPin(3,POSITIVE);
    lcd.setBacklight(HIGH);
    
    // Inicializo el estado inicial de la maquina de estados
    current_state = ST_INIT;
    
    //inician variables auxiliares
    timeout = false;
    lct     = millis();
    dibujarDisplayOff();
    inicializar_timer_por_HW();
  }

//----------------------------------------------------------
//#########################################
//### Funciones para dibujar el Display ###
//#########################################
//----------------------------------------------------------

//Dibuja el display cuando el sistema esta en estado ST_ACTIVO_ON
void dibujarDisplayOn()
  {
    lcd.setCursor(LCD_COLUMNA_0,LCD_FILA_0);
    lcd.print("Vel:");
    lcd.print(value.Velocidad);
    lcd.setCursor(LCD_COLUMNA_8,LCD_FILA_0);
    lcd.print(" Ter: ");
    lcd.print(value.Termostato);
    lcd.setCursor(LCD_COLUMNA_0,LCD_FILA_1);
    //imprime espacios de mas para borrar la temperatura anterior
    lcd.print("Temperatura:    ");
    lcd.setCursor(LCD_COLUMNA_13,LCD_FILA_1); 
    lcd.print(value.Temperatura); 
  }

//Dibuja el display cuando el sistema esta en estado ST_ACTIVO_OFF
void dibujarDisplayOff()
  {
    lcd.setCursor(LCD_COLUMNA_0,LCD_FILA_0);
    lcd.print("                ");
    lcd.setCursor(LCD_COLUMNA_0,LCD_FILA_1);
    //imprime espacios de mas para borrar la temperatura anterior
    lcd.print("Temperatura:    ");
    lcd.setCursor(LCD_COLUMNA_13,LCD_FILA_1);
    lcd.print(value.Temperatura); 
  }

//----------------------------------------------------------
//##################################################
//### Funciones que modifican variables globales ###
//##################################################
//----------------------------------------------------------

//Actualiza el valor de la variable global de temperatura
void leerTemperatura()
  {
    value.Temperatura = pasarAGradosCelsius(analogRead(PIN_SENSOR_TEMPERATURA));

    Respuesta = BT_CMD_UPDATE_TEMP;
    Respuesta +=  value.Temperatura ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
  }
//----------------------------------------------------------
void cambioVelocidad()
  {    
    //Incrementa Velocidad en 1
    value.Velocidad = value.Velocidad + 1;
    //Si supera maxima velocidad vuelve al minimo
    if(value.Velocidad > MAX_VELOCIDAD)
      {
        value.Velocidad = MIN_VELOCIDAD;
      }
      
      if(value.Velocidad == MIN_VELOCIDAD)
      {
        analogWrite(motor.pin_velocidad, (value.Velocidad * DELTA_VELOCIDAD) + MIN_VEL_150 ); 
      }else{
       
    //establece la velocidad en el motor
    analogWrite(motor.pin_velocidad, (value.Velocidad * DELTA_VELOCIDAD) + MIN_VEL_150);    
      }

    Respuesta = BT_CMD_CAMBIO_VELOCIDAD;
    Respuesta +=  value.Velocidad ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
  }
//----------------------------------------------------------
void termostatoMenos()
  {
    //Decremento el termostato solo si no es el minimo
    if(value.Termostato > MIN_TERMOSTATO)
        value.Termostato--;
        
    Respuesta = BT_CMD_TERM_DOWN;
    Respuesta +=  value.Termostato ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
  }

//----------------------------------------------------------
void termostatoMas()
  {
    //Incremento el termostato solo si no es el maximo
    if(value.Termostato < MAX_TERMOSTATO)
        value.Termostato++;

    Respuesta = BT_CMD_TERM_UP;
    Respuesta +=  value.Termostato ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
  }
//----------------------------------------------------------
//########################################################
//### Funciones verificar eventos (para get_new_event) ###
//########################################################
//----------------------------------------------------------
bool verificarBT()
  {

    if (miBT.available()){      // si hay informacion disponible desde modulo
      ComandoBTRecibido = miBT.read();

      switch (ComandoBTRecibido)
        {
          case BT_CMD_UPDATE_TEMP :
              new_event = EV_UPDATE_TEMP;
              return true;
              break;
          case BT_CMD_ON_OFF :
              new_event = EV_ON_OFF;
              return true;
              break;
          case BT_CMD_CAMBIO_VELOCIDAD :
              new_event = EV_CAMBIO_VELOCIDAD;
              return true;
              break;
          case BT_CMD_TERM_DOWN :
              new_event = EV_TERM_DOWN;
              return true;
              break;
          case BT_CMD_TERM_UP :
              new_event = EV_TERM_UP;
              return true;
              break;
          case BT_CMD_INIT :
              new_event = EV_BT_INIT;
              return true;
              break;
          default:
               return false;
               break;
        }
      }
      
    return false;
  }
//----------------------------------------------------------
bool verificarTemperatura()
  {
    
    //Verifica si la temperatura cambio, para disparar el EV_UPDATE_TEMP
    if( value.Temperatura != sensores[SENSOR_TEMPERATURA].valor)
      {
        sensores[SENSOR_TEMPERATURA].valor = value.Temperatura;
        new_event = EV_UPDATE_TEMP;
        return true;
      }
    
    return false;
  }
//----------------------------------------------------------
bool verificarOnOff()
  {
    //Si el pulsador en PIN_PULSADOR_ON_OFF se presiona dispara el evento
    if(pulsadorPresionado(SENSOR_PULSADOR_ON_OFF))
      {        
        new_event = EV_ON_OFF;
        return true;
      }
    return false;
  }
//----------------------------------------------------------
bool verificarVel()
  {
    //Si el pulsador en PIN_PULSADOR_VELOCIDAD se presiona dispara el evento
    if(pulsadorPresionado(SENSOR_PULSADOR_VELOCIDAD))
      {
        new_event = EV_CAMBIO_VELOCIDAD;
        return true;
      }
    return false;
  }
//----------------------------------------------------------
bool verificarTerMenos()
  {
    //Si el pulsador en PIN_PULSADOR_MENOS se presiona dispara el evento
    if(pulsadorPresionado(SENSOR_PULSADOR_TERMOSTATO_MENOS))
      {
        new_event = EV_TERM_DOWN;
        return true;
      }
    return false;
  }
//----------------------------------------------------------
bool verificarTerMas()
  {
    //Si el pulsador en PIN_PULSADOR_MAS se presiona dispara el evento
    if(pulsadorPresionado(SENSOR_PULSADOR_TERMOSTATO_MAS))
      {
        new_event = EV_TERM_UP;
        return true;
      }
    return false;
  }
//----------------------------------------------------------
//Verifica que un pulsador se presione, solo envia true al detectar un cambio de LOW a HIGH
bool pulsadorPresionado(int pulsador)
  {
    int  lectura = digitalRead(sensores[pulsador].pin);
    //Verifica si hay cambio de estado en el pin
    if(sensores[pulsador].valor != lectura)
      {
        //Si cambio el estado se actualiza
        sensores[pulsador].valor = lectura;
        //verifico que sea un cambio de LOW a HIGH
        if( lectura == HIGH )
          {
            
            return true;
          }
      }
    return false;
  }
  
  bool verificarUmbralTermostato()
  {
    //Verifico si hay que cambiar el estado del motor en base al termostato y la temperatura
    if( (motor.estado == MOTOR_APAGADO && value.Termostato < value.Temperatura) || (motor.estado == MOTOR_ENCENDIDO && value.Termostato >= value.Temperatura) )
      {
        new_event = EV_MOTOR_CAMBIAR;
        return true;
      }
    
    return false;
  }
  
//----------------------------------------------------------
//############################
//### Funciones transition ###
//############################
//----------------------------------------------------------
//Inicializa el primer estado de ST_INIT a ST_REPOSO
void init_()
  {
    DebugPrintEstado(states_s[current_state], events_s[new_event]);
    
    current_state = ST_REPOSO;
    
    DebugPrintEstadoActual(states_s[current_state]);
  }
//----------------------------------------------------------
void error()
  {
    DebugPrint("#######   ERROR   ###########");
    //Serial.println("#######   ERROR   ###########");
  }
//----------------------------------------------------------
void none()
  {
    //No hace nada.
  }
//----------------------------------------------------------
//Funcion que actualiza la temperatura en el display cuando el sistema esta en estado ST_REPOSO
void update_temp()
  {
    //Actualizar display
    dibujarDisplayOff();
  }
  //Funcion que actualiza la temperatura en el display cuando el sistema esta en estado ST_ACTIVO_OFF o ST_ACTIVO_ON
void update_temp_on()
  {
    //Actualizar display
    dibujarDisplayOn();
  }
//----------------------------------------------------------
//Funcion para pasar del estado ST_REPOSO a alguno de los estados ST_ACTIVO_OFF
void activar()
  {
  motor.estado = MOTOR_APAGADO;
    current_state = ST_ACTIVO_OFF;
    value.Estado = ACTIVO;
    
    //Actualizar display
    dibujarDisplayOn();

    Respuesta = BT_CMD_ON_OFF;
    Respuesta +=  value.Estado ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
  }
//----------------------------------------------------------
//Funcion que se lanza cuando hay que apagar el motor porque se alcanzo la temperatura del termostato pasa de el estado ST_ACTIVO_ON, al estado ST_ACTIVO_OFF
void apagar_motor()
  {
    //Apago motor y cambio a estado ST_ACTIVO_OFF
    digitalWrite( motor.pin_on_off , LOW);
  motor.estado = MOTOR_APAGADO;
    current_state = ST_ACTIVO_OFF;
    
    //Actualizar display
    dibujarDisplayOn();
  }
//----------------------------------------------------------
//Funcion que se lanza cuando hay que prender el motor porque la temperatura del termostato es menor a la temperatura actual, pasa de el estado ST_ACTIVO_OFF, al estado ST_ACTIVO_ON
void prender_motor()
  {
    //Prendo motor y cambio a estado ST_ACTIVO_ON
    digitalWrite( motor.pin_on_off , HIGH);
  motor.estado = MOTOR_ENCENDIDO;
    current_state = ST_ACTIVO_ON;
    
    //Actualizar display
    dibujarDisplayOn();
  }
//----------------------------------------------------------
//Funcion para pasar de cualquiera de los estados ST_ACTIVO_OFF o ST_ACTIVO_ON, al estado ST_REPOSO
void reposar()
  {
    //Apago motor y cambio a estado ST_REPOSO
    digitalWrite( motor.pin_on_off , LOW);
  motor.estado = MOTOR_REPOSO;
    current_state = ST_REPOSO;
    value.Estado = REPOSO;
    //Actualizar display
    dibujarDisplayOff();

    Respuesta = BT_CMD_ON_OFF;
    Respuesta +=  value.Estado ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
  }
//----------------------------------------------------------
//Cambia la velocidad y actualiza el display
void update_vel()
  {
    cambioVelocidad();
    //Actualizar display
    dibujarDisplayOn();
  }
//----------------------------------------------------------
//Decrementa el termostato y actualiza el display
void t_abajo()
  {
    termostatoMenos();
    //Actualizar display
    dibujarDisplayOn();
  }  
//----------------------------------------------------------
//Incrementa el termostato y actualiza el display
void t_arriba()
  {
    termostatoMas();
    //Actualizar display
    dibujarDisplayOn();
  }
  //----------------------------------------------------------
//Envia por bluetooth los valores del arduino para que se inicializen en el android
void bt_init()
  {
    //Envia estado on/off
    Respuesta = BT_CMD_ON_OFF;
    Respuesta +=  value.Estado ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
    //Envia Temperatura
    Respuesta = BT_CMD_UPDATE_TEMP;
    Respuesta +=  value.Temperatura ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
    //Envia Termostato
    Respuesta = BT_CMD_TERM_UP;
    Respuesta +=  value.Termostato ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
    //Envia velocidad
    Respuesta = BT_CMD_CAMBIO_VELOCIDAD;
    Respuesta +=  value.Velocidad ;
    Respuesta +=  BT_END_MSG ;
    miBT.print( Respuesta );   // envia a Bluetooth
    
  }
//----------------------------------------------------------
//################################################
//### Funciones generales de estados y eventos ###
//################################################
//----------------------------------------------
void get_new_event( )
  {
    long ct = millis();
    int  diferencia = (ct - lct);
    timeout = (diferencia > DELTA_TIMEOUT)? (true):(false);
    
    if( timeout )
      {
        // Doy acuse de la recepcion del timeout
        timeout = false;
        lct     = ct;
        
        if( (verificarBT() == true) || (verificarTemperatura() == true) || (verificarOnOff() == true) || (verificarVel() == true) || (verificarTerMenos() == true) || (verificarTerMas() == true) || (verificarUmbralTermostato() == true))
          {
            return;
          }
      }
  
    // Genero evento dummy ....
    new_event = EV_CONT;
  }
//----------------------------------------------


//####################

transition state_table[MAX_STATES][MAX_EVENTS] =
  {
      {init_    , error           , error         , error               , error         , error       , error            , error       } , // state ST_INIT
      {none     , update_temp     , activar       , none                , none          , none        , none             ,bt_init    } , // state ST_REPOSO
      {none     , update_temp_on  , reposar       , update_vel          , t_abajo       , t_arriba    , apagar_motor     ,bt_init    } , // state ST_ACTIVO_ON
      {none     , update_temp_on  , reposar       , update_vel          , t_abajo       , t_arriba    , prender_motor    ,bt_init    } , // state ST_ACTIVO_OFF
      {error    , error           , error         , error               , error         , error       , error            , error       }   // state ST_ERROR
      
     //EV_CONT  , EV_UPDATE_TEMP  , EV_ON_OFF     , EV_CAMBIO_VELOCIDAD , EV_TERM_DOWN  , EV_TERM_UP  ,EV_MOTOR_CAMBIAR ,EV_BT_INIT
  };
  
//####################



void maquina_de_estados()
  {
    get_new_event();
    
    if( (new_event >= 0) && (new_event < MAX_EVENTS) && (current_state >= 0) && (current_state < MAX_STATES) )
      {
        if( new_event != EV_CONT )
          {
            DebugPrintEstado(states_s[current_state], events_s[new_event]);
          }
        
        state_table[current_state][new_event]();
        
        if( new_event != EV_CONT )
          {
            DebugPrintEstadoActual(states_s[current_state]);
          }
      }
    else
      {
        DebugPrintEstado(states_s[ST_ERROR], events_s[EV_UNKNOW]);
      }
  
    // Consumo el evento...
    new_event   = EV_CONT;
  }
//----------------------------------------------

//############################
//### Funciones de Arduino ###
//############################
//----------------------------------------------
void setup()
  {
    do_init(); 
  }
//----------------------------------------------
void loop()
  {
    maquina_de_estados();  
  }
