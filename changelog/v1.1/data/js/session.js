// Kurz: Kapselt das Session-Modul.
// Detail: IIFE fuer klare Abgrenzung der Session-Helper.
(function (window) {
  'use strict';

  // Kurz: Erzwingt eine aktive Session.
  // Detail: Wenn kein Nutzer angemeldet ist, wird zur Startseite umgeleitet.
  function requireSession() {
    var username = getCurrentUser();
    if (!username) {
      location.href = 'index.html';
      return null;
    }
    return username;
  }

  // Kurz: Loescht alle Sessiondaten.
  // Detail: Entfernt Nutzer und Lobby aus sessionStorage sowie das Start-Flag aus
  //         localStorage. Das Start-Flag liegt bewusst in localStorage (nicht session),
  //         damit das storage-Event auch in anderen Tabs desselben Browsers ausgeloest wird.
  function clearAllSession() {
    clearCurrentUser();
    clearLobby();
    localStorage.removeItem(GAME_START_KEY);
  }

  window.requireSession = requireSession;
  window.clearAllSession = clearAllSession;
})(window);
