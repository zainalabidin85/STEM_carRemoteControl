const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no, viewport-fit=cover">
<meta name="theme-color" content="#0d1116">
<title>UniMAP RC Controller</title>
<style>
  /* Layout: landscape ground-station. Throttle lever | telemetry + controls | steering lever.
     Palette: graphite with a blue bias, implement-yellow accent, semantic red/green/orange.
     System fonts only: the ESP32 access point has no internet. */
  :root {
    --bg: #0d1116;
    --surface: #151b22;
    --surface-2: #1c242d;
    --line: #2a3440;
    --fg: #e8edf2;
    --muted: #8593a3;
    --accent: #f2a900;
    --accent-ink: #1d1500;
    --stop: #e23a3d;
    --stop-hi: #f2595b;
    --ok: #3ccf7a;
    --warn: #ff8a3d;
    --font: system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    --mono: ui-monospace, "SF Mono", Menlo, Consolas, "Roboto Mono", monospace;
    --r: 10px;
    color-scheme: dark;
  }
  * { box-sizing: border-box; }
  html, body { height: 100%; }
  body {
    margin: 0;
    background: var(--bg);
    color: var(--fg);
    font: 14px/1.35 var(--font);
    overflow: hidden;
    touch-action: none;
    overscroll-behavior: none;
    user-select: none;
    -webkit-user-select: none;
    -webkit-tap-highlight-color: transparent;
  }
  button { font: inherit; color: inherit; }
  button:focus-visible, input:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
  .mono { font-family: var(--mono); font-variant-numeric: tabular-nums; }

  /* ---------- Portrait gate ---------- */
  #gate {
    position: fixed; inset: 0; z-index: 40;
    display: none; flex-direction: column; align-items: center; justify-content: center;
    gap: 12px; padding: 24px; text-align: center; background: var(--bg);
  }
  #gate svg { width: 64px; height: 64px; color: var(--accent); animation: turn 2.2s ease-in-out infinite; }
  #gate h2 { margin: 0; font-size: 17px; }
  #gate p { margin: 0; max-width: 260px; color: var(--muted); }
  @keyframes turn { 0%, 30% { transform: rotate(0); } 60%, 100% { transform: rotate(-90deg); } }
  @media (orientation: portrait) and (max-width: 700px) {
    #gate { display: flex; }
    #app { display: none !important; }
  }

  /* ---------- App frame ---------- */
  #app {
    height: 100%;
    display: grid;
    grid-template-rows: auto 1fr;
    padding-block: 8px 10px;
    padding-left: max(16px, env(safe-area-inset-left, 0px));
    padding-right: max(16px, env(safe-area-inset-right, 0px));
    gap: 8px;
  }

  /* Top bar */
  .bar { display: flex; align-items: center; gap: 10px; min-height: 34px; }
  .brand { display: flex; align-items: baseline; gap: 8px; white-space: nowrap; }
  .brand b { font-size: 15px; letter-spacing: .2px; }
  .brand span { color: var(--muted); font-size: 12px; }
  .bar .spacer { flex: 1; }
  .pill {
    display: inline-flex; align-items: center; gap: 7px;
    height: 28px; padding: 0 11px; border-radius: 999px;
    background: var(--surface); border: 1px solid var(--line);
    font-size: 12px; font-weight: 600; letter-spacing: .6px; text-transform: uppercase; white-space: nowrap;
  }
  .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--muted); flex: none; }
  #mode[data-mode="rec"] { border-color: color-mix(in srgb, var(--stop) 55%, transparent); }
  #mode[data-mode="rec"] .dot { background: var(--stop); animation: blink 1s steps(2) infinite; }
  #mode[data-mode="replay"] { border-color: color-mix(in srgb, var(--accent) 55%, transparent); }
  #mode[data-mode="replay"] .dot { background: var(--accent); }
  #mode[data-mode="hold"] { background: color-mix(in srgb, var(--warn) 18%, var(--surface)); border-color: var(--warn); color: var(--warn); }
  #mode[data-mode="hold"] .dot { background: var(--warn); animation: blink .6s steps(2) infinite; }
  @keyframes blink { 50% { opacity: .25; } }
  #link .dot { background: var(--ok); }
  #link[data-state="lost"] { color: var(--stop-hi); border-color: color-mix(in srgb, var(--stop) 60%, transparent); }
  #link[data-state="lost"] .dot { background: var(--stop); }
  #link[data-state="wait"] .dot { background: var(--muted); }
  .icon-btn {
    width: 34px; height: 34px; display: grid; place-items: center; flex: none;
    border-radius: 9px; background: var(--surface); border: 1px solid var(--line); cursor: pointer;
  }
  .icon-btn svg { width: 17px; height: 17px; }
  .icon-btn:active { background: var(--surface-2); }

  /* Main deck */
  .deck {
    min-height: 0;
    display: grid;
    grid-template-columns: 1fr clamp(250px, 36vw, 340px) 1fr;
    gap: 14px;
  }

  /* ---------- Levers ---------- */
  .zone {
    position: relative; min-width: 0; min-height: 0;
    display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 10px;
    border-radius: 16px; touch-action: none; cursor: grab;
  }
  .zone.held { cursor: grabbing; }
  .zone-head { display: flex; align-items: baseline; justify-content: space-between; gap: 10px; width: min(100%, 240px); }
  .zone-head .label { font-size: 11px; font-weight: 700; letter-spacing: 1.4px; text-transform: uppercase; color: var(--muted); }
  .zone-head .dir { font-size: 11px; font-weight: 700; letter-spacing: 1px; color: var(--muted); min-width: 3.5em; text-align: right; }
  .zone.held .zone-head .dir, .zone.engaged .zone-head .dir { color: var(--accent); }
  .readout { font-family: var(--mono); font-variant-numeric: tabular-nums; font-size: 26px; font-weight: 600; line-height: 1; }
  .readout small { font-size: 14px; color: var(--muted); margin-left: 1px; }

  .lever { --long: clamp(150px, 56vh, 290px); --short: clamp(66px, 17vh, 88px); position: relative; display: flex; align-items: center; gap: 8px; }
  .lever.v { flex-direction: row; }
  .lever.h { flex-direction: column; }
  .track {
    position: relative; border-radius: 999px; flex: none;
    background: var(--surface);
    border: 1px solid var(--line);
    box-shadow: inset 0 2px 10px rgb(0 0 0 / .45);
  }
  .v .track { width: var(--short); height: var(--long); }
  .h .track { width: var(--long); height: var(--short); }
  .track::before { content: ""; position: absolute; background: var(--line); }
  .v .track::before { left: 50%; top: 14px; bottom: 14px; width: 2px; margin-left: -1px; }
  .h .track::before { top: 50%; left: 14px; right: 14px; height: 2px; margin-top: -1px; }
  .fill { position: absolute; background: color-mix(in srgb, var(--accent) 30%, transparent); border-radius: 4px; }
  .v .fill { left: 50%; width: 6px; margin-left: -3px; }
  .h .fill { top: 50%; height: 6px; margin-top: -3px; }
  .knob {
    position: absolute; left: 50%; top: 50%;
    width: calc(var(--short) - 14px); height: calc(var(--short) - 14px);
    margin: calc((var(--short) - 14px) / -2) 0 0 calc((var(--short) - 14px) / -2);
    border-radius: 50%;
    background: linear-gradient(180deg, #3a4654, #26303b);
    border: 2px solid #4a5767;
    box-shadow: 0 4px 12px rgb(0 0 0 / .5), inset 0 1px 0 rgb(255 255 255 / .08);
    display: grid; place-items: center;
    will-change: transform;
  }
  .knob::after { content: ""; width: 34%; height: 34%; border-radius: 50%; border: 2px solid #5b6878; }
  .zone.held .knob, .zone.engaged .knob { border-color: var(--accent); }
  .zone.held .knob::after, .zone.engaged .knob::after { border-color: var(--accent); background: color-mix(in srgb, var(--accent) 25%, transparent); }
  .knob.spring { transition: transform .14s cubic-bezier(.3, 1.4, .6, 1); }
  .scale { position: relative; flex: none; color: var(--muted); font: 9px/1 var(--mono); }
  .v .scale { width: 22px; height: var(--long); }
  .h .scale { height: 14px; width: var(--long); }
  .scale i { position: absolute; background: var(--line); }
  .v .scale i { left: 0; height: 1px; width: 6px; }
  .v .scale i.major { width: 10px; background: var(--muted); }
  .h .scale i { top: 0; width: 1px; height: 5px; }
  .h .scale i.major { height: 8px; background: var(--muted); }
  .scale em { position: absolute; font-style: normal; }
  .v .scale em { left: 13px; transform: translateY(-50%); }
  .h .scale em { top: 10px; transform: translateX(-50%); display: none; }

  /* ---------- Centre column ---------- */
  .center { min-width: 0; min-height: 0; display: flex; flex-direction: column; gap: 8px; justify-content: center; }
  .panel { background: var(--surface); border: 1px solid var(--line); border-radius: var(--r); padding: 10px 12px; }
  .panel-head { display: flex; justify-content: space-between; align-items: baseline; gap: 8px; margin-bottom: 8px; }
  .panel-head h3 { margin: 0; font-size: 11px; font-weight: 700; letter-spacing: 1.2px; text-transform: uppercase; color: var(--muted); }
  .panel-head .val { font-size: 12px; color: var(--fg); }
  .panel-head .val b { color: var(--accent); font-weight: 600; }

  /* Motor output: bidirectional bars, centre = 0 */
  .motors { display: grid; grid-template-columns: auto 1fr auto; gap: 6px 10px; align-items: center; }
  .motors .side { font-size: 11px; font-weight: 700; color: var(--muted); width: 1.2em; }
  .motors .num { font-size: 12px; text-align: right; min-width: 4.4ch; }
  .mbar { position: relative; height: 10px; border-radius: 3px; background: var(--surface-2); overflow: hidden; }
  .mbar::after { content: ""; position: absolute; left: 50%; top: -2px; bottom: -2px; width: 1px; background: var(--muted); opacity: .6; }
  .mbar .m { position: absolute; top: 0; bottom: 0; left: 50%; width: 0; background: var(--accent); }
  .mbar .m.neg { background: color-mix(in srgb, var(--accent) 55%, var(--muted)); }

  /* Speed limit */
  .range {
    --p: 74%;
    -webkit-appearance: none; appearance: none; width: 100%; height: 28px; margin: 0; background: transparent; cursor: pointer;
  }
  .range::-webkit-slider-runnable-track { height: 6px; border-radius: 3px; background: linear-gradient(90deg, var(--accent) var(--p), var(--surface-2) var(--p)); }
  .range::-moz-range-track { height: 6px; border-radius: 3px; background: linear-gradient(90deg, var(--accent) var(--p), var(--surface-2) var(--p)); }
  .range::-webkit-slider-thumb { -webkit-appearance: none; width: 22px; height: 22px; margin-top: -8px; border-radius: 50%; background: var(--fg); border: 4px solid var(--accent); }
  .range::-moz-range-thumb { width: 14px; height: 14px; border-radius: 50%; background: var(--fg); border: 4px solid var(--accent); }
  .ticks { display: flex; justify-content: space-between; font: 10px/1 var(--mono); color: var(--muted); margin-top: 2px; }

  /* Route memory */
  .mem { display: grid; grid-template-columns: 1fr 1fr 1fr auto; gap: 6px; }
  .btn {
    height: 38px; padding: 0 10px; border-radius: 8px; cursor: pointer;
    display: inline-flex; align-items: center; justify-content: center; gap: 6px;
    background: var(--surface-2); border: 1px solid var(--line);
    font-size: 13px; font-weight: 600; white-space: nowrap;
  }
  .btn svg { width: 14px; height: 14px; flex: none; }
  .btn:active { filter: brightness(1.15); }
  .btn.on { background: var(--accent); border-color: var(--accent); color: var(--accent-ink); }
  #recBtn.on { background: var(--stop); border-color: var(--stop); color: #fff; }
  #clearBtn { width: 38px; padding: 0; color: var(--muted); }
  #clearBtn.confirm { width: auto; padding: 0 10px; color: #fff; background: var(--stop); border-color: var(--stop); }

  /* Emergency stop */
  .stop {
    position: relative; height: clamp(46px, 13vh, 60px); border: 0; border-radius: var(--r); cursor: pointer;
    background: var(--stop); color: #fff;
    font-size: 18px; font-weight: 800; letter-spacing: 3px;
    box-shadow: 0 0 0 1px var(--stop-hi) inset, 0 6px 18px color-mix(in srgb, var(--stop) 35%, transparent);
  }
  .stop:active { background: var(--stop-hi); transform: translateY(1px); }
  .stop kbd { position: absolute; right: 12px; top: 50%; transform: translateY(-50%); font: 600 10px/1 var(--mono); letter-spacing: .5px; padding: 4px 6px; border-radius: 4px; background: rgb(0 0 0 / .22); display: none; }
  @media (hover: hover) and (pointer: fine) { .stop kbd { display: block; } }

  /* Log drawer */
  #logSheet {
    position: fixed; z-index: 30; top: 0; right: 0; bottom: 0; width: min(360px, 90vw);
    background: var(--surface); border-left: 1px solid var(--line);
    padding: max(12px, env(safe-area-inset-top, 0px)) max(16px, env(safe-area-inset-right, 0px)) 12px 16px;
    display: flex; flex-direction: column; gap: 10px;
    transform: translateX(105%); transition: transform .2s ease;
    box-shadow: -20px 0 40px rgb(0 0 0 / .4);
  }
  #logSheet.open { transform: none; }
  #logSheet header { display: flex; align-items: center; justify-content: space-between; }
  #logSheet h3 { margin: 0; font-size: 13px; letter-spacing: 1px; text-transform: uppercase; }
  #logBody { flex: 1; min-height: 0; overflow: auto; touch-action: pan-y; margin: 0; padding: 10px; border-radius: 8px; background: var(--bg); font: 12px/1.6 var(--mono); color: var(--muted); white-space: pre-wrap; user-select: text; -webkit-user-select: text; }
  #logBody .ev-ob { color: var(--warn); }
  #logBody .ev-end { color: var(--ok); }

  /* Toast */
  #toast {
    position: fixed; z-index: 35; left: 50%; bottom: max(14px, env(safe-area-inset-bottom, 0px));
    transform: translate(-50%, 20px); opacity: 0; pointer-events: none;
    padding: 8px 14px; border-radius: 8px; background: var(--fg); color: var(--bg); font-size: 13px; font-weight: 600;
    transition: opacity .18s, transform .18s;
  }
  #toast.show { opacity: 1; transform: translate(-50%, 0); }

  /* Link-lost veil over the levers */
  .deck.lost .zone { opacity: .45; }

  /* Short screens (phones in landscape with browser chrome) */
  @media (max-height: 380px) {
    #app { gap: 6px; padding-block: 6px 6px; }
    .brand span { display: none; }
    .panel { padding: 8px 10px; }
    .panel-head { margin-bottom: 4px; }
    .ticks { display: none; }
    .btn { height: 34px; }
    #clearBtn { width: 34px; }
    .readout { font-size: 22px; }
    .zone { gap: 6px; }
  }
  @media (max-height: 320px) { #motorPanel { display: none; } }
  @media (max-width: 760px) { .brand span { display: none; } .deck { gap: 10px; } }
  @media (prefers-reduced-motion: reduce) {
    *, *::before, *::after { animation: none !important; transition: none !important; }
  }
</style>
</head>
<body>
<div id="gate" role="alert">
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><rect x="7" y="2.5" width="10" height="19" rx="2"/><path d="M11 18.5h2"/></svg>
  <h2>Turn your phone sideways</h2>
  <p>The controller needs landscape so both thumbs reach a lever.</p>
</div>

<div id="app">
  <header class="bar">
    <div class="brand"><b>UniMAP RC</b><span>Agricultural Engineering</span></div>
    <div class="spacer"></div>
    <div class="pill" id="mode" data-mode="manual" aria-live="polite"><span class="dot"></span><span id="modeText">Manual</span></div>
    <div class="pill mono" id="link" data-state="wait" title="Round-trip time to the car"><span class="dot"></span><span id="linkText">Connecting</span></div>
    <button class="icon-btn" id="logBtn" aria-label="Show replay log">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M4 6h16M4 12h16M4 18h10"/></svg>
    </button>
    <button class="icon-btn" id="fsBtn" aria-label="Full screen">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"/></svg>
    </button>
  </header>

  <main class="deck" id="deck">
    <section class="zone" id="thrZone" aria-label="Throttle lever">
      <div class="zone-head"><span class="label">Throttle</span><span class="dir" id="thrDir">IDLE</span></div>
      <div class="lever v">
        <div class="track" id="thrTrack"><div class="fill" id="thrFill"></div><div class="knob spring" id="thrKnob"></div></div>
        <div class="scale" id="thrScale"></div>
      </div>
      <div class="readout" id="thrOut">0<small>%</small></div>
    </section>

    <section class="center">
      <div class="panel" id="motorPanel">
        <div class="panel-head"><h3>Motor output</h3><span class="val mono">PWM of 255</span></div>
        <div class="motors">
          <span class="side">L</span><div class="mbar"><div class="m" id="mL"></div></div><span class="num mono" id="nL">0</span>
          <span class="side">R</span><div class="mbar"><div class="m" id="mR"></div></div><span class="num mono" id="nR">0</span>
        </div>
      </div>

      <div class="panel">
        <div class="panel-head"><h3><label for="limit">Speed limit</label></h3><span class="val mono"><b id="limitPct">78%</b> · PWM <span id="limitPwm">200</span></span></div>
        <input class="range" type="range" id="limit" min="40" max="255" step="5" value="200">
        <div class="ticks"><span>40</span><span>100</span><span>150</span><span>200</span><span>255</span></div>
      </div>

      <div class="panel">
        <div class="panel-head"><h3>Route memory</h3><span class="val mono" id="memInfo">Ready</span></div>
        <div class="mem">
          <button class="btn" id="recBtn"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="7" fill="currentColor"/></svg><span id="recLabel">Record</span></button>
          <button class="btn" id="playBtn"><svg viewBox="0 0 24 24"><path d="M7 4.5v15l13-7.5z" fill="currentColor"/></svg>Play</button>
          <button class="btn" id="loopBtn" aria-pressed="false"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round"><path d="M17 2l3 3-3 3"/><path d="M4 11V9a4 4 0 014-4h12"/><path d="M7 22l-3-3 3-3"/><path d="M20 13v2a4 4 0 01-4 4H4"/></svg>Loop</button>
          <button class="btn" id="clearBtn" aria-label="Clear recorded route"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 7h16M10 11v6M14 11v6M6 7l1 13h10l1-13M9 7V4h6v3"/></svg></button>
        </div>
      </div>

      <button class="stop" id="stopBtn">STOP<kbd>SPACE</kbd></button>
    </section>

    <section class="zone" id="strZone" aria-label="Steering lever">
      <div class="zone-head"><span class="label">Steering</span><span class="dir" id="strDir">CENTRE</span></div>
      <div class="lever h">
        <div class="track" id="strTrack"><div class="fill" id="strFill"></div><div class="knob spring" id="strKnob"></div></div>
        <div class="scale" id="strScale"></div>
      </div>
      <div class="readout" id="strOut">0<small>%</small></div>
    </section>
  </main>
</div>

<aside id="logSheet" aria-label="Replay log">
  <header><h3>Replay log</h3><button class="icon-btn" id="logClose" aria-label="Close log"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M6 6l12 12M18 6L6 18"/></svg></button></header>
  <pre id="logBody">No replay yet. Record a route, then press Play.</pre>
</aside>
<div id="toast" role="status"></div>

<script>
(function () {
  'use strict';
  var $ = function (id) { return document.getElementById(id); };
  var clamp = function (v, a, b) { return Math.max(a, Math.min(b, v)); };
  var DEADZONE = 0.08;

  var st = {
    thr: 0, str: 0,          // -1..1 lever positions after dead zone
    limit: 200,              // PWM cap applied to both axes
    recording: false, recStart: 0,
    loop: false, replayKick: false,
    log: '', fails: 0
  };

  document.addEventListener('contextmenu', function (e) { e.preventDefault(); });

  // ---------- Network ----------
  function req(path, timeout) {
    var ctl = window.AbortController ? new AbortController() : null;
    var timer = setTimeout(function () { if (ctl) ctl.abort(); }, timeout || 900);
    var t0 = performance.now();
    return fetch(path, { cache: 'no-store', signal: ctl ? ctl.signal : undefined })
      .then(function (r) { if (!r.ok) throw new Error(r.status); return r.text(); })
      .then(function (txt) { linkOk(performance.now() - t0); return txt; })
      .catch(function (e) { linkFail(); throw e; })
      .finally(function () { clearTimeout(timer); });
  }
  function cmd(name) { return req('/control?cmd=' + name); }

  function linkOk(ms) {
    st.fails = 0;
    $('link').dataset.state = 'ok';
    $('linkText').textContent = Math.round(ms) + ' ms';
    $('deck').classList.remove('lost');
  }
  function linkFail() {
    st.fails++;
    if (st.fails >= 3) {
      $('link').dataset.state = 'lost';
      $('linkText').textContent = 'No link';
      $('deck').classList.add('lost');
    }
  }

  // ---------- Feedback ----------
  var toastTimer;
  function toast(msg) {
    var t = $('toast');
    t.textContent = msg;
    t.classList.add('show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(function () { t.classList.remove('show'); }, 1800);
  }
  function buzz(ms) { if (navigator.vibrate) try { navigator.vibrate(ms); } catch (e) {} }

  // ---------- Lever scales ----------
  function buildScale(el, axis) {
    var html = '';
    for (var i = -4; i <= 4; i++) {
      var pos = 50 - i * 10.5;              // keep ticks inside the knob's travel
      if (axis === 'h') pos = 50 + i * 10.5;
      var major = i % 2 === 0;
      var prop = axis === 'v' ? 'top' : 'left';
      html += '<i class="' + (major ? 'major' : '') + '" style="' + prop + ':' + pos + '%"></i>';
      if (axis === 'v' && major && i !== 0) html += '<em style="top:' + pos + '%">' + (i > 0 ? '+' : '') + i * 25 + '</em>';
    }
    el.innerHTML = html;
  }
  buildScale($('thrScale'), 'v');
  buildScale($('strScale'), 'h');

  // ---------- Levers ----------
  // Relative drag: wherever the thumb lands is zero, so touching never makes the car jump.
  function lever(zone, track, knob, fill, axis, onValue) {
    var pid = null, origin = 0, travel = 1, raw = 0;

    function measure() {
      var r = track.getBoundingClientRect();
      var len = axis === 'v' ? r.height : r.width;
      travel = Math.max(20, len / 2 - knob.offsetWidth / 2 - 6);
    }
    function draw(v) {
      var px = v * travel;
      knob.style.transform = axis === 'v' ? 'translateY(' + (-px) + 'px)' : 'translateX(' + px + 'px)';
      var a = Math.abs(px);
      if (axis === 'v') {
        fill.style.height = a + 'px';
        fill.style.top = v > 0 ? 'calc(50% - ' + a + 'px)' : '50%';
      } else {
        fill.style.width = a + 'px';
        fill.style.left = v < 0 ? 'calc(50% - ' + a + 'px)' : '50%';
      }
    }
    function set(v, fromPointer) {
      raw = clamp(v, -1, 1);
      knob.classList.toggle('spring', !fromPointer);
      draw(raw);
      var out = Math.abs(raw) < DEADZONE ? 0 : Math.sign(raw) * (Math.abs(raw) - DEADZONE) / (1 - DEADZONE);
      zone.classList.toggle('engaged', out !== 0);
      onValue(out);
    }

    zone.addEventListener('pointerdown', function (e) {
      if (pid !== null || e.button > 0) return;
      e.preventDefault();
      pid = e.pointerId;
      try { zone.setPointerCapture(pid); } catch (err) {}
      measure();
      origin = axis === 'v' ? e.clientY : e.clientX;
      zone.classList.add('held');
      buzz(6);
    });
    zone.addEventListener('pointermove', function (e) {
      if (e.pointerId !== pid) return;
      e.preventDefault();
      var d = axis === 'v' ? origin - e.clientY : e.clientX - origin;
      set(d / travel, true);
    });
    function release(e) {
      if (e.pointerId !== pid) return;
      pid = null;
      zone.classList.remove('held');
      set(0, false);
    }
    zone.addEventListener('pointerup', release);
    zone.addEventListener('pointercancel', release);
    zone.addEventListener('lostpointercapture', release);
    window.addEventListener('resize', measure);
    measure();

    return { set: set, held: function () { return pid !== null; } };
  }

  function pct(v) { return Math.round(Math.abs(v) * 100); }
  var thr = lever($('thrZone'), $('thrTrack'), $('thrKnob'), $('thrFill'), 'v', function (v) {
    st.thr = v;
    $('thrOut').innerHTML = (v > 0 ? '+' : v < 0 ? '−' : '') + pct(v) + '<small>%</small>';
    $('thrDir').textContent = v > 0 ? 'FORWARD' : v < 0 ? 'REVERSE' : 'IDLE';
    updateMotors();
  });
  var str = lever($('strZone'), $('strTrack'), $('strKnob'), $('strFill'), 'h', function (v) {
    st.str = v;
    $('strOut').innerHTML = pct(v) + '<small>%</small>';
    $('strDir').textContent = v > 0 ? 'RIGHT' : v < 0 ? 'LEFT' : 'CENTRE';
    updateMotors();
  });

  // ---------- Drive mixing preview (mirrors driveArcade() in the sketch) ----------
  function driveValues() {
    return { t: Math.round(st.thr * st.limit), s: Math.round(st.str * st.limit) };
  }
  function motorBar(bar, num, val) {
    var w = Math.abs(val) / 255 * 50;
    bar.style.width = w + '%';
    bar.style.left = val >= 0 ? '50%' : (50 - w) + '%';
    bar.classList.toggle('neg', val < 0);
    num.textContent = (val > 0 ? '+' : val < 0 ? '−' : '') + Math.abs(val);
  }
  function updateMotors() {
    var d = driveValues();
    motorBar($('mL'), $('nL'), clamp(d.t + d.s, -255, 255));
    motorBar($('mR'), $('nR'), clamp(d.t - d.s, -255, 255));
  }

  // ---------- Drive stream ----------
  // Streams at 10 Hz while a lever is off-centre. When idle it sends one zero
  // packet and then goes quiet, so it never fights a route replay.
  var lastSent = '0,0', inflight = false;
  setInterval(function () {
    var d = driveValues();
    var key = d.t + ',' + d.s;
    if (key === '0,0' && lastSent === '0,0') return;
    if (inflight) return;
    inflight = true;
    req('/control?cmd=drive&throttle=' + d.t + '&steer=' + d.s, 600)
      .then(function () { lastSent = key; })
      .catch(function () {})
      .finally(function () { inflight = false; });
  }, 100);

  // ---------- Speed limit ----------
  var limitEl = $('limit');
  function applyLimit() {
    st.limit = parseInt(limitEl.value, 10);
    var p = (st.limit - 40) / (255 - 40) * 100;
    limitEl.style.setProperty('--p', p + '%');
    $('limitPct').textContent = Math.round(st.limit / 255 * 100) + '%';
    $('limitPwm').textContent = st.limit;
    updateMotors();
  }
  try { var saved = localStorage.getItem('rc.limit'); if (saved) limitEl.value = saved; } catch (e) {}
  limitEl.addEventListener('input', function () {
    applyLimit();
    try { localStorage.setItem('rc.limit', limitEl.value); } catch (e) {}
  });
  applyLimit();

  // ---------- Stop ----------
  function emergencyStop(reason) {
    thr.set(0); str.set(0);
    lastSent = '0,0';
    setLoop(false);
    var tries = 0;
    (function send() {
      cmd('stop').catch(function () { if (++tries < 3) send(); });
    })();
    buzz(40);
    toast(reason || 'Stopped');
  }
  $('stopBtn').addEventListener('click', function () { emergencyStop(); });
  document.addEventListener('visibilitychange', function () {
    if (document.hidden && (st.thr || st.str)) emergencyStop('Stopped: app left the screen');
  });

  // ---------- Route memory ----------
  function setRecording(on) {
    st.recording = on;
    st.recStart = Date.now();
    $('recBtn').classList.toggle('on', on);
    $('recLabel').textContent = on ? '0:00' : 'Record';
    renderMode();
  }
  $('recBtn').addEventListener('click', function () {
    if (!st.recording) {
      cmd('start_recording').then(function () { setRecording(true); toast('Recording. Drive the route now.'); })
        .catch(function () { toast('Car not reachable'); });
    } else {
      cmd('stop_recording').then(function () { setRecording(false); toast('Route saved'); })
        .catch(function () { toast('Car not reachable'); });
    }
  });
  setInterval(function () {
    if (!st.recording) return;
    var s = Math.floor((Date.now() - st.recStart) / 1000);
    $('recLabel').textContent = Math.floor(s / 60) + ':' + String(s % 60).padStart(2, '0');
  }, 250);

  function play() {
    var p = st.recording ? cmd('stop_recording').then(function () { setRecording(false); }) : Promise.resolve();
    return p.then(function () { return cmd('replay'); }).then(function (txt) {
      if (/no path/i.test(txt)) { setLoop(false); toast('No route recorded yet'); return false; }
      return true;
    }).catch(function () { toast('Car not reachable'); return false; });
  }
  $('playBtn').addEventListener('click', function () { play(); });

  function setLoop(on) {
    st.loop = on;
    $('loopBtn').classList.toggle('on', on);
    $('loopBtn').setAttribute('aria-pressed', on);
    renderMode();
  }
  $('loopBtn').addEventListener('click', function () {
    var on = !st.loop;
    setLoop(on);
    if (on && replayState().phase !== 'running') play();
  });

  var clearArm = null;
  $('clearBtn').addEventListener('click', function () {
    var b = $('clearBtn');
    if (!clearArm) {
      b.classList.add('confirm');
      b.textContent = 'Clear?';
      clearArm = setTimeout(disarm, 2500);
      return;
    }
    disarm();
    setLoop(false);
    cmd('clear_memory').then(function () { setRecording(false); toast('Route cleared'); })
      .catch(function () { toast('Car not reachable'); });
  });
  var clearIcon = $('clearBtn').innerHTML;
  function disarm() {
    clearTimeout(clearArm); clearArm = null;
    var b = $('clearBtn');
    b.classList.remove('confirm');
    b.innerHTML = clearIcon;
  }

  // ---------- Replay log + mode ----------
  function replayState() {
    var lines = st.log.trim().split('\n').filter(Boolean);
    var last = lines[lines.length - 1] || '';
    var step = 0;
    for (var i = lines.length - 1; i >= 0; i--) {
      var m = /^Step (\d+)/.exec(lines[i]);
      if (m) { step = +m[1]; break; }
    }
    if (!lines.length) return { phase: 'none', step: 0 };
    if (/Replay finished/.test(last)) return { phase: 'done', step: step };
    if (/Replay stopped/.test(last)) return { phase: 'stopped', step: step };
    if (/Obstacle detected/.test(last)) return { phase: 'hold', step: step };
    return { phase: 'running', step: step };
  }

  function renderMode() {
    var r = replayState(), mode = 'manual', text = 'Manual', info = 'Ready';
    if (st.recording) { mode = 'rec'; text = 'Recording'; info = 'Recording'; }
    else if (r.phase === 'hold') { mode = 'hold'; text = 'Obstacle · paused'; info = 'Step ' + r.step; }
    else if (r.phase === 'running') { mode = 'replay'; text = (st.loop ? 'Loop' : 'Replay') + ' · step ' + r.step; info = 'Step ' + r.step; }
    else if (st.loop) { mode = 'replay'; text = 'Loop'; }
    else if (r.phase === 'done') { info = 'Finished · ' + r.step + ' steps'; }
    else if (r.phase === 'stopped') { info = 'Stopped at step ' + r.step; }
    var el = $('mode');
    el.dataset.mode = mode;
    $('modeText').textContent = text;
    $('memInfo').textContent = info;
  }

  function renderLog() {
    var body = $('logBody');
    if (!st.log.trim()) { body.textContent = 'No replay yet. Record a route, then press Play.'; return; }
    var esc = function (s) { return s.replace(/[&<>]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]; }); };
    body.innerHTML = st.log.trim().split('\n').map(function (l) {
      var cls = /Obstacle detected|Replay stopped/.test(l) ? 'ev-ob' : /finished/.test(l) ? 'ev-end' : '';
      return cls ? '<span class="' + cls + '">' + esc(l) + '</span>' : esc(l);
    }).join('\n');
    body.scrollTop = body.scrollHeight;
  }

  function pollLog() {
    req('/log', 1200).then(function (txt) {
      if (txt !== st.log) {
        var prev = replayState().phase;
        st.log = txt;
        renderLog();
        var now = replayState().phase;
        if (now === 'hold' && prev !== 'hold') { buzz([30, 60, 30]); toast('Obstacle ahead. Car paused.'); }
        if (now === 'stopped' && prev !== 'stopped') setLoop(false);
      }
      renderMode();
      // Loop: start the next pass once the firmware reports the route finished.
      if (st.loop && replayState().phase === 'done' && !st.replayKick) {
        st.replayKick = true;
        play().finally(function () { st.replayKick = false; });
      }
    }).catch(function () {}).finally(function () { setTimeout(pollLog, 500); });
  }
  pollLog();

  // ---------- Log drawer, full screen ----------
  $('logBtn').addEventListener('click', function () { $('logSheet').classList.add('open'); });
  $('logClose').addEventListener('click', function () { $('logSheet').classList.remove('open'); });

  $('fsBtn').addEventListener('click', function () {
    var d = document, el = d.documentElement;
    if (d.fullscreenElement) { d.exitFullscreen().catch(function () {}); return; }
    var p = el.requestFullscreen ? el.requestFullscreen() : Promise.reject();
    p.then(function () {
      if (screen.orientation && screen.orientation.lock) screen.orientation.lock('landscape').catch(function () {});
    }).catch(function () { toast('Full screen is not available here'); });
  });

  // ---------- Keyboard (laptop driving / classroom demo) ----------
  var keys = {};
  var KEYMAP = { ArrowUp: 'u', KeyW: 'u', ArrowDown: 'd', KeyS: 'd', ArrowLeft: 'l', KeyA: 'l', ArrowRight: 'r', KeyD: 'r' };
  function applyKeys() {
    if (!thr.held()) thr.set((keys.u ? 1 : 0) - (keys.d ? 1 : 0));
    if (!str.held()) str.set((keys.r ? 1 : 0) - (keys.l ? 1 : 0));
  }
  document.addEventListener('keydown', function (e) {
    if (e.target === limitEl) return;
    if (e.code === 'Space') { e.preventDefault(); if (!e.repeat) emergencyStop(); return; }
    if (e.code === 'Escape') { $('logSheet').classList.remove('open'); return; }
    var k = KEYMAP[e.code];
    if (!k) return;
    e.preventDefault();
    if (!keys[k]) { keys[k] = true; applyKeys(); }
  });
  document.addEventListener('keyup', function (e) {
    var k = KEYMAP[e.code];
    if (k && keys[k]) { keys[k] = false; applyKeys(); }
  });
  window.addEventListener('blur', function () { keys = {}; applyKeys(); });

  updateMotors();
})();
</script>
</body>
</html>
)rawliteral";
