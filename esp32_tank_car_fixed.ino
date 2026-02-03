/*
 * ESP32 Internet-Controlled Tank-Style RC Car
 * 
 * Hardware Connections:
 * - ESC Channel 1 (Right Motors) Signal → GPIO 25
 * - ESC Channel 2 (Left Motors) Signal → GPIO 26
 * - ESC Grounds → ESP32 GND
 * - Power ESP32 via USB or ESC VCC (5V) to VIN pin
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ========== CONFIGURATION - CHANGE THESE ==========
const char* ssid = "Wi-Fi";        // Replace with your hotspot name
const char* password = "sapt090059"; // Replace with your hotspot password

// GPIO Pins for ESC control
const int RIGHT_MOTORS_PIN = 25;  // Channel 1 - Right side motors
const int LEFT_MOTORS_PIN = 26;   // Channel 2 - Left side motors

// PWM Settings (typical RC values: 1000-2000 microseconds)
const int PWM_MIN = 1000;     // Full reverse
const int PWM_NEUTRAL = 1500; // Stop/Neutral
const int PWM_MAX = 2000;     // Full forward

// Mixing sensitivity (how much steering affects motor speed)
const float STEERING_MIX = 0.5;  // Reduced from 1.0 for more predictable steering

// Keyboard control speed for turning
const int TURN_SPEED = 40;  // Speed when pressing left/right arrow keys (40%)

// ===================================================

Servo rightMotors;
Servo leftMotors;
WebServer server(80);

// Control inputs (-100 to +100)
int throttleInput = 0;  // Forward/Reverse: -100 (reverse) to +100 (forward)
int steeringInput = 0;  // Left/Right: -100 (left) to +100 (right)

// Calculated motor speeds
int rightSpeed = 0;
int leftSpeed = 0;

// Function to calculate differential steering
void calculateMotorSpeeds() {
  // Start with throttle as base speed
  float rightBase = throttleInput;
  float leftBase = throttleInput;
  
  // Apply steering mixing
  // Positive steering = right turn (slow down right, speed up left)
  // Negative steering = left turn (speed up right, slow down left)
  rightBase -= steeringInput * STEERING_MIX;
  leftBase += steeringInput * STEERING_MIX;
  
  // Constrain to -100 to +100 range
  rightSpeed = constrain((int)rightBase, -100, 100);
  leftSpeed = constrain((int)leftBase, -100, 100);
}

// Map control values (-100 to +100) to PWM microseconds
int mapToPWM(int value) {
  return map(value, -100, 100, PWM_MIN, PWM_MAX);
}

// Update motor speeds
void updateMotors() {
  calculateMotorSpeeds();
  
  int rightPWM = mapToPWM(rightSpeed);
  int leftPWM = mapToPWM(leftSpeed);
  
  rightMotors.writeMicroseconds(rightPWM);
  leftMotors.writeMicroseconds(leftPWM);
  
  Serial.printf("Throttle: %d, Steering: %d | Right: %d (%d us), Left: %d (%d us)\n", 
                throttleInput, steeringInput, rightSpeed, rightPWM, leftSpeed, leftPWM);
}

// HTML webpage for control interface
String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>ESP32 Tank Car Control</title>";
  html += "<style>";
  html += "* {box-sizing: border-box; margin: 0; padding: 0;}";
  html += "body {font-family: Arial, sans-serif; background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);";
  html += "color: white; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px;}";
  html += ".container {background: rgba(255,255,255,0.1); padding: 30px; border-radius: 20px;";
  html += "backdrop-filter: blur(10px); box-shadow: 0 8px 32px rgba(0,0,0,0.3); max-width: 800px; width: 100%;}";
  html += "h1 {text-align: center; margin-bottom: 30px; font-size: 2em;}";
  html += ".control-section {margin: 30px 0;}";
  html += ".control-section h3 {margin-bottom: 15px; font-size: 1.2em;}";
  html += ".slider-container {display: flex; align-items: center; gap: 15px; margin: 15px 0;}";
  html += ".slider-label {min-width: 80px; font-size: 0.9em;}";
  html += "input[type=\"range\"] {flex-grow: 1; height: 10px; border-radius: 5px;";
  html += "background: rgba(255,255,255,0.2); outline: none; -webkit-appearance: none;}";
  html += "input[type=\"range\"]::-webkit-slider-thumb {-webkit-appearance: none; width: 30px;";
  html += "height: 30px; border-radius: 50%; background: white; cursor: pointer; box-shadow: 0 2px 8px rgba(0,0,0,0.4);}";
  html += "input[type=\"range\"]::-moz-range-thumb {width: 30px; height: 30px; border-radius: 50%;";
  html += "background: white; cursor: pointer; box-shadow: 0 2px 8px rgba(0,0,0,0.4); border: none;}";
  html += ".value-display {min-width: 60px; text-align: center; font-weight: bold;";
  html += "font-size: 1.3em; background: rgba(0,0,0,0.2); padding: 8px; border-radius: 8px;}";
  html += ".button-grid {display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin: 20px 0;}";
  html += "button {padding: 15px; font-size: 1em; border: none; border-radius: 10px; cursor: pointer;";
  html += "background: white; color: #1e3c72; font-weight: bold; box-shadow: 0 4px 8px rgba(0,0,0,0.2);";
  html += "transition: transform 0.1s;}";
  html += "button:active {transform: translateY(2px); box-shadow: 0 2px 4px rgba(0,0,0,0.2);}";
  html += ".status {text-align: center; margin-top: 25px; padding: 15px;";
  html += "background: rgba(0,0,0,0.2); border-radius: 10px; font-size: 1.1em;}";
  html += ".motor-display {display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin: 20px 0;}";
  html += ".motor-info {background: rgba(0,0,0,0.2); padding: 15px; border-radius: 10px; text-align: center;}";
  html += ".motor-value {font-size: 2em; font-weight: bold;}";
  html += ".info-box {background: rgba(255,255,255,0.1); padding: 15px; border-radius: 10px; margin: 20px 0; font-size: 0.9em;}";
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>&#x1F69C; Tank Car Control</h1>";
  
  html += "<div class=\"info-box\">";
  html += "<strong>Keyboard Controls:</strong><br>";
  html += "Arrow Up = Forward (50%) | Arrow Down = Reverse (50%)<br>";
  html += "Arrow Left = Turn Left (40%) | Arrow Right = Turn Right (40%)<br>";
  html += "Release keys to AUTO-STOP";
  html += "</div>";
  
  html += "<div class=\"control-section\">";
  html += "<h3>Throttle (Forward/Reverse)</h3>";
  html += "<div class=\"slider-container\">";
  html += "<span class=\"slider-label\">Reverse</span>";
  html += "<input type=\"range\" id=\"throttle\" min=\"-100\" max=\"100\" value=\"0\" oninput=\"updateThrottle(this.value)\">";
  html += "<span class=\"slider-label\">Forward</span>";
  html += "</div><div class=\"value-display\" id=\"throttleValue\">0</div></div>";
  
  html += "<div class=\"control-section\">";
  html += "<h3>Steering (Left/Right)</h3>";
  html += "<div class=\"slider-container\">";
  html += "<span class=\"slider-label\">Left</span>";
  html += "<input type=\"range\" id=\"steering\" min=\"-100\" max=\"100\" value=\"0\" oninput=\"updateSteering(this.value)\">";
  html += "<span class=\"slider-label\">Right</span>";
  html += "</div><div class=\"value-display\" id=\"steeringValue\">0</div></div>";
  
  html += "<div class=\"motor-display\">";
  html += "<div class=\"motor-info\"><h4>Left Motors</h4><div class=\"motor-value\" id=\"leftMotorValue\">0</div></div>";
  html += "<div class=\"motor-info\"><h4>Right Motors</h4><div class=\"motor-value\" id=\"rightMotorValue\">0</div></div>";
  html += "</div>";
  
  html += "<div class=\"button-grid\">";
  html += "<button onclick=\"quickMove(-50, 50)\">&#8598; Rev Left</button>";
  html += "<button onclick=\"quickMove(-100, 0)\">&#8593; Reverse</button>";
  html += "<button onclick=\"quickMove(-50, -50)\">&#8599; Rev Right</button>";
  html += "<button onclick=\"quickMove(0, -100)\">&#8592; Left</button>";
  html += "<button onclick=\"stopCar()\">&#9940; STOP</button>";
  html += "<button onclick=\"quickMove(0, 100)\">Right &#8594;</button>";
  html += "<button onclick=\"quickMove(50, 50)\">&#8601; Fwd Left</button>";
  html += "<button onclick=\"quickMove(100, 0)\">&#8595; Forward</button>";
  html += "<button onclick=\"quickMove(50, -50)\">&#8600; Fwd Right</button>";
  html += "</div>";
  
  html += "<div class=\"status\" id=\"status\">Ready - Use Arrow Keys</div>";
  html += "</div>";
  
  html += "<script>";
  html += "let currentThrottle = 0;";
  html += "let currentSteering = 0;";
  html += "const TURN_SPEED = 40;";
  
  html += "function updateThrottle(value) {";
  html += "currentThrottle = parseInt(value);";
  html += "document.getElementById('throttleValue').textContent = currentThrottle;";
  html += "document.getElementById('throttle').value = currentThrottle;";
  html += "sendCommand();}";
  
  html += "function updateSteering(value) {";
  html += "currentSteering = parseInt(value);";
  html += "document.getElementById('steeringValue').textContent = currentSteering;";
  html += "document.getElementById('steering').value = currentSteering;";
  html += "sendCommand();}";
  
  html += "function quickMove(throttle, steering) {";
  html += "currentThrottle = throttle;";
  html += "currentSteering = steering;";
  html += "document.getElementById('throttleValue').textContent = currentThrottle;";
  html += "document.getElementById('steeringValue').textContent = currentSteering;";
  html += "document.getElementById('throttle').value = currentThrottle;";
  html += "document.getElementById('steering').value = currentSteering;";
  html += "sendCommand();}";
  
  html += "function stopCar() {";
  html += "currentThrottle = 0;";
  html += "currentSteering = 0;";
  html += "document.getElementById('throttleValue').textContent = 0;";
  html += "document.getElementById('steeringValue').textContent = 0;";
  html += "document.getElementById('throttle').value = 0;";
  html += "document.getElementById('steering').value = 0;";
  html += "sendCommand();";
  html += "document.getElementById('status').textContent = 'STOPPED';}";
  
  html += "function sendCommand() {";
  html += "fetch('/control?throttle=' + currentThrottle + '&steering=' + currentSteering)";
  html += ".then(response => response.json())";
  html += ".then(data => {";
  html += "document.getElementById('status').textContent = 'T:' + data.throttle + ' S:' + data.steering;";
  html += "document.getElementById('leftMotorValue').textContent = data.left;";
  html += "document.getElementById('rightMotorValue').textContent = data.right;";
  html += "}).catch(error => {";
  html += "document.getElementById('status').textContent = 'Error: ' + error;});}";
  
  // Arrow key controls - AUTO STOP when released
  html += "let keysPressed = {};";
  
  html += "document.addEventListener('keydown', function(e) {";
  html += "if (keysPressed[e.key]) return;";
  html += "keysPressed[e.key] = true;";
  
  html += "if (e.key === 'ArrowUp') {";
  html += "e.preventDefault();";
  html += "quickMove(50, 0);";
  html += "}";
  
  html += "else if (e.key === 'ArrowDown') {";
  html += "e.preventDefault();";
  html += "quickMove(-50, 0);";
  html += "}";
  
  html += "else if (e.key === 'ArrowLeft') {";
  html += "e.preventDefault();";
  html += "quickMove(0, -TURN_SPEED);";
  html += "}";
  
  html += "else if (e.key === 'ArrowRight') {";
  html += "e.preventDefault();";
  html += "quickMove(0, TURN_SPEED);";
  html += "}";
  html += "});";
  
  html += "document.addEventListener('keyup', function(e) {";
  html += "keysPressed[e.key] = false;";
  
  html += "if (e.key === 'ArrowUp' || e.key === 'ArrowDown' || e.key === 'ArrowLeft' || e.key === 'ArrowRight') {";
  html += "e.preventDefault();";
  html += "stopCar();";
  html += "}";
  html += "});";
  
  html += "</script></body></html>";
  
  return html;
}

// Handle root webpage request
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// Handle control commands
void handleControl() {
  if (server.hasArg("throttle") && server.hasArg("steering")) {
    throttleInput = constrain(server.arg("throttle").toInt(), -100, 100);
    steeringInput = constrain(server.arg("steering").toInt(), -100, 100);
    
    updateMotors();
    
    // Send JSON response with motor speeds
    String json = "{";
    json += "\"throttle\":" + String(throttleInput) + ",";
    json += "\"steering\":" + String(steeringInput) + ",";
    json += "\"left\":" + String(leftSpeed) + ",";
    json += "\"right\":" + String(rightSpeed);
    json += "}";
    
    server.send(200, "application/json", json);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// Handle not found
void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32 Tank Car Starting ===");
  
  // Initialize servo outputs for motors
  rightMotors.attach(RIGHT_MOTORS_PIN, PWM_MIN, PWM_MAX);
  leftMotors.attach(LEFT_MOTORS_PIN, PWM_MIN, PWM_MAX);
  
  // Set to neutral/stop position
  rightMotors.writeMicroseconds(PWM_NEUTRAL);
  leftMotors.writeMicroseconds(PWM_NEUTRAL);
  
  Serial.println("Motors initialized to STOP position");
  Serial.print("Right motors: GPIO ");
  Serial.println(RIGHT_MOTORS_PIN);
  Serial.print("Left motors: GPIO ");
  Serial.println(LEFT_MOTORS_PIN);
  
  // Connect to WiFi
  Serial.print("\nConnecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.println("========================================");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("========================================");
    Serial.println("Open this IP in your browser to control");
    Serial.println();
  } else {
    Serial.println("Failed to connect to WiFi!");
    Serial.println("Please check SSID and password");
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.onNotFound(handleNotFound);
  
  // Start server
  server.begin();
  Serial.println("Web server started!");
  Serial.println("Ready for commands...\n");
}

void loop() {
  server.handleClient();
}
