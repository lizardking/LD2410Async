/**
* @file
* @example Webservice for Presence Detection with LD2410Async and ESP Async WebServer
* 
* @brief
* This example demonstrates how to use the LD2410Async library with an ESP32 to provide a web-based
* presence detection service. The ESP32 connects to a WiFi network, starts a web server, and serves
* a simple HTML page. The page connects via WebSocket to receive live presence detection data
* (presenceDetected, detectedDistance) from the LD2410 radar sensor.
*
*
* Dependencies:
*   - ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer
*   - AsyncTCP: https://github.com/me-no-dev/AsyncTCP
*   - LD2410Async library
*
* @warning
* Make sure to install the required libraries and adjust the configuration section below
* to match your hardware and network setup.
* 
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LD2410Async.h"

 // ========================= USER CONFIGURATION =========================

 /**
  * @name UART Configuration
  * @{
  */
#define RADAR_RX_PIN 16   ///< ESP32 pin that receives data from the radar (radar TX)
#define RADAR_TX_PIN 17   ///< ESP32 pin that transmits data to the radar (radar RX)
#define RADAR_BAUDRATE 256000 ///< UART baudrate for the radar sensor (default is 256000)
  /** @} */

  /**
   * @name WiFi Configuration
   * @{
   */
const char* WIFI_SSID = "YOUR_WIFI_SSID";         ///< WiFi SSID
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"; ///< WiFi password

// Set to true to use static IP, false for DHCP
#define USE_STATIC_IP true

// Static network configuration (only used if USE_STATIC_IP is true)
IPAddress LOCAL_IP(192, 168, 1, 123);    ///< ESP32 static IP address
IPAddress GATEWAY(192, 168, 1, 1);       ///< Network gateway
IPAddress SUBNET(255, 255, 255, 0);      ///< Network subnet mask
/** @} */

/**
 * @name Webserver Configuration
 * @{
 */
#define WEBSERVER_PORT 80 ///< HTTP/WebSocket server port
 /** @} */

 // ======================================================================

 /**
  * @brief HardwareSerial instance for UART1 (LD2410 sensor)
  */
HardwareSerial RadarSerial(1);

/**
 * @brief LD2410Async instance for radar communication
 */
LD2410Async radar(RadarSerial);

/**
 * @brief AsyncWebServer instance for HTTP/WebSocket
 */
AsyncWebServer server(WEBSERVER_PORT);

/**
 * @brief AsyncWebSocket instance for live data updates
 */
AsyncWebSocket ws("/ws");

/**
 * @brief Stores the latest presence detection state
 */
volatile bool latestPresenceDetected = false;

/**
 * @brief Stores the latest detected distance (cm)
 */
volatile uint16_t latestDetectedDistance = 0;

/**
 * @brief Notifies all connected WebSocket clients with the latest presence data.
 *
 * Sends a JSON string with the fields "presenceDetected" (bool) and "detectedDistance" (uint16_t).
 */
void notifyClients() {
    String msg = "{\"presenceDetected\":";
    msg += (latestPresenceDetected ? "true" : "false");
    msg += ",\"detectedDistance\":";
    msg += latestDetectedDistance;
    msg += "}";
    ws.textAll(msg);
}

/**
 * @brief Callback function called whenever new detection data arrives from the radar.
 *
 * @param sender Pointer to the LD2410Async instance (for multi-sensor setups)
 * @param presenceDetected True if presence is detected, false otherwise
 * @param userData User data (not used)
 */
void onDetectionDataReceived(LD2410Async* sender, bool presenceDetected, byte userData) {
    const LD2410Types::DetectionData& data = sender->getDetectionDataRef();
    latestPresenceDetected = data.presenceDetected;
    latestDetectedDistance = data.detectedDistance;
    notifyClients();
}

/**
 * @brief Handles WebSocket events (client connect/disconnect).
 *
 * @param server Pointer to the AsyncWebSocket server
 * @param client Pointer to the connected client
 * @param type Event type (connect, disconnect, etc.)
 * @param arg Event argument
 * @param data Event data
 * @param len Data length
 */
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
    void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected\n", client->id());
        // Send initial state
        notifyClients();
    }
    else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
}

/**
 * @brief HTML page served to clients.
 *
 * Connects to the WebSocket and displays live presence and distance data.
 */
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>LD2410 Presence Detection</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 2em; }
        .status { font-size: 2em; margin-bottom: 1em; }
        .distance { font-size: 1.5em; }
        .present { color: green; }
        .absent { color: red; }
    </style>
</head>
<body>
    <h2>LD2410 Presence Detection</h2>
    <div class="status" id="presence">Connecting...</div>
    <div class="distance" id="distance"></div>
    <script>
        let ws = new WebSocket('ws://' + location.host + '/ws');
        ws.onmessage = function(event) {
            let data = JSON.parse(event.data);
            let presence = document.getElementById('presence');
            let distance = document.getElementById('distance');
            if (data.presenceDetected) {
                presence.textContent = "Presence detected!";
                presence.className = "status present";
                distance.textContent = "Distance: " + data.detectedDistance + " cm";
            } else {
                presence.textContent = "No presence detected.";
                presence.className = "status absent";
                distance.textContent = "";
            }
        };
        ws.onopen = function() {
            document.getElementById('presence').textContent = "Waiting for data...";
        };
        ws.onclose = function() {
            document.getElementById('presence').textContent = "WebSocket disconnected.";
            document.getElementById('presence').className = "status";
        };
    </script>
</body>
</html>
)rawliteral";

/**
 * @brief Connects to WiFi using the user configuration.
 *
 * If USE_STATIC_IP is true, configures a static IP, subnet, and gateway.
 * Prints connection status and IP address to Serial.
 */
void setupWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
#if USE_STATIC_IP
    if (!WiFi.config(LOCAL_IP, GATEWAY, SUBNET)) {
        Serial.println("STA Failed to configure static IP");
    }
#endif
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (++retries > 40) { // 20 seconds timeout
            Serial.println("\nFailed to connect to WiFi!");
            return;
        }
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

/**
 * @brief Arduino setup function.
 *
 * Initializes serial, WiFi, radar sensor, and webserver.
 * Registers the detection data callback and sets up the WebSocket and HTTP handlers.
 */
void setup() {
    // Serial for debug output
    Serial.begin(115200);
    while (!Serial) { ; }
    Serial.println("LD2410Async Web Presence Detection Example");

    // Setup WiFi
    setupWiFi();

    // Start UART for radar
    RadarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);

    // Start radar background task
    if (radar.begin()) {
        Serial.println("Radar task started successfully.");
        radar.registerDetectionDataReceivedCallback(onDetectionDataReceived, 0);
    }
    else {
        Serial.println("ERROR! Could not start radar task.");
    }

    // WebSocket setup
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Serve HTML page at "/"
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", index_html);
        });

    // Start server
    server.begin();
    Serial.println("Webserver started.");
}

/**
 * @brief Arduino loop function.
 *
 * No code required; all logic is handled by callbacks and background tasks.
 */
void loop() {
    // Nothing to do here, everything is handled by callbacks and background tasks
}