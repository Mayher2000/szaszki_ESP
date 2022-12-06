#include <Arduino.h>
#include "Globals.h"
#include <vector>

#define WHITE 0
#define BLACK 1

short chessFieldState[8][8] = {{4, 0, 0, 0, 5, 0, 0, 1},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 12, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 11, 0, 0, 0}};

int moved[3][2] = {{0, 0}, {0, 0}, {0, 0}};
bool engineColor = BLACK; // BLACK silnik zagra czarnymi, WHITE silnik zagra białymi
String move = "";
bool stringComplete = true;
int asciiToMove[8] = {7, 6, 5, 4, 3, 2, 1, 0}; 

void getserialchar();
void makeEngineMove();

void setup(){
    Serial.begin(500000);
    setupEngine();
    cfsToBoard(chessFieldState, moved, engineColor);
    serialBoard();
}

void loop() {
    while (stringComplete == false) {
        getserialchar();
    }
    move = "";
    stringComplete = false;
}

void getserialchar() {
  while (Serial.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    move += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      makeEngineMove();
    }
  }
}

void makeEngineMove(){
  int xStart, yStart, xEnd, yEnd;
  char c[4];

  xStart = asciiToMove[int(move.charAt(1) - 49)];
  yStart = int(move.charAt(0) - 97);
  xEnd = asciiToMove[int(move.charAt(3) - 49)];
  yEnd = int(move.charAt(2) - 97);

  chessFieldState[xEnd][yEnd] = chessFieldState[xStart][yStart];
  chessFieldState[xStart][yStart] = 0;

  cfsToBoard(chessFieldState, moved, engineColor);
  
  // strcpy(chessEngineReturn, chessEngine());
  std::vector<std::vector<char>> chessEngineReturn = chessEngine();

  // boardTocfs(chessEngineReturn[1]);

  // translacja ruchu na współrzędne
  xStart = asciiToMove[int(chessEngineReturn[0][1] - 49)];
  yStart = int(chessEngineReturn[0][0] - 97);
  xEnd = asciiToMove[int(chessEngineReturn[0][3] - 49)];
  yEnd = int(chessEngineReturn[0][2] - 97);

  // wyświetlenie szachownicy
  serialBoard();
            
  chessFieldState[xEnd][yEnd] = chessFieldState[xStart][yStart];
  chessFieldState[xStart][yStart] = 0;
}