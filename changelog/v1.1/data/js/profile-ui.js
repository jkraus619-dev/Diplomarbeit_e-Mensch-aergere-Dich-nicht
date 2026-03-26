// Kurz: Kapselt das Profil-UI.
// Detail: IIFE, damit Profil-Logik nicht global herumliegt.
(function (window) {
  'use strict';

  // Kurz: Schreibt die Profil-Statistik in die UI.
  // Detail: Zeigt Gesamtspiele, Siege, Niederlagen und Win-Rate an.
  function renderProfileStats(stats) {
    var data = stats || { total: 0, won: 0, lost: 0 };
    setText('profileTotal', data.total);
    setText('profileWon', data.won);
    setText('profileLost', data.lost);
    var rate = winRate(data);
    setText('profileWinRate', rate ? rate.toFixed(1) : '0.0');
  }

  // Kurz: Initialisiert die Profilseite.
  // Detail: Prueft Session, fuellt Daten und registriert Button-Handler.
  function initProfile() {
    var username = requireSession();
    if (!username) {
      return;
    }
    var info = getUsersAndIndex(username);
    if (info.index < 0) {
      clearCurrentUser();
      location.href = 'index.html';
      return;
    }

    setText('profileName', username);
    renderProfileStats(info.user.stats);

    var pwForm = $('passwordForm');
    var pwFeedback = $('pwFeedback');
    if (pwForm) {
      // Kurz: Behandelt das Passwort-Formular.
      // Detail: Validiert Eingaben, prueft das aktuelle Passwort und speichert neu.
      pwForm.addEventListener('submit', function (event) {
        event.preventDefault();
        var currentPw = getValue('currentPassword');
        var newPw = getValue('newPassword');
        var confirmPw = getValue('confirmPassword');

        if (!currentPw || !newPw || !confirmPw) {
          setFeedback(pwFeedback, 'Bitte alle Felder ausf\u00fcllen.', '#b32121');
          return;
        }
        var lookup = getUsersAndIndex(username);
        if (lookup.index < 0) {
          setFeedback(pwFeedback, 'Benutzer nicht gefunden.', '#b32121');
          return;
        }
        if (lookup.user.password !== currentPw) {
          setFeedback(pwFeedback, 'Aktuelles Passwort stimmt nicht.', '#b32121');
          return;
        }
        if (newPw.length < 4) {
          setFeedback(pwFeedback, 'Neues Passwort muss mindestens 4 Zeichen haben.', '#b32121');
          return;
        }
        if (newPw !== confirmPw) {
          setFeedback(pwFeedback, 'Best\u00e4tigung stimmt nicht.', '#b32121');
          return;
        }

        lookup.user.password = newPw;
        saveUsers(lookup.users);
        clearValue('currentPassword');
        clearValue('newPassword');
        clearValue('confirmPassword');
        setFeedback(pwFeedback, 'Passwort aktualisiert.', '#186e0e');
      });
    }

    var resetButton = $('resetStatsBtn');
    var resetFeedback = $('resetFeedback');
    if (resetButton) {
      // Kurz: Setzt die Statistik zurueck.
      // Detail: Fragt nach und setzt alle Stats auf 0.
      resetButton.addEventListener('click', function () {
        if (!window.confirm('Statistik wirklich zur\u00fccksetzen?')) {
          return;
        }
        // Kurz: Setzt die Stats im Nutzerobjekt zurueck.
        // Detail: Mutator-Funktion fuer total/won/lost.
        var stats = mutateStats(username, function (data) {
          data.total = 0;
          data.won = 0;
          data.lost = 0;
        }) || { total: 0, won: 0, lost: 0 };
        renderProfileStats(stats);
        setFeedback(resetFeedback, 'Statistik zur\u00fcckgesetzt.', '#186e0e');
        // Kurz: Entfernt die Reset-Meldung nach kurzer Zeit.
        // Detail: Macht das Feedback sauber, ohne extra Klick.
        setTimeout(function () {
          setFeedback(resetFeedback, '');
        }, 2000);
      });
    }

    // Kurz: Aktualisiert das Profil bei Storage-Updates.
    // Detail: Reagiert auf Aenderungen der Nutzerdaten im LocalStorage.
    window.addEventListener('storage', function (event) {
      if (event.key === 'ludo_users') {
        renderProfileStats(getUserStats(username) || { total: 0, won: 0, lost: 0 });
      }
    });
  }

  window.renderProfileStats = renderProfileStats;
  window.initProfile = initProfile;
})(window);
