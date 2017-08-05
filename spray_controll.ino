// Sample Arduino Json Web Client
// Downloads and parse http://jsonplaceholder.typicode.com/users/1
//
// Copyright Benoit Blanchon 2014-2017
// MIT License
//
// Arduino JSON library
// https://bblanchon.github.io/ArduinoJson/
// If you like this project, please add a star!

#include <ArduinoJson.h>
#include <SPI.h>
#include <LWiFi.h>

#define SPRAY_PIN 7
char ssid[] = "***";      //  your network SSID (name)
char pass[] = "***";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)

//EthernetClient client;
int status = WL_IDLE_STATUS;
// Initialize the Wifi client library
WiFiClient client;

const char* server = "smartfarm.ap-northeast-1.elasticbeanstalk.com";  // server's address
const char* resource = "/api/v1/controllers/21001722-ce96-4ebc-a0f3-cbca69a7caa4/mode";                    // http resource

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 1L * 1000L; // delay between updates, in milliseconds

const unsigned long BAUD_RATE = 9600;                 // serial connection speed
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

// The type of data that we want to extract from the page
struct UserData {
  char mode[32];
  char delay_time[32];
};

// ARDUINO entry point #1: runs once when you press reset or power the board
void setup() {
    //Initialize serial and wait for port to open:
  Serial.begin(9600);
  pinMode(SPRAY_PIN, OUTPUT);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
}

// ARDUINO entry point #2: runs over and over again forever
void loop() {
    if (millis() - lastConnectionTime > postingInterval) {
  if (connect(server)) {
    if (sendRequest(server, resource) && skipResponseHeaders()) {
      UserData userData;
      if (readReponseContent(&userData)) {
        printUserData(&userData);
      }
    }
  }
  disconnect();
    }
}

// Open connection to the HTTP server
bool connect(const char* hostName) {
  Serial.print("Connect to ");
  Serial.println(hostName);

  bool ok = client.connect(hostName, 80);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

// Send the HTTP GET request to the server
bool sendRequest(const char* host, const char* resource) {
  Serial.print("GET ");
  Serial.println(resource);

  client.print("GET ");
  client.print(resource);
  client.println(" HTTP/1.0");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
  lastConnectionTime = millis();
  return true;
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }

  return ok;
}

bool readReponseContent(struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // See https://bblanchon.github.io/ArduinoJson/assistant/
  const size_t BUFFER_SIZE =
      JSON_OBJECT_SIZE(8)    // the root object has 8 elements
      + JSON_OBJECT_SIZE(5)  // the "address" object has 5 elements
      + JSON_OBJECT_SIZE(2)  // the "geo" object has 2 elements
      + JSON_OBJECT_SIZE(3)  // the "company" object has 3 elements
      + MAX_CONTENT_SIZE;    // additional space for strings

  // Allocate a temporary memory pool
  DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);

  JsonObject& root = jsonBuffer.parseObject(client);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  strcpy(userData->mode, root["mode"]);
  strcpy(userData->delay_time, root["delay_time"]);
  if(root["mode"]=="on"){
    digitalWrite(SPRAY_PIN, HIGH);
  }else if(root["mode"]=="off"){
    digitalWrite(SPRAY_PIN, LOW);
    }else if(root["mode"]=="auto"){
      digitalWrite(SPRAY_PIN, HIGH);
      delay(root["delay_time"]);
      digitalWrite(SPRAY_PIN, LOW);
      }
  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
}

// Print the data extracted from the JSON
void printUserData(const struct UserData* userData) {
  Serial.print("Mode = ");
  Serial.println(userData->mode);
  Serial.println("Delay = ");
  Serial.println(userData->delay_time);
}

// Close the connection with the HTTP server
void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
