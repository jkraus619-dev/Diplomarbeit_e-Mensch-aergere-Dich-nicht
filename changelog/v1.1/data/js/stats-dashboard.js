// Kurz: Kapselt die Statistik-Logik fuer das Dashboard.
// Detail: IIFE, damit nur die API-Funktionen am window landen.
(function (window) {
  'use strict';

  // Kurz: Aendert einen Nutzer in der gespeicherten Liste.
  // Detail: Holt Nutzer + Index, ruft einen Mutator auf und speichert danach.
  function mutateUser(username, mutator) {
    var info = getUsersAndIndex(username);
    if (info.index < 0) {
      return null;
    }
    var result = mutator(info.user, info.users, info.index);
    saveUsers(info.users);
    return { user: info.user, users: info.users, index: info.index, result: result };
  }

  // Kurz: Aendert nur die Stats eines Nutzers.
  // Detail: Nutzt mutateUser, gibt danach die aktualisierten Stats zurueck.
  function mutateStats(username, updater) {
    // Kurz: Ruft den Stats-Updater im Nutzerobjekt auf.
    // Detail: Mutator fuer stats-Objekt innerhalb des Nutzers.
    var info = mutateUser(username, function (user) {
      return updater(user.stats);
    });
    return info ? info.user.stats : null;
  }

  // Kurz: Holt die Stats eines Nutzers.
  // Detail: Gibt null zurueck, wenn der Nutzer nicht existiert.
  function getUserStats(username) {
    var info = getUsersAndIndex(username);
    return info.index >= 0 ? info.user.stats : null;
  }

  // Kurz: Aktualisiert die Statistik-Anzeige im Dashboard.
  // Detail: Setzt Total, Wins, Losses und berechnet die Win-Rate.
  function updateStatsDisplay(stats) {
    var data = stats || { total: 0, won: 0, lost: 0 };
    setText('totalGames', data.total);
    setText('wonGames', data.won);
    setText('lostGames', data.lost);
    var rate = winRate(data);
    setText('winRate', (rate ? rate.toFixed(1) : '0.0') + '%');
  }

  var dashMessageTimer = null;
  // Kurz: Zeigt eine kurze Meldung im Dashboard.
  // Detail: Setzt Feedback-Text, loescht ihn nach kurzer Zeit wieder.
  function showDashFeedback(message, color) {
    var feedback = $('dashFeedback');
    if (!feedback) {
      return;
    }
    if (dashMessageTimer) {
      clearTimeout(dashMessageTimer);
      dashMessageTimer = null;
    }
    setFeedback(feedback, message, color);
    if (message) {
      // Kurz: Loescht die Meldung nach einem Timeout.
      // Detail: Sorgt fuer automatisches Ausblenden.
      dashMessageTimer = setTimeout(function () {
        setFeedback(feedback, '');
        dashMessageTimer = null;
      }, 2500);
    }
  }

  // Kurz: Speichert Sieg oder Niederlage.
  // Detail: Erhoeht passende Zaehler und aktualisiert die Anzeige.
  function recordMatchResult(outcome) {
    var current = getCurrentUser();
    if (!current) {
      location.href = 'index.html';
      return;
    }
    if (outcome !== 'win' && outcome !== 'loss') {
      showDashFeedback('Unbekannter Spielausgang.', '#b32121');
      return;
    }

    // Kurz: Aktualisiert die Stats fuer das Ergebnis.
    // Detail: Erhoeht total und je nach Ergebnis won oder lost.
    var stats = mutateStats(current, function (data) {
      data.total += 1;
      if (outcome === 'win') {
        data.won += 1;
      } else {
        data.lost += 1;
      }
    });

    if (!stats) {
      showDashFeedback('Benutzer nicht gefunden.', '#b32121');
      return;
    }

    updateStatsDisplay(stats);
    showDashFeedback(outcome === 'win' ? 'Sieg erfasst.' : 'Niederlage erfasst.', '#186e0e');
  }

  // Kurz: Initialisiert das Dashboard.
  // Detail: Prueft Session, setzt UI, lauscht auf Storage-Updates und verbindet WS.
  function initDashboard() {
    var username = requireSession();
    if (!username) {
      return;
    }
    setText('userDisplay', username);
    updateStatsDisplay(getUserStats(username) || { total: 0, won: 0, lost: 0 });

    // Kurz: Aktualisiert die Anzeige bei Storage-Updates.
    // Detail: Reagiert auf Aenderungen an den gespeicherten Nutzerdaten.
    window.addEventListener('storage', function (event) {
      if (event.key === 'ludo_users') {
        updateStatsDisplay(getUserStats(username) || { total: 0, won: 0, lost: 0 });
      }
    });

    connectWS('dashboard');
  }

  window.mutateUser = mutateUser;
  window.mutateStats = mutateStats;
  window.getUserStats = getUserStats;
  window.updateStatsDisplay = updateStatsDisplay;
  window.showDashFeedback = showDashFeedback;
  window.recordMatchResult = recordMatchResult;
  window.initDashboard = initDashboard;
})(window);
