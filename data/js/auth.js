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

    var info = getUsersAndIndex(username);
    if (info.user && info.user.password === password) {
      setCurrentUser(username);
      setFeedback(log, 'Willkommen!', '#186e0e');
      // Kurz: Wartet kurz vor dem Seitenwechsel.
      // Detail: Gibt dem Nutzer Zeit, die Erfolgsmeldung zu sehen.
      setTimeout(function () {
        location.href = 'dashboard.html';
      }, 400);
      return;
    }

    setFeedback(log, 'Benutzername oder Passwort falsch.', '#b32121');
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
