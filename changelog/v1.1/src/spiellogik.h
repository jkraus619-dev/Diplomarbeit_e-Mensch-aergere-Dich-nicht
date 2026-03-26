#ifndef SPIELLOGIK_H
#define SPIELLOGIK_H

#include <Arduino.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <iostream>

// ------------------- Konstanten -------------------
#define RINGSIZE 40    // spielfeldgröße des rings
#define FARBFELDSIZE 4 // 0-3 Ziel  //Zielfeldgröße
#define MAXSPIELERANZAHL 4
#define SPIELERANZAHL 4 // Spieleranzahl
#define FIGURENANZAHL 4 // Figurenanzahl

extern bool spielRunning;   // ob das Spiel gerade läuft
extern int turnround;       // 0-nicht initialisiert,1-würfeln,2-figuren ziehen
extern int wurfrunde;       // für wenn man eine 6 würfelt
extern int currentFigur[2]; // 0-figurindex, 1-movablesindex

// ------------------- Structs -------------------
// struct für movables zum einfachen übergeben des bewegungsgrundes ohne neu zu berechnen
struct Move
{
    int figurIndex;
    int typ; // 0 = aus Reserve raus, 1 = ins Zielfeld, 2 = im Zielfeld, 3 = auf Ring bewegen
};

// ------------------- Globale Variablen -------------------
// als extern deklarieren, damit sie nur in einer cpp definiert werden
extern std::string farben[4];     // Farben
extern int spielring[RINGSIZE];   // Spielfeld: 0=leer, 1-4=Farbe
extern const int startPos[4];     // Startpositionen auf Ring für jede Farbe
extern int rotfeld[FARBFELDSIZE]; // Zielfelder: 0-3 Ziel
extern int blaufeld[FARBFELDSIZE];
extern int gruenfeld[FARBFELDSIZE];
extern int gelbfeld[FARBFELDSIZE];

// ------------------- Funktionen -------------------
int *getZielfeld(int farbe);
void spielinit();

// ------------------- Klasse Figur -------------------
class Figur
{
public:
    int posRing;       // Position auf Hauptring (-1 = in Reserve)
    int farbe;         // 0-3
    bool imZiel;       // schon im Zielfeld
    int zielfeldIndex; // -1 nicht im Ziel | 0-n im Ziel

    Figur(int f);

    // funktion für wenn jemand gecaptured wird und diese figur wieder dran kommt (verbessern)
    void getcaptured();

    // ----- Bewegungsfunktionen -----
    void moveToRing(int startPosRing);             // Figur aus Reserve auf Startfeld setzen
    void moveOnRing(int steps);                    // Figur auf Spielfeld bewegen
    void moveToZielfeld(int steps, int *zielfeld); // Figur auf Zielfeld vom Spielfeld bewegen
    void moveInZielfeld(int steps, int *zielfeld); // Figur im Zielfeld bewegen

    // zur Ausgabe der einzelnen Figur Werte (im Endprodukt nicht benötigt)
    void print() const;
};

// ------------------- Klasse Spieler -------------------
class Spieler
{
public:
    int farbe;                  // 0-3
    std::vector<Figur> figuren; // Figuren des Spielers
    int *zielfeld = nullptr;    // Pointer auf das Zielfeld der jeweiligen Farbe

    Spieler(int f);                                // Konstruktor
    void initZielfeld();                           // Zielfeld initialisieren
    bool allInZiel();                              // Prüfen ob alle Figuren im Ziel sind
    void print();                                  // Daten aller Figuren ausgeben
    int rollDice();                                // Würfeln
    std::vector<Move> getMovableFigures(int wurf); // Prüfe welche Figuren gezogen werden können
};

// ------------------- Klasse AlleSpieler -------------------
class AlleSpieler
{
public:
    std::vector<Spieler> spieler; // Liste aller Spieler
    int currentPlayer;            // aktueller Spieler

    AlleSpieler();                                                  // Konstruktor
    bool inMovables(int choice, const std::vector<Move> &movables); // Prüfen ob Figur gewählt werden kann
    void printSpielring();                                          // Spielfeldring ausgeben
    void printAll();                                                // alle Spieler mit deren Figuren ausgeben
    void wuerfeln();
    void nextTurn();               // Erster Teil (bis Figurwahl)
    void continueTurn(int choice); // Zweiter Teil (nach Spieler-Eingabe)
    void changeCurrentFigur();
    void changeCurrentFigur(int);
private:
    int lastRoll = 0; // Speichert letzten Wurf
    std::vector<Move> movables;
};

extern AlleSpieler alle;

#endif