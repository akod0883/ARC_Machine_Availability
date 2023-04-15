#include <ArduinoQueue.h>
#include <PubSubClient.h>   // Connect and publish to the MQTT broker
#include <WiFi.h>           // Enables ESP32 to connect to the local network (via WiFi)
#include "time.h"

#define INTERVAL 1000
#define QUEUE_SIZE 5

// WiFi login
const char* ssid = "Akhil";
const char* wifi_password = "12345678";

// MQTT
const char* mqtt_server = "172.20.10.7";  // IP of the MQTT broker
const char* topic = "esp32/machine_availability";
// const char* mqtt_username = ""; // MQTT username
// const char* mqtt_password = ""; // MQTT password
const char* client_id = "1"; // MQTT client ID
const int port = 1883;

// Initialize the WiFI and MQTT Client objects
WiFiClient wifi_client;
PubSubClient client(mqtt_server, port, wifi_client);

// Connect to MQTT broker via WiFi
void connect_MQTT(){
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Establish WiFi connection
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Debugging - Output the IP Address of the ESP32
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(client_id)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}



const int button = 25;            // GPIO 25 for the button
const int led = 27;                // GPIO 27 for the LED
const int pir = 26;               // GPIO 26 for PIR sensor
int led_flag = 0;                   // LED status flag
int pir_flag = 0;                   // PIR status flag
int pir_zero_count = QUEUE_SIZE;        // set zero count to queue size since queue is filled with zeros at start
int pir_ones_count = 0;                 // set ones count to zero 
int pir_queue_pop = -1;            // initialize to -1
int pir_failsafe_counter = QUEUE_SIZE;      // amount of time to wait before failsafe triggers
unsigned long prev_button_press_time = 0;   // stores last time LED was updated
unsigned long prev_pir_check_time = 0;      // stores last time the pir has sent an output
ArduinoQueue<int> q(QUEUE_SIZE);
String chill = "1 ";
String tmp = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -21600; // central time is GMT-6
const int   daylightOffset_sec = 3600;

void setup() {
  Serial.begin(2400);
  pinMode(button,INPUT);         // define button as an input 
  pinMode(led,OUTPUT);           // define LED as an output
  pinMode(pir, INPUT);           // define PIR as an input
  digitalWrite(led,LOW);         // turn output off just in case
  for(int i = 0; i < QUEUE_SIZE; i++)  q.enqueue(0);
  connect_MQTT();
  // get current time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  unsigned long current_time_millis = millis(); // update the current time 

  if (digitalRead(button)==HIGH){ // if button is pressed
    // Serial.println("button pressed");
    if (led_flag==0) {             // and the status flag is LOW
      led_flag=1;                  // make status flag HIGH
      digitalWrite(led,HIGH);     // and turn on the LED

      }                           // 
    else {                        // otherwise...
      led_flag=0;                  // make status flag LOW
      digitalWrite(led,LOW);      // and turn off the LED
    }


    // PUBLISH to the MQTT Broker on button press
    // Everytime LED changes, we must publish this information to the broker
    tmp = chill+(String(led_flag));
    // Serial.println("HEYDUDE@@@@@");
    // Serial.println(tmp);
    // delay(100000);

    if (client.publish(topic, tmp.c_str())) {
        Serial.println("Availability sent!");
    }
    // client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
    else {
        Serial.println("Availability failed to send. Reconnecting to MQTT Broker and trying again");
        client.connect(client_id);
        delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
        client.publish(topic, tmp.c_str());
    }

    // for(int i = 0; i < QUEUE_SIZE; i++){
    //   q.dequeue();
    //   q.enqueue(0);
    // }
    // for(int i = 0; i < QUEUE_SIZE; i++)
    //   Serial.print(q.getHead());
    // Serial.println();
    // pir_failsafe_counter = QUEUE_SIZE;
    prev_button_press_time = current_time_millis; // every time button clicked, set prev time button was pressed = current time
    // Serial.print("LED Flag: ");
    // Serial.println(led_flag);
    delay(INTERVAL);                    // wait a sec for the 
  }  


  // PIR motion sensor queue updated every 1 second
  if(current_time_millis - prev_pir_check_time >= INTERVAL){
    prev_pir_check_time = current_time_millis;
    pir_queue_pop = q.dequeue();
    if(pir_queue_pop == 1) pir_ones_count--;
    else pir_zero_count--;


    if (digitalRead(pir)==HIGH){ // if PIR sensor is high then push 1 to the queue
      q.enqueue(1);
      pir_ones_count++;
      Serial.println("PIR HIGH");
    }
    else{                        // if PIR sensor is low then push 0 to the queue
      q.enqueue(0); 
      pir_zero_count++;
      Serial.println("PIR LOW");
    }
    if(float(pir_ones_count)/float(QUEUE_SIZE) >= 0.7) pir_flag = 1;
    else if(float(pir_ones_count)/float(QUEUE_SIZE) < 0.4) pir_flag = 0;

    for(int i = 0; i < QUEUE_SIZE; i++){
      int temp = q.dequeue();
      Serial.print(temp);
      q.enqueue(temp);
    }
    Serial.println();
    
    Serial.print("ones: ");
    Serial.println(pir_ones_count);
    Serial.print("zero: ");
    Serial.println(pir_zero_count);
    Serial.println();
  }

  // PIR Failsafe
  // Engages every INTERVAL * QUEUE_SIZE seconds AFTER button press
  // If someone presses button, we want a delay of INTERVAL * QUEUE seconds to allow for user interaction / leaving period + accurate ratio
  // If button has not been pressed for a while current_time large, prev_button_press_time small thus, want to check if possible error in usage
  if(current_time_millis - prev_button_press_time >= INTERVAL * QUEUE_SIZE){
    if(pir_flag && !led_flag){
        led_flag = 1;
        digitalWrite(led,HIGH);     // and turn on the LED
        Serial.println("FAILSAFE: Activity but LED OFF");
        prev_button_press_time = current_time_millis; // every time failsafe activates clicked, set prev time button was pressed = current time

        tmp = chill+(String(led_flag));
        // PUBLISH to the MQTT Broker on button press
        // Everytime LED changes, we must publish this information to the broker
        if (client.publish(topic,tmp.c_str())) {
            Serial.println("Availability sent!");
        }
        // client.publish will return a boolean value depending on whether it succeded or not.
        // If the message failed to send, we will try again, as the connection may have broken.
        else {
            Serial.println("Availability failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(client_id);
            delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
          client.publish(topic, tmp.c_str());
        }
    }
    else if(!pir_flag && led_flag){
        led_flag = 0;
        digitalWrite(led,LOW);     // and turn on the LED
        Serial.println("FAILSAFE: No activity but LED ON");
        prev_button_press_time = current_time_millis; // every time failsafe activates clicked, set prev time button was pressed = current time

        tmp = chill+(String(led_flag));
        // PUBLISH to the MQTT Broker on button press
        // Everytime LED changes, we must publish this information to the broker
        if (client.publish(topic, tmp.c_str())) {
            Serial.println("Availability sent!");
        }
        // client.publish will return a boolean value depending on whether it succeded or not.
        // If the message failed to send, we will try again, as the connection may have broken.
        else {
            Serial.println("Availability failed to send. Reconnecting to MQTT Broker and trying again");
            client.connect(client_id);
            delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
            client.publish(topic, tmp.c_str());
        }
    }   
  }
  // check if it's time to deep sleep 
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  if (!strcmp(timeHour,"11")){
    esp_sleep_enable_timer_wakeup(5000000);
    Serial.println("Entering Deep Sleep Mode...zzzzzzzzzzzzzzzz");
    esp_deep_sleep_start();
  }                      
}                                 // begin again