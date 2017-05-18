#include <SoftwareSerial.h>

//Pines: RX conectado a D3, TX conectado a D2
SoftwareSerial mySerial(3, 2);

#define SSID   "smartLockAP"
#define PASS   "pwd"

//Guarda el mensaje recibido por puerto serial
String recibido;


//****************************************** FUNCIONES****************************************************************//
/**
 * Función que permite conectarse al servidor específicado en el string server
 */
void connectToWiFi() {
  //Disables echo
  SendCmd("ATE1", 5);
  
  //SendCmd("AT+GSLP=1500", 5);//Enter Deepsleep for 1.5s

/*
  //Begins client Mode
  SendCmd("AT+CWMODE=1", 5);
  
  // Comando AT para la conexión WiFi seleccionada con el SSID y PASS
  String AP = "AT+CWJAP=\""; AP += SSID; AP += "\",\""; AP += PASS; AP += "\"";
  SendCmd(AP, 5);
*/
  //Starts its own AP
  SendCmd("AT+CWMODE=2", 5);

  //Configures its own AP
  String AP = "AT+CWSAP=\"";
  AP += SSID;
  AP += "\",\"";
  AP += PASS;
  AP += "\",5,0,4";
  
  SendCmd(AP, 5);
  
  //Enables up to 4 connections
  SendCmd("AT+CIPMUX=1", 5);

  //Start Server on port 333
  SendCmd("AT+CIPSERVER=1", 5);
}

/**
 * Obtiene los datos del módulo WiFi y ejecuta la instrucción correcta
 */
void parseData(){
  //Abre la puerta
  if(recibido.charAt(0) == '0'){
    Serial.println("abrir la puerta");
  }
  //Cierra la puerta
  else if(recibido.charAt(0) == '1'){
    Serial.println("cerrar la puerta");
  }
  //Agrega una nueva tarjeta a los posibles registros
  else if(recibido.charAt(0) == '2'){
    Serial.print("Agregar la tarjeta: ");
    Serial.println(recibido.substring(2));
  }
  //Elimina una tarjeta de los posibles registros
  else if(recibido.charAt(0) == '3'){
    Serial.print("Eliminar la tarjeta: ");
    Serial.println(recibido.substring(2));
  }
  //Pregunta por el status
  else if(recibido.charAt(0) == '4'){
    Serial.println("Status?");
  }
  //Mensaje desconocido
  else{
    Serial.println("Unknown status");
  }
}

/**
 * Lee datos del módulo WiFi
 */
void leer() {
  //Si hay datos para leer
  if (mySerial.available()) {
    //Lee todos los datos disponibles
    recibido = mySerial.readString();

    //Busca el inicio del mensaje en sí (el módulo escribe "+IDP, 0,1:msj")
    int index = recibido.indexOf(':') + 1;
    recibido = recibido.substring(index);

    //Utiliza los datos
    parseData();
  }
}

/**
 * Envia un comando al módulo WiFi
 * @param Atcmd comando AT a enviar al módulo WiFi
 * @param Tespera tiempo a esperar luego de enviar el comando
 */
void SendCmd (String ATcmd, int Tespera) {
  //Envia el dato al ESP8266
  mySerial.println(ATcmd);
  //Espera Tespera
  delay(Tespera);
}

//-----------------------------------------------------------------------------------------------------------------
/**
 * Inicializa el arduino
 */
void setup() {
  // Inicializacion del Monitor Serial a 115200 para Debugeo
  Serial.begin(38400);
  // Inicializacion de un puerto serial para conectarse a Wifi
  mySerial.begin(38400);
  //Pone el máximo tiempo de espera a 50ms
  mySerial.setTimeout(50);

  // Función para crear un AP con el módulo ESP8266
  connectToWiFi();
  delay(20);
}

/**
 * Inicializa el loop de control
 */
void loop() {
  //Chequea si hay datos a leer del módulo WiFi
  leer();
}
