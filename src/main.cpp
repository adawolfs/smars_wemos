#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <wifiAuth.h>
#define USE_SERIAL Serial
#define MOVE_DELAY 100
// Create a webpage with a JS WS Client implementation
// This is stored at flash memory, ref: https://arduino.stackexchange.com/questions/77770/confuse-about-progmem-and-r
char webpage[] PROGMEM = R"=====(
<html>
<head>
  <title>SMARS Controller</title>
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
  <style>
    body {
      background-color: #000000;
      color: #ffffff;
      font-family: Arial, Helvetica, sans-serif;
      font-size: 1.5em;
      text-align: center;
    }
    input[type="button"]{
      background-color: #ffffff;
      color: #000000;
      font-size: 1.5em;
      padding: 10px;
      margin: 10px;
      width: 100px;
      border-radius: 10px;
      border: 1px solid #000000;
    }
    input[type="button"]:focus {
        background-color: red;
    }
  </style>
</head>

<body>
<h2>SMARS Control:</h2>
<br/>
<br/>
<table>
  <tr>
    <td></td>
    <td><input type="button" value="&uarr;" onclick="connection.send('U')" /></td>
    <td></td>
    <td width="100px"></td>
    <td><input type="button" value="ON" onclick="sendON();" /></td>
  </tr>
  <tr>
    <td><input type="button" value="&larr;" onclick="connection.send('L')" /></td>
    <td></td>
    <td><input type="button" value="&rarr;" onclick="connection.send('R')" /></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td></td>
    <td><input type="button" value="&darr;" onclick="connection.send('D')" /></td>
    <td></td>
    <td></td>
    <td><input type="button" value="OFF" onclick="sendOFF();" /></td>
  </tr>
</table>
  
  
</body>
</html>
)=====";

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);



void up()
{
  // Motor 1
  digitalWrite(D5, 1);
  digitalWrite(D6, 0);
  // Motor 2
  digitalWrite(D7, 1);
  digitalWrite(D8, 0);
}

void down()
{
  // Motor 1
  digitalWrite(D5, 0);
  digitalWrite(D6, 1);
  // Motor 2
  digitalWrite(D7, 0);
  digitalWrite(D8, 1);
}

void right()
{
  // Motor 1
  digitalWrite(D5, 1);
  digitalWrite(D6, 0);
  // Motor 2
  digitalWrite(D7, 0);
  digitalWrite(D8, 1);
}

void left()
{
  // Motor 1
  digitalWrite(D5, 0);
  digitalWrite(D6, 1);
  // Motor 2
  digitalWrite(D7, 1);
  digitalWrite(D8, 0);
}

void stop()
{
  // Motor 1
  digitalWrite(D5, 0);
  digitalWrite(D6, 0);
  // Motor 2
  digitalWrite(D7, 0);
  digitalWrite(D8, 0);
}


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
      // LED ON
      digitalWrite(LED_BUILTIN, 0);
    }
    if (payload[0] == '-')
    {
      digitalWrite(LED_BUILTIN, 1);
    }
    if (payload[0] == 'U')
    {
      up();
      delay(MOVE_DELAY);
      stop();
    }
    if (payload[0] == 'D')
    {
      down();
      delay(MOVE_DELAY);
      stop();
    }
    if (payload[0] == 'R')
    {
      right();
      delay(MOVE_DELAY);
      stop();
    }
    if (payload[0] == 'L')
    {
      left();
      delay(MOVE_DELAY);
      stop();
    }
    break;
  }

}

// Load the basic led control webpage
void handleRoot()
{
  digitalWrite(LED_BUILTIN, 1);
  server.send_P(200, "text/html", webpage);
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
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
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