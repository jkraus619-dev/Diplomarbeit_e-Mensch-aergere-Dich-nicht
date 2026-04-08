// Kurz: Kapselt die Leaderboard-Logik.
// Detail: IIFE, damit nur die benoetigten Funktionen exportiert werden.
(function (window) {
  'use strict';

  // Kurz: Berechnet die Rangliste aus den gespeicherten Nutzern.
  // Detail: Filtert gueltige User, kopiert Stats und sortiert nach Win-Rate.
  function computeLeaderboard(users) {
    return users
      // Kurz: Filtert gueltige Nutzereintraege.
      // Detail: Laesst nur Eintraege mit Usernamen durch.
      .filter(function (entry) {
        return entry && typeof entry.username === 'string' && entry.username.length;
      })
      // Kurz: Mappt Nutzer auf ein Anzeige-Objekt.
      // Detail: Sichert Stats ab und berechnet die Win-Rate.
      .map(function (entry) {
        var stats = entry.stats || { total: 0, won: 0, lost: 0 };
        return {
          username: entry.username,
          stats: {
            total: stats.total,
            won: stats.won,
            lost: stats.lost
          },
          rate: winRate(stats)
        };
      })
      // Kurz: Sortiert die Rangliste.
      // Detail: Erst nach Rate, dann Siegen, dann Gesamtspielen, dann Name.
      .sort(function (a, b) {
        if (b.rate !== a.rate) {
          return b.rate - a.rate;
        }
        if (b.stats.won !== a.stats.won) {
          return b.stats.won - a.stats.won;
        }
        if (b.stats.total !== a.stats.total) {
          return b.stats.total - a.stats.total;
        }
        return a.username.localeCompare(b.username);
      });
  }

  // Kurz: Rendert die Statistik-Tabelle.
  // Detail: Fuellt Summary, Tabelle und Highlights fuer den aktuellen Nutzer.
  function renderStatsTable(username) {
    var users = getUsers();
    var table = $('statsTable');
    var tbody = table ? table.querySelector('tbody') : null;
    var emptyState = $('emptyState');

    if (!users.length) {
      if (emptyState) {
        emptyState.hidden = false;
      }
      if (table) {
        table.style.display = 'none';
      }
      setFeedback($('summaryPlayers'), '0');
      setFeedback($('summaryGames'), '0');
      setFeedback($('summaryLeader'), 'Keine Daten');
      setFeedback($('summaryRank'), '-');
      setFeedback($('tableTotalGames'), '0');
      setFeedback($('tableTotalWins'), '0');
      setFeedback($('tableTotalLosses'), '0');
      return;
    }

    if (emptyState) {
      emptyState.hidden = true;
    }
    if (table) {
      table.style.display = '';
    }

    var leaderboard = computeLeaderboard(users);
    var totals = { total: 0, won: 0, lost: 0 };

    if (tbody) {
      tbody.textContent = '';
      // Kurz: Baut die Tabellenzeilen.
      // Detail: Summiert Totals und markiert den aktuellen Nutzer.
      leaderboard.forEach(function (entry, idx) {
        totals.total += entry.stats.total;
        totals.won += entry.stats.won;
        totals.lost += entry.stats.lost;

        var tr = document.createElement('tr');
        if (entry.username === username) {
          tr.classList.add('is-current');
        }
        var rateText = entry.stats.total ? entry.rate.toFixed(1) + '%' : '0.0%';

        // Kurz: Spalten per textContent befüllen (kein innerHTML → kein XSS).
        // Detail: Jede Zelle wird einzeln erstellt, damit keine HTML-Injection möglich ist.
        [String(idx + 1), entry.username, String(entry.stats.total),
          String(entry.stats.won), String(entry.stats.lost), rateText
        ].forEach(function (text) {
          var td = document.createElement('td');
          td.textContent = text;
          tr.appendChild(td);
        });

        tbody.appendChild(tr);
      });
    }

    setFeedback($('summaryPlayers'), String(users.length));
    setFeedback($('summaryGames'), String(totals.total));

    // Kurz: Sucht den aktuellen Leader.
    // Detail: Nimmt den ersten Spieler mit mindestens einem Spiel.
    var leader = leaderboard.find(function (entry) {
      return entry.stats.total > 0;
    });
    if (leader) {
      setFeedback($('summaryLeader'), leader.username + ' (' + leader.rate.toFixed(1) + '%)');
    } else {
      setFeedback($('summaryLeader'), 'Noch keine Spiele');
    }

    // Kurz: Findet den Rang des aktuellen Nutzers.
    // Detail: Sucht den Username in der Rangliste und gibt den Index zurueck.
    var rankIndex = leaderboard.findIndex(function (entry) {
      return entry.username === username;
    });
    setFeedback($('summaryRank'), rankIndex >= 0 ? '#' + (rankIndex + 1) : 'Keine Platzierung');

    setFeedback($('tableTotalGames'), String(totals.total));
    setFeedback($('tableTotalWins'), String(totals.won));
    setFeedback($('tableTotalLosses'), String(totals.lost));
  }

  // Kurz: Initialisiert die Statistik-Seite.
  // Detail: Prueft Session, rendert Tabelle und lauscht auf Storage-Updates.
  function initStats() {
    var username = requireSession();
    if (!username) {
      return;
    }
    renderStatsTable(username);

    // Kurz: Aktualisiert Tabelle bei Storage-Updates.
    // Detail: Reagiert auf Aenderungen der Nutzerdaten.
    window.addEventListener('storage', function (event) {
      if (event.key === 'ludo_users') {
        renderStatsTable(username);
      }
    });
  }

  window.computeLeaderboard = computeLeaderboard;
  window.renderStatsTable = renderStatsTable;
  window.initStats = initStats;
})(window);
