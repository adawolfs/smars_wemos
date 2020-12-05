#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <wifiAuth.h>
#define USE_SERIAL Serial

// Create a webpage with a JS WS Client implementation
// This is stored at flash memory, ref: https://arduino.stackexchange.com/questions/77770/confuse-about-progmem-and-r
char webpage[] PROGMEM = R"=====(
<html>
<head>
  <script>
    var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
    connection.onopen = function () {
      connection.send('Connect ' + new Date());
    };
    connection.onerror = function (error) {
      console.log('WebSocket Error ',
        error);
    };
    connection.onmessage = function (e) {
      console.log('Server: ', e.data);
    };

    function sendON(){
      connection.send("+")
    }
    function sendOFF() {
        connection.send("-")
    }
  </script>
</head>

<body>LED Control:<br /><br />
  <input type="button" value="ON" onclick="sendON();" />
  <input type="button" value="OFF" onclick="sendOFF();" />
</body>
</html>
)=====";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    USE_SERIAL.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  }
  break;
  case WStype_TEXT:
    USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

    if (payload[0] == '+')
    {
      //LED ON
      digitalWrite(LED_BUILTIN, 0);
    }
    if (payload[0] == '-')
    {
      //LED OFF
      digitalWrite(LED_BUILTIN, 1);
    }

    break;
  }
}

// Load the basic led control webpage
void handleRoot()
{
  digitalWrite(LED_BUILTIN, 1);
  server.send_P(200, "text/plain", webpage);
  digitalWrite(LED_BUILTIN, 0);
  delay(500);
}

// This is an standard setup for not found handle
void handleNotFound()
{
  digitalWrite(LED_BUILTIN, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  delay(500);
  digitalWrite(LED_BUILTIN, 0);
}

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  USE_SERIAL.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  USE_SERIAL.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    USE_SERIAL.print(".");
  }
  USE_SERIAL.println("");
  USE_SERIAL.print("Connected to ");
  USE_SERIAL.println(ssid);
  USE_SERIAL.print("IP address: ");
  USE_SERIAL.println(WiFi.localIP());

  // Start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if (MDNS.begin("esp8266"))
  {
    USE_SERIAL.println("MDNS responder started");
  }

  // Define server handlers
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();

  USE_SERIAL.println("HTTP server started");

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
}

void loop(void)
{
  webSocket.loop();
  server.handleClient();
}