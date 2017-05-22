#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include <Servo.h>
#include <SoftwareSerial.h>
//Pines: RX conectado a D3, TX conectado a D2
SoftwareSerial mySerial(3, 2);

#define SSID   "smartLockAP"
#define PASS   "pwd"

//Guarda el mensaje recibido por puerto serial
String recibido;

bool PuertaAbierta=false;

/*
  Instead of a Relay maybe you want to use a servo
  Servos can lock and unlock door locks too
  There are examples out there.
*/


/*
  For visualizing whats going on hardware
  we need some leds and
  to control door lock a relay and a wipe button
  (or some other hardware)
  Used common anode led,digitalWriting HIGH turns OFF led
  Mind that if you are going to use common cathode led or
  just seperate leds, simply comment out #define COMMON_ANODE,
*/

#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define LEDalarma 6
#define sensorMagnetico 5
bool magneticoAbierto = false;

#define pinServo 4
Servo servoMotor;

boolean match = false;          // initialize card match to false
boolean programMode = false;  // initialize programming mode to false
boolean replaceMaster = false;

int successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM

/*
  We need to define MFRC522's pins and create instance
  Pin layout should be as follows (on Arduino Uno):
  MOSI: Pin 11 / ICSP-4
  MISO: Pin 12 / ICSP-1
  SCK : Pin 13 / ICSP-3
  SS : Pin 10 (Configurable)
  RST : Pin 9 (Configurable)
  look MFRC522 Library for
  other Arduinos' pin configuration
*/

#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.


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
    PuertaAbierta = true;
  }
  //Cierra la puerta
  else if(recibido.charAt(0) == '1'){
    PuertaAbierta = false;
  }
  //Agrega una nueva tarjeta a los posibles registros
  else if(recibido.charAt(0) == '2'){
    
   char newID[8];
   
   recibido.toCharArray(newID, 8);
   
    writeID(newID);
  }
  //Elimina una tarjeta de los posibles registros
  else if(recibido.charAt(0) == '3'){
  }
  //Pregunta por el status
  else if(recibido.charAt(0) == '4'){
  }
  //Mensaje desconocido
  else{
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
    int index = recibido.indexOf(":") + 1;
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

  pinMode(sensorMagnetico, INPUT);
  digitalWrite(sensorMagnetico, HIGH);
  
  //Este valor depende del firmware del ESP8266
  mySerial.begin(115200);
  //Pone el máximo tiempo de espera a 50ms
  mySerial.setTimeout(50);
  // Función para crear un AP con el módulo ESP8266
  connectToWiFi();
  delay(20);
  
  
  //Arduino Pin Configuration
  servoMotor.attach(pinServo);

  //Protocol Configuration
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      delay(200);
      delay(200);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
  }
  for ( int i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
  }
}


///////////////////////////////////////// Main Control Loop ///////////////////////////////////
void loop () {
  
  
  //Chequea si hay datos a leer del módulo WiFi
  leer();

  
  estadoMagnetico();
   
  successRead = getID();
  if(successRead){
    rfidSetup();
  }
  updateServo();  
  
  
}

void estadoMagnetico(){
if(!PuertaAbierta and (digitalRead(sensorMagnetico) == HIGH)){
    //Serial.println("Alarma activada");
    digitalWrite(LEDalarma,HIGH);
  }else{
    digitalWrite(LEDalarma,LOW);
  }
}

void rfidSetup(){
  if (programMode) {
    if ( isMaster(readCard) ) { //If master card scanned again exit program mode
      Serial.println(F("Master Card Scanned"));
      Serial.println(F("Exiting Program Mode"));
      Serial.println(F("-----------------------------"));
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // If scanned card is known delete it
        deleteID(readCard);
      }
      else {                    // If scanned card is not known add it
        writeID(readCard);
      }
    }
  }
  else {
    if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID enter program mode
      programMode = true;
      int count = EEPROM.read(0);   // Read the first Byte of EEPROM that
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        PuertaAbierta=!PuertaAbierta;
      }
    }
  }  
}
/////////////////////////////////////////  Motor position    ///////////////////////////////////
bool previuosState= PuertaAbierta;
void updateServo(){
  if(PuertaAbierta and (previuosState != PuertaAbierta)){
    servoMotor.write(0);
    previuosState = PuertaAbierta;
  }else if(previuosState != PuertaAbierta){
    servoMotor.write(90); 
    previuosState = PuertaAbierta;
  } else{
    previuosState = PuertaAbierta;  
  }
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (v == 0x91)
    ;
  else if (v == 0x92)
    ;
  else
    ;
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    ;
    while (true); // do not go further
  }
}


//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2;    // Figure out starting position
  for ( int i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( int j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    ;
  }
  else {
    int num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    int slot;       // Figure out the slot number of the card
    int start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    int looping;    // The number of times the loop repeats
    int j;
    int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( int k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL )       // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( int k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}

