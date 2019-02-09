/*
   Simpleton Sonoff Touch firmware with MQTT support
   Supports OTA update
   David Pye (C) 2016 GNU GPL v3
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <OneWire.h>

#define BUTTON_PIN_1 0
#define RELAY_PIN_1 12
#define LED_PIN 13
#define BUFFER_SIZE 100

const char *ssidE = "ESPap";
IPAddress apIP(192, 168, 0, 1);
IPAddress apGateway(192, 168, 0, 1);
IPAddress apSubmask(255, 255, 255, 0);
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

WiFiUDP udp;

IPAddress ip(192, 168, 0, 176);
String nameLs = "lswitch1";
String ssid = "Beda";
String pass = "7581730dd";
String mqtt_port = "1883";
String mqtt_user = "Fent";
String mqtt_pass = "7581730dd"; 
String ipS = "192.168.0.176";  
String CmndTopicMode = "cmnd/"+nameLs+"/mode";
String CmndTopic1 = "cmnd/"+nameLs+"/light1";
String StatusTopic_1 = "status/"+nameLs+"/light1";

char *NAME;
char *SSIDd;
char *PASS;
char *MQTT_USER;
char *MQTT_PASS;
int MQTT_PORT;
char *cmndTopicMode;
char *cmndTopic1;
char *statusTopic_1; 


volatile int desiredRelayState_1 = 0;

volatile int relayState_1 = 0;

volatile unsigned long millisSinceChange_1 = 0;

bool setupConfig;

volatile unsigned long millisAttach = 0;

unsigned long lastMQTTCheck = -5000; //This will force an immediate check on init.

WiFiClient espClient;
PubSubClient client(espClient);
bool printedWifiToSerial = false;


void initWifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSIDd);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSIDd, PASS);
}

void initConfig(){
  NAME = new char[nameLs.length()+1];
  nameLs.toCharArray(NAME,nameLs.length()+1);
  Serial.println(NAME);
  SSIDd = new char[ssid.length()+1];
  ssid.toCharArray(SSIDd,ssid.length()+1);
  Serial.println(SSIDd);
  PASS = new char[pass.length()+1];
  pass.toCharArray(PASS,pass.length()+1);
  Serial.println(PASS);
  int count = mqtt_port.length();
  int mn=0;
  int m=1;
  for(int i=count-1;i>=0;i--){
      mn+=(mqtt_port[i]-48)*m;
      m*=10;
  }
  MQTT_PORT = mn;
  Serial.println(MQTT_PORT);
  MQTT_USER = new char[mqtt_user.length()+1];
  mqtt_user.toCharArray(MQTT_USER,mqtt_user.length()+1);
  Serial.println(MQTT_USER);
  MQTT_PASS = new char[mqtt_pass.length()+1];
  mqtt_pass.toCharArray(MQTT_PASS,mqtt_pass.length()+1);
  Serial.println(MQTT_PASS);
  ip.fromString(ipS);
  CmndTopicMode = "cmnd/"+nameLs+"/mode";
  CmndTopic1 = "cmnd/"+nameLs+"/light1";
  StatusTopic_1 = "status/"+nameLs+"/light1";

  cmndTopicMode = new char[CmndTopicMode.length()+1];
  CmndTopicMode.toCharArray(cmndTopicMode,CmndTopicMode.length()+1);
  cmndTopic1 = new char[CmndTopic1.length()+1];
  CmndTopic1.toCharArray(cmndTopic1,CmndTopic1.length()+1);
  statusTopic_1 = new char[StatusTopic_1.length()+1];
  StatusTopic_1.toCharArray(statusTopic_1,StatusTopic_1.length()+1);
}

void checkMQTTConnection() {
  Serial.print("Checking MQTT connection: ");
  if (client.connected()) {
    //flagWifi = false;
    Serial.println("OK");
  }
  else {
    if (WiFi.status() == WL_CONNECTED) {
      //Wifi connected, attempt to connect to server
      Serial.print("new connection: ");
      if (client.connect(NAME,MQTT_USER,MQTT_PASS)) {
        //flagWifi = false;
        Serial.println("connected");
        client.subscribe(cmndTopic1);
      } else {
        //flagWifi = true;
        //espClient
        //while()
        Serial.print("failed, rc=");
        Serial.println(client.state());
      }
    }
    else {
      //Wifi isn't connected, so no point in trying now.
      //flagWifi = true;
      Serial.println(" Not connected to WiFI AP, abandoned connect.");
    }
  }
  //Set the status LED to ON if we are connected to the MQTT server
  if (client.connected()) 
      digitalWrite(LED_PIN, LOW);
  else 
      digitalWrite(LED_PIN, HIGH);
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  if (!strcmp(topic, cmndTopic1)) {
    if ((char)payload[0] == '1' || ! strncasecmp_P((char *)payload, "on", length)) {
        desiredRelayState_1 = 1;
        millisAttach = millis();
    }
    else if ((char)payload[0] == '0' || ! strncasecmp_P((char *)payload, "off", length)) {
      desiredRelayState_1 = 0;
      millisAttach = millis();
    }
    else if ( ! strncasecmp_P((char *)payload, "toggle", length)) {
      desiredRelayState_1 = !desiredRelayState_1;
      millisAttach = millis();
    }
  }
}

void shortPress_1() {
  desiredRelayState_1 = !desiredRelayState_1; //Toggle relay state.
  millisAttach = millis();
  Serial.println("shortPr");
}

void longPress_1(){

  if(setupConfig){
    initConfig();
    initWifi();
    setupConfig=false;  
    client.setServer(ip, MQTT_PORT);
    client.setCallback(MQTTcallback);
  }else{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidE);
    WiFi.softAPConfig(apIP, apGateway, apSubmask);
    Serial.println("Connecting:");
    Serial.println(WiFi.softAPIP());
    udp.begin(8888);
    setupConfig = true;
  }
  Serial.println("longPr");
}

void buttonChangeCallback_1() {
  if (digitalRead(BUTTON_PIN_1) == 0) {
    //Button has been released, trigger one of the two possible options.
    if(millis() - millisSinceChange_1 > 5000){
        longPress_1();
    }else if (millis() - millisSinceChange_1 > 100){
      //Short press
        shortPress_1();
    } 
  }else {
    //Just been pressed - do nothing until released. 
    millisSinceChange_1 = millis();
  }
  
 
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initialising");
  pinMode(RELAY_PIN_1, OUTPUT);

  digitalWrite(RELAY_PIN_1, LOW);

  pinMode(BUTTON_PIN_1, INPUT);

  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH); //LED off.
  initConfig();
  
  initWifi();

  setupConfig = false;
  
  client.setServer(ip, MQTT_PORT);
  client.setCallback(MQTTcallback);

  //Enable interrupt for button press

  Serial.println("Enabling touch switch interrupt");
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_1), buttonChangeCallback_1, CHANGE);

}



void loop() {
  
  if(setupConfig){
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      int packetSize = udp.parsePacket();
      if (packetSize){
          Serial.print("Received packet of size ");
          Serial.println(packetSize);
          Serial.print("From ");
          IPAddress remote = udp.remoteIP();
          for (int i = 0; i < 4; i++){
              Serial.print(remote[i], DEC);
              if (i < 3){
                  Serial.print(".");
              }
          }
          Serial.print(", port ");
          Serial.println(udp.remotePort());
          udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
          if(packetBuffer[0]==49){
              int i=0;
              nameLs="";
              ssid="";
              pass="";
              mqtt_user="";
              mqtt_port="";
              mqtt_pass="";
              ipS="";
              for(i=0;i<packetBuffer[1];i++){
                  nameLs+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(nameLs);
          }else if(packetBuffer[0]==50){
              int i=0;
              for(i=0;i<packetBuffer[1];i++){
                  ssid+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(ssid);
          }else if(packetBuffer[0]==51){
              int i=0;
              for(i=0;i<packetBuffer[1];i++){
                  pass+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(pass);
          }else if(packetBuffer[0]==52){
              int i=0;
              for(i=0;i<packetBuffer[1];i++){
                  mqtt_user+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(mqtt_user);
          }else if(packetBuffer[0]==53){
              int i=0;
              for(i=0;i<packetBuffer[1];i++){
                  mqtt_port+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(mqtt_port);
          }else if(packetBuffer[0]==54){
              int i=0;
              for(i=0;i<packetBuffer[1];i++){
                  mqtt_pass+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(mqtt_pass);
          }else if(packetBuffer[0]==55){
              int i=0;
              for(i=0;i<packetBuffer[1];i++){
                  ipS+=packetBuffer[2+i];
              }
              Serial.print("Contents:");
              Serial.println(ipS);
          }
          udp.beginPacket(udp.remoteIP(), 8888);
          udp.write(packetBuffer);
          udp.endPacket();
      }
      delay(100);
  }else{
  
    if (!printedWifiToSerial && WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      printedWifiToSerial = true;
    }
    if (!setupConfig&&millis() - lastMQTTCheck >= 5000) {
      checkMQTTConnection();
      lastMQTTCheck = millis();
    }

    //Handle any pending MQTT messages
    client.loop();

    //Relay state is updated via the interrupt *OR* the MQTT callback.
    if (relayState_1 != desiredRelayState_1) {
      Serial.print("Changing state to ");
      Serial.println(desiredRelayState_1);

      digitalWrite(RELAY_PIN_1, desiredRelayState_1);
      relayState_1 = desiredRelayState_1;

      Serial.print("Sending MQTT status update ");
      Serial.print(relayState_1);
      Serial.print(" to ");
      Serial.println(statusTopic_1);

      client.publish(statusTopic_1, relayState_1 == 0 ? "0" : "1");
    }
    delay(50);
  }

  

  


  
  
}

