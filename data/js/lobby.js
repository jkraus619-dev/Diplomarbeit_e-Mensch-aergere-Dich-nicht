// Kurz: Kapselt die Lobby-Logik.
// Detail: IIFE, damit Lobby-Funktionen sauber isoliert sind.
(function (window) {
  'use strict';

  var LOBBY_ACTION_KEY = 'ludo_lobby_action';

  function saveLobbyAction(action) {
    if (action) {
      sessionStorage.setItem(LOBBY_ACTION_KEY, action);
    } else {
      sessionStorage.removeItem(LOBBY_ACTION_KEY);
    }
  }

  function loadLobbyAction() {
    return sessionStorage.getItem(LOBBY_ACTION_KEY) || '';
  }

  function sendLobbyCommand(action, payload) {
    var cmd = {
      cmd: 'lobby',
      action: action
    };
    if (payload && typeof payload === 'object') {
      Object.keys(payload).forEach(function (key) {
        cmd[key] = payload[key];
      });
    }
    return sendWS(JSON.stringify(cmd));
  }

  function requestLobbySync() {
    return sendLobbyCommand('get');
  }

  // Kurz: Deaktiviert "Lobby erstellen" wenn eine Lobby aktiv ist.
  function updatePlayHubButtons(lobbyExists) {
    var btn = $('btnCreateLobby');
    if (btn) {
      btn.disabled = lobbyExists;
    }
  }

  // Kurz: Verarbeitet Lobby-Updates vom Server.
  // Detail: Speichert Snapshot lokal und rendert neu.
  function handleLobbyPayload(message) {
    if (!message || message.type !== 'lobby') {
      return;
    }
    var players = Array.isArray(message.players) ? message.players : [];
    saveLobby({ players: players });
    updatePlayHubButtons(players.length > 0);
    renderLobby();
  }

  function handleStartPayload(message) {
    if (!message || message.type !== 'start' || !message.ok) {
      return;
    }
    try {
      localStorage.setItem(GAME_START_KEY, String(Date.now()));
    } catch (errStore) {
      console.warn('Konnte Start-Flag nicht speichern', errStore);
    }
    location.href = 'game.html';
  }

  registerWsHandler(handleLobbyPayload);
  registerWsHandler(handleStartPayload);

  // Kurz: Rendert die Lobbyliste in der UI.
  // Detail: Baut die Spielerliste, Farbauswahl und Start-Status dynamisch auf.
  function renderLobby() {
    var lobby = loadLobby();
    var list = $('playerList');
    var feedback = $('pregameFeedback');
    var startBtn = $('pregameStartBtn');
    if (!list) {
      return;
    }

    list.textContent = '';
    var used = usedColors(lobby);

    if (!lobby.players.length) {
      var empty = document.createElement('p');
      empty.className = 'empty-state';
      empty.textContent = 'Noch keine Spieler beigetreten.';
      list.appendChild(empty);
    }

    // Kurz: Erstellt UI-Zeilen fuer jeden Spieler.
    // Detail: Zeigt Name, Farbauswahl und Entfernen-Button.
    lobby.players.forEach(function (player) {
      var row = document.createElement('div');
      row.className = 'lobby-row';

      var name = document.createElement('div');
      name.className = 'lobby-name';
      name.textContent = player.name;
      row.appendChild(name);

      var selectWrap = document.createElement('div');
      selectWrap.className = 'lobby-color';
      var select = document.createElement('select');
      var placeholder = document.createElement('option');
      placeholder.value = '';
      placeholder.textContent = 'Farbe w\u00e4hlen';
      select.appendChild(placeholder);
      // Kurz: Fuellt die Farbauswahl.
      // Detail: Deaktiviert bereits vergebene Farben.
      LOBBY_COLORS.forEach(function (choice) {
        var option = document.createElement('option');
        option.value = choice.value;
        option.textContent = choice.label;
        if (player.color === choice.value) {
          option.selected = true;
        }
        if (used[choice.value] && player.color !== choice.value) {
          option.disabled = true;
        }
        select.appendChild(option);
      });
      // Kurz: Reagiert auf Farbaenderung.
      // Detail: Speichert die neue Farbe in der Lobby.
      select.addEventListener('change', function (event) {
        changeLobbyColor(player.name, event.target.value);
      });
      selectWrap.appendChild(select);
      row.appendChild(selectWrap);

      var removeBtn = document.createElement('button');
      removeBtn.type = 'button';
      removeBtn.className = 'btn-ghost';
      removeBtn.textContent = 'Entfernen';
      // Kurz: Entfernt den Spieler aus der Lobby.
      // Detail: Loescht den Eintrag und rendert die Liste neu.
      removeBtn.addEventListener('click', function () {
        removeLobbyPlayer(player.name);
      });
      row.appendChild(removeBtn);

      list.appendChild(row);
    });

    var validation = lobbyValidation(lobby);
    if (startBtn) {
      startBtn.disabled = !validation.ok;
    }
    setFeedback(feedback, validation.message || '', validation.ok ? '#186e0e' : '#b32121');
  }

  // Kurz: Entfernt einen Spieler per Name.
  // Detail: Wenn WS verbunden, sendet Befehl an Server – UI-Update kommt via Snapshot.
  //         Offline-Fallback: lokale Aenderung + sofortiges Re-Render.
  function removeLobbyPlayer(playerName) {
    if (isWsReady()) {
      sendLobbyCommand('remove', { user: playerName });
      // Server antwortet mit Lobby-Snapshot → handleLobbyPayload → renderLobby
    } else {
      var lobby = loadLobby();
      var index = findPlayerIndex(lobby, playerName);
      if (index < 0) {
        return;
      }
      lobby.players.splice(index, 1);
      saveLobby(lobby);
      renderLobby();
    }
  }

  // Kurz: Setzt die Farbe eines Spielers.
  // Detail: Wenn WS verbunden, sendet Befehl an Server – UI-Update kommt via Snapshot.
  //         Offline-Fallback: lokale Aenderung + sofortiges Re-Render.
  function changeLobbyColor(playerName, color) {
    var normalized = normalizeLobbyColor(color);
    if (isWsReady()) {
      sendLobbyCommand('setColor', { user: playerName, color: normalized });
      // Server antwortet mit Lobby-Snapshot → handleLobbyPayload → renderLobby
    } else {
      var lobby = loadLobby();
      var index = findPlayerIndex(lobby, playerName);
      if (index < 0) {
        return;
      }
      lobby.players[index].color = normalized;
      saveLobby(lobby);
      renderLobby();
    }
  }

  // Kurz: Erstellt eine neue Lobby und oeffnet die Lobby-Seite.
  // Detail: Setzt alte Daten zurueck, fuegt aktuellen Nutzer hinzu und navigiert.
  function createLobbyAndOpen() {
    var current = getCurrentUser();
    if (!current) {
      location.href = 'index.html';
      return;
    }
    saveLobbyAction('create');
    localStorage.removeItem(GAME_START_KEY);
    location.href = 'pregame.html';
  }

  // Kurz: Tritt einer Lobby vom Dashboard aus bei.
  // Detail: Prueft Login, fuegt den Nutzer hinzu und wechselt zur Lobby-Seite.
  function joinLobbyFromDashboard() {
    if (!getCurrentUser()) {
      location.href = 'index.html';
      return;
    }
    saveLobbyAction('join');
    location.href = 'pregame.html';
  }

  // Kurz: Startet das Spiel aus der Lobby.
  // Detail: Validiert die Lobby und sendet startGame an den Server.
  //         Die Navigation zu game.html erfolgt fuer ALLE Clients ueber handleStartPayload,
  //         sobald der Server mit {"type":"start","ok":true} antwortet und broadcastet.
  //         Das stellt sicher, dass alle verbundenen Geraete gleichzeitig weitergeleitet werden.
  function launchGame() {
    var lobby = loadLobby();
    var feedback = $('pregameFeedback');
    var validation = lobbyValidation(lobby);
    if (!validation.ok) {
      setFeedback(feedback, validation.message, '#b32121');
      return;
    }
    if (!isWsReady()) {
      setFeedback(feedback, 'Keine Verbindung zum Server.', '#b32121');
      return;
    }
    setFeedback(feedback, 'Spiel startet...', '#186e0e');
    sendWS(JSON.stringify({ cmd: 'startGame', players: lobby.players }));
    // Navigation erfolgt in handleStartPayload wenn Server antwortet
  }

  // Kurz: Initialisiert die Lobby-Seite.
  // Detail: Stellt Session sicher, tritt der Lobby bei und verbindet WS.
  function initPregame() {
    var username = requireSession();
    if (!username) {
      return;
    }
    connectWS('pregame');
    var onOpen = function () {
      var action = loadLobbyAction();
      if (action === 'create') {
        sendLobbyCommand('create', { user: username });
      } else {
        sendLobbyCommand('join', { user: username });
      }
      requestLobbySync();
      saveLobbyAction('');
    };
    if (isWsReady()) {
      onOpen();
    } else {
      window.addEventListener('ws-open', onOpen, { once: true });
    }
    renderLobby();
  }

  // Kurz: Initialisiert die Spiel-Startseite.
  // Detail: Setzt Button-Zustand sofort aus sessionStorage, dann live per WS-Sync.
  function initPlayHub() {
    requireSession();
    // Kurz: Sofortiger Button-Zustand aus gecachten Lobby-Daten.
    updatePlayHubButtons(loadLobby().players.length > 0);
    // Kurz: Live-Sync vom Server um aktuellen Lobby-Status zu erhalten.
    // Detail: Auch andere Geraete sehen so den korrekten Button-Zustand.
    connectWS('spiel');
    var onOpen = function () {
      requestLobbySync();
    };
    if (isWsReady()) {
      onOpen();
    } else {
      window.addEventListener('ws-open', onOpen, { once: true });
    }
  }

  window.renderLobby = renderLobby;
  window.removeLobbyPlayer = removeLobbyPlayer;
  window.changeLobbyColor = changeLobbyColor;
  window.createLobbyAndOpen = createLobbyAndOpen;
  window.joinLobbyFromDashboard = joinLobbyFromDashboard;
  window.launchGame = launchGame;
  window.initPregame = initPregame;
  window.initPlayHub = initPlayHub;
})(window);
