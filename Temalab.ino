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

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
Ticker timer;
File fsUploadFile;

//-------------------------------------------------------------------------------------------SETUP-------------------------------------------------------------------------------------------

void setup() {
  startWifi();
  startOta();
  setupFSBrowser();
  startWebSocket();
  startSpiffs();
  timer.attach(5, getData);
}

//-------------------------------------------------------------------------------------------LOOP-------------------------------------------------------------------------------------------

void loop() {
  ArduinoOTA.handle();
  webSocket.loop();
  server.handleClient();
}

//-------------------------------------------------------------------------------------------getData-------------------------------------------------------------------------------------------

void getData(){
  long rssi = WiFi.RSSI();
  Serial.println(rssi);
  File f = SPIFFS.open("/data.txt", "w");
  if (!f) {
      Serial.println("file open failed");
  }
  f.println(rssi);
  f.close();
  String json = "{\"value\":";
  json += rssi;
  json += "}";
  webSocket.broadcastTXT(json.c_str(), json.length());
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
    float dataRate = (float) atof((const char *) &payload[0]);
    timer.detach();
    timer.attach(dataRate, getData);
  }
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
  if (path.endsWith("/")) {
    path += "chart.html";
  }
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
