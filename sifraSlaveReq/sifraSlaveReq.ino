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
#define LBLU 16

// SW
#define MASTER    3822969472  //alfa
#define sensor_rate 50  // milis
#define msg_rate    400
#define indicator   500
#define sound       150
#define wtd_rate    10  // seconds  
#define wtd_rest    3

// Objetcts
MFRC522 mfrc522(SS_PIN, RST_PIN);
Scheduler userScheduler;
painlessMesh  mesh;

// Global variables
unsigned int access;
unsigned int mac;
String uid;
short turn_buzzer;

// Functions structure
void readRFID(); 
void sendMessage();
void sendRegister();
void wtd();
void showIndicator();
void soundBuzzer();
void matCapture();

// Task declaration
Task taskReadRFID(TASK_MILLISECOND *sensor_rate, TASK_FOREVER, &readRFID);
Task taskSendMessage(TASK_MILLISECOND *msg_rate, TASK_FOREVER, &sendMessage);
Task taskWatchDog(TASK_SECOND *wtd_rate, TASK_FOREVER, &wtd);  
Task taskIndicator(TASK_MILLISECOND *indicator, TASK_FOREVER, &showIndicator);  
Task taskBuzzer(TASK_MILLISECOND *sound, TASK_FOREVER, &soundBuzzer);  

void setup() {
  //HW
  Serial.begin(9600);
  delay(10);

  pinMode(LBLU, OUTPUT);
  digitalWrite(LBLU, LOW);
  pinMode(LRED, OUTPUT);
  digitalWrite(LRED, LOW);
  pinMode(LGRE, OUTPUT);
  digitalWrite(LGRE, LOW);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(SELECT, INPUT);
  
  SPI.begin();
  mfrc522.PCD_Init();

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
  userScheduler.addTask(taskBuzzer);

  taskWatchDog.enable(); 
  taskReadRFID.disable();
  taskSendMessage.disable();   
  taskIndicator.disable();
  taskBuzzer.disable();
}

void loop() {
  mesh.update();
}

void wtd(){
  static short watchCount, first;
  char strBuf[40];

  if(mesh.isConnected(MASTER))
  {
    digitalWrite(LBLU, HIGH);
    watchCount = 0;
    if(!first){
      taskReadRFID.enable();
      Serial.println("Master online");
      first = 1;
    }    
  }
  else
  {
    digitalWrite(LBLU, LOW);
    taskReadRFID.disable();
    first = 0;
    watchCount++;
    if(watchCount>=wtd_rest) ESP.reset();
    sprintf(strBuf, "Master not found! Watchdog Count: %d of %d", watchCount, wtd_rest);
    Serial.println(strBuf);    
  }
}

void readRFID() {
  if ( mfrc522.PICC_IsNewCardPresent()) 
     { 
         taskBuzzer.enable();
         if ( mfrc522.PICC_ReadCardSerial()) 
         {            
            for (byte i = 0; i < mfrc522.uid.size; i++) {
                if(mfrc522.uid.uidByte[i] < 0x10) uid+= "0";
                uid += String(mfrc522.uid.uidByte[i], HEX);
            }
            uid.toUpperCase();            
            mfrc522.PICC_HaltA();
          }
          taskSendMessage.enable();
          taskReadRFID.disable();
      }
}

void soundBuzzer() {  
  if(!turn_buzzer)
  {
    digitalWrite(BUZZER, HIGH);
    turn_buzzer=1;
  }
  else
  {
    turn_buzzer=0;
    digitalWrite(BUZZER, LOW);
    taskBuzzer.disable();
  }   
}

void sendMessage(){
  static short turn;

  if(!turn)
  {
    StaticJsonDocument<100> doc; 

    short BTN = digitalRead(SELECT);
    if(!BTN) doc["type"] = "query";
    else doc["type"] = "request";
    
    doc["mac"] = String(mac);
    doc["uid"] = uid;
    uid = "";

    serializeJsonPretty(doc, Serial);
    Serial.println();  
    
    String output;
    serializeJson(doc, output);
    mesh.sendSingle(MASTER, output);
    turn=1;
  }
  else
  {
    turn=0;
    taskReadRFID.enable();
    taskSendMessage.disable(); 
  }
}

void receivedCallback( uint32_t from, String &msg ) {
  digitalWrite(BUZZER, LOW);
  taskBuzzer.disable();
  turn_buzzer=0;
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  access = msg.toInt();
  digitalWrite(LBLU, LOW);
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
      analogWrite(BUZZER, 10);
    }
    turn=1;
  }
  else
  {
    turn=0;
    digitalWrite(LGRE, LOW);
    digitalWrite(LRED, LOW);
    digitalWrite(BUZZER, LOW);
    digitalWrite(LBLU, HIGH);
    taskIndicator.disable();
  }  
}
