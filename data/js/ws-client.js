// Kurz: Kapselt den WebSocket-Client.
// Detail: IIFE, damit nur die benoetigten Funktionen global sichtbar sind.
(function (window) {
  'use strict';

  var socket = null;
  var wsReady = false;
  var wsHandlers = [];

  // Kurz: Schreibt einen Status ins WebSocket-Log.
  // Detail: Zeigt einfache Verbindungsmeldungen im UI an.
  function setWsLog(text) {
    var log = $('wsLog');
    if (log) {
      log.textContent = text;
    }
  }

  // Kurz: Registriert einen Handler fuer WS-Nachrichten.
  // Detail: Nur Funktionen werden aufgenommen, damit kein kaputter Handler gespeichert wird.
  function registerWsHandler(handler) {
    if (typeof handler === 'function') {
      wsHandlers.push(handler);
    }
  }

  // Kurz: Baut die WebSocket-Verbindung auf.
  // Detail: Erstellt die WS-URL, verbindet sich und setzt alle Event-Handler.
  function connectWS(context) {
    var host = location.hostname || '192.168.4.1';
    var url = (location.protocol === 'https:' ? 'wss://' : 'ws://') + host + '/ws';

    try {
      socket = new WebSocket(url);
    } catch (error) {
      console.warn('WebSocket creation failed', error);
      setWsLog('WebSocket: Verbindung fehlgeschlagen');
      return;
    }

    setWsLog('WebSocket: verbinde...');

    // Kurz: Reagiert auf erfolgreiche Verbindung.
    // Detail: Meldet "verbunden" und fragt optional den Batteriestatus an.
    socket.onopen = function () {
      wsReady = true;
      setWsLog('WebSocket: verbunden');
      window.dispatchEvent(new Event('ws-open'));
      if (context === 'battery') {
        try {
          sendWS('battery?');
        } catch (err) {
          console.warn('Battery request failed', err);
        }
      }
    };

    // Kurz: Reagiert auf Verbindungsende.
    // Detail: Zeigt im Log, dass der Socket getrennt ist.
    socket.onclose = function () {
      wsReady = false;
      setWsLog('WebSocket: getrennt');
      window.dispatchEvent(new Event('ws-close'));
    };

    // Kurz: Reagiert auf WS-Fehler.
    // Detail: Setzt eine Fehlermeldung im Log.
    socket.onerror = function () {
      setWsLog('WebSocket: Fehler');
    };

    // Kurz: Verarbeitet eingehende WS-Nachrichten.
    // Detail: Versucht JSON zu parsen, triggert Handler und zeigt eine Logzeile.
    socket.onmessage = function (event) {
      var text = event.data;
      try {
        var payload = JSON.parse(event.data);
        // Kurz: Ruft alle registrierten Handler auf.
        // Detail: Gibt das geparste Payload an jeden Callback weiter.
        wsHandlers.forEach(function (handler) {
          handler(payload);
        });
        var value = typeof payload.value !== 'undefined' ? ' ' + payload.value : '';
        setWsLog('Server: ' + (payload.type || '-') + value);
      } catch (parseError) {
        setWsLog('Server: ' + text);
      }
    };
  }

  // Kurz: Sendet einen Befehl ueber WebSocket.
  // Detail: Prueft zuerst die Verbindung und gibt true/false fuer Erfolg zurueck.
  function sendWS(cmd) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
      setWsLog('WebSocket: nicht verbunden – bitte Seite neu laden.');
      return false;
    }
    socket.send(cmd);
    return true;
  }

  function isWsReady() {
    return !!socket && wsReady && socket.readyState === WebSocket.OPEN;
  }

  window.registerWsHandler = registerWsHandler;
  window.connectWS = connectWS;
  window.sendWS = sendWS;
  window.isWsReady = isWsReady;
})(window);
