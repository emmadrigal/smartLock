#include <SoftwareSerial.h>
#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices

//Pines: RX conectado a D3, TX conectado a D2
SoftwareSerial mySerial(3, 2);

#define SSID   "smartLockAP"
#define PASS   "pwd"

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define redLed 7    // Set Led Pins
#define greenLed 6
#define blueLed 5

#define relay 4     // Set Relay Pin
#define wipeB 3     // Button pin for WipeMode

#define SS_PIN 10
#define RST_PIN 9

boolean match = false;          // initialize card match to false
boolean programMode = false;  // initialize programming mode to false
boolean replaceMaster = false;

int successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM

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

    //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);    // Enable pin's pull up resistor
  pinMode(relay, OUTPUT);
  //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  digitalWrite(relay, HIGH);    // Make sure door is locked
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off

  //Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  Serial.println(F("Access Control v3.4"));   // For debugging purposes
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  //Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
  if (digitalRead(wipeB) == LOW) {  // when button pressed pin should get low, button connected to ground
    digitalWrite(redLed, LED_ON); // Red Led stays on to inform user we are going to wipe
    Serial.println(F("Wipe Button Pressed"));
    Serial.println(F("You have 15 seconds to Cancel"));
    Serial.println(F("This will be remove all records and cannot be undone"));
    delay(15000);                        // Give user enough time to cancel operation
    if (digitalRead(wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
      Serial.println(F("Starting Wiping EEPROM"));
      for (int x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
        if (EEPROM.read(x) == 0) {              //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
        }
      }
      Serial.println(F("EEPROM Successfully Wiped"));
      digitalWrite(redLed, LED_OFF);  // visualize successful wipe
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
    }
    else {
      Serial.println(F("Wiping Cancelled"));
      digitalWrite(redLed, LED_OFF);
    }
  }
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    Serial.println(F("No Master Card Defined"));
    Serial.println(F("Scan A PICC to Define as Master Card"));
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    Serial.println(F("Master Card Defined"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Master Card's UID"));
  for ( int i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Everything Ready"));
  Serial.println(F("Waiting PICCs to be scanned"));
  cycleLeds();    // Everything ready lets give user some feedback by cycling leds
  
}

/**
 * Inicializa el loop de control
 */
void loop() {
  //Chequea si hay datos a leer del módulo WiFi
  leer();
}
