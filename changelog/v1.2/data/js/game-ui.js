// Kurz: Kapselt die Game-UI Logik.
// Detail: IIFE, damit Spiel-UI Funktionen sauber getrennt bleiben.
(function (window) {
  'use strict';
 
  var currentPlayerColor = null;

  // Kurz: Mappt Server-Farb-Index auf den Lobby-Farbwert.
  // Detail: Server: 0=Rot, 1=Blau, 2=Gruen, 3=Gelb → Lobby: 'red','blue','green','yellow'
  var farbenMap = ['red', 'blue', 'green', 'yellow'];

// Kurz: Hebt den aktuellen Spieler in der Liste hervor.
  // Detail: Vergleicht item.dataset.color direkt mit currentPlayerColor.
  function applyCurrentPlayerHighlight() {
    var list = $('gamePlayers');
    if (!list) {
      return;
    }
    var items = list.querySelectorAll('.game-player');

    items.forEach(function (item) {
      var isCurrent = !!(currentPlayerColor && item.dataset.color === currentPlayerColor);
      item.classList.toggle('is-current', isCurrent);
      var status = item.querySelector('.game-player-status');
      if (status) {
        status.textContent = isCurrent ? 'Am Zug' : '';
      }
    });
  }
 
  // Kurz: Rendert die Spieler fuer die Spielseite.
  // Detail: Baut die Liste aus der Lobby.
  function renderGamePlayers() {
    var lobby = loadLobby();
    var list = $('gamePlayers');
 
    if (list) {
      list.textContent = '';
      if (!lobby.players.length) {
        var empty = document.createElement('p');
        empty.className = 'empty-state';
        empty.textContent = 'Keine Spielerliste gefunden.';
        list.appendChild(empty);
      } else {
        // Kurz: Baut die Spieler-Liste.
        // Detail: Zeigt Name und Farbpill pro Spieler, sortiert nach Farbreihenfolge.
        var sorted = lobby.players.slice().sort(function (a, b) {
          return farbenMap.indexOf(a.color) - farbenMap.indexOf(b.color);
        });
        sorted.forEach(function (player) {
          var item = document.createElement('li');
          item.className = 'game-player';
          item.dataset.color = player.color || '';
 
          var left = document.createElement('div');
          left.className = 'game-player-left';
 
          var name = document.createElement('span');
          name.className = 'game-player-name';
          name.textContent = player.name;
 
          var status = document.createElement('span');
          status.className = 'game-player-status';
 
          var color = document.createElement('span');
          color.className = 'color-pill color-' + (player.color || 'none');
          color.textContent = player.color ? colorLabel(player.color) : 'Keine Farbe';
 
          left.appendChild(name);
          left.appendChild(status);
          item.appendChild(left);
          item.appendChild(color);
          list.appendChild(item);
        });
      }
    }
 
    applyCurrentPlayerHighlight();
  }
 
  // Kurz: Liest die Auswahl fuer eine Spielfigur.
  // Detail: Validiert die Figuren-Nummer und gibt ein Objekt zurueck.
  function getPieceSelection() {
    var feedback = $('gameFeedback');
    var figure = Number(getValue('pieceNumber'));
 
    if (!Number.isFinite(figure) || figure < 1 || figure > 4) {
      setFeedback(feedback, 'Figur 1 bis 4 ausw\u00e4hlen.', '#b32121');
      return null;
    }
 
    return { figure: figure, feedback: feedback };
  }
 
  // Kurz: Sendet einen Wuerfelwurf an den Server.
  // Detail: Schickt "roll" ueber WS und gibt Feedback aus.
  function rollDiceButton() {
    var feedback = $('gameFeedback');
    if (sendWS('roll')) {
      setFeedback(feedback, 'Wurf gesendet.', '#186e0e');
    }
  }
 
  // Kurz: Sendet die im Dropdown gewaehlte Figur an den Server.
  // Detail: Wird durch das change-Event des Dropdowns ausgeloest.
  function selectedPiece() {
    var figure = Number(getValue('pieceNumber'));
    if (!figure || figure < 1) {
      return;
    }
    var cmd = JSON.stringify({ cmd: 'selected', data: figure });
    sendWS(cmd);
  }

  // Kurz: Bestaetigt den Zug.
  // Detail: Baut den confirm-Befehl und sendet ihn ueber WS.
  function confirmSelection() {
    var selection = getPieceSelection();
    if (!selection) {
      return;
    }
    var cmd = JSON.stringify({ cmd: 'auswahl', data: selection.figure }); // Figurenauswahl als Number senden
    if (sendWS(cmd)) {
      setFeedback(selection.feedback, 'Zug best\u00e4tigt.', '#186e0e');
    }
  }
 
  // Kurz: Verarbeitet den aktuellen Spieler aus dem WS-Stream.
  // Detail: Nutzt den payload.type "spieler" und aktualisiert die Hervorhebung.
  function handleCurrentPlayerPayload(message) {
    if (!message) {
      return;
    }
    var type = String(message.type || '').toLowerCase();
    if (type !== 'spieler' && type !== 'player' && type !== 'current_player' && type !== 'currentplayer') {
      return;
    }
    var value = typeof message.value !== 'undefined' ? message.value : message.intdata;
    var parsed = Number(value);
    if (!Number.isFinite(parsed)) {
      return;
    }
    currentPlayerColor = farbenMap[Math.trunc(parsed)] || '';
    applyCurrentPlayerHighlight();
  }
 
  // Kurz: Verarbeitet Wuerfel-Ergebnisse vom Server.
  // Detail: Setzt die Wuerfelanzeige basierend auf dem Payload.
  function handleDicePayload(message) {
    if (!message || message.type !== 'dice_result') {
      return;
    }
    setText('diceResult', typeof message.value !== 'undefined' ? message.value : '-');
  }
 
  // Kurz: Verarbeitet verfuegbare Figuren vom Server.
  // Detail: Aktualisiert Dropdown, Hervorhebung und Feedback.
  function handleFigurenPayload(message) {
    if (!message || message.type !== 'figuren') {
      return;
    }
    currentPlayerColor = farbenMap[message.farbe] || '';
    applyCurrentPlayerHighlight();
    var figures = Array.isArray(message.intdata)
      ? message.intdata.filter(function (n) { return n > 0; })
      : [];
    // Kurz: Dropdown mit tatsaechlich verfuegbaren Figuren befuellen.
    var select = $('pieceNumber');
    if (select) {
      select.textContent = '';
      if (figures.length === 0) {
        var placeholder = document.createElement('option');
        placeholder.value = '0';
        placeholder.disabled = true;
        placeholder.selected = true;
        placeholder.textContent = 'Kein Zug m\u00f6glich';
        select.appendChild(placeholder);
      } else {
        figures.forEach(function (n) {
          var opt = document.createElement('option');
          opt.value = String(n);
          opt.textContent = 'Figur ' + n;
          select.appendChild(opt);
        });
      }
    }
    var feedback = $('gameFeedback');
    if (figures.length === 0) {
      setFeedback(feedback, 'Kein Zug m\u00f6glich \u2013 n\u00e4chster Spieler.', '#b32121');
    } else {
      setFeedback(feedback, 'Verf\u00fcgbare Figuren: ' + figures.join(', '), '#186e0e');
    }
  }

  registerWsHandler(handleDicePayload);
  registerWsHandler(handleCurrentPlayerPayload);
  registerWsHandler(handleFigurenPayload);

  // Kurz: Initialisiert die Spielseite.
  // Detail: Prueft Session, befuellt Dropdown, rendert Spieler und lauscht auf Lobby-Updates.
  function initGame() {
    var username = requireSession();
    if (!username) {
      return;
    }
    // Kurz: Figurenauswahl dynamisch aus Lobby befuellen.
    // Detail: Platzhalter + N Eintraege fuer N Spieler.
    var select = $('pieceNumber');
    if (select) {
      select.textContent = '';
      var placeholder = document.createElement('option');
      placeholder.value = '0';
      placeholder.disabled = true;
      placeholder.selected = true;
      placeholder.textContent = 'Figur w\u00e4hlen';
      select.appendChild(placeholder);
      var lobby = loadLobby();
      var count = (lobby.players && lobby.players.length) ? lobby.players.length : 0;
      for (var i = 1; i <= count; i++) {
        var opt = document.createElement('option');
        opt.value = String(i);
        opt.textContent = 'Figur ' + i;
        select.appendChild(opt);
      }
    }
    renderGamePlayers();
    // Kurz: Aktualisiert die Spielerliste bei Lobby-Aenderung.
    // Detail: Reagiert auf Storage-Updates der Lobby.
    window.addEventListener('storage', function (event) {
      if (event.key === LOBBY_KEY) {
        renderGamePlayers();
      }
    });
    connectWS('game');
  }
 
  window.renderGamePlayers = renderGamePlayers;
  window.getPieceSelection = getPieceSelection;
  window.rollDiceButton = rollDiceButton;
  window.selectedPiece = selectedPiece;
  window.confirmSelection = confirmSelection;
  window.handleDicePayload = handleDicePayload;
  window.handleCurrentPlayerPayload = handleCurrentPlayerPayload;
  window.handleFigurenPayload = handleFigurenPayload;
  window.initGame = initGame;
})(window);
 
 