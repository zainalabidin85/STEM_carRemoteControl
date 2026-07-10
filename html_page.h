const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en" oncontextmenu="return false;">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <title>RC UniMAP</title>
  <style>
    :root {
      --bg: #0b0e14;
      --panel: #151a24;
      --panel-2: #1c2230;
      --accent: #22d3ee;
      --accent-2: #3b82f6;
      --text: #e6edf3;
      --text-dim: #8b96a5;
      --danger: #ef4444;
      --ok: #22c55e;
    }
    * { box-sizing: border-box; }
    html, body {
      touch-action: none;
      overscroll-behavior: none;
      height: 100%;
    }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
      background: radial-gradient(circle at top, #131826 0%, var(--bg) 60%);
      color: var(--text);
      margin: 0;
      user-select: none;
      -webkit-user-select: none;
      overflow: hidden;
    }

    /* --- Rotate-device gate (portrait) --- */
    #rotateGate {
      position: fixed;
      inset: 0;
      background: var(--bg);
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 14px;
      z-index: 50;
      text-align: center;
      padding: 24px;
    }
    #rotateGate .icon { font-size: 3rem; animation: tilt 1.6s ease-in-out infinite; }
    #rotateGate h2 { margin: 0; font-size: 1.1rem; }
    #rotateGate p { margin: 0; color: var(--text-dim); font-size: 0.85rem; max-width: 260px; }
    @keyframes tilt {
      0%, 100% { transform: rotate(0deg); }
      50% { transform: rotate(90deg); }
    }
    @media (orientation: landscape) {
      #rotateGate { display: none; }
    }

    /* --- Landscape dashboard --- */
    #dashboard {
      display: none;
      width: 100vw;
      height: 100vh;
      padding: 10px 18px;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
    }
    @media (orientation: landscape) {
      #dashboard { display: flex; }
    }

    .stick-col {
      flex: 0 0 auto;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
    }
    .stick-col .caption {
      font-size: 0.72rem;
      color: var(--text-dim);
      text-transform: uppercase;
      letter-spacing: 1px;
    }
    .stick-col .readout {
      font-size: 0.78rem;
      color: var(--accent);
      min-height: 1.1em;
      font-weight: 600;
    }

    /* Vertical throttle track (left stick) */
    .track-v {
      position: relative;
      width: 84px;
      height: 190px;
      border-radius: 42px;
      background: radial-gradient(circle at 35% 30%, #232b3d, #10141d 70%);
      border: 1px solid #2a3244;
      box-shadow: inset 0 6px 18px rgba(0,0,0,0.55);
      touch-action: none;
    }
    .track-v::before {
      content: "";
      position: absolute;
      left: 50%; top: 10%; bottom: 10%; width: 1px;
      background: #232b3d;
      transform: translateX(-50%);
    }

    /* Horizontal steer track (right stick) */
    .track-h {
      position: relative;
      width: 190px;
      height: 84px;
      border-radius: 42px;
      background: radial-gradient(circle at 35% 30%, #232b3d, #10141d 70%);
      border: 1px solid #2a3244;
      box-shadow: inset 0 6px 18px rgba(0,0,0,0.55);
      touch-action: none;
    }
    .track-h::before {
      content: "";
      position: absolute;
      top: 50%; left: 10%; right: 10%; height: 1px;
      background: #232b3d;
      transform: translateY(-50%);
    }

    .knob {
      position: absolute;
      left: 50%;
      top: 50%;
      width: 64px;
      height: 64px;
      margin-left: -32px;
      margin-top: -32px;
      border-radius: 50%;
      background: linear-gradient(160deg, var(--accent), var(--accent-2));
      box-shadow: 0 6px 16px rgba(34, 211, 238, 0.35), 0 0 0 6px rgba(34, 211, 238, 0.08);
      touch-action: none;
      transition: transform 0.05s linear;
    }

    /* Center column */
    .center-col {
      flex: 1 1 auto;
      max-width: 260px;
      height: 100%;
      display: flex;
      flex-direction: column;
      justify-content: center;
      gap: 8px;
      padding: 4px 0;
    }
    .brand {
      text-align: center;
      margin-bottom: 2px;
    }
    .brand h1 {
      font-size: 0.95rem;
      margin: 0;
      background: linear-gradient(90deg, var(--accent), var(--accent-2));
      -webkit-background-clip: text;
      background-clip: text;
      color: transparent;
    }
    .brand span { font-size: 0.68rem; color: var(--text-dim); }

    .card {
      background: var(--panel);
      border: 1px solid #232a38;
      border-radius: 14px;
      padding: 10px 12px;
      box-shadow: 0 8px 20px rgba(0,0,0,0.3);
    }
    .stop-btn {
      width: 100%;
      padding: 10px;
      font-size: 0.95rem;
      font-weight: 700;
      letter-spacing: 1px;
      background: linear-gradient(160deg, #ff5b5b, var(--danger));
      color: white;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      box-shadow: 0 6px 16px rgba(239, 68, 68, 0.3);
    }
    .speed-card label {
      display: flex;
      justify-content: space-between;
      font-size: 0.75rem;
      color: var(--text-dim);
      margin-bottom: 6px;
    }
    .speed-card #speedValue { color: var(--accent); font-weight: 700; }
    input[type="range"] { width: 100%; accent-color: var(--accent); }

    .memory-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 6px;
    }
    .mem-btn {
      padding: 8px;
      font-size: 0.72rem;
      font-weight: 600;
      background: var(--panel-2);
      color: var(--text);
      border: 1px solid #2a3244;
      border-radius: 8px;
      cursor: pointer;
    }
    .mem-btn.active {
      background: linear-gradient(160deg, var(--accent), var(--accent-2));
      color: #04121a;
      border-color: transparent;
    }

    .status-card { display: flex; flex-direction: column; gap: 4px; }
    .status-card .debug { font-size: 0.72rem; color: var(--ok); }
    .log {
      background: #05070c;
      color: #9fe6c5;
      padding: 6px 8px;
      height: 40px;
      font-size: 0.68rem;
      font-family: "SFMono-Regular", Menlo, Consolas, monospace;
      white-space: pre-wrap;
      overflow-y: auto;
      border-radius: 8px;
      border: 1px solid #1c2230;
    }
    .icon-btn {
      background: var(--panel-2);
      color: var(--text);
      border: 1px solid #2a3244;
      border-radius: 8px;
      padding: 6px 8px;
      font-size: 0.7rem;
      cursor: pointer;
    }
  </style>
</head>
<body ontouchstart="">
  <div id="rotateGate">
    <div class="icon">📱</div>
    <h2>Rotate your phone</h2>
    <p>This controller is designed for landscape mode. Turn your device sideways to drive.</p>
  </div>

  <div id="dashboard">
    <div class="stick-col">
      <span class="caption">Throttle</span>
      <div class="track-v" id="throttleTrack">
        <div class="knob" id="throttleKnob"></div>
      </div>
      <span class="readout" id="throttleReadout">0%</span>
    </div>

    <div class="center-col">
      <div class="brand">
        <h1>UniMAP RC Control</h1>
        <span>Agricultural Engineering</span>
      </div>

      <button class="stop-btn" onclick="sendStop()">STOP</button>

      <div class="card speed-card">
        <label for="speedSlider">Max Speed <span id="speedValue">200</span></label>
        <input type="range" id="speedSlider" min="40" max="255" value="200">
      </div>

      <div class="card">
        <div class="memory-grid">
          <button class="mem-btn" id="recordBtn" onclick="handleRecord()">● Record</button>
          <button class="mem-btn" id="replayBtn" onclick="handleReplay()">▶ Replay</button>
          <button class="mem-btn" onclick="sendCommand('clear_memory')">🗑 Clear</button>
          <button class="icon-btn" onclick="requestLandscape()">⛶ Lock View</button>
        </div>
      </div>

      <div class="card status-card">
        <div class="debug" id="debug">Ready</div>
        <div class="log" id="log">Replay log</div>
      </div>
    </div>

    <div class="stick-col">
      <span class="caption">Steering</span>
      <div class="track-h" id="steerTrack">
        <div class="knob" id="steerKnob"></div>
      </div>
      <span class="readout" id="steerReadout">0%</span>
    </div>
  </div>

  <script>
    // --- Command helpers ---
    function sendStop() {
      throttleVal = 0; steerVal = 0;
      resetKnob(throttleKnob); resetKnob(steerKnob);
      fetch('/control?cmd=stop').then(() => updateDebug('STOP issued'));
    }
    function sendCommand(cmd) {
      fetch('/control?cmd=' + cmd)
        .then(r => r.text())
        .then(d => {
          if (cmd === 'clear_memory') updateDebug('Memory cleared.');
          else updateDebug(d);
        });
    }
    function updateDebug(msg) {
      document.getElementById('debug').textContent = msg;
    }
    function requestLandscape() {
      const el = document.documentElement;
      const p = el.requestFullscreen ? el.requestFullscreen() : Promise.resolve();
      p.then(() => {
        if (screen.orientation && screen.orientation.lock) {
          screen.orientation.lock('landscape').catch(() => {});
        }
      }).catch(() => {});
    }

    // --- Max speed slider ---
    let maxSpeed = 200;
    const speedSlider = document.getElementById('speedSlider');
    speedSlider.addEventListener('input', () => {
      maxSpeed = parseInt(speedSlider.value, 10);
      document.getElementById('speedValue').textContent = maxSpeed;
    });

    // --- Shared drive state, streamed to the ESP32 at a fixed rate ---
    let throttleVal = 0; // -255..255
    let steerVal = 0;    // -255..255

    function resetKnob(knob) { knob.style.transform = 'translate(0px, 0px)'; }

    // Generic single-axis stick. axis: 'v' (vertical/throttle) or 'h' (horizontal/steer)
    function setupStick(trackEl, knobEl, axis, onChange, readoutEl, labelFn) {
      let dragging = false;
      let travel = 0;
      let centerX = 0, centerY = 0;

      function begin(clientX, clientY) {
        const r = trackEl.getBoundingClientRect();
        centerX = r.left + r.width / 2;
        centerY = r.top + r.height / 2;
        travel = axis === 'v' ? (r.height - knobEl.offsetHeight) / 2 : (r.width - knobEl.offsetWidth) / 2;
        dragging = true;
        move(clientX, clientY);
      }

      function move(clientX, clientY) {
        if (!dragging) return;
        let value;
        if (axis === 'v') {
          let dy = clientY - centerY;
          dy = Math.max(-travel, Math.min(travel, dy));
          knobEl.style.transform = `translate(0px, ${dy}px)`;
          value = -dy / travel; // up = positive
        } else {
          let dx = clientX - centerX;
          dx = Math.max(-travel, Math.min(travel, dx));
          knobEl.style.transform = `translate(${dx}px, 0px)`;
          value = dx / travel; // right = positive
        }
        const deadzone = 0.1;
        if (Math.abs(value) < deadzone) value = 0;
        onChange(value);
        readoutEl.textContent = labelFn(value);
      }

      function end() {
        dragging = false;
        resetKnob(knobEl);
        onChange(0);
        readoutEl.textContent = labelFn(0);
      }

      knobEl.addEventListener('pointerdown', (e) => {
        e.preventDefault();
        knobEl.setPointerCapture(e.pointerId);
        begin(e.clientX, e.clientY);
      });
      knobEl.addEventListener('pointermove', (e) => {
        if (!dragging) return;
        e.preventDefault();
        move(e.clientX, e.clientY);
      });
      knobEl.addEventListener('pointerup', (e) => { e.preventDefault(); end(); });
      knobEl.addEventListener('pointercancel', end);
    }

    setupStick(
      document.getElementById('throttleTrack'),
      document.getElementById('throttleKnob'),
      'v',
      (v) => { throttleVal = Math.round(v * maxSpeed); },
      document.getElementById('throttleReadout'),
      (v) => (v === 0 ? '0%' : (v > 0 ? 'Fwd ' : 'Rev ') + Math.round(Math.abs(v) * 100) + '%')
    );

    setupStick(
      document.getElementById('steerTrack'),
      document.getElementById('steerKnob'),
      'h',
      (v) => { steerVal = Math.round(v * maxSpeed); },
      document.getElementById('steerReadout'),
      (v) => (v === 0 ? '0%' : (v > 0 ? 'Right ' : 'Left ') + Math.round(Math.abs(v) * 100) + '%')
    );

    // Stream the blended drive command to the ESP32 at a steady rate so
    // simultaneous throttle + steer actually mix into a real turn.
    setInterval(() => {
      fetch('/control?cmd=drive&throttle=' + throttleVal + '&steer=' + steerVal);
    }, 100);

    // --- Record toggle ---
    let recording = false;
    function handleRecord() {
      recording = !recording;
      const btn = document.getElementById('recordBtn');
      if (recording) {
        sendCommand('start_recording');
        btn.textContent = '■ Stop Rec';
        btn.classList.add('active');
      } else {
        sendCommand('stop_recording');
        btn.textContent = '● Record';
        btn.classList.remove('active');
      }
    }

    // --- Smart replay: OFF -> Single -> Loop -> OFF ---
    let replayMode = 0;
    let loopCheckInterval = null;
    function handleReplay() {
      replayMode = (replayMode + 1) % 3;
      const btn = document.getElementById('replayBtn');
      clearInterval(loopCheckInterval);

      if (replayMode === 1) {
        sendCommand('replay');
        btn.textContent = '⟲ Loop';
        btn.classList.remove('active');
      } else if (replayMode === 2) {
        btn.textContent = '⏹ Replay Off';
        btn.classList.add('active');
        loopCheckInterval = setInterval(() => {
          fetch('/log').then(r => r.text()).then(d => {
            if (d.includes('Replay finished')) sendCommand('replay');
          });
        }, 500);
      } else {
        btn.textContent = '▶ Replay';
        btn.classList.remove('active');
      }
    }

    // Poll the /log endpoint every 500ms to update the replay log
    setInterval(() => {
      fetch('/log').then(r => r.text()).then(d => {
        document.getElementById('log').textContent = d;
      });
    }, 500);
  </script>
</body>
</html>
)rawliteral";
