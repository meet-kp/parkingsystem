#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>
#include <Hash.h>

#define LED_PIN D8
#define RELAY_PIN D7
#define BUTTON_PIN D1
#define LOCK_DURATION 10000 // 10 seconds in milliseconds

// Global boolean variables
bool ledState = false;
bool relayState = false;
bool buttonState = LOW;
bool lastButtonState = LOW;
int buttonPressCount = 0;
unsigned long buttonPressTime = 0;

ESP8266WiFiMulti WiFiMulti;
SocketIOclient socketIO;

// Select the IP address according to your local network
IPAddress serverIP(49, 249, 200, 132);
uint16_t serverPort = 3000; //8080;    //3000;

void socketIOEvent(const socketIOmessageType_t& type, uint8_t* payload, const size_t& length);
void sendInstructionToStation(const String& status);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  while (!Serial)
  {
    delay(100);
  }

  Serial.println("\nStart ESP8266_WebSocketClientSocketIO");

  // Disable AP
  if (WiFi.getMode() & WIFI_AP)
  {
    WiFi.softAPdisconnect(true);
  }

  WiFiMulti.addAP("mkp", "11111111");
  WiFiMulti.addAP("LEKEAMP", "87654321");

  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println();

  // Client address
  Serial.print("WebSockets Client started @ IP address: ");
  Serial.println(WiFi.localIP());

  // Server address, port, and URL
  Serial.print("Connecting to WebSockets Server @ IP address: ");
  Serial.print(serverIP);
  Serial.print(", port: ");
  Serial.println(serverPort);

  // Set the reconnect interval to 10s, new from v2.5.1 to avoid flooding the server. Default is 0.5s
  socketIO.setReconnectInterval(10000);

  socketIO.setExtraHeaders("Authorization: 1234567890");
  // 1= User Name, 2= userId, 3=usertype
  socketIO.setExtraHeaders("data:test,6707ab2b-70c7-480e-83ee-d472143a8b57,station");

  socketIO.begin(serverIP, serverPort);

  // Event handler
  socketIO.onEvent(socketIOEvent);
}

void loop()
{
  socketIO.loop();

  // Check button state
  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState != lastButtonState)
  {
    if (buttonState == LOW)
    {
      buttonPressTime = millis();
      buttonPressCount++;

      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      digitalWrite(RELAY_PIN, ledState);

      Serial.print("Button Pressed! Count: ");
      Serial.println(buttonPressCount);

      // Send update to server
      String status = ledState ? "lock" : "unlock";
      sendData(status);

      delay(2000);
    }

    lastButtonState = buttonState;
  }

  if (ledState && (millis() - buttonPressTime >= LOCK_DURATION))
  {
    ledState = false;
    relayState = false;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);

    Serial.println("LED and relay turned off");
    sendData("unlock");
  }
}

void socketIOEvent(const socketIOmessageType_t& type, uint8_t* payload, const size_t& length)
{
  switch (type)
  {
    case sIOtype_CONNECT:
      Serial.print("[IOc] Connected to url: ");
      Serial.println((char*)payload);

      // Join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");

      break;

    case sIOtype_EVENT:
      Serial.print("[IOc] Get event: ");
      Serial.println((char*)payload);

      // Handle incoming event
      if (strcmp((char*)payload, "SendInstructiontoStation") == 0)
      {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
          Serial.println("Failed to parse JSON");
          return;
        }
        String status = doc["status"].as<String>();

        // String lockId = doc["lockId"].as<String>();
        // String userId = doc["userId"].as<String>();
        // String stationId = doc["stationId"].as<String>();
        // String status = doc["status"].as<String>();
         
         if (status == "unlock")
    {
      // Turn on the relay and LED
      ledState = true;
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Unlock command received from serevr");

      // Wait for 10 seconds
      delay(10000);

      // Turn off the relay and LED
      ledState = false;
      digitalWrite(LED_PIN, LOW);
      digitalWrite(RELAY_PIN, LOW);

      Serial.println("Unlock command executed");
    }
      else if (status == "lock")
    {
      // Turn on the relay and LED
      ledState = true;
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(RELAY_PIN, HIGH);

      Serial.println("Lock command received");
    }


        // Serial.println("Received data:");
        // Serial.println("Lock ID: ");
        // Serial.println(lockId);
        // Serial.println("User ID: ");
        // Serial.println(userId);
        // Serial.println("Station ID: ");
        // Serial.println(stationId);
        Serial.println("Status: ");
        Serial.println(status);
      }

      break;

    case sIOtype_ACK:
      Serial.print("[IOc] Get ack: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_ERROR:
      Serial.print("[IOc] Get error: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_BINARY_EVENT:
      Serial.print("[IOc] Get binary: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_BINARY_ACK:
      Serial.print("[IOc] Get binary ack: ");
      Serial.println(length);

      hexdump(payload, length);
      break;

    case sIOtype_PING:
      Serial.println("[IOc] Get PING");
      break;

    case sIOtype_PONG:
      Serial.println("[IOc] Get PONG");
      break;

    default:
      break;
  }
}

void sendData(const String& status)
{
  // Prepare the JSON payload
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();
  array.add("sendData");
  JsonObject param1 = array.createNestedObject();
  param1["lockId"] = "1cde0ac5-1b4d-4342-9cd7-c152cb2fb0f8";
  param1["userId"] = "cf421bcd-ef92-4606-8868-b5cf2b02d6a3";
  param1["stationId"] = "6707ab2b-70c7-480e-83ee-d472143a8b57";
  param1["status"] = status;

  // Serialize the JSON document to a string
  String output;
  serializeJson(doc, output);

  // Send the JSON payload to the server
  socketIO.sendEVENT(output);
}
