/*
 * Gateway to count S0 pulses and send via MQTT to client
 * Copyright K.Schmolders 08/2017
 */

#include "wifi_credentials.h"

 //required for MQTT
 #include <ESP8266WiFi.h>
 //required for OTA updater
 #include <WiFiClient.h>
 #include <ESP8266WebServer.h>
 #include <ESP8266mDNS.h>
 #include <ESP8266HTTPUpdateServer.h>
 //end OTA requirements
 #include <PubSubClient.h>

 
 //timer
 int timer_update_state_count;
 int timer_update_state = 60000; //update status via MQTT every minute
 
 //pulse counting support
 uint32_t pulseCount = 0;
 uint32_t lastBlink = 0;
 
 //MQTT
 WiFiClient espClient;
 PubSubClient client(espClient);
 //all wifi credential and MQTT Server importet through wifi_credential.h
 
 //sensor gateway is for heating counting
 const char* inTopic = "cmnd/ss_heat/#";
 const char* outTopic = "stat/ss_heat/";
 const char* mqtt_id = "ss_heat";
 
 //Pins
 uint16_t VOL_PIN=14; //GPIO14 = D5;
 uint16_t Q_PIN=12; //GPIO15 = D6;
 
 //OTA
 ESP8266WebServer httpServer(80);
 ESP8266HTTPUpdateServer httpUpdater;
 
 void setup_wifi() {
   delay(10);
   // We start by connecting to a WiFi network
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
   WiFi.persistent(false);
   WiFi.mode(WIFI_OFF);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
     
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
 
   httpUpdater.setup(&httpServer);
   httpServer.begin();
 }
 
 
 //callback function for MQTT client
 void callback(char* topic, byte* payload, unsigned int length) {
   payload[length]='\0'; // Null terminator used to terminate the char array
   String message = (char*)payload;
 
   Serial.print("Message arrived on topic: [");
   Serial.print(topic);
   Serial.print("]: ");
   Serial.println(message);
   
   //get last part of topic 
   char* cmnd = "test";
   char* cmnd_tmp=strtok(topic, "/");
 
   while(cmnd_tmp !=NULL) {
     cmnd=cmnd_tmp; //take over last not NULL string
     cmnd_tmp=strtok(NULL, "/"); //passing Null continues on string
     //Serial.println(cmnd_tmp);    
   }
   
   
 
   if (!strcmp(cmnd, "status")) {
     Serial.print("Received status reqeust. sending status");
     send_status();
   }
   if (!strcmp(cmnd, "reset")) {
    Serial.print(F("Reset requested. Resetting..."));
    //software_Reset();
  }
 }

 void send_status()
 {
   char outTopic_status[50];
   char msg[50];
   //IP Address
   strcpy(outTopic_status,outTopic);
   strcat(outTopic_status,"ip_address");
   //Arduino IP
   //IPAddress ip=WiFi.localIP();
   //sprintf(msg, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
   //ESP IP
   WiFi.localIP().toString().toCharArray(msg,50);
   client.publish(outTopic_status,msg ); 
 }
 
 //send Values via MQTT
 void send_values(){
    
    char outTopic_status[50];
    char msg[50];
 
    
   //roomtemp from BME280
   /*  strcpy(outTopic_status,outTopic);
    dtostrf(bme280_temperature,2,2,msg); 
    strcat(outTopic_status,"temperature");
    client.publish(outTopic_status, msg);
 */
 
 }
 
 void reconnect() {
   // Loop until we're reconnected
   
   while (!client.connected()) {
     Serial.print("Attempting MQTT connection...");
     // Attempt to connect
     if (client.connect(mqtt_id)) {
       Serial.println("connected");
       
       client.publish(outTopic, "ss_heat station booted");
       
       //send current Status via MQTT to world
       send_status();
       // ... and resubscribe
       client.subscribe(inTopic);
 
     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" try again in 5 seconds");      
       delay(5000);
     }
   }
 }
 
 void update_values() {
   //NEEDED?
 }

 //Interrupt Callback Function
 void onPulse()
 {
    Serial.println("Interrupt");      
    
    // if (!SLEEP_MODE) {
         uint32_t newBlink = micros();
         uint32_t interval = newBlink-lastBlink;
         if (interval<10000L) { // Sometimes we get interrupt on RISING
             return;
         }
         //watt = (3600000000.0 /interval) / ppwh;
         lastBlink = newBlink;
     //}
     pulseCount++;
     Serial.println("Detected new pulse");      
     
 }
 
 void setup() {
   // Status message will be sent to the PC at 115200 baud
   Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
   
   //INIT TIMERS
   timer_update_state_count=millis();
 
   //interrupt for the S0 Pins
   pinMode(VOL_PIN, INPUT_PULLUP);
   attachInterrupt(digitalPinToInterrupt(VOL_PIN), onPulse, RISING);

   update_values(); 
 
   //WIFI and MQTT
   setup_wifi();                   // Connect to wifi 
   client.setServer(mqtt_server, 1883);
   client.setCallback(callback);
}
 
 
 void loop() {
   if (!client.connected()) {
     reconnect();
   }
   client.loop();
 
   update_values(); 
 
   //http Updater for OTA
   httpServer.handleClient(); 
 
   //send status update via MQTT every xxx
   if(millis()-timer_update_state_count > timer_update_state) {
    //addLog_P(LOG_LEVEL_INFO, PSTR("Serial Timer triggerd."));
    timer_update_state_count=millis();
    send_values();
    
   }
   
 }
 