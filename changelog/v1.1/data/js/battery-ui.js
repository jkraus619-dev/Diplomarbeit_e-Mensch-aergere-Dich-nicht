// Kurz: Kapselt die Battery-UI Logik.
// Detail: IIFE, damit Chart und UI sauber im Modul bleiben.
(function (window) {
  'use strict';

  var batterySamples = [];

  // Kurz: Aktualisiert die Batterieanzeige.
  // Detail: Setzt Balkenbreite, Farbe und Text fuer Prozent und Spannung.
  function setBatteryUI(percent, millivolt) {
    var pct = Math.max(0, Math.min(100, Number(percent) || 0));
    var mv = Number(millivolt) || 0;
    var fill = $('batteryFill');
    if (fill) {
      fill.style.width = pct + '%';
      var fillColor = '#2e7d32';
      if (pct <= 30) {
        fillColor = '#c62828';
      } else if (pct <= 70) {
        fillColor = '#f9a825';
      }
      fill.style.background = fillColor;
    }
    setText('percent', pct.toFixed(0));
    setText('voltage', (mv / 1000).toFixed(2));
  }

  // Kurz: Zeichnet den Batterie-Verlauf im Canvas.
  // Detail: Plottet die Samples als Linie mit einfacher Achse.
  function drawBatteryChart() {
    var canvas = $('batChart');
    if (!canvas) {
      return;
    }
    var ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    if (batterySamples.length < 2) {
      return;
    }

    var padding = 20;
    var xMin = batterySamples[0].time;
    var xMax = batterySamples[batterySamples.length - 1].time;
    if (xMax === xMin) {
      xMax += 1;
    }

    var width = canvas.width - padding * 2;
    var height = canvas.height - padding * 2;

    // Kurz: Mappt Zeit auf die X-Koordinate.
    // Detail: Skaliert Zeitwerte auf die Zeichenflaeche.
    function mapX(time) {
      return padding + ((time - xMin) / (xMax - xMin)) * width;
    }

    // Kurz: Mappt Prozent auf die Y-Koordinate.
    // Detail: Skaliert 0-100% auf die Zeichenflaeche.
    function mapY(percent) {
      return canvas.height - padding - (percent / 100) * height;
    }

    ctx.strokeStyle = '#999';
    ctx.beginPath();
    ctx.moveTo(padding, padding);
    ctx.lineTo(padding, canvas.height - padding);
    ctx.lineTo(canvas.width - padding, canvas.height - padding);
    ctx.stroke();

    ctx.strokeStyle = '#1b6d1a';
    ctx.beginPath();
    // Kurz: Zeichnet die Sample-Linie.
    // Detail: Verbinden der Punkte in Reihenfolge ihres Zeitstempels.
    batterySamples.forEach(function (sample, index) {
      var x = mapX(sample.time);
      var y = mapY(sample.percent);
      if (index === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    });
    ctx.stroke();
  }

  // Kurz: Fuegt einen neuen Batteriesample hinzu.
  // Detail: Begrenzt die Liste und triggert das Neuzeichnen.
  function pushBatterySample(percent) {
    batterySamples.push({
      time: Date.now(),
      percent: Math.max(0, Math.min(100, Number(percent) || 0))
    });
    if (batterySamples.length > 120) {
      batterySamples.shift();
    }
    drawBatteryChart();
  }

  // Kurz: Verarbeitet Battery-Payloads vom Server.
  // Detail: Aktualisiert UI und speichert den Verlaufspunkt.
  function handleBatteryPayload(message) {
    if (!message || message.type !== 'battery') {
      return;
    }
    setBatteryUI(message.percent, message.mv);
    pushBatterySample(message.percent);
  }

  registerWsHandler(handleBatteryPayload);

  // Kurz: Initialisiert die Batterie-Seite.
  // Detail: Prueft Session und startet die WS-Verbindung.
  function initBattery() {
    if (!requireSession()) {
      return;
    }
    connectWS('battery');
  }

  window.setBatteryUI = setBatteryUI;
  window.drawBatteryChart = drawBatteryChart;
  window.pushBatterySample = pushBatterySample;
  window.handleBatteryPayload = handleBatteryPayload;
  window.initBattery = initBattery;
})(window);
