#include <WiFi.h>
#include <WebServer.h>
#include "html_page.h" // Include the external HTML file

const char* ssid = "rc_UNIMAP";
const char* password = "12345678";

// Define pins for motor driver
const int pwm1 = 26;
const int dir1 = 25;
const int pwm2 = 27;
const int dir2 = 33;

int pwmSpeed = 128;
WebServer server(80);

// Path memory structure
struct Command {
  String cmd;
  uint32_t duration;
  uint8_t speed;
};
#define MAX_RECORDS 200
Command recordedPath[MAX_RECORDS];
int pathIndex = 0;
bool isRecording = false;
bool isReplaying = false;
int replayIndex = 0;
uint32_t replayStartTime = 0;
uint32_t lastCommandTime = 0;
String currentCmd = "";
String replayLog = "";

void setMotorSpeed(int motor, int speed, int direction) {
  if (motor == 1) {
    digitalWrite(dir1, direction);
    analogWrite(pwm1, speed);
  } else if (motor == 2) {
    digitalWrite(dir2, direction);
    analogWrite(pwm2, speed);
  }
}

void forward() {
  Serial.println("Command Received: Forward");
  setMotorSpeed(1, pwmSpeed, LOW);
  setMotorSpeed(2, pwmSpeed, LOW);
}

void reverse() {
  Serial.println("Command Received: Reverse");
  setMotorSpeed(1, pwmSpeed, HIGH);
  setMotorSpeed(2, pwmSpeed, HIGH);
}

void left() {
  Serial.println("Command Received: Left");
  setMotorSpeed(1, pwmSpeed, LOW);
  setMotorSpeed(2, pwmSpeed, HIGH);
}

void right() {
  Serial.println("Command Received: Right");
  setMotorSpeed(1, pwmSpeed, HIGH);
  setMotorSpeed(2, pwmSpeed, LOW);
}

void stopMotors() {
  Serial.println("Command Received: Stop");
  setMotorSpeed(1, 0, HIGH);
  setMotorSpeed(2, 0, HIGH);
}

void executeCommand(const Command& cmd) {
  pwmSpeed = cmd.speed;
  if (cmd.cmd == "forward") forward();
  else if (cmd.cmd == "reverse") reverse();
  else if (cmd.cmd == "left") left();
  else if (cmd.cmd == "right") right();
  else stopMotors();
}

void handleControl() {
  if (!server.hasArg("cmd")) return;
  String cmd = server.arg("cmd");

  if (cmd == "speed" && server.hasArg("value")) {
    pwmSpeed = server.arg("value").toInt();
    Serial.printf("Speed Updated: %d\n", pwmSpeed);
    server.send(200, "text/plain", String(pwmSpeed));
    return;
  }

  if (cmd == "start_recording") {
    isRecording = true;
    pathIndex = 0;
    server.send(200, "text/plain", "Recording started");
    return;
  }
  if (cmd == "stop_recording") {
    isRecording = false;
    server.send(200, "text/plain", "Recording stopped");
    return;
  }
  if (cmd == "replay") {
    if (pathIndex == 0) {
      server.send(200, "text/plain", "No path recorded");
      return;
    }
    isReplaying = true;
    replayIndex = 0;
    replayStartTime = millis();
    replayLog = "";
    executeCommand(recordedPath[0]);
    replayLog += "Step 1: " + recordedPath[0].cmd + " for " + String(recordedPath[0].duration) + " ms\n";
    server.send(200, "text/plain", "Replaying...");
    return;
  }
  if (cmd == "clear_memory") {
    pathIndex = 0;
    isRecording = false;
    isReplaying = false;
    currentCmd = "";
    replayLog = "";
    server.send(200, "text/plain", "Memory cleared");
    return;
  }

  if (cmd == "press") {
    currentCmd = server.arg("value");
    lastCommandTime = millis();
    if (currentCmd == "forward") forward();
    else if (currentCmd == "reverse") reverse();
    else if (currentCmd == "left") left();
    else if (currentCmd == "right") right();
    server.send(200, "text/plain", "Pressed " + currentCmd);
    return;
  }

  if (cmd == "release") {
    stopMotors();
    if (isRecording && pathIndex < MAX_RECORDS && currentCmd != "") {
      uint32_t duration = millis() - lastCommandTime;
      recordedPath[pathIndex++] = {currentCmd, duration, pwmSpeed};
      currentCmd = "";
    }
    server.send(200, "text/plain", "Released");
    return;
  }

  if (cmd == "stop") {
    stopMotors();
    server.send(200, "text/plain", "Stopped");
    return;
  }

  server.send(200, "text/plain", cmd);
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleLog() {
  server.send(200, "text/plain", replayLog);
}

void setup() {
  pinMode(pwm1, OUTPUT);
  pinMode(dir1, OUTPUT);
  pinMode(pwm2, OUTPUT);
  pinMode(dir2, OUTPUT);

  stopMotors();
  WiFi.softAP(ssid, password);
  Serial.begin(115200);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/log", handleLog);

  server.begin();
  Serial.println("Web Server Started");
}

void loop() {
  server.handleClient();

  if (isReplaying && replayIndex < pathIndex) {
    if (millis() - replayStartTime >= recordedPath[replayIndex].duration) {
      replayIndex++;
      if (replayIndex >= pathIndex) {
        isReplaying = false;
        stopMotors();
        replayLog += "Replay finished.\n";
      } else {
        executeCommand(recordedPath[replayIndex]);
        replayStartTime = millis();
        replayLog += "Step " + String(replayIndex + 1) + ": " + recordedPath[replayIndex].cmd + " for " + String(recordedPath[replayIndex].duration) + " ms\n";
      }
    }
  }
}
