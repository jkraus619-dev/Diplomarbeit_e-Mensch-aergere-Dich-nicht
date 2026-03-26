// Kurz: Kapselt den Seiten-Initializer.
// Detail: IIFE fuer saubere Initialisierung pro HTML-Seite.
(function (window) {
  'use strict';

  // Kurz: Liest den Dateinamen der aktuellen Seite.
  // Detail: Entfernt Query und Hash, liefert default index.html.
  function getPageName() {
    var path = (location.pathname || '').split('?')[0].split('#')[0];
    var name = path.split('/').pop();
    return name ? name.toLowerCase() : 'index.html';
  }

  // Kurz: Lauscht auf den Spielstart in anderen Tabs.
  // Detail: Wenn GAME_START_KEY gesetzt wird, wird zur Game-Seite gewechselt.
  function listenForGameStart() {
    // Kurz: Reagiert auf Storage-Event fuer den Spielstart.
    // Detail: Wechselt automatisch zur Game-Seite, wenn ein Start-Flag gesetzt wird.
    window.addEventListener('storage', function (event) {
      if (event.key === GAME_START_KEY && event.newValue && getCurrentUser()) {
        location.href = 'game.html';
      }
    });
  }

  // Kurz: Initialisiert die Seite nach dem Laden.
  // Detail: Setzt Session zurueck wenn noetig und ruft die passende Init-Funktion.
  document.addEventListener('DOMContentLoaded', function () {
    listenForGameStart();
    var page = getPageName();
    if (page === 'index.html' || page === '') {
      clearAllSession();
      connectWS('login'); // WS verbinden damit der ESP32 Login gegen SD pruefen kann
      return;
    }
    if (page === 'register.html') {
      clearAllSession();
      connectWS('register'); // WS verbinden damit der ESP32 den neuen Benutzer auf SD speichern kann
      return;
    }

    var initMap = {
      'dashboard.html': window.initDashboard,
      'spiel.html': window.initPlayHub,
      'battery.html': window.initBattery,
      'profile.html': window.initProfile,
      'stats.html': window.initStats,
      'pregame.html': window.initPregame,
      'game.html': window.initGame
    };

    var init = initMap[page];
    if (typeof init === 'function') {
      init();
    }
  });

  window.getPageName = getPageName;
})(window);
