#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

// Struktura pro definici pinů motoru
struct MOTOR_PINS
{
  int pinIN1;
  int pinIN2;
};
// Seznam pinů pro jednotlivé motory
std::vector<MOTOR_PINS> motorPins =
    {
        {13, 15}, // Pinování pro PRAVÝ MOTOR (IN1, IN2)
        {14, 2},  // Pinování pro LEVÝ MOTOR  (IN3, IN4)
};
// Definice pinu pro světlo
#define LIGHT_PIN 4
// Konstanty pro směry pohybu tanku
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0
// Identifikátory pro jednotlivé motory
#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1
// Směry pohybu motorů
#define FORWARD 1
#define BACKWARD -1
// /*Slouží k nastavení parametrů PWM (Pulse Width Modulation) pro kontrolu různých funkcí, jako je řízení rychlosti nebo intenzity světla u kamery*/
const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int PWMLightChannel = 3;
// Nastavení přístupových údajů k Wi-Fi síti
const char *ssid = "Tank";
const char *password = "12345678";
// Inicializace serveru a WebSocketů
AsyncWebServer server(80);
AsyncWebSocket wsTankInput("/TankInput");

// HTML kód pro stránku
const char *htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
 <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <style>
    body {
      background-color: #28008F;
      background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='100%25' height='100%25' viewBox='0 0 1600 800'%3E%3Cg %3E%3Cpath fill='%233600a5' d='M486 705.8c-109.3-21.8-223.4-32.2-335.3-19.4C99.5 692.1 49 703 0 719.8V800h843.8c-115.9-33.2-230.8-68.1-347.6-92.2C492.8 707.1 489.4 706.5 486 705.8z'/%3E%3Cpath fill='%234600bc' d='M1600 0H0v719.8c49-16.8 99.5-27.8 150.7-33.5c111.9-12.7 226-2.4 335.3 19.4c3.4 0.7 6.8 1.4 10.2 2c116.8 24 231.7 59 347.6 92.2H1600V0z'/%3E%3Cpath fill='%235800d2' d='M478.4 581c3.2 0.8 6.4 1.7 9.5 2.5c196.2 52.5 388.7 133.5 593.5 176.6c174.2 36.6 349.5 29.2 518.6-10.2V0H0v574.9c52.3-17.6 106.5-27.7 161.1-30.9C268.4 537.4 375.7 554.2 478.4 581z'/%3E%3Cpath fill='%236c00e9' d='M0 0v429.4c55.6-18.4 113.5-27.3 171.4-27.7c102.8-0.8 203.2 22.7 299.3 54.5c3 1 5.9 2 8.9 3c183.6 62 365.7 146.1 562.4 192.1c186.7 43.7 376.3 34.4 557.9-12.6V0H0z'/%3E%3Cpath fill='%238200FF' d='M181.8 259.4c98.2 6 191.9 35.2 281.3 72.1c2.8 1.1 5.5 2.3 8.3 3.4c171 71.6 342.7 158.5 531.3 207.7c198.8 51.8 403.4 40.8 597.3-14.8V0H0v283.2C59 263.6 120.6 255.7 181.8 259.4z'/%3E%3Cpath fill='%237304eb' d='M1600 0H0v136.3c62.3-20.9 127.7-27.5 192.2-19.2c93.6 12.1 180.5 47.7 263.3 89.6c2.6 1.3 5.1 2.6 7.7 3.9c158.4 81.1 319.7 170.9 500.3 223.2c210.5 61 430.8 49 636.6-16.6V0z'/%3E%3Cpath fill='%236608d7' d='M454.9 86.3C600.7 177 751.6 269.3 924.1 325c208.6 67.4 431.3 60.8 637.9-5.3c12.8-4.1 25.4-8.4 38.1-12.9V0H288.1c56 21.3 108.7 50.6 159.7 82C450.2 83.4 452.5 84.9 454.9 86.3z'/%3E%3Cpath fill='%23590bc4' d='M1600 0H498c118.1 85.8 243.5 164.5 386.8 216.2c191.8 69.2 400 74.7 595 21.1c40.8-11.2 81.1-25.2 120.3-41.7V0z'/%3E%3Cpath fill='%234e0db1' d='M1397.5 154.8c47.2-10.6 93.6-25.3 138.6-43.8c21.7-8.9 43-18.8 63.9-29.5V0H643.4c62.9 41.7 129.7 78.2 202.1 107.4C1020.4 178.1 1214.2 196.1 1397.5 154.8z'/%3E%3Cpath fill='%23440F9F' d='M1315.3 72.4c75.3-12.6 148.9-37.1 216.8-72.4h-723C966.8 71 1144.7 101 1315.3 72.4z'/%3E%3C/g%3E%3C/svg%3E");
      background-attachment: fixed;
      background-size: cover;
    }

    .container {
      display: flex;
      gap: 10px;
      flex-direction: column;
      align-items: center;
    }

    #cameraImage {
      width: clamp(250px, 50%, 800px);
      aspect-ratio: 16/9;
      box-shadow: 0 0 10px #22222250;
      border-radius: 15px;
      overflow: hidden;
      padding: 10px;
      background-color: #00000055
    }

    .button-layout {
      display: grid;
      max-width: 400px;
      grid-template-columns: repeat(3, 1fr);
      grid-template-rows: repeat(3, 1fr);
    }

    .button-layout>button:nth-of-type(1) {
      grid-column: 2;
      grid-rows: 2;
    }

    .button-layout>button:nth-of-type(2) {
      grid-column: 1;
      grid-rows: 2;
    }

    .button-layout>button:nth-of-type(3) {
      grid-column: 3;
      grid-rows: 2;
    }

    .button-layout>button:nth-of-type(4) {
      grid-column: 2;
      grid-rows: 3;
    }

    .button-layout>button {
      display: grid;
      place-content: center;
      background-color: transparent;
      border-radius: 12px;
      border: 2px solid #EEEEEE;
      transition: 100ms ease;
      cursor: pointer;
      backdrop-filter: blur(10px);
    }

    .button-layout>button:hover {
      background: #FFFFFF85;
    }

    .button-layout>button:active {
      background: #FFFFFFA5;
    }

    .button-layout>button>span {
      width: fit-content;
      height: fit-content;
    }

    .arrows {
      font-size: 40px;
      scale: 1.2;
      padding: 24px;
      color: #EEEEEE;
      filter: drop-shadow(5px);
    }

    .noselect {
      -webkit-touch-callout: none;
      /* iOS Safari */
      -webkit-user-select: none;
      /* Safari */
      -khtml-user-select: none;
      /* Konqueror HTML */
      -moz-user-select: none;
      /* Firefox */
      -ms-user-select: none;
      /* Internet Explorer/Edge */
      user-select: none;
      /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }

    .slider__container {}

    .slider__header {
      font-family: sans-serif;
      font-size: 2rem;
      margin-block: 0;
      padding-block: 1rem;
      color: whitesmoke;
    }

    .slider {
      -webkit-appearance: none;
      position: relative;
      width: 100%;
      height: 15px;
      border-radius: 5px;
      background: #a785ff00;
      outline: none;
      -webkit-transition: .2s;
      transition: opacity .2s;
      z-index: 10;
    }

    .slider__wrapper {
      --width: 0px;
      position: relative;
    }

    .slider__wrapper::before {
      content: '';
      height: 15px;
      width: 100%;
      background-color: #a785ff;
      position: absolute;
      left: 2px;
      top: 2px;
      border-radius: 5px;
    }

    .slider__wrapper::after {
      content: '';
      height: 15px;
      width: var(--width);
      background-color: #350782;
      position: absolute;
      left: 2px;
      top: 2px;
      border-radius: 5px;
    }

    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 25px;
      height: 25px;
      border-radius: 50%;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 25px;
      height: 25px;
      border-radius: 50%;
      background: #ac95e6;
      border: 2px solid whitesmoke;
      cursor: pointer;
    }
  </style>

</head>

<body class="noselect" align="center" style="background-color:white">

  <!--h2 style="color: teal;text-align:center;">Wi-Fi Camera &#128663; Control</h2-->

  <div class="container">
    <div class="button-layout">
      <button type="button" class="button" ontouchstart='sendButtonInput("MoveTank","1")' ontouchend='sendButtonInput("MoveTank","0")' onmousedown='sendButtonInput("MoveTank","1")' onmouseup='sendButtonInput("MoveTank","0")'><svg class="arrows" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
          <g transform="rotate(90 12 12)">
            <path fill="currentColor" d="m4.431 12.822l13 9A1 1 0 0 0 19 21V3a1 1 0 0 0-1.569-.823l-13 9a1.003 1.003 0 0 0 0 1.645" />
          </g>
        </svg></button>
      <button type="button" class="button" ontouchstart='sendButtonInput("MoveTank","3")' ontouchend='sendButtonInput("MoveTank","0")' onmousedown='sendButtonInput("MoveTank","3")' onmouseup='sendButtonInput("MoveTank","0")'><svg class="arrows" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
          <path fill="currentColor" d="m4.431 12.822l13 9A1 1 0 0 0 19 21V3a1 1 0 0 0-1.569-.823l-13 9a1.003 1.003 0 0 0 0 1.645" />
        </svg></button>
      <button type="button" class="button" ontouchstart='sendButtonInput("MoveTank","4")' ontouchend='sendButtonInput("MoveTank","0")' onmousedown='sendButtonInput("MoveTank","4")' onmouseup='sendButtonInput("MoveTank","0")'><svg class="arrows" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
          <g transform="rotate(180 12 12)">
            <path fill="currentColor" d="m4.431 12.822l13 9A1 1 0 0 0 19 21V3a1 1 0 0 0-1.569-.823l-13 9a1.003 1.003 0 0 0 0 1.645" />
          </g>
        </svg></button>
      <button type="button" class="button" ontouchstart='sendButtonInput("MoveTank","2")' ontouchend='sendButtonInput("MoveTank","0")' onmousedown='sendButtonInput("MoveTank","2")' onmouseup='sendButtonInput("MoveTank","0")'><svg class="arrows" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
          <g transform="rotate(-90 12 12)">
            <path fill="currentColor" d="m4.431 12.822l13 9A1 1 0 0 0 19 21V3a1 1 0 0 0-1.569-.823l-13 9a1.003 1.003 0 0 0 0 1.645" />
          </g>
        </svg></button>
    </div>
  </div>

  <div class="slider__container">
    <h6 class="slider__header">Light:</h6>
    <div class="slider__wrapper" id="wrapper">
      <input type="range" min="0" max="255" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
    </div>
  </div>
  
    <script>
      const slider = document.getElementById("Light")
      const wrapper = document.getElementById("wrapper")
      window.addEventListener("input", (e) => {
      const left = (slider.clientWidth / e.target.max) * e.target.value
      wrapper.style.setProperty('--width', left + "px");
      })
      var webSocketTankInputUrl = "ws:\/\/" + window.location.hostname + "/TankInput";      
      var websocketTankInput;
      
      function initTankInputWebSocket() 
      {
        websocketTankInput = new WebSocket(webSocketTankInputUrl);
        websocketTankInput.onopen    = function(event)
        {
          var lightButton = document.getElementById("Light");
          sendButtonInput("Light", lightButton.value);
        };
        websocketTankInput.onclose   = function(event){setTimeout(initTankInputWebSocket, 2000);};
        websocketTankInput.onmessage = function(event){};        
      }
      
      function initWebSocket() 
      {
        initTankInputWebSocket();
      }

      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketTankInput.send(data);
      }
    
      window.onload = initWebSocket;    
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

// Funkce pro ovládání rotace motoru podle směru
void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);
  }
}
// Funkce pro pohyb tanku na základě zadaného vstupu
void moveTank(int inputValue)
{
  Serial.printf("Got value as %d\n", inputValue);
  switch (inputValue)
  {

  case UP:
    rotateMotor(RIGHT_MOTOR, FORWARD);
    rotateMotor(LEFT_MOTOR, FORWARD);
    break;

  case DOWN:
    rotateMotor(RIGHT_MOTOR, BACKWARD);
    rotateMotor(LEFT_MOTOR, BACKWARD);
    break;

  case LEFT:
    rotateMotor(RIGHT_MOTOR, FORWARD);
    rotateMotor(LEFT_MOTOR, BACKWARD);
    break;

  case RIGHT:
    rotateMotor(RIGHT_MOTOR, BACKWARD);
    rotateMotor(LEFT_MOTOR, FORWARD);
    break;

  case STOP:
    rotateMotor(RIGHT_MOTOR, STOP);
    rotateMotor(LEFT_MOTOR, STOP);
    break;

  default:
    rotateMotor(RIGHT_MOTOR, STOP);
    rotateMotor(LEFT_MOTOR, STOP);
    break;
  }
}
// Obsluha root adresy pro zobrazení úvodní stránky
void handleRoot(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "File Not Found");
}
// Funkce pro zpracování WebSocket událostí pro ovládání tanku
void onTankInputWebSocketEvent(AsyncWebSocket *server,
                              AsyncWebSocketClient *client,
                              AwsEventType type,
                              void *arg,
                              uint8_t *data,
                              size_t len)
{
  // Zpracování různých událostí WebSocket pro ovládání tanku 
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    moveTank(0);
    ledcWrite(PWMLightChannel, 0);
    break;
  case WS_EVT_DATA:
    AwsFrameInfo *info;
    info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
      std::string myData = "";
      myData.assign((char *)data, len);
      std::istringstream ss(myData);
      std::string key, value;
      std::getline(ss, key, ',');
      std::getline(ss, value, ',');
      Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
      int valueInt = atoi(value.c_str());
      if (key == "MoveTank")
      {
        moveTank(valueInt);
      }
      else if (key == "Light")
      {
        ledcWrite(PWMLightChannel, valueInt);
      }
    }
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  default:
    break;
  }
}

void setUpPinModes()
{
  // Funkce pro nastavení režimů pinů
  ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);

  for (int i = 0; i < motorPins.size(); i++)
  {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
  }
  moveTank(STOP);

  pinMode(LIGHT_PIN, OUTPUT);
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);
}

void setup(void) 
{
  // Inicializace sériové komunikace
  setUpPinModes();
  Serial.begin(115200);

  // Nastavení přístupového bodu pro WiFi se 
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

//Nastavení a přiřazení eventů.
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsTankInput.onEvent(onTankInputWebSocketEvent);
  server.addHandler(&wsTankInput);

// Inicializace serveru a spuštění kamery
  server.begin();
  Serial.println("HTTP server started");
  setupCamera();
}

void loop()
{
  wsTankInput.cleanupClients();
}
