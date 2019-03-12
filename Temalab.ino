#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <Ticker.h>
#include <SoftwareSerial.h>

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
Ticker timer;
File fsUploadFile;
SoftwareSerial wemosSerial(D7, D8); // RX, TX

const byte numChars = 32;
char receivedChars[numChars];   // an array to store the received data
boolean newData = false;
int myInt = 999;

int sensor1 = 0;

int sensorData[8];
int number = 0;
//-------------------------------------------------------------------------------------------SETUP-------------------------------------------------------------------------------------------

void setup() {
  startWifi();
  startOta();
  setupFSBrowser();
  startWebSocket();
  startSpiffs();
  setupSerialCom();
  
}

//-------------------------------------------------------------------------------------------LOOP-------------------------------------------------------------------------------------------

void loop() {
  ArduinoOTA.handle();
  webSocket.loop();
  server.handleClient();
  recieveSerialData();
  if(number==0){
    Serial.println(getSpiffsData(1));
    //sendSensorData(1);
    number++;
  }
}

//-------------------------------------------------------------------------------------------saveData,getSpiffsData-------------------------------------------------------------------------------------------

void saveData(){
  File f = SPIFFS.open("/data.txt", "a");
  if(!f) {
    Serial.println("failed to save data to spiffs");  
  }
  for(int i = 0; i < 7; ++i){
    f.print(sensorData[i]);
    f.print(',');
  }
  f.println(sensorData[7]);
  f.close();
  /*
  f = SPIFFS.open("/data.txt", "r");
  while(f.available()){
    Serial.write(f.read());
  }
  f.close();*/
}
/*
 * Visszaadja egy String-be a paraméterként megadott szenzornak a Spiffs-ben tárolt értékeit
 */
String getSpiffsData(int sensor){
  String result;
  File f = SPIFFS.open("/data.txt", "r");
  if(!f) {
    Serial.println("failed to save data to spiffs");  
  }
  while(f.available()>0){
    for(int i=1; i<sensor;++i){
      f.find(',');
    }
    if(sensor!=8){
      result += f.readStringUntil(',');
      result += ",";
      f.find('\n');
    }
    else{
      result += f.readStringUntil('\n');
      result += ",";
    }
  }
  return result;
}
//-------------------------------------------------------------------------------------------startWIFI-------------------------------------------------------------------------------------------

void startWifi() {
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  //wifiManager.startConfigPortal();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

//-------------------------------------------------------------------------------------------startOta-------------------------------------------------------------------------------------------

void startOta() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    SPIFFS.end();
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

//-------------------------------------------------------------------------------------------startWebsocket-------------------------------------------------------------------------------------------

void startWebSocket() {
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  if(type == WStype_TEXT){
    if((char)payload[0] == 's'){
      wemosSerial.print((char) payload[1]);
      Serial.print((char) payload[1]);
    }
    else if((char)payload[0] == 'g'){
      sendSensorData(1);
    }
  }
}

void sendSensorData(int sensor){
  String data = getSpiffsData(sensor);
  String tmp;
  String json;
  while(data.indexOf(',')>0){
    tmp = data.substring(0, data.indexOf(','));
    json = "{\"value\":";
    json += tmp;
    json += "}";
    webSocket.broadcastTXT(json.c_str(), json.length());
    data = data.substring(data.indexOf(',')+1);
  }
  webSocket.broadcastTXT(data.c_str(), data.length());
}

void getData(){
  String json = "{\"value\":";
  json += sensorData[0];
  json += "}";
  webSocket.broadcastTXT(json.c_str(), json.length());
}

    

//-------------------------------------------------------------------------------------------startSpiffs-------------------------------------------------------------------------------------------

void startSpiffs() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

//-------------------------------------------------------------------------------------------StartServer && FSBrowser-------------------------------------------------------------------------------------------

void setupFSBrowser(void) {
  Serial.setDebugOutput(true);
  //SERVER INIT
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  server.begin();
  Serial.println("HTTP server started");

}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  // handling different URIs
  if (path.endsWith("/"))
    path += "chart.html";
  if (path.endsWith("/MainPage"))
    path += "MainPage.html";
  if (path.endsWith("/SensorChart"))
    path += "SensorChart.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz)) {
      path += ".gz";
    }
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------------------SerialCom.WithArduinoNano-------------------------------------------------------------------------------------------

void setupSerialCom(){
  wemosSerial.begin(115200);
  pinMode(D7,INPUT);
  pinMode(D8,OUTPUT);
}

void recieveSerialData(){
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
    
    while (wemosSerial.available() > 0 && newData == false) {
        rc = wemosSerial.read();

        if (rc != endMarker) {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars) {
                ndx = numChars - 1;
            }
        }
        else {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        getSensorData(receivedChars);
        //saveData();//SPIFFSBE kiírás
        newData = false;
    }
}

void getSensorData(char *str){
  String data(str);
  for(int i = 0; i < 7; ++i){
    String substr = data.substring(0, data.indexOf(','));
    sensorData[i] = substr.toInt();
    data = data.substring(data.indexOf(',')+1);
  }
  sensorData[7] = data.toInt();
  /*for(int k = 0; k < 8; ++k){
    Serial.println(sensorData[k]);
  }*/
}

///////////////////////////////////////////
