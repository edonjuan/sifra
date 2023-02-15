// LIBRARIES
#include <SPI.h>
#include <MFRC522.h>
#include <painlessMesh.h>

// NETWORK
#define   MESH_PREFIX     "AccessCTRL"
#define   MESH_PASSWORD   "AccessCTRL1"
#define   MESH_PORT       5555

// HARDWARE
#define RST_PIN  0    // RC522_RESET
#define SS_PIN  2     // RC522_SDA
#define BUZZER 10
#define SELECT 15
#define LRED 4
#define LGRE 5

// SW
#define MASTER    3822969472  //alfa
#define sensor_rate 50  // milis
#define msg_rate    100
#define indicator   500
#define wtd_rate    10  // seconds  
#define wtd_rest    3

// Objetcts
MFRC522 mfrc522(SS_PIN, RST_PIN);
Scheduler userScheduler;
painlessMesh  mesh;

// Global variables
unsigned int access;
unsigned int mac;

// Functions structure
void readRFID(); 
void sendMessage();
void wtd();
void showIndicator();

// Task declaration
Task taskReadRFID(TASK_MILLISECOND *sensor_rate, TASK_FOREVER, &readRFID);
Task taskSendMessage(TASK_MILLISECOND *msg_rate, TASK_FOREVER, &sendMessage);
Task taskWatchDog(TASK_SECOND *wtd_rate, TASK_FOREVER, &wtd);  
Task taskIndicator(TASK_MILLISECOND *indicator, TASK_FOREVER, &showIndicator);  

void setup() {
  //HW
  Serial.begin(9600); //Iniciamos la comunicación  serial
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !LOW);
  
  pinMode(LRED, OUTPUT);
  digitalWrite(LRED, LOW);
  pinMode(LGRE, OUTPUT);
  digitalWrite(LGRE, LOW);
  
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(SELECT, INPUT);
  
  SPI.begin();        //Iniciamos el Bus SPI
  mfrc522.PCD_Init(); // Iniciamos  el MFRC522

  // Network
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  delay(5000);
  mac = mesh.getNodeId();

  // SW
  userScheduler.addTask(taskReadRFID);
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskWatchDog);
  userScheduler.addTask(taskIndicator);

  taskReadRFID.enable();
  taskSendMessage.disable();  
  taskWatchDog.enable();  
  taskIndicator.disable();
}

void loop() {
  mesh.update();
}

void readRFID() {
  if ( mfrc522.PICC_IsNewCardPresent()) 
        {  
          taskSendMessage.enable();   // Leer tarjeta
          taskReadRFID.disable();     // Desactivar sensor
  } 
}

void sendMessage(){
  static short count;
  String ident;
  
  if ( mfrc522.PICC_ReadCardSerial()) 
     {
        digitalWrite(BUZZER, HIGH);
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          if(mfrc522.uid.uidByte[i] < 0x10) ident+= "0";
          ident += String(mfrc522.uid.uidByte[i], HEX);
        } 
        ident.toUpperCase();              
        mfrc522.PICC_HaltA();                     

        StaticJsonDocument<500> doc; 
        short BTN = digitalRead(SELECT);
        
        if(BTN) doc["type"] = "register";
        else doc["type"] = "query";
        doc["mac"] = String(mac);
        doc["uid"] = ident;
        
        serializeJsonPretty(doc, Serial);
        Serial.println();
        
        String output;
        serializeJson(doc, output);
        mesh.sendSingle(MASTER, output);
    }
  else
    {
        count++;
        if(count>=1) digitalWrite(BUZZER, LOW);
        if(count>=3)
        {
           count=0;
           taskSendMessage.disable();   // Leer tarjeta
           taskReadRFID.enable();     // Desactivar sensor
        }
     }
}

void wtd(){
  static short watchCount, first;
  char strBuf[40];

  if(mesh.isConnected(MASTER))
  {
    digitalWrite(LED_BUILTIN, !HIGH);
    watchCount = 0;
    if(!first){
      taskSendMessage.enable();
      Serial.println("Master online");
      first = 1;
    }    
  }
  else
  {
    digitalWrite(LED_BUILTIN, !LOW);
    taskSendMessage.disable();
    first = 0;
    watchCount++;
    if(watchCount>=wtd_rest) ESP.reset();
    sprintf(strBuf, "Master not found! Watchdog Count: %d of %d", watchCount, wtd_rest);
    Serial.println(strBuf);    
  }
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  access = msg.toInt();
  taskIndicator.enable();
}

void showIndicator()
{
  static short turn;

  if(!turn)
  {
    if(access)
    {
      digitalWrite(LGRE, HIGH);
    }
    else
    {
      digitalWrite(LRED, HIGH);
    }
    turn++;
  }
  else
  {
    turn=0;
    digitalWrite(LGRE, LOW);
    digitalWrite(LRED, LOW);
    taskIndicator.disable();
  }  
}
