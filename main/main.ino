/*
   El ESP8266 debe estar pre-configurado a 115200 Baudios
   Conectar el ESP8266 de acuerdo al diagrama UNO_ESP_JSON
*/

#include <SoftwareSerial.h>

SoftwareSerial mySerial(3, 2); //Pines: RX conectado a D3, TX conectado a D2

//
#define DEBUG_ESP8266 //Comentar si no se quiere imprimir la respuesta del ESP8266
#define ESP8266_OK //Confirmar si el comando AT fue recibido

#define SSID   "FMC_Sala"
#define PASS   "25912496"

//****************************************** FUNCIONES****************************************************************//

void connectToWiFi() {       // Función que permite conectarse al servidor específicado en el string server
  SendCmd("ATE1", 60);//Disables echo
  
  //SendCmd("AT+GSLP=1500", 60);//Enter Deepsleep for 1.5s

  //Begins client Mode
  SendCmd("AT+CWMODE=1", 60);
  
  // Comando AT para la conexión WiFi seleccionada con el SSID y PASS
  String AP = "AT+CWJAP=\""; AP += SSID; AP += "\",\""; AP += PASS; AP += "\"";
  SendCmd(AP, 60);

  //Enables up to 4 connections
  SendCmd("AT+CIPMUX=1", 60);

  //Start Server on port 333
  SendCmd("AT+CIPSERVER=1", 60);
}

void resetESP() {
  Serial.println("RST");
  mySerial.println("AT+RST");        // Deshabilita el echo de los comandos enviados
  delay(20);
  connectToWiFi();                 // Función para conectar el módulo ESP8266 a la red WiFi selecccionada
}

void leer() {
  while (mySerial.available()) {
    String recibido = "";
    if (mySerial.available()) {
      recibido += (char)mySerial.read();
    }
    Serial.print(recibido);
  }
}

bool findOK() {                     //Función que permite verificar el resultado "OK" del comando AT
  if (mySerial.find("OK"))         // Si se localiza OK en la respuesta del ESP8266
  {
    Serial.println("OK");
    return true;                    // Devuelve "True"
  }
  else
  {
    //Serial.println("!OK");
    return false;                   // Retorna "False"
  }
}

void SendCmd (String ATcmd, int Tespera) {

#ifdef DEBUG_ESP8266
  mySerial.println(ATcmd);
  delay(10); //Tiempo a esperar para abrir el puerto mySerial
  leer();
#endif

#ifdef ESP8266_OK
  while (!findOK()) {
    mySerial.println(ATcmd);
    delay(Tespera); //Tiempo a esperar para abrir el puerto mySerial
  }
#endif
  delay(60);
}

//----------------------------------------------------------------------------------------------------------------

bool askApprovedID(){

  return true;
}



//-----------------------------------------------------------------------------------------------------------------
void setup() {

  Serial.begin(38400);             // Inicializacion del Monitor Serial a 115200 para Debugeo
  mySerial.begin(38400);           // Inicializacion de un puerto serial para conectarse a Wifi

  connectToWiFi();                 // Función para conectar el módulo ESP8266 a la red WiFi selecccionada
  delay(20);
}

void loop() {
  //Chequea si hay datos a leer
  leer();
}
