#include <WiFi.h>
#include <WebServer.h>
#include "html_page1.h" // Include the external HTML file

const char* ssid = "rc_UNIMAP";
const char* password = "12345678";

// JSN-SR04T pins  (use a voltage divider on echoPin to 3.3V)
const int trigPin = 5;
const int echoPin = 18;

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
  uint32_t duration;  // ms
  uint8_t speed;      // 0-255
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

// --- Obstacle pause state (replay only) ---
bool obstacleHold = false;
uint32_t obstacleStart = 0;
uint32_t lastObstacleLog = 0;

// --- Utils ---
static inline uint32_t nowMs() { return millis(); }

// Single distance measurement (cm). Returns -1 on timeout/invalid.
long readDistanceOnceCM() {
  // Ensure clean LOW before trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(4);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // PulseIn timeout 30,000 us (~5 m)
  long duration = pulseIn(echoPin, HIGH, 30000UL);
  if (duration == 0) return -1;  // timeout -> invalid

  // Speed of sound ~343 m/s -> 0.0343 cm/us; divide by 2 for round trip
  long distance = (long)(duration * 0.0343f / 2.0f);
  return distance;
}

// Median-of-3 sampling to reduce noise/spikes
long readDistanceCM() {
  long a = readDistanceOnceCM();
  long b = readDistanceOnceCM();
  long c = readDistanceOnceCM();

  // If any is invalid, try to salvage the others
  long vals[3] = {a, b, c};
  int validCount = 0;
  long valid[3];
  for (int i = 0; i < 3; ++i) {
    if (vals[i] >= 0) valid[validCount++] = vals[i];
  }
  if (validCount == 0) return -1;

  // Sort small array (simple)
  for (int i = 0; i < validCount - 1; ++i) {
    for (int j = i + 1; j < validCount; ++j) {
      if (valid[j] < valid[i]) {
        long t = valid[i]; valid[i] = valid[j]; valid[j] = t;
      }
    }
  }
  // Median
  return valid[validCount / 2];
}

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
    obstacleHold = false;              // reset any old hold
    replayIndex = 0;
    replayStartTime = nowMs();
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
    obstacleHold = false;
    server.send(200, "text/plain", "Memory cleared");
    return;
  }

  if (cmd == "press") {
    currentCmd = server.arg("value");
    lastCommandTime = nowMs();
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
      uint32_t duration = nowMs() - lastCommandTime;
      recordedPath[pathIndex++] = {currentCmd, duration, (uint8_t)pwmSpeed};
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
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW); // default low

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
    // Check obstacle only in replay mode
    long distance = readDistanceCM();

    // Treat values < 0 as "no measurement", ignore them.
    // NOTE: JSN-SR04T practical minimum is usually >20 cm; 1 cm is extremely tight.
    bool obstacle = (distance >= 0 && distance < 20);

    if (obstacle) {
      if (!obstacleHold) {
        obstacleHold = true;
        obstacleStart = nowMs();
        stopMotors();
        // prevent log spam
        if (nowMs() - lastObstacleLog > 500) {
          replayLog += "Obstacle detected! Car paused.\n";
          lastObstacleLog = nowMs();
        }
      }
      // While holding, we DO NOT advance time — handled by shifting replayStartTime after release.
      return;
    } else if (obstacleHold) {
      // Resume: shift replayStartTime forward by paused duration so step duration is preserved
      uint32_t pausedMs = nowMs() - obstacleStart;
      replayStartTime += pausedMs;
      obstacleHold = false;
      // Re-issue the same command to ensure motors are running again
      executeCommand(recordedPath[replayIndex]);
      replayLog += "Obstacle cleared. Resuming step " + String(replayIndex + 1) + ".\n";
    }

    // Normal replay timing (only when not paused)
    if (!obstacleHold && (nowMs() - replayStartTime >= recordedPath[replayIndex].duration)) {
      replayIndex++;
      if (replayIndex >= pathIndex) {
        isReplaying = false;
        stopMotors();
        replayLog += "Replay finished.\n";
      } else {
        executeCommand(recordedPath[replayIndex]);
        replayStartTime = nowMs();
        replayLog += "Step " + String(replayIndex + 1) + ": " + recordedPath[replayIndex].cmd + " for " + String(recordedPath[replayIndex].duration) + " ms\n";
      }
    }
  }
}
