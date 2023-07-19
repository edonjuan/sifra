// LIBRARIES
#include <SPI.h>
#include <MFRC522.h>
#include <painlessMesh.h>
#include <Adafruit_NeoPixel.h> 

// NETWORK
#define   MESH_PREFIX     "AccessCTRL"
#define   MESH_PASSWORD   "AccessCTRL1"
#define   MESH_PORT       5555

// HARDWARE
#define RST_PIN           4     
#define SS_PIN            16    
#define BUZZER            0
#define SELECT            5
#define NEOPIXELPIN       2
#define NEOPIXELLEDS      4

// SW
#define MASTER    3822969472  //alfa
#define sensor_rate 50  // milis
#define msg_rate    400
#define indicator   500
#define sound       150
#define wtd_rate    10  // seconds  
#define wtd_rest    4

// Objetcts
Adafruit_NeoPixel neo = Adafruit_NeoPixel(NEOPIXELLEDS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800); 
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
void allColor(short r, short g, short b);

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

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(SELECT, INPUT_PULLUP);

  neo.begin();             
  neo.setBrightness(255);
  allColor(255,255,0);  // Yellow
  
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
    allColor(0,0,255);  // Blue
    watchCount = 0;
    if(!first){
      taskReadRFID.enable();
      Serial.println("Master online");
      first = 1;
    }    
  }
  else
  {
    allColor(255,255,0);  // Yellow
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
  //pinMode(BUZZER, OUTPUT);
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
    if(!BTN) doc["type"] = "request";  // request    //     CAMBIAR AQUI 2/2
    else doc["type"] = "query";
    
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
  taskIndicator.enable();  
}

void showIndicator()
{
  static short turn;

  if(!turn)
  {
    if(access)
    {
      allColor(0,255,0);  // Green
    }
    else
    {
      allColor(255,0,0);  // Red
      analogWrite(BUZZER, 10);
    }
    turn=1;
  }
  else
  {
    turn=0;
    digitalWrite(BUZZER, LOW);
    allColor(0,0,255);  // Blue
    taskIndicator.disable();
  }
}

void allColor(short r, short g, short b)
{
  for(unsigned i=0; i<NEOPIXELLEDS; i++)
  {
    neo.setPixelColor(i,r,g,b); // (POS; R;G;B)
  }
  neo.show();
}
