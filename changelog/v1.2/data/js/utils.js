// Kurz: Kapselt das Utils-Modul.
// Detail: IIFE, damit Variablen lokal bleiben und nur explizit exportiert wird.
(function (window) {
  'use strict';

  // Kurz: Holt ein DOM-Element per ID.
  // Detail: Kleine Abkuerzung fuer document.getElementById, damit der Code lesbarer bleibt.
  function $(id) {
    return document.getElementById(id);
  }

  // Kurz: Liest den Wert eines Inputs sauber aus.
  // Detail: Akzeptiert ID oder Element und kann optional trimmen, damit Eingaben ohne Leerzeichen kommen.
  function getValue(target, trim) {
    var el = typeof target === 'string' ? $(target) : target;
    if (!el || typeof el.value === 'undefined') {
      return '';
    }
    var value = String(el.value);
    return trim ? value.trim() : value;
  }

  // Kurz: Schreibt Text in ein Element.
  // Detail: Akzeptiert ID oder Element und setzt textContent sicher, auch wenn der Wert leer ist.
  function setText(target, message) {
    var el = typeof target === 'string' ? $(target) : target;
    if (!el) {
      return;
    }
    el.textContent = message === null || typeof message === 'undefined' ? '' : String(message);
  }

  // Kurz: Leert ein Inputfeld.
  // Detail: Funktioniert mit ID oder direktem Element und fasst nur Inputs mit value an.
  function clearValue(target) {
    var el = typeof target === 'string' ? $(target) : target;
    if (el && typeof el.value !== 'undefined') {
      el.value = '';
    }
  }

  // Kurz: Zeigt eine Meldung in einem Ziel-Element an.
  // Detail: Setzt Text und Farbe fuer Feedback, z.B. fuer Fehler oder Erfolgsmeldungen.
  function setFeedback(target, message, color) {
    if (!target) {
      return;
    }
    target.textContent = message || '';
    target.style.color = color || '#333';
  }

  // Kurz: Laedt die Nutzerliste roh aus dem LocalStorage.
  // Detail: Gibt immer ein Array zurueck und faengt JSON-Fehler sauber ab.
  function rawLoadUsers() {
    try {
      return JSON.parse(localStorage.getItem('ludo_users') || '[]');
    } catch (err) {
      console.warn('User list parse failed', err);
      return [];
    }
  }

  // Kurz: Speichert die Nutzerliste.
  // Detail: Schreibt die Liste als JSON in den LocalStorage.
  function saveUsers(users) {
    localStorage.setItem('ludo_users', JSON.stringify(users));
  }

  // Kurz: Stellt sicher, dass Stats sauber aufgebaut sind.
  // Detail: Legt fehlende Stats an und korrigiert ungueltige Werte auf 0.
  function normalizeUserStats(user) {
    if (!user || typeof user !== 'object') {
      return false;
    }
    if (!user.stats || typeof user.stats !== 'object') {
      user.stats = { total: 0, won: 0, lost: 0 };
      return true;
    }
    var dirty = false;
    // Kurz: Prueft alle relevanten Stats-Felder.
    // Detail: Stellt sicher, dass total/won/lost gueltige Zahlen sind.
    ['total', 'won', 'lost'].forEach(function (key) {
      var value = Number(user.stats[key]);
      if (!Number.isFinite(value) || value < 0) {
        value = 0;
      }
      if (user.stats[key] !== value) {
        user.stats[key] = value;
        dirty = true;
      }
    });
    return dirty;
  }

  // Kurz: Laedt alle Nutzer und repariert Stats falls noetig.
  // Detail: Verhindert kaputte Eintraege, speichert korrigierte Daten zurueck.
  function getUsers() {
    var users = rawLoadUsers();
    if (!Array.isArray(users)) {
      users = [];
    }
    var dirty = false;
    // Kurz: Normalisiert jeden Nutzer.
    // Detail: Korrigiert fehlerhafte Stats und merkt, ob gespeichert werden muss.
    users.forEach(function (user) {
      if (normalizeUserStats(user)) {
        dirty = true;
      }
    });
    if (dirty) {
      saveUsers(users);
    }
    return users;
  }

  // Kurz: Sucht einen Nutzer und gibt Index + Daten zurueck.
  // Detail: Nimmt den Username, sucht in der Liste und liefert auch das Array mit.
  function getUsersAndIndex(username) {
    var users = getUsers();
    // Kurz: Sucht den Index des Nutzers.
    // Detail: Gibt -1 wenn der Nutzer nicht gefunden wird.
    var index = users.findIndex(function (entry) {
      return entry && entry.username === username;
    });
    return {
      users: users,
      index: index,
      user: index >= 0 ? users[index] : null
    };
  }

  // Kurz: Liest den aktuell angemeldeten Nutzer.
  // Detail: Holt den Usernamen aus der Session, falls gesetzt.
  function getCurrentUser() {
    return sessionStorage.getItem('ludo_user');
  }

  // Kurz: Setzt den aktuell angemeldeten Nutzer.
  // Detail: Speichert den Usernamen in der Session.
  function setCurrentUser(username) {
    sessionStorage.setItem('ludo_user', username);
  }

  // Kurz: Meldet den Nutzer ab.
  // Detail: Loescht den Session-Eintrag fuer den aktuellen User.
  function clearCurrentUser() {
    sessionStorage.removeItem('ludo_user');
  }

  // Kurz: Berechnet die Siegrate in Prozent.
  // Detail: Liefert 0 wenn es keine Spiele gibt, sonst einen Prozentwert.
  function winRate(stats) {
    if (!stats || !stats.total) {
      return 0;
    }
    return (stats.won / stats.total) * 100;
  }

  // Kurz: Definiert die erlaubten Lobby-Farben.
  // Detail: Liste mit value + Label fuer Auswahl und Anzeige.
  var LOBBY_COLORS = [
    { value: 'red', label: 'Rot' },
    { value: 'blue', label: 'Blau' },
    { value: 'yellow', label: 'Gelb' },
    { value: 'green', label: 'Gr\u00fcn' }
  ];

  // Kurz: Gibt den Anzeigenamen fuer eine Farbe.
  // Detail: Sucht das Label in der Lobby-Farbliste und faellt auf "Unbekannt" zurueck.
  function colorLabel(color) {
    // Kurz: Findet die Farbe in der Liste.
    // Detail: Sucht nach passendem value.
    var match = LOBBY_COLORS.find(function (entry) {
      return entry.value === color;
    });
    return match ? match.label : 'Unbekannt';
  }

  // Kurz: Prueft und normalisiert eine Lobby-Farbe.
  // Detail: Wandelt in Kleinbuchstaben um und erlaubt nur Farben aus der Liste.
  function normalizeLobbyColor(value) {
    var color = (value || '').toLowerCase();
    // Kurz: Prueft, ob die Farbe erlaubt ist.
    // Detail: Gibt true, wenn die Farbe in der Liste vorkommt.
    var match = LOBBY_COLORS.some(function (entry) {
      return entry.value === color;
    });
    return match ? color : '';
  }

  // Kurz: Erstellt ein leeres Lobby-Objekt.
  // Detail: Nutzt ein fixes Format, damit der Rest des Codes stabil bleibt.
  function emptyLobby() {
    return { players: [] };
  }

  // Kurz: Laedt die Lobby aus der Session.
  // Detail: Validiert das Format und bereinigt ungueltige Spieler.
  function loadLobby() {
    try {
      var parsed = JSON.parse(sessionStorage.getItem('ludo_lobby') || 'null');
      if (!parsed || !Array.isArray(parsed.players)) {
        return emptyLobby();
      }
      // Kurz: Schneidet und mappt die Spielerliste.
      // Detail: Begrenzt auf 4 Spieler und normalisiert Name/Farbe.
      var players = parsed.players.slice(0, 4).map(function (entry) {
        var name = entry && entry.name ? String(entry.name).trim() : '';
        var color = normalizeLobbyColor(entry && entry.color);
        return name ? { name: name, color: color } : null;
      }).filter(Boolean);
      return { players: players };
    } catch (err) {
      console.warn('Lobby parse failed', err);
      return emptyLobby();
    }
  }

  // Kurz: Speichert die Lobby in der Session.
  // Detail: Begrenzung auf 4 Spieler, damit die Daten immer kompakt bleiben.
  function saveLobby(lobby) {
    var players = lobby && Array.isArray(lobby.players) ? lobby.players.slice(0, 4) : [];
    sessionStorage.setItem('ludo_lobby', JSON.stringify({ players: players }));
  }

  // Kurz: Loescht die Lobby.
  // Detail: Entfernt den Session-Eintrag komplett.
  function clearLobby() {
    sessionStorage.removeItem('ludo_lobby');
  }

  // Kurz: Baut eine Map der bereits verwendeten Farben.
  // Detail: Damit kann man schnell pruefen, welche Farben noch frei sind.
  function usedColors(lobby) {
    var players = lobby && Array.isArray(lobby.players) ? lobby.players : [];
    var map = {};
    // Kurz: Markiert verwendete Farben.
    // Detail: Baut eine Map mit allen Farben aus der Lobby.
    players.forEach(function (player) {
      if (player && player.color) {
        map[player.color] = true;
      }
    });
    return map;
  }

  // Kurz: Findet die naechste freie Farbe.
  // Detail: Geht die Farbliste durch und nimmt die erste, die noch niemand nutzt.
  function nextFreeColor(lobby) {
    var used = usedColors(lobby);
    // Kurz: Sucht die erste freie Farbe.
    // Detail: Nimmt die erste Farbe, die noch nicht vergeben ist.
    var available = LOBBY_COLORS.find(function (entry) {
      return !used[entry.value];
    });
    return available ? available.value : '';
  }

  // Kurz: Validiert die Lobby vor dem Spielstart.
  // Detail: Prueft Spielerzahl, Namen und dass jede Farbe nur einmal vergeben ist.
  function lobbyValidation(lobby) {
    var players = lobby && Array.isArray(lobby.players) ? lobby.players : [];
    if (players.length < 2) {
      return { ok: false, message: 'Mindestens 2 Spieler.' };
    }
    var seenColors = {};
    for (var i = 0; i < players.length; i += 1) {
      var player = players[i];
      if (!player || !player.name) {
        return { ok: false, message: 'Spielernamen fehlen.' };
      }
      if (!player.color) {
        return { ok: false, message: 'Jeder Spieler braucht eine Farbe.' };
      }
      if (seenColors[player.color]) {
        return { ok: false, message: 'Alle Farben m\u00fcssen verschieden sein.' };
      }
      seenColors[player.color] = true;
    }
    return { ok: true, message: 'Bereit.' };
  }

  // Kurz: Sucht einen Spieler in der Lobby.
  // Detail: Vergleicht Namen case-insensitive und gibt den Index zurueck.
  function findPlayerIndex(lobby, name) {
    var players = lobby && Array.isArray(lobby.players) ? lobby.players : [];
    // Kurz: Findet den Spielerindex per Name.
    // Detail: Vergleich ohne Gross/Kleinschreibung.
    return players.findIndex(function (player) {
      return player && player.name && player.name.toLowerCase() === name.toLowerCase();
    });
  }

  // Kurz: Fuegt den aktuellen Nutzer zur Lobby hinzu.
  // Detail: Prueft Login, Lobby-Groesse und reserviert eine freie Farbe.
  function joinCurrentUserToLobby(options) {
    var username = getCurrentUser();
    if (!username) {
      return { ok: false, message: 'Bitte zuerst anmelden.', lobby: emptyLobby() };
    }
    var lobby = options && options.reset ? emptyLobby() : loadLobby();
    var existingIndex = findPlayerIndex(lobby, username);
    if (existingIndex >= 0) {
      return { ok: true, lobby: lobby, joined: false };
    }
    if (lobby.players.length >= 4) {
      return { ok: false, message: 'Lobby ist voll (max. 4 Spieler).', lobby: lobby };
    }
    lobby.players.push({ name: username, color: nextFreeColor(lobby) });
    saveLobby(lobby);
    return { ok: true, lobby: lobby, joined: true };
  }

  window.$ = $;
  window.getValue = getValue;
  window.setText = setText;
  window.clearValue = clearValue;
  window.setFeedback = setFeedback;
  window.rawLoadUsers = rawLoadUsers;
  window.saveUsers = saveUsers;
  window.normalizeUserStats = normalizeUserStats;
  window.getUsers = getUsers;
  window.getUsersAndIndex = getUsersAndIndex;
  window.getCurrentUser = getCurrentUser;
  window.setCurrentUser = setCurrentUser;
  window.clearCurrentUser = clearCurrentUser;
  window.winRate = winRate;
  window.LOBBY_COLORS = LOBBY_COLORS;
  window.colorLabel = colorLabel;
  window.normalizeLobbyColor = normalizeLobbyColor;
  window.emptyLobby = emptyLobby;
  window.loadLobby = loadLobby;
  window.saveLobby = saveLobby;
  window.clearLobby = clearLobby;
  window.usedColors = usedColors;
  window.nextFreeColor = nextFreeColor;
  window.lobbyValidation = lobbyValidation;
  window.findPlayerIndex = findPlayerIndex;
  window.joinCurrentUserToLobby = joinCurrentUserToLobby;
  window.LOBBY_KEY = 'ludo_lobby';
  window.GAME_START_KEY = 'ludo_game_start';
})(window);
