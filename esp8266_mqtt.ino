#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
typedef struct _func1{
  int head;
  int state;
  unsigned long period;
}func1;
typedef struct _func2{
  int head;
  int hourON;
  int minON;
  int secON;
}func2;
typedef struct _func3{
  int head;
  int hourOFF;
  int minOFF;
  int secOFF;
}func3;
typedef struct _dev{
  func1 on_off;
  func2 scheduON; 
  func3 scheduOFF;
}device;
device dev;
const char *ssid = "Redmi"; 
const char *password = "12345678";  


const char *mqtt_broker = "broker.emqx.io";
const char *topic = "esp8266/test";
const char *mqtt_username = "1";
const char *mqtt_password = "123";
const int mqtt_port = 1883;

int head;
unsigned long timestamp;
unsigned long period;
int state1;
int state2;
int hour;
int minute;
int sec;

bool volatile endProc = 0;
bool pendingON = 0;
bool pendingOFF = 0;
bool pendingTIME = 0;
bool volatile cmd_flag = 0;
char cmd[20];
bool state_init = 1;

WiFiClient espClient;
WiFiUDP ntpUDP;
WiFiUDP Udp;
PubSubClient client(espClient);
NTPClient timeClient(ntpUDP);
void setup() {
 int cnt = 0;
 pinMode(2,OUTPUT);
 digitalWrite(2,HIGH);
 Serial.begin(115200);
 //WiFi.mode(WIFI_STA);
 WiFi.begin(ssid,password);
 while (WiFi.status() != WL_CONNECTED) {
     delay(1000);
     Serial.println("Connecting to WiFi..");
     if(cnt++ >= 10){
       WiFi.beginSmartConfig();
       while(1){
           delay(1000);
           if(WiFi.smartConfigDone()){
             Serial.println("SmartConfig Success");
             break;
           }
       }
    }
 }
 client.setServer(mqtt_broker, mqtt_port);
 client.setCallback(callback);
 while (!client.connected()) {
     String client_id = "esp8266-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
         Serial.println("Public emqx mqtt broker connected");
     } else {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
 }

 client.publish("outTopic", "hello emqx");
 client.subscribe(topic);
 timeClient.begin();

}

void callback(char *topic, byte *payload, unsigned int length) {
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 memset(cmd,0,20);
 for (int i = 0; i < length; i++) {
     Serial.print((char) payload[i]);
     cmd[i] =(char)payload[i];
 }
 byte* p = (byte*)malloc(length);
 memcpy(p,payload,length);
 client.publish("outTopic", p, length);
 free(p);
 if(cmd_flag == 1)
 {
  endProc = 1;
 }
 cmd_flag = 1;
 Serial.println();  
 Serial.println("-----------------------");
}

void loop() {
  
 if(cmd_flag == 1)
 {
  cmd_flag = 0;
  if(strncmp(cmd,"1",1) == 0)
  {
    sscanf(cmd,"%d,%d,%d", &dev.on_off.head, &dev.on_off.state, &dev.on_off.period);
    if(state_init == 0) state_init = 1;
  }
  if(strncmp(cmd,"2",1) == 0)
  {
    sscanf(cmd,"%d,%d,%d,%d", &dev.scheduON.head, &dev.scheduON.hourON, &dev.scheduON.minON, &dev.scheduON.secON);
  }
  if(strncmp(cmd,"3",1) == 0)
  {
    sscanf(cmd,"%d,%d,%d,%d", &dev.scheduOFF.head, &dev.scheduOFF.hourOFF, &dev.scheduOFF.minOFF, &dev.scheduOFF.secOFF);
  }
 }
 if(dev.on_off.head == 1)
 {
  if(state_init == 1)
  {
    if(dev.on_off.state == 1) 
    {
      digitalWrite(2,LOW);
      client.publish("outTopic","1");
    }
    if(dev.on_off.state == 0)
    {
      digitalWrite(2,HIGH);
      client.publish("outTopic","0");
    }
    timestamp = millis();
    state_init = 0;  
  }
  if((dev.on_off.period > 0) && (unsigned long)(millis() - timestamp) > dev.on_off.period) 
  {
    if(dev.on_off.state == 1)
    {
      digitalWrite(2,HIGH);
      client.publish("outTopic","0");
    }
    if(dev.on_off.state == 0) 
    {
      digitalWrite(2,LOW);
      client.publish("outTopic","1");
    }
    dev.on_off.head = 0;
    state_init = 1;
  }
 }
 if(dev.scheduON.head == 2)
 {
  timeClient.update();
  if((timeClient.getHours()+7) == dev.scheduON.hourON && timeClient.getMinutes() == dev.scheduON.minON && timeClient.getSeconds() == dev.scheduON.secON)
  {
    digitalWrite(2,LOW);
    client.publish("outTopic","1");
    dev.scheduON.head = 0;
  }
 }
 if(dev.scheduOFF.head == 3)
 {
  timeClient.update();
  if((timeClient.getHours()+7) == dev.scheduOFF.hourOFF && timeClient.getMinutes() == dev.scheduOFF.minOFF && timeClient.getSeconds() == dev.scheduOFF.secOFF)
  {
    digitalWrite(2,HIGH);
    client.publish("outTopic","0");
    dev.scheduOFF.head = 0;
  }
 } 
 client.loop();
}
