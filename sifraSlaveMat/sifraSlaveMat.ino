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
#define mat_capture  10
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
Task taskSendRegister(TASK_MILLISECOND *msg_rate, TASK_FOREVER, &sendRegister);
Task taskMatCapture(TASK_MILLISECOND *mat_capture, TASK_FOREVER, &matCapture);
Task taskWatchDog(TASK_SECOND *wtd_rate, TASK_FOREVER, &wtd);  
Task taskIndicator(TASK_MILLISECOND *indicator, TASK_FOREVER, &showIndicator);  
Task taskBuzzer(TASK_MILLISECOND *sound, TASK_FOREVER, &soundBuzzer);  

void setup() {
  //HW
  Serial.begin(9600); //Iniciamos la comunicación  serial
  delay(10);

  pinMode(LBLU, OUTPUT);
  digitalWrite(LBLU, !LOW);
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
  userScheduler.addTask(taskSendRegister);
  userScheduler.addTask(taskWatchDog);
  userScheduler.addTask(taskIndicator);
  userScheduler.addTask(taskBuzzer);
  userScheduler.addTask(taskMatCapture);

  taskWatchDog.enable(); 
  taskReadRFID.disable();
  taskSendMessage.disable();   
  taskSendRegister.disable();   
  taskIndicator.disable();
  taskBuzzer.disable();
  taskMatCapture.disable();  
}

void loop() {
  mesh.update();
}

void wtd(){
  static short watchCount, first;
  char strBuf[40];

  if(mesh.isConnected(MASTER))
  {
    digitalWrite(LBLU, !HIGH);
    watchCount = 0;
    if(!first){
      taskReadRFID.enable();
      Serial.println("Master online");
      first = 1;
    }    
  }
  else
  {
    digitalWrite(LBLU, !LOW);
    taskReadRFID.disable();
    first = 0;
    watchCount++;
    if(watchCount>=wtd_rest) ESP.reset();
    sprintf(strBuf, "Master not found! Watchdog Count: %d of %d", watchCount, wtd_rest);
    Serial.println(strBuf);    
  }
}

String ident;

void readRFID() {
  if ( mfrc522.PICC_IsNewCardPresent()) 
     { 
         taskBuzzer.enable();
         if ( mfrc522.PICC_ReadCardSerial()) 
         {            
            for (byte i = 0; i < mfrc522.uid.size; i++) {
                if(mfrc522.uid.uidByte[i] < 0x10) ident+= "0";
                ident += String(mfrc522.uid.uidByte[i], HEX);
            }
            ident.toUpperCase();              
            mfrc522.PICC_HaltA();    
          }
          short BTN = digitalRead(SELECT);
          if(BTN)
          {
            Serial.print("Ingrese Matricula: ");
            digitalWrite(LRED, HIGH);
            digitalWrite(LGRE, HIGH);
            taskMatCapture.enable();
          }
          else    taskSendMessage.enable();    
          taskReadRFID.disable();
      }
}

short turn_buzzer;

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


char arreglo[15];

void matCapture(){  
  static int pointer;
  
  if (Serial.available() > 0) {
    // read the incoming string:
    char data = Serial.read(); 
    Serial.print(data);   // MIRROW
    if((data == '\r')||(data == '\n'))
    {
      digitalWrite(LGRE, LOW);
      digitalWrite(LRED, LOW);
      pointer = 0;
      while (Serial.available()>0)  Serial.read();
      taskSendRegister.enable();   
      taskMatCapture.disable();      
    }
    else
    {
      arreglo[pointer] = data;
      pointer++;
    }
}
}

void sendMessage(){
  static short turn;

  if(!turn)
  {
    StaticJsonDocument<500> doc; 
    doc["type"] = "query";
    doc["mac"] = String(mac);
    doc["uid"] = ident;
    ident = "";

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

void sendRegister(){ 
  StaticJsonDocument<500> doc; 
  doc["type"] = "register";
  doc["mac"] = String(mac);
  doc["uid"] = ident;
  doc["mat"] = arreglo;
  ident = "";
  memset(arreglo,0,sizeof(arreglo));


  Serial.println();
  serializeJsonPretty(doc, Serial);
  Serial.println();

  taskReadRFID.enable();
  taskSendRegister.disable(); 
  //String output;
  //serializeJson(doc, output);
  //mesh.sendSingle(MASTER, output);
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
    taskIndicator.disable();
  }  
}
