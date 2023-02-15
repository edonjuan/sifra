// LIBRARIES
#include "painlessMesh.h"

// NETWORK
#define   MESH_PREFIX     "AccessCTRL"
#define   MESH_PASSWORD   "AccessCTRL1"
#define   MESH_PORT       5555

//SW
#define node_rate 5
#define wtd_rest  3
#define serie_rate 50 // mili

// Objects
Scheduler userScheduler;
painlessMesh  mesh;

// Functions structure
void node_task();
void serie_task();

// Task declaration
Task taskNodes(TASK_SECOND *node_rate, TASK_FOREVER, &node_task);
Task taskSerie(TASK_MILLISECOND *serie_rate, TASK_FOREVER, &serie_task);

void setup() {  
  // HW
  Serial.begin(9600);
  delay(10);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !LOW);

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  delay(5000);

  //Serial.println(mesh.getNodeId()); //GET MASTER ID
  userScheduler.addTask(taskNodes);
  userScheduler.addTask(taskSerie);
  taskNodes.enable();
  taskSerie.enable();
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String&msg ) {  
  // Send to RaspberryPi
  Serial.print(msg);
  Serial.println(); 
}

void serie_task(){  
  // Receive from RaspberryPi
  if(Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    String sub = msg.substring(0,msg.indexOf(','));

    char buf[sub.length()+1];
    sub.toCharArray(buf, sub.length()+1);
    uint32_t from = atoll(buf);
    sub = msg.substring(msg.indexOf(',')+1,msg.indexOf('\n'));    
    
    mesh.sendSingle (from, sub);
  } 
}

void node_task(){
  SimpleList<uint32_t> nodes;
  static short watchCount;
  char strBuf[40];

  nodes = mesh.getNodeList();

  if(nodes.size() == 0)
  {
    watchCount++;
    if(watchCount>=wtd_rest) ESP.reset();
    sprintf(strBuf, "No devices found! Watchdog Count: %d of %d", watchCount, wtd_rest);
    //Serial.println(strBuf);   // DEBUGGING MSG
    digitalWrite(LED_BUILTIN, !LOW);
  }
  else
  {
    digitalWrite(LED_BUILTIN, !HIGH);   
  }
}
