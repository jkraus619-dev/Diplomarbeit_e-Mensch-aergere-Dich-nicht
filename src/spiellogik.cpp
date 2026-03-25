#include "spiellogik.h"
#include "wifi_ap.h"
#include "hardware.h"
#include "lcd_dice.h"

// ------------------- Globale Variablen -------------------
std::string farben[4] = {"Rot", "Blau", "Grün", "Gelb"}; // Farben
int spielring[RINGSIZE] = {0};                           // Spielfeldring
const int startPos[4] = {0, 10, 20, 30};                 // Startpositionen
int rotfeld[FARBFELDSIZE] = {0};                         // Zielfelder
int blaufeld[FARBFELDSIZE] = {0};
int gruenfeld[FARBFELDSIZE] = {0};
int gelbfeld[FARBFELDSIZE] = {0};

int turnround = -1; // -1-nicht initialisiert,1-würfeln,2-figuren ziehen
int wurfrunde = 0;
bool spielRunning = false;

int currentFigur[2] = {};

AlleSpieler alle;

// ------------------- Hilfsfunktion -------------------
int *getZielfeld(int farbe)
{
    switch (farbe)
    {
    case 0:
        return rotfeld;
    case 1:
        return blaufeld;
    case 2:
        return gruenfeld;
    case 3:
        return gelbfeld;
    default:
        return nullptr;
    }
}

// ------------------- Klasse Figur -------------------
Figur::Figur(int f) : posRing(-1), farbe(f), imZiel(false), zielfeldIndex(-1) {}

// funktion für wenn jemand gecaptured wird
void Figur::getcaptured()
{
    posRing = -1;
    zielfeldIndex = -1;
    imZiel = false;
}

// Figur aus Reserve auf Ring setzen
void Figur::moveToRing(int startPosRing)
{
    posRing = startPosRing;

    // Prüfen, ob dort eine gegnerische Figur steht
    if (spielring[posRing] != 0 && spielring[posRing] != (farbe + 1))
    {
        // Gegnerische Figur holen
        for (auto &sp : alle.spieler)
        {
            for (auto &f : sp.figuren)
            {
                if (f.posRing == posRing)
                {
                    f.getcaptured(); // sofort captured
                }
            }
        }
    }
    spielring[posRing] = farbe + 1;
}

// Figur auf Ring bewegen
void Figur::moveOnRing(int steps)
{
    spielring[posRing] = 0;
    posRing = (posRing + steps) % RINGSIZE;

    // Prüfen, ob dort eine gegnerische Figur steht
    if (spielring[posRing] != 0 && spielring[posRing] != (farbe + 1))
    {
        // Gegnerische Figur holen
        for (auto &sp : alle.spieler)
        {
            for (auto &f : sp.figuren)
            {
                if (f.posRing == posRing)
                {
                    f.getcaptured(); // sofort captured
                }
            }
        }
    }
    spielring[posRing] = farbe + 1;
}

// Figur ins Zielfeld vom Ring bewegen
void Figur::moveToZielfeld(int steps, int *zielfeld)
{
    zielfeldIndex = steps;
    spielring[posRing] = 0;
    posRing = -1;
    zielfeld[zielfeldIndex] = farbe + 1;
    imZiel = true;
}

// Figur im Zielfeld bewegen
void Figur::moveInZielfeld(int steps, int *zielfeld)
{
    zielfeld[zielfeldIndex] = 0;
    zielfeldIndex += steps;
    zielfeld[zielfeldIndex] = farbe + 1;
}

// zur Ausgabe der Figur
void Figur::print() const
{
    if (posRing >= 0)
        Serial.printf("Ring: %d\n", posRing + 1);
    else if (imZiel)
        Serial.printf("im Ziel %d", zielfeldIndex + 1);
    else
        Serial.printf("In Reserve");
}

// ------------------- Klasse Spieler -------------------
Spieler::Spieler(int f) : farbe(f)
{
    for (int i = 0; i < FIGURENANZAHL; i++)
        figuren.push_back(Figur(farbe));
}

// Zielfeld initialisieren
void Spieler::initZielfeld()
{
    zielfeld = getZielfeld(farbe);
}

// Prüfen ob alle Figuren im Ziel sind
bool Spieler::allInZiel()
{
    for (auto &f : figuren)
        if (!f.imZiel)
            return false;
    return true;
}

// Spieler ausgeben
void Spieler::print()
{
    Serial.printf("Spieler %s:\n", farben[farbe].c_str());
    for (int i = 0; i < FIGURENANZAHL; i++)
    {
        Serial.printf("  Figur %d: ", i + 1);
        figuren[i].print();
        Serial.printf("\n");
    }
}

// Würfeln
int Spieler::rollDice()
{
    return rand() % 6 + 1;
}

// Prüfen welche Figuren gezogen werden können
std::vector<Move> Spieler::getMovableFigures(int wurf)
{
    std::vector<Move> moves;
    for (int i = 0; i < FIGURENANZAHL; i++)
    {
        Figur &f = figuren[i];

        if ((f.posRing != -1) && (spielring[f.posRing] != (f.farbe + 1)))
        {
            f.getcaptured();
        }

        int zielStart = startPos[f.farbe];
        int distZumZiel = (zielStart - f.posRing + RINGSIZE) % RINGSIZE;
        int zielzugpos = f.imZiel ? wurf + f.zielfeldIndex : wurf + f.zielfeldIndex - distZumZiel + 1;

        bool ringfarbe = (spielring[(wurf + f.posRing) % RINGSIZE] != f.farbe + 1);

        // 1) Figur noch in Reserve
        if (f.posRing == -1 && !f.imZiel && spielring[zielStart] != f.farbe + 1 && wurf == 6)
            moves.push_back({i, 0});
        // 2) Figur auf Ring -> kann ins Zielfeld ziehen
        else if (!f.imZiel && wurf >= distZumZiel && zielzugpos < FARBFELDSIZE && zielfeld[zielzugpos] == 0 && f.posRing != zielStart && f.posRing != -1)
            moves.push_back({i, 1});
        // 3) Figur schon im Zielfeld -> weiterziehen
        else if (f.imZiel && zielzugpos < FARBFELDSIZE && zielfeld[zielzugpos] == 0)
            moves.push_back({i, 2});
        // 4) Figur noch auf Ring -> normal weiterziehen
        else if ((f.posRing >= 0 && wurf < distZumZiel || f.posRing == zielStart) && ringfarbe)
            moves.push_back({i, 3});
    }
    return moves;
}

// ------------------- Klasse AlleSpieler -------------------
AlleSpieler::AlleSpieler()
{
    static int currentPlayer = 0;
    for (int i = 0; i < SPIELERANZAHL; i++)
    {
        spieler.push_back(Spieler(i));
        spieler[i].initZielfeld();
    }
}

// prüfen ob die Auswahl möglich ist
bool AlleSpieler::inMovables(int choice, const std::vector<Move> &movables)
{
    for (const Move &m : movables)
        if (m.figurIndex == choice)
            return false;
    return true;
}

// Ohne Index: eine Position weiterschalten
void AlleSpieler::changeCurrentFigur()
{
    if (movables.empty())
    {
        alle.printAll();
        return;
    }
    if (movables.size() <= 1)
    {
        return;
    }

    currentFigur[1] = (currentFigur[1] + 1) % (int)movables.size();
    currentFigur[0] = movables[currentFigur[1]].figurIndex;

    highlightFigur(currentFigur[0]);
    return;
}

// Mit Index: direkt auf bestimmte Figur springen
void AlleSpieler::changeCurrentFigur(int index)
{
    if (movables.empty())
    {
        alle.printAll();
        return;
    }

    // Prüfen ob index im gültigen Bereich liegt
    if (index < 0 || index >= (int)movables.size())
    {
        return;
    }

    currentFigur[1] = index;
    currentFigur[0] = movables[index].figurIndex;

    highlightFigur(currentFigur[0]);
}

// Spielfeldring ausgeben
void AlleSpieler::printSpielring()
{

    Serial.printf("Spielfeldring: ");
    for (int i = 0; i < RINGSIZE; i++)
        Serial.printf("%d ", spielring[i]);
    Serial.printf("\n");
}

// alle Spieler ausgeben
void AlleSpieler::printAll()
{
    printSpielring();
    for (auto &sp : spieler)
    {
        sp.print();
    }
    int start[MAXSPIELERANZAHL] = {0};
    int ziel[MAXSPIELERANZAHL][FARBFELDSIZE] = {0};

    int j = 0;

    for (auto &sp : spieler)
    {
        for (auto &fig : sp.figuren)
        {
            if (fig.posRing == -1 && fig.imZiel == false)
            {
                start[j]++;
            }

            if (fig.imZiel == true)
            {
                ziel[j][fig.zielfeldIndex] = fig.farbe + 1;
            }
        }
        j++;
    }
    sendLEDdata(spielring, start, ziel);
}

void AlleSpieler::wuerfeln()
{
    Spieler &sp = spieler[currentPlayer];

    lastRoll = sp.rollDice();
    lcdDiceRoll(lastRoll, currentPlayer);
    Serial.printf("Würfel zeigt: %d\n", lastRoll);
    Datenschicken("dice_result", lastRoll);
    nextTurn();
}

void AlleSpieler::nextTurn()
{
    Datenschicken("Spieler", currentPlayer);

    Spieler &sp = spieler[currentPlayer];
    Serial.printf("Spieler %s ist dran.\n", farben[sp.farbe].c_str());

    movables = {};

    movables = sp.getMovableFigures(lastRoll);
    changeCurrentFigur(0);

    if (movables.empty())
    {
        Serial.printf("Keine Figur kann gezogen werden.\n");
        Datenschicken("figuren", sp.farbe, 0, 0);

        currentPlayer = (currentPlayer + 1) % SPIELERANZAHL;
        // lcdDiceShowCurrentPlayer(currentPlayer);
        playerButtonLock(currentPlayer);
        printf("%d", currentPlayer);
        return;
    }
    else
    {
        turnround = 2;

        Serial.printf("Verfügbare Figuren: ");
        int figmove[FIGURENANZAHL] = {};
        int i = 0;
        for (const Move &m : movables)
        {
            Serial.printf("%d ", m.figurIndex + 1);
            figmove[i] = m.figurIndex + 1;
            i++;
        }
        Serial.printf("\nWarte auf Eingabe über Web...\n\n");
        Datenschicken("figuren", sp.farbe, figmove, i);

        // Jetzt wird auf WebSocket-Eingabe gewartet.
        // => continueTurn(int choice) wird dann extern aufgerufen.
        return;
    }
}

void AlleSpieler::continueTurn(int choice)
{
    Spieler &sp = spieler[currentPlayer];
    choice--; // Anpassung auf Indexbasis 0

    // Den Move finden
    Move chosen;
    bool found = false;
    for (const Move &m : movables)
    {
        if (m.figurIndex == choice)
        {
            chosen = m;
            found = true;
            break;
        }
    }

    if (!found)
    {
        Serial.printf("Fehler: Figur %d nicht in movables!\n", choice + 1);
        return;
    }

    Figur &f = sp.figuren[choice];
    int zielStart = startPos[f.farbe];
    int distZumZiel = (zielStart - f.posRing + RINGSIZE) % RINGSIZE;

    if (chosen.typ == 0)
        f.moveToRing(startPos[sp.farbe]);
    else if (chosen.typ == 1)
        f.moveToZielfeld(lastRoll - distZumZiel, sp.zielfeld);
    else if (chosen.typ == 2)
        f.moveInZielfeld(lastRoll, sp.zielfeld);
    else if (chosen.typ == 3)
        f.moveOnRing(lastRoll);

    if (sp.allInZiel())
    {
        Serial.printf("\nSpieler %s hat gewonnen!\n", farben[sp.farbe].c_str());
        Datenschicken("sieg", sp.farbe);
        gameStarted = false;
        turnround = -1;
        spielRunning = false;
        rolled = false;
        playerButtonLock(0);
        return;
    }

    if (lastRoll == 6 && wurfrunde == 0)
    {
        wurfrunde += 1;
        turnround = 1;
        printAll();
        return;
    }

    // Nächster Spieler
    wurfrunde = 0;
    currentPlayer = (currentPlayer + 1) % SPIELERANZAHL;
    lcdDiceShowCurrentPlayer(currentPlayer);
    Datenschicken("spieler", currentPlayer);
    playerButtonLock(currentPlayer);
    printf("%d", currentPlayer);
    printAll();

    turnround = 1;
}

// ------------------- Hauptprogramm -------------------
void spielinit()
{
    srand(time(NULL));

    alle.printAll();
    turnround = 1;
    Serial.printf("Enter für nächsten Spieler...\n");
    lcdDiceShowCurrentPlayer(alle.currentPlayer);
    Datenschicken("Spieler", alle.currentPlayer);
    playerButtonLock(alle.currentPlayer);
    printf("%d", alle.currentPlayer);
    return;
}
