//Para usar la librería FHT.h hay que declarar los #define antes del #include
#define FHT_N 256 //Establece el tamaño de las FHT. Lo establezco en 256 y luego trataré de bajarlo para ver
                  //si usando la de 16 posiciones puedo representarlo en tiras de LED´s o en una matriz
#define LOG_OUT 1 //Se usa la amplitud que registrará cada uno de las 256 posiciones en dB
#include <FHT.h>//http://wiki.openmusiclabs.com/wiki/ArduinoFHT

///En estas lineas definimos las macros para definir los registros del ADC. http://electronics.arunsworld.com/basic-avr/
//The sbi() is a macro to set the bit(the second argument) of the address(the first argument) to 1.
//It sets the bit-th bit of the content of sfr to 1.
//The _SFR_BYTE() is a macro that returns a byte of data of the specified address. The _BV() is a macro that shifts 1 to left by the specified numnber.
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))//Establece bit a bit el registro sfr
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))//Limpiamos bit a bi el registro sfr(special function register)

void acelerar_ADC(){//En esta función vamos a modificar el ADC para aumentar su frecuencia de meustreo, que se cumpla Nyquist y por tanto podamos muestrear señales de hasta 19 kHz
                    //https://maxembedded.com/2011/06/the-adc-of-the-avr/
  
  ADCSRA = 0xE5; //Prescaler de 32  // 11100101
                 //Bit 7 – ADEN – ADC Enable
                 //Bit 6 – ADSC – ADC Start Conversion
                 //Bit 5 – ADATE – ADC Auto Trigger Enable
                 //Bit 4 – ADIF – ADC Interrupt Flag
                 //Bit 3 – ADIE – ADC Interrupt Enable
                 //Bits 2:0 – ADPS2:0 – ADC Prescaler Select Bits                 
  ADMUX = 0x5; // ADC 5 sin configurar Vref // 00000101
               //Bits 7:6 – REFS1:0 – Reference Selection Bits: 00-->Internal Vref off
               //Bit 5 – ADLAR – ADC Left Adjust Result
               //Bits 4:0 – MUX4:0 – Analog Channel and Gain Selection Bits: 00101-->ADC5
               
  DIDR0 = 0x20; // DIDR0: Digital Input Disable Register 0. Para deshabilitar la entrada digital de los pines analógicos.
  analogReference(EXTERNAL); //Cambiamos la referencia analógica con este comando. Hay que aplicar el voltaje en AREF y que nunca supere los 5V
                             //Nosotros usaremos 3.3V para que la precisión sea mayor, y así aprovechamos el propio pin de 3,3V de arduino
  sbi(ADCSRA, ADPS2);//Ponemos a 1 el bit ADPS2
  cbi(ADCSRA, ADPS1);//Ponemos a 0 el bit ADPS1
  sbi(ADCSRA, ADPS0);//Ponemos a 1 el bit ADPS0
}

void datosFHT(){
  ///Primero guardaremos los 256 datos, aunque realmente solo vayamos a usar la mitad
  for(int i; i<FHT_N; i++){
    while(!ADCSRA & _BV(ADIF)));//Esperamos a que esté preparado el ADC (ADIF salta cuando está completa la conversión)
    sbi(ADCSRA, ADIF);//Reseteamos el ADC
    byte valorADCAlto = ADCH;//Guardamos los valores del ADC, tanto el alto como el bajo
    byte valorADCBajo = ADCL;
    int valor_total = ((int)valorADCAlto << 8) | valorADCBajo;// Aquí estamos juntando en una nueva variable los bytes recogidos del ADC
                                                              //Ya que estos se guardan en dos bytes distintos. Desplazamos los altos 8 posiciones
                                                              //a la izquierda y le sumamos los valores bajos. De esta manera tenemos un int con 
                                                              //el valor completo registrado por el ADC
    valor_total -= 0x0200;//Con esta sentencia convertirmos el in en un int con signo
    valor_total <<= 6; // Se convierte a signed int de 16 bits
    /*
     Estas dos últimas conversiones que hemos realizados se deben a que una señal de audio es tanto
     positiva como negativa. El ADC solo funciona con valores positivos, por lo que se le suman 2,5V a
     la señal para centrarla entre 0V y 5V. Al hacer la operación -=0x0200, lo que estamos haciendo
     es restarle a la señal 512, es decir, eliminamos los 2,5V añadidos anteriormente por el ADC.
     Por último, al convertir el resultado a un entero de 16 bits, lo que hacemos es obtener respuestas
     mas precisas de la FHT, ya que esta trabaja con valores de +-32767
     */

     fht_input[i] = valor_total//Almacenamos la lectura en el vector FHT, el cual ya viene definido en la biblioteca
                               //Esta matriz contiene 1 valor de 16 bits por cada punto de datos de FHT
  }
  fht_window();//esta función multiplica los datos de entrada por una función de ventana para ayudar a 
               //aumentar la resolución de frecuencia de los datos de la FHT. Esta función procesa
               //los datos en fht_input, por lo que los datos deben colocarse primero en esa matriz 
               //antes de llamarlos. También debe llamarse antes de fht_reorder() o fht_run()
  fht_reorder();//Esto reordena las entradas FHT para prepararlas para ser procesadas por el algoritmo
                //FHT que procesa los datos. Se debe llamar a esta función antes de fht_run. Esto se ejecutará
                //en la matriz fht_input[], por lo que los datos deben completarse primero en esa matriz antes
                //de llamar a esta función
  fht_run();//Esta función es la llamada principal a la función FHT. Asume que ya hay un bloque de datos
            //en SRAM y que ya está reordenado
  fht_mag_log();//Esta función nos da la amplitud de los datos almacenados en la matriz en decibelios
}

void setup() {
  // put your setup code here, to run once:
  acelerar_ADC();///Configuramos el ADC
}

void loop() {
  // put your main code here, to run repeatedly:
  datosFHT();//Llamamos a la función que realiza los cálculos de la FHT

}