// Libraries
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <math.h>
#include "logo.h"
#include "gears.h"
#include "wifi_animation.h"
#include "wifi_failed.h"
#include "wifi_connected.h"

// Firebase Credentials
#define FIREBASE_API_KEY "<-copy firebase api key here->"
#define FIREBASE_DATABASE_URL "<-copy database URL here->"

// Screen properties
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MQ2 and MQ9 Analog Pins
#define MQ2_PIN 34
#define MQ9_PIN 36

float MQ2_RL = 10.0; // MQ2 RL value
float MQ9_RL = 10.0; // MQ9 RL value

float Ro_MQ2 = 10.0;// MQ2 R0 value
float Ro_MQ9 = 2.0; // MQ2 R0 value

float calculateRS(int pin, float RL); // Call function to calculate RS
float getHCppm(float ratio); // Call fucntion to calculate HC in PPM
float getCOppm(float ratio); // Call function to calculate CO in PPM

String ssid = ""; // SSID variable
String password = ""; // Password variable

// Firebase config
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// Time  client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);

// Keypad declaration
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Keypad Pins
byte rowPins[ROWS] = {14, 27, 26, 25};
byte colPins[COLS] = {33, 32, 18, 19};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String inputBuffer = ""; // Input Buffer
unsigned long lastKeyTime = 0;
int keyIndex = 0;
int specialIndex = 0;
char lastKey = '\0';
bool vehicleInputDone = false; // Vehicle number input
bool ssidInputDone = false; // SSID input
bool passwdInputDone = false; // Password input
bool useUpperCase = false; // Switch between Upper and lower case

// Upper case keypad
const char* t9MapUpper[10] = {
  " 0", "-1", "ABC2", "DEF3", "GHI4", "JKL5",
  "MNO6", "PQRS7", "TUV8", "WXYZ9"
};

// Lower case keypad
const char* t9MapLower[10] = {
  " 0", "-1", "abc2", "def3", "ghi4", "jkl5",
  "mno6", "pqrs7", "tuv8", "wxyz9"
};

// Speacial characters
const char specialCycle[] = "*#$";


// WiFi connection icon bitmap
const unsigned char wifi_icon[] PROGMEM = {
  0x00, 0x00, 0x01, 0x00, 0x01, 0x80, 0x01, 0x80, 0x0d, 0x80, 0x0d, 0x80, 0x6d, 0x80, 0x6d, 0x80, 
	0x6d, 0x80, 0x00, 0x00
};

void setup() {
  Serial.begin(9600); // Initiate serial monitor
  Wire.begin(21, 22); // Initiate screen
  delay (500); // 0.5s delay

  // Check if the screen initiated properly
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  } 

  logoAnimation(); // Show the logo animation
  delay(1000); // 1s delay

  // Show gears spinning animation
  bitmapAnimation288(GEAR_FRAME_WIDTH, GEAR_FRAME_HEIGHT, GEAR_FRAME_COUNT, GEAR_FRAME_DELAY, gear_frames);

  displaySSIDInput(); // Display to enter SSID
}

void loop() {
  char key = keypad.getKey(); // Variable for keypad input

  // Check if the SSID input is done or not
  if(!ssidInputDone){
    handleSSIDInput(key); // Call SSID input function
    return;
  }

  // Check if the password input is done or not
  if(!passwdInputDone){
    handlePasswordInput(key); // Call password input function
    return;
  }

  // Check if the vehicle number input is done or not
  if (!vehicleInputDone) {
    handleVehicleInput(key); // Call vehicle number input function
    return;
  }

  // If 'B' key is pressed go back to the vehicle input display
  if (key == 'B') {
    vehicleInputDone = false;
    inputBuffer = "";
    Firebase.deleteNode(firebaseData, "/currentVehicle");
    displayVehicleInput();
    return;
  }

  static unsigned long lastReadingTime = 0;
  if (millis() - lastReadingTime >= 1000) {
    lastReadingTime = millis();

    float rs_mq2 = calculateRS(MQ2_PIN, MQ2_RL); // Calculate MQ2 RS value
    float rs_mq9 = calculateRS(MQ9_PIN, MQ9_RL); // Calculate MQ9 RS value

    float ratio_mq2 = rs_mq2 / Ro_MQ2; // Calculate RS/R0 for MQ2
    float ratio_mq9 = rs_mq9 / Ro_MQ9; // Calculate RS/R0 for MQ9

    Serial.print("MQ2: "); Serial.print(ratio_mq2); // Display CO in PPM 
    Serial.print(" | MQ9: "); Serial.println(ratio_mq9);

    Serial.print("MQ2: "); Serial.print(analogRead(36)); // Display CO in PPM 
    Serial.print(" | MQ9: "); Serial.println(analogRead(34));

    float ppm_hc = getPPM(ratio_mq2, -0.47, 1.28); // Calculate HC ppm value
    float ppm_co = getPPM(ratio_mq9, -0.77, 1.45); // Calculate CO ppm value

    Serial.print("MQ2: "); Serial.print(ppm_hc); // Display CO in PPM 
    Serial.print(" | MQ9: "); Serial.println(ppm_co); // Display HC in PPM

    display.clearDisplay();
    drawStatusIcons(); // Show the time and wifi connectivity on the top of screen

    display.setTextSize(1);
    display.setCursor(0, 10);
    display.print(F("Vehicle: "));
    display.println(inputBuffer); // Show the current vehicle number
    display.drawLine(64, 20, 64, SCREEN_HEIGHT, WHITE);

    // Display HC and CO in PPM on screen
    display.setTextSize(2);
    display.setCursor(8, 30); display.print(F("HC"));
    display.setCursor(70, 30); display.print(F("CO"));
    display.setTextSize(1);
    display.setCursor(33, 30); display.print(F("(ppm)"));
    display.setCursor(96, 30); display.print(F("(ppm)"));
    display.setCursor(10, 50); display.print(ppm_hc, 1);
    display.setCursor(74, 50); display.print(ppm_co, 2);
    display.display();

    String currentVehicle; // Variable for current vehicle
    // Check if the current vehicle field has a value
    if (Firebase.getString(firebaseData, "/currentVehicle")) {
      currentVehicle = firebaseData.stringData();
      currentVehicle.trim();
      if (currentVehicle.length() == 0) return;
    } else {
      Serial.println("[FAILED] Get currentVehicle: " + firebaseData.errorReason());
      return;
    }

    // Updatae the time
    timeClient.update(); 
    String timestamp = timeClient.getFormattedTime();
    String epoch = String(timeClient.getEpochTime());

    // Send MQ2 and MQ9 values to the database along with a time stamp
    sendReading(currentVehicle, "MQ2", ppm_hc, timestamp, epoch);
    sendReading(currentVehicle, "MQ9", ppm_co, timestamp, epoch);
  }
}

// Calculate sensor RS value
float calculateRS(int pin, float RL) {
  int adc = analogRead(pin);
  if (adc <= 0) adc = 1;  // Avoid division by 0
  float vrl = (adc / 1023.0) * 5.0;
  return ((5.0 - vrl) / vrl) * RL;
}

// Return sensor value in PPM
float getPPM(float ratio, float m, float b) {
  if (ratio <= 0 || isnan(ratio) || isinf(ratio)) return 0.0;
  float log_ppm = (log10(ratio) - b) / m;
  return pow(10, log_ppm);
}

// Take WiFi SSID as input form user
void handleSSIDInput(char key) {
  if (!key) return;

  unsigned long now = millis();

  if (key == 'D') { // press 'D' to confirm entry
    ssidInputDone = true;
    Serial.println(inputBuffer);
    ssid = inputBuffer;
    inputBuffer = "";
    displayPasswordInput(); // Call password input function
    return;
  }

  if (key == 'C') { // press 'C' to clear input
    inputBuffer = "";
    displaySSIDInput();
    return;
  }

  if (key == 'A') { // press 'A' to backspace
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      displaySSIDInput();
    }
    return;
  }

  if (key == '#') { // press '#' to switch between lower and upper case
  useUpperCase = !useUpperCase;
  displaySSIDInput();
  return;
  }

  if (key == '*') { // press '*' to cycle between speacial characters
    const char* cycle = specialCycle;
    if (!inputBuffer.isEmpty() && lastKey == '*') {
      if ((now - lastKeyTime) < 800) {
        specialIndex = (specialIndex + 1) % strlen(cycle);
        inputBuffer.setCharAt(inputBuffer.length() - 1, cycle[specialIndex]);
      } else {
        specialIndex = 0;
        inputBuffer += cycle[specialIndex];
      }
    } else {
      specialIndex = 0;
      inputBuffer += cycle[specialIndex];
    }
    lastKeyTime = now;
    lastKey = '*';
    displaySSIDInput();
    return;
  }

  // Reads input characters
  if (isdigit(key)) {
    int index = key - '0';
    // Check if upper case or lower case is selected
    const char* map = useUpperCase ? t9MapUpper[index] : t9MapLower[index];
    if (key == lastKey && (now - lastKeyTime) < 800) {
      keyIndex = (keyIndex + 1) % strlen(map);
      inputBuffer.setCharAt(inputBuffer.length() - 1, map[keyIndex]);
    } else {
      keyIndex = 0;
      inputBuffer += map[keyIndex];
    }
    lastKeyTime = now;
    lastKey = key;
    displaySSIDInput();
  }
}

// Display SSID input sreen
void displaySSIDInput() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  display.println(F("Enter WiFi SSID"));
  display.drawLine(0, 20, SCREEN_WIDTH, 20, WHITE);

  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(inputBuffer, 0, 0, &x1, &y1, &w, &h);
  int16_t cx = (SCREEN_WIDTH - w) / 2;
  display.setCursor(cx, 28);
  display.print(inputBuffer);
  display.display();
}

// Take WiFi password as input form user
void handlePasswordInput(char key) {
  if (!key) return;

  unsigned long now = millis();

  if (key == 'D') { // press 'D' to confirm entry
    passwdInputDone = true;
    Serial.println(inputBuffer);
    password = inputBuffer;
    WiFi.begin(ssid, password);
    display.clearDisplay();
    display.display();
    // Show wifi connecting animation
    bitmapAnimation288(WIFI_FRAME_WIDTH, WIFI_FRAME_HEIGHT, WIFI_FRAME_COUNT, WIFI_FRAME_DELAY, wifi_frames);
    Serial.println("Connecting to wifi");
    delay(5000); // 5s delay to wait for the WiFi to connect
    // Check if WiFi is connected
    if(WiFi.status() == WL_CONNECTED)
    {
      // If WiFi is connected
      inputBuffer = "";
      Serial.println("Connected to wifi");
      display.clearDisplay();
      display.display();
      // Show WiFi connected animation
      bitmapAnimation128(CONNECTED_FRAME_WIDTH, CONNECTED_FRAME_HEIGHT, CONNECTED_FRAME_COUNT, CONNECTED_FRAME_DELAY, connected_frames);
      // Firebase login
      auth.user.email = "<-copy user email here->";
      auth.user.password = "<-copy user password here->";
      config.api_key = FIREBASE_API_KEY;
      config.database_url = FIREBASE_DATABASE_URL;
      // Initiate firebase
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);
      
      timeClient.begin(); // Initiate  time client
      displayVehicleInput(); // Display vehicle number input screen
    }
    else{
      // If WiFi not connected
      inputBuffer = "";
      Serial.println("Connection failed");
      display.clearDisplay();
      display.display();
      ssidInputDone = false;
      passwdInputDone = false;
      // Show WiFi not connected animation
      bitmapAnimation128(FAILED_FRAME_WIDTH, FAILED_FRAME_HEIGHT, FAILED_FRAME_COUNT, FAILED_FRAME_DELAY, failed_frames);

      delay(500); // 0.5s delay
      displaySSIDInput(); // Return back to SSID input screen
    }
    return;
  }

  if (key == 'C') { // press 'C' to clear input
    inputBuffer = "";
    displayPasswordInput();
    return;
  }

  if (key == 'A') { // press 'A' to backspace
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      displayPasswordInput();
    }
    return;
  }

  if (key == 'B') { // press 'B' to go back to SSID input screen
    vehicleInputDone = false;
    passwdInputDone = false;
    ssidInputDone = false;
    displaySSIDInput();
    return;
  }

  if (key == '#') { // press '#' to switch between lower and upper case
  useUpperCase = !useUpperCase;
  displayPasswordInput();
  return;
  }

  if (key == '*') { // press '*' to cycle between speacial characters
    const char* cycle = specialCycle;
    if (!inputBuffer.isEmpty() && lastKey == '*') {
      if ((now - lastKeyTime) < 800) {
        specialIndex = (specialIndex + 1) % strlen(cycle);
        inputBuffer.setCharAt(inputBuffer.length() - 1, cycle[specialIndex]);
      } else {
        specialIndex = 0;
        inputBuffer += cycle[specialIndex];
      }
    } else {
      specialIndex = 0;
      inputBuffer += cycle[specialIndex];
    }
    lastKeyTime = now;
    lastKey = '*';
    displayPasswordInput();
    return;
  }

  // Reads input characters
  if (isdigit(key)) {
    int index = key - '0';
    // Check if upper case or lower case is selected
    const char* map = useUpperCase ? t9MapUpper[index] : t9MapLower[index];
    if (key == lastKey && (now - lastKeyTime) < 800) {
      keyIndex = (keyIndex + 1) % strlen(map);
      inputBuffer.setCharAt(inputBuffer.length() - 1, map[keyIndex]);
    } else {
      keyIndex = 0;
      inputBuffer += map[keyIndex];
    }
    lastKeyTime = now;
    lastKey = key;
    displayPasswordInput();
  }
}

// Display WiFi password input screen
void displayPasswordInput() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.setTextColor(WHITE);
  display.println(F("Enter WiFi Password"));
  display.drawLine(0, 20, SCREEN_WIDTH, 20, WHITE);

  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(inputBuffer, 0, 0, &x1, &y1, &w, &h);
  int16_t cx = (SCREEN_WIDTH - w) / 2;
  display.setCursor(cx, 28);
  display.print(inputBuffer);
  display.display();
}

void handleVehicleInput(char key) {
  if (!key) return;

  unsigned long now = millis();

  if (key == 'D') { // press 'D' to confirm entry
    vehicleInputDone = true;
    // Change the current vehicle field in the database to inputBuffer
    if (Firebase.setString(firebaseData, "/currentVehicle", inputBuffer)) {
      Serial.println("[OK] Vehicle Set: " + inputBuffer);
    } else {
      Serial.println("[FAILED] Set Vehicle: " + firebaseData.errorReason());
    }
    return;
  }

  if (key == 'C') { // press 'C' to clear input
    inputBuffer = "";
    displayVehicleInput();
    return;
  }

  if (key == 'A') { // press 'A' to backspace
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      displayVehicleInput();
    }
    return;
  }

  if (key == 'B') { // press 'B' to go back to WiFi SSID input
    vehicleInputDone = false;
    ssidInputDone = false;
    passwdInputDone = false;
    displaySSIDInput();
    return;
  }

  if (isdigit(key)) {
    // Only able to enter Upper case characters
    int index = key - '0';
    if (key == lastKey && (now - lastKeyTime) < 800) {
      keyIndex = (keyIndex + 1) % strlen(t9MapUpper[index]);
      inputBuffer.setCharAt(inputBuffer.length() - 1, t9MapUpper[index][keyIndex]);
    } else {
      keyIndex = 0;
      inputBuffer += t9MapUpper[index][keyIndex];
    }
    lastKeyTime = now;
    lastKey = key;
    displayVehicleInput();
  }
}

// Display vehicle number input screen
void displayVehicleInput() {
  display.clearDisplay();
  drawStatusIcons();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println(F("Enter Vehicle Number"));
  display.drawLine(0, 30, SCREEN_WIDTH, 30, WHITE);

  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(inputBuffer, 0, 0, &x1, &y1, &w, &h);
  int16_t cx = (SCREEN_WIDTH - w) / 2;
  display.setCursor(cx, 38);
  display.print(inputBuffer);
  display.display();
}

// Display status bar on top of screen
void drawStatusIcons() {
  // Check if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    // Update time
    timeClient.update();
    String timeStr = timeClient.getFormattedTime().substring(0, 5);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(timeStr);
  }

  // Display WiFi icon only if connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    display.drawBitmap(SCREEN_WIDTH - 10, 0, wifi_icon, 10, 9, WHITE);
  }
}

// Sends data to the firebase reatime database
void sendReading(String vehicle, String sensor, float value, String timestamp, String epoch) {
  String path = "/vehicles/" + vehicle + "/readings/" + sensor + "/" + epoch; // Directory
  FirebaseJson json;
  json.set("value", value); // send value
  json.set("time", timestamp); // send time stamp
  if (Firebase.setJSON(firebaseData, path, json)) {
    Serial.println("[OK] " + sensor + " = " + String(value) + " at " + timestamp);
  } else {
    Serial.println("[FAILED] " + firebaseData.errorReason());
  }
}

// Logo Animation function
void logoAnimation() {
  int y;
  for (y = -64; y < 0; y += 8) {
    display.clearDisplay();
    display.drawBitmap(0, y, epd_bitmap_logo, 128, 64, WHITE);
    display.display();
    delay(25);
  }
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.drawBitmap(0, y + 3, epd_bitmap_logo, 128, 64, WHITE);
    display.display();
    delay(50);
    display.clearDisplay();
    display.drawBitmap(0, y, epd_bitmap_logo, 128, 64, WHITE);
    display.display();
    delay(50);
  }
}

// Function for bitmap animations for 48 bit resolution
void bitmapAnimation288(int frame_width, int frame_height, int frame_count, int frame_delay, const byte frames[][288]) {
  for (int i = 0; i < frame_count; i++) {
    display.clearDisplay();
    display.drawBitmap((SCREEN_WIDTH - frame_width) / 2, (SCREEN_HEIGHT - frame_height) / 2, frames[i], frame_width, frame_height, WHITE);
    display.display();
    delay(frame_delay);
  }
}

// Function for bitmap animation for 32 bit resolutin
void bitmapAnimation128(int frame_width, int frame_height, int frame_count, int frame_delay, const byte frames[][128]) {
  for (int i = 0; i < frame_count; i++) {
    display.clearDisplay();
    display.drawBitmap((SCREEN_WIDTH - frame_width) / 2, (SCREEN_HEIGHT - frame_height) / 2, frames[i], frame_width, frame_height, WHITE);
    display.display();
    delay(frame_delay);
  }
}
