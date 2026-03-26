// Kurz: Kapselt das Auth-Modul.
// Detail: IIFE, damit Login/Registrierung sauber im eigenen Scope bleiben.
(function (window) {
  'use strict';

  // Kurz: Fuehrt den Login durch.
  // Detail: Liest Benutzername/Passwort, prueft gegen gespeicherte Nutzer und leitet weiter.
  function login() {
    var username = getValue('username', true);
    var password = getValue('password');
    var log = $('log');

    if (!username || !password) {
      setFeedback(log, 'Bitte alle Felder ausf\u00fcllen.', '#b32121');
      return;
    }

    if (!isWsReady()) {
      setFeedback(log, 'Keine Verbindung zum Board \u2013 Seite neu laden.', '#b32121');
      return;
    }

    setFeedback(log, 'Anmelden...', '#555');

    // Handler fuer die Antwort des ESP32 – Flag verhindert Mehrfachausfuehrung
    var loginHandled = false;
    registerWsHandler(function(payload) {
      if (loginHandled) return;
      if (payload.type !== 'loginOk' && payload.type !== 'loginErr') return;
      loginHandled = true;

      if (payload.type === 'loginOk') {
        setCurrentUser(payload.user);
        setFeedback(log, 'Willkommen!', '#186e0e');
        setTimeout(function () { location.href = 'dashboard.html'; }, 400);
      } else {
        setFeedback(log, payload.msg || 'Anmeldung fehlgeschlagen.', '#b32121');
      }
    });

    sendWS(JSON.stringify({ cmd: 'login', user: username, password: password }));
  }

  // Kurz: Registriert einen neuen Nutzer.
  // Detail: Prüft Eingaben, validiert Passwort, legt den Nutzer mit Stats an und leitet um.
  function register() {
    var username = getValue('r_username', true);
    var password = getValue('r_password');
    var passwordRepeat = getValue('r_password2');
    var log = $('rlog');

    if (!username || !password || !passwordRepeat) {
      setFeedback(log, 'Bitte alle Felder ausf\u00fcllen.', '#b32121');
      return;
    }
    if (password !== passwordRepeat) {
      setFeedback(log, 'Passw\u00f6rter stimmen nicht \u00fcberein.', '#b32121');
      return;
    }
    if (password.length < 4) {
      setFeedback(log, 'Passwort muss mindestens 4 Zeichen haben.', '#b32121');
      return;
    }

    var users = getUsers();
    // Kurz: Prueft, ob der Benutzername schon existiert.
    // Detail: Durchsucht die aktuelle Nutzerliste nach einem Treffer.
    var exists = users.some(function (entry) {
      return entry && entry.username === username;
    });
    if (exists) {
      setFeedback(log, 'Benutzername bereits vergeben.', '#b32121');
      return;
    }

    users.push({
      username: username,
      password: password,
      stats: { total: 0, won: 0, lost: 0 }
    });
    saveUsers(users);

    // Neuen Benutzer auch auf der SD-Karte des ESP32 speichern
    var wsMsg = JSON.stringify({ cmd: 'register', user: username, password: password });
    if (isWsReady()) {
      sendWS(wsMsg);
    } else {
      // WS noch nicht verbunden → warten bis Verbindung steht, dann senden
      window.addEventListener('ws-open', function handler() {
        window.removeEventListener('ws-open', handler);
        sendWS(wsMsg);
      });
    }

    setFeedback(log, 'Registrierung fertig. Weiterleitung ...', '#186e0e');
    // Kurz: Wartet kurz vor dem Zurueckleiten.
    // Detail: Laesst die Erfolgsmeldung kurz stehen.
    setTimeout(function () {
      location.href = 'index.html';
    }, 600);
  }

  // Kurz: Meldet den Nutzer ab.
  // Detail: Loescht Sessiondaten und geht zur Startseite.
  function logout() {
    clearAllSession();
    location.href = 'index.html';
  }

  window.login = login;
  window.register = register;
  window.logout = logout;
})(window);
