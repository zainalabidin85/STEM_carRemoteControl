const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en" oncontextmenu="return false;" style="-webkit-user-select: none; user-select: none;">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RC UniMAP</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: white;
      color: #007bff;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      user-select: none;
    }
    h2 {
      margin-top: 50px;
      text-align: center;
    }
    .fullscreen {
      position: absolute;
      top: 10px;
      right: 10px;
      background-color: white;
      color: #007bff;
      border: 2px solid #007bff;
      border-radius: 5px;
      padding: 5px 10px;
      cursor: pointer;
    }
    .controller {
      display: flex;
      justify-content: center;
      align-items: center;
      margin-top: 10px;
    }
    .column, .center-column {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin: 0 20px;
    }
    .row {
      display: flex;
      justify-content: center;
      margin: 5px;
    }
    .slider-container {
      width: 80%;
      margin: 20px auto;
      text-align: center;
    }
    .slider {
      width: 100%;
    }
    .actions {
      display: grid;
      grid-template-columns: repeat(2, auto);
      gap: 10px;
      margin: 10px auto;
    }
    button {
      margin: 5px;
      padding: 15px 25px;
      font-size: 1rem;
      background: white;
      color: #007bff;
      border: 2px solid #007bff;
      border-radius: 5px;
      cursor: pointer;
    }
    button:hover {
      background-color: #007bff;
      color: white;
    }
    .stop {
      color: red;
      border-color: red;
    }
    .memory-btn {
      color: indigo;
      border-color: indigo;
    }
    .debug {
      margin: 10px;
      font-size: 1.1rem;
      color: orange;
    }
    .log {
      margin-top: 5px;
      background-color: black;
      color: white;
      padding: 10px;
      height: 50px;
      width: 300px;
      font-size: 0.9rem;
      white-space: pre-wrap;
      overflow-y: auto;
      overflow-x: hidden;
      border: 1px solid #ccc;
    }
  </style>
</head>
<body ontouchstart="">
  <button class="fullscreen" onclick="toggleFullscreen()">Fullscreen</button>
  <h2>Agricultural Engineering<br>UniMAP</h2>

  <div class="controller">
    <div class="column">
      <button onmousedown="sendPress('forward')" onmouseup="sendRelease()" ontouchstart="sendPress('forward')" ontouchend="sendRelease()">Forward</button>
      <button class="stop" onclick="sendStop()">STOP</button>
      <button onmousedown="sendPress('reverse')" onmouseup="sendRelease()" ontouchstart="sendPress('reverse')" ontouchend="sendRelease()">Reverse</button>
    </div>
    <div class="center-column">
      <div class="row">
        <button onmousedown="sendPress('left')" onmouseup="sendRelease()" ontouchstart="sendPress('left')" ontouchend="sendRelease()">Left</button>
        <button onmousedown="sendPress('right')" onmouseup="sendRelease()" ontouchstart="sendPress('right')" ontouchend="sendRelease()">Right</button>
      </div>
    </div>
  </div>

  <div class="slider-container">
    <label for="speedSlider">Speed: <span id="speedValue">128</span></label>
    <input type="range" id="speedSlider" class="slider" min="0" max="255" value="128" oninput="updateSpeed(this.value)">
  </div>

  <div class="actions">
    <button class="memory-btn" onclick="sendCommand('start_recording')">Memory Start</button>
    <button class="memory-btn" onclick="sendCommand('stop_recording')">Memory Stop</button>
    <button class="memory-btn" id="replayBtn" onclick="handleReplay()">Replay</button>
    <button class="memory-btn" onclick="sendCommand('clear_memory')">Memory Clear</button>
  </div>

  <div class="log" id="log">Replay log</div>
  <div class="debug" id="debug">debug response here</div>

  <script>
    function sendPress(value) {
      fetch('/control?cmd=press&value=' + value)
        .then(response => response.text())
        .then(data => updateDebug("Command Sent: " + data));
    }

    function sendRelease() {
      fetch('/control?cmd=release')
        .then(response => response.text())
        .then(data => updateDebug("Command Sent: " + data));
    }

    function sendStop() {
      fetch('/control?cmd=stop')
        .then(response => response.text())
        .then(data => updateDebug("STOP command issued."));
    }

    function updateSpeed(value) {
      document.getElementById('speedValue').textContent = value;
      fetch('/control?cmd=speed&value=' + value)
        .then(response => response.text())
        .then(data => updateDebug("Speed Set: " + data));
    }

    function sendCommand(cmd) {
      fetch('/control?cmd=' + cmd)
        .then(response => response.text())
        .then(data => {
          if (cmd === 'replay') {
            updateDebug("Replaying movements...");
          } else if (cmd === 'clear_memory') {
            updateDebug("Memory cleared.");
          } else {
            updateDebug("Command Sent: " + data);
          }
        });
    }

    function updateDebug(msg) {
      document.getElementById('debug').textContent = msg;
    }

    function toggleFullscreen() {
      if (!document.fullscreenElement) {
        document.documentElement.requestFullscreen();
      } else {
        document.exitFullscreen();
      }
    }

    // --- Smart Replay Button ---
    let replayMode = 0; // 0 = OFF, 1 = Single Replay, 2 = Loop
    let loopCheckInterval = null;

    function handleReplay() {
      replayMode = (replayMode + 1) % 3;
      const replayBtn = document.getElementById('replayBtn');

      if (replayMode === 1) {
        // Single Replay
        sendCommand('replay');
        updateDebug("Replay once.");
        replayBtn.textContent = "Loop Replay";
        replayBtn.style.backgroundColor = "white";
        replayBtn.style.color = "indigo";
        clearInterval(loopCheckInterval);

      } else if (replayMode === 2) {
        // Loop Replay
        updateDebug("Loop Replay ON");
        replayBtn.textContent = "Replay OFF";
        replayBtn.style.backgroundColor = "indigo";
        replayBtn.style.color = "white";

        loopCheckInterval = setInterval(() => {
          fetch('/log')
            .then(response => response.text())
            .then(data => {
              if (data.includes("Replay finished")) {
                sendCommand('replay'); // immediately restart
              }
            });
        }, 500); // check every 0.5s

      } else {
        // OFF
        updateDebug("Replay OFF");
        replayBtn.textContent = "Replay";
        replayBtn.style.backgroundColor = "white";
        replayBtn.style.color = "indigo";
        clearInterval(loopCheckInterval);
      }
    }

    // Poll the /log endpoint every 500ms to update the replay log
    setInterval(() => {
      fetch('/log')
        .then(response => response.text())
        .then(data => {
          document.getElementById('log').textContent = data;
        });
    }, 500);
  </script>
</body>
</html>
)rawliteral";
