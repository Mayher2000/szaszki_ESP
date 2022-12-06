#include <Arduino.h>
#include <FastLED.h>
#include <SPI.h>

// konfiguracja paska LEDowego
#define LED_PIN         23
#define NUM_LEDS        384
#define BRIGHTNESS      10 // 255
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

// piny kontrolne MUXa
#define S0              26
#define S1              25
#define S2              33
#define S3              32

// piny sygnałowe MUXa
#define SIG_PIN1        35
#define SIG_PIN2        34
#define SIG_PIN3        39
#define SIG_PIN4        36

// tablica ledów
CRGB leds[NUM_LEDS];

// tablica do odczytywania sygnałów z multiplekserów, uporządkowana jak pola szachownicy tj. A1 - (mux1, ch0), B1 - (mux1, ch1)... A2 - (mux1, ch8) ... A3 - (mux2, ch0) ...
const short muxMapArray[8][8][2] = {{{SIG_PIN1, 7},  {SIG_PIN1, 6},   {SIG_PIN1, 5},  {SIG_PIN1, 4},  {SIG_PIN1, 3},  {SIG_PIN1, 2},  {SIG_PIN1, 1},  {SIG_PIN1, 0}},
                                    {{SIG_PIN1, 15}, {SIG_PIN1, 14},  {SIG_PIN1, 13}, {SIG_PIN1, 12}, {SIG_PIN1, 11}, {SIG_PIN1, 10}, {SIG_PIN1, 9},  {SIG_PIN1, 8}},
                                    {{SIG_PIN2, 7},  {SIG_PIN2, 6},   {SIG_PIN2, 5},  {SIG_PIN2, 4},  {SIG_PIN2, 3},  {SIG_PIN2, 2},  {SIG_PIN2, 1},  {SIG_PIN2, 0}},
                                    {{SIG_PIN2, 15}, {SIG_PIN2, 14},  {SIG_PIN2, 13}, {SIG_PIN2, 12}, {SIG_PIN2, 11}, {SIG_PIN2, 10}, {SIG_PIN2, 9},  {SIG_PIN2, 8}},
                                    {{SIG_PIN3, 7},  {SIG_PIN3, 6},   {SIG_PIN3, 5},  {SIG_PIN3, 4},  {SIG_PIN3, 3},  {SIG_PIN3, 2},  {SIG_PIN3, 1},  {SIG_PIN3, 0}},
                                    {{SIG_PIN3, 15}, {SIG_PIN3, 14},  {SIG_PIN3, 13}, {SIG_PIN3, 12}, {SIG_PIN3, 11}, {SIG_PIN3, 10}, {SIG_PIN3, 9},  {SIG_PIN3, 8}},
                                    {{SIG_PIN4, 7},  {SIG_PIN4, 6},   {SIG_PIN4, 5},  {SIG_PIN4, 4},  {SIG_PIN4, 3},  {SIG_PIN4, 2},  {SIG_PIN4, 1},  {SIG_PIN4, 0}},
                                    {{SIG_PIN4, 15}, {SIG_PIN4, 14},  {SIG_PIN4, 13}, {SIG_PIN4, 12}, {SIG_PIN4, 11}, {SIG_PIN4, 10}, {SIG_PIN4, 9},  {SIG_PIN4, 8}}};

// tablica do odczytywania numeru ledów, uporządkowana tak jak biegnął ledy pod szachownicą
short ledMapArray[8][8][6];

// tablica pozycji bierek (domyślnie uzupełniona)
// pierwsza współrzędna to wiersz, druga to kolumna
// BR1, BN1, BB1, BQ, BK, BB2, BN2, BR2
// BP1, BP2, BP3, BP4, BP5, BP6, BP7, BP8
// WP1, WP2, WP3, WP4, WP5, WP6, WP7, WP8,
// WR1, WN1, WB1, WQ, WK, WB2, WN2, WR2, 
// figura poza szachownicą posiada kordy (8,8)
// figura podniesiona posiada kordy (9,9)
// short piecesPosition[32][2] = {{0,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{0,7},
//                                {1,0},{1,1},{1,2},{1,3},{1,4},{1,5},{1,6},{1,7},
//                                {6,0},{6,1},{6,2},{6,3},{6,4},{6,5},{6,6},{6,7},
//                                {7,0},{7,1},{7,2},{7,3},{7,4},{7,5},{7,6},{7,7}};
// A8 - 0.0 B8 - 0.1 C8 - 0.2 D8 - 3.3 E8 - 0.4  F8 - 0.5 G8 - 0.6 H8 - 0.7  
// A7 - 1.0 B7 - 1.1 C7 - 1.2 D7 - 3.3 E7 - 1.4  F7 - 1.5 G7 - 1.6 H7 - 1.7  
// A6 - 2.0 B6 - 2.1 C6 - 2.2 D6 - 3.3 E6 - 2.4  F6 - 2.5 G6 - 2.6 H6 - 2.7  
// A5 - 3.0 B5 - 3.1 C5 - 3.2 D5 - 3.3 E5 - 3.4  F5 - 3.5 G5 - 3.6 H5 - 3.7  
// A4 - 4.0 B4 - 4.1 C4 - 4.2 D4 - 4.3 E4 - 4.4  F4 - 4.5 G4 - 4.6 H4 - 4.7  
// A3 - 5.0 B3 - 5.1 C3 - 5.2 D3 - 5.3 E3 - 5.4  F3 - 5.5 G3 - 5.6 H3 - 5.7 
// A2 - 6.0 B2 - 6.1 C2 - 6.2 D2 - 6.3 E2 - 6.4  F2 - 6.5 G2 - 6.6 H2 - 6.7  
// A1 - 7.0 B1 - 7.1 C1 - 7.2 D1 - 7.3 E1 - 7.4  F1 - 7.5 G1 - 7.6 H1 - 7.7 
short piecesPosition[32][2] =  {{0,0},{8,8},{8,8},{8,8},{0,4},{8,8},{8,8},{0,7},
                                {8,8},{1,1},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},
                                {8,8},{8,8},{6,2},{8,8},{8,8},{8,8},{8,8},{8,8},
                                {7,0},{8,8},{8,8},{8,8},{7,4},{8,8},{8,8},{7,7}};

// tablica określająca czy pionek otrzymał promocję oraz na jaką figurę
short pawnsPromotions[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char *piecesNames[33] = {"BR","BN","BB","BQ","BK","BB","BN","BR",
                               "BP","BP","BP","BP","BP","BP","BP","BP",
                               "WP","WP","WP","WP","WP","WP","WP","WP",
                               "WR","WN","WB","WQ","WK","WB","WN","WR",
                               "E"};

short chessFieldState[8][8] = {{0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0}};

int prevPosFlag = 0;
bool turn = 0; // 0 ruch białych, 1 ruch czarnych
short raisedPieceName = 33;
bool notAllowedMoveFlag = true;

// tablice ruchów dla bierek
int knightMoves[8][2] = {{-1, -2}, {1, -2}, {-1, 2}, {1, 2}, {2, -1}, {-2, -1}, {2, 1}, {-2, 1}};
int pawnMoves[8][2] = {{-1, 1}, {0, 0}, {-2, 2}, {0, 0}, {-1, 1}, {1, 1}, {-1, 1}, {-1, -1}};  
int kingMoves[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
int rookMoves[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; 
int bishopMoves[4][2] = {{1 ,1}, {-1, -1}, {1, -1}, {-1, 1}};

// tablica przechowująca flagi sprawdzające pierwszy ruch króli oraz wież po lewej i wież po prawej
int moved[3][2] = {{0, 0}, {0, 0}, {0, 0}};

int pawnMoved[2] = {0, 0};

// deklaracje funkcji
void lightChessBoard();
void printChessBoard();
int readMux(int SIG_pin, int channel);
void allowedMoves(int pice, int x, int y);
void setLed(int x, int y, CRGB c);
void fillChessFieldState();
void mapLeds();

int rook(int x, int y, int x1, int y1);
int bishop(int x, int y, int x1, int y1);
int queen(int x, int y, int x1, int y1);
int horse(int x, int y, int x1, int y1);
int pawn(int x, int y, int x1, int y1);

void knightCheck(int knight, int x, int y); 
void pawnCheck(int pawn, int x, int y);
void queenCheck(int queen, int x, int y, int moves[][2]);
void kingCheck(int king, int x, int y);

bool checkIfCheck(int def = 1, int x = 9, int y = 9);
void pawnPromotion(int pawn, int x, int y);
void castle(int x, int y, int y1, int rook);

void setup(){
  // inicjalizacja portu szeregowego (wysoki baudrate, żeby "konsolowa animacja" była bardziej dynamiczna)
  Serial.begin(500000);
  // inicjalizacja paska ledów
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // inicjalizacja wyjść multiplekserów i ustawienie ich stanów
  pinMode(S0, OUTPUT); 
  pinMode(S1, OUTPUT); 
  pinMode(S2, OUTPUT); 
  pinMode(S3, OUTPUT); 

  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);

  printChessBoard();
  fillChessFieldState();
  // funkcja mapująca ledy
  mapLeds();
  turn ? Serial.println("Ruch czarnych") : Serial.println("Ruch białych");
}


void loop(){
  // zawsze na start w loop() będzie rysowana szachownica w konsoli portu szeregowego
  lightChessBoard();
  // przejście po szachownicy
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      
      // odczytujemy czy pole szachowe jest puste
      if (readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) < 2500 && readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) > 1500){
        // wiemy, że pole jest puste, sprawdzamy czy coś na tym polu stało wcześniej (w tablicy pozycji pionków)
        for (int k = 0; k < 32; k++){
          // jeśli na polu coś wcześniej stało a teraz tego nie ma to:
          // kordy podniesionego pionka i - kord x, j - kord y
          if (piecesPosition[k][0] == i && piecesPosition[k][1] == j){
            if ((!turn && k + 1 <= 16) || (turn && k + 1 > 16)){
              setLed(i, j, CRGB(255,0,0));
              FastLED.show();
              while (readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) < 2500 && readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) > 1500){}
              break;
            }
            chessFieldState[i][j] = 0;
            // zapisujemy jaki to był pionek (1 - czarna wieża itd.)
            raisedPieceName = k + 1;
            // zmieniamy kordy pionka na podniesione (9,9)
            piecesPosition[k][0] = 9;
            piecesPosition[k][1] = 9;
            allowedMoves(raisedPieceName, i, j);
            printChessBoard();
            // zaczynamy szukać pola, na które pionek został odłożnoy
            while (piecesPosition[raisedPieceName - 1][0] == 9){
              // przejście po wszystkich polach szachownicy
              for (int l = 0; l < 8; l++){
                for (int m = 0; m < 8; m++){
                  // opuszczenie flagi służącej do sprawdzenia czy pionek wcześniej stał już na danym polu
                  prevPosFlag = 0;
                  // odczytanie czy na polu szachowym znajduje się pionek
                  int hall = readMux(muxMapArray[l][m][0], muxMapArray[l][m][1]);
                  //########################################################################################################################################################################################################################################################################################################################################
                  // tura czarnych i czarny bierek lub tura białych i biały bierek, oznacza, że został on odłożony w inne miejsce
                  if (hall > 2500 || hall < 1500){
                    // sprawdzenie czy pionek wcześniej znajdował się na danym polu
                    for (int n = 0; n < 32; n++){
                      // jeśli tak, to podnosimy pepeFlage
                      if (prevPosFlag == false && piecesPosition[n][0] == l && piecesPosition[n][1] == m){
                        prevPosFlag = 1;
                        if ((n + 1 > 16 && hall < 1500) || (n + 1 <= 16 && hall > 2500)){
                          piecesPosition[raisedPieceName - 1][0] = l;
                          piecesPosition[raisedPieceName - 1][1] = m;
                          piecesPosition[n][0] = 8;
                          piecesPosition[n][1] = 8;
                          chessFieldState[l][m] = raisedPieceName; //tej lini chyba nie trzeba bo w nastepnym ifie i tak sie to wykona
                          turn ? turn = 0 : turn = 1;
                          printChessBoard();
                          turn ? Serial.println("Ruch czarnych") : Serial.println("Ruch białych");
                        }
                        break;
                      }
                    }
                    // jeśli flaga jest opuszczona, oznacza to że sprawdzane pole wcześniej było puste
                    if (!prevPosFlag){
                      // zmiana kordów opuszczonej bierki (umożliwia wyjście z pętli while)
                      piecesPosition[raisedPieceName - 1][0] = l;
                      piecesPosition[raisedPieceName - 1][1] = m;
                      chessFieldState[l][m] = raisedPieceName;
                      
                      // promocja pionka
                      if (raisedPieceName > 8 && raisedPieceName < 25)
                      	pawnPromotion(raisedPieceName, l , m);
                      
                      // flaga wykonania ruchu o 2 pola przez pionka
                      if (raisedPieceName > 8 && raisedPieceName < 25){
                        if (i == 1 && l == 3) // czarne
                          pawnMoved[1] = raisedPieceName;
                        else if (i == 6 && l == 4) // białe
                          pawnMoved[0] = raisedPieceName;
                        else if (i == 1 && l == 2)
                          pawnMoved[1] = 0;
                        else
                          pawnMoved[0] = 0;
                      }
                      else if (raisedPieceName <= 8) pawnMoved[1] = 0;
                      else pawnMoved[0] = 0;
                      
                      Serial.print(pawnMoved[0]);
                      Serial.print("		");
                      Serial.println(pawnMoved[1]);
                      
                      // roszada, wymuszenie ruchu wieżą
                      if (raisedPieceName == 5){
                        lightChessBoard();
                        if (j == 4 && i == 0){
                          if (m == 2){
                            castle(0, 0, 3, 1);
                          }
                          else if (m == 6){
                            castle(0, 7, 5, 8);
                          }
                        }
                      }
                      else if (raisedPieceName == 29){
                        lightChessBoard();
                        if (j == 4 && i == 7){
                          if (m == 2){
                            castle(7, 0, 3, 25);
                          }
                          else if (m == 6){
                            castle(7, 7, 5, 32);
                          }
                        }
                      }
                      //ustawianie flagi, gdy ruszona zostanie wieża lub król w celu późniejszego sprawdzenia możliwości wykonania roszady
                      //warunek if sprawdza czy nie odłożyliśmy bierki w to samo miejsce
                      if(i != l || j != m){
                        switch (raisedPieceName){
                          case 5:   moved[0][1] = 1; break;
                          case 29:  moved[0][0] = 1; break;
                          case 1:   moved[1][1] = 1; break;
                          case 25:  moved[1][0] = 1; break;
                          case 8:   moved[2][1] = 1; break;
                          case 32:  moved[2][0] = 1; break;
                      	}
                        turn ? turn = 0 : turn = 1;
                      }
                      // else if (prevPosFlag == 2){
                      // 	piecesPosition[raisedPieceName - 1][0] = l;
                      //   piecesPosition[raisedPieceName - 1][1] = m;
                      //   piecesPosition[n][0] = 8;
                      //   piecesPosition[n][1] = 8;
                      //   chessFieldState[l][m] = raisedPieceName;
                      //   turn ? turn = 0 : turn = 1;
                      // }

                      printChessBoard();
                      // po odłożeniu bierki następuje zmiana kolejki zawodnika
                      turn ? Serial.println("Ruch czarnych") : Serial.println("Ruch białych");
                    }
                  }
                }
              }
              //printChessBoard();
              // Serial.println(readMux(muxMapArray[0][0][0], muxMapArray[0][0][1]));
              //delay(1000);
            }
          }
        }
      }
    }
  }
  FastLED.show();

  // wyświetlanie szachownicy w serial monitorze
  // Serial.println(readMux(muxMapArray[0][0][0], muxMapArray[0][0][1]));
}

void lightChessBoard(){
  // startujemy od 0 (A1), inkrementujemy o 2
  for (int i = 0; i < 8; i += 2){
    for (int j = 0; j < 8; j++){
      for (int k = 0; k < 6; k++){
        if (j % 2 == 1){
          leds[ledMapArray[i][j][k]] = CRGB(255, 255, 255);
          leds[ledMapArray[i + 1][j][k]] = CRGB(0, 0, 0);
        }
        else {
          leds[ledMapArray[i][j][k]] = CRGB(0, 0, 0);
          leds[ledMapArray[i + 1][j][k]] = CRGB(255, 255, 255);
        }
      }
    }
  }
  FastLED.show();
}

void printChessBoard(){
  // rysowanie szachownicy w konsoli
  Serial.println("---------------------------------------------------------------------------------------------------------------------------------");
  for (int i = 0; i < 8; i++){
    Serial.print("|");
    for (int j = 0; j < 8; j++){
      Serial.print("\t");
      Serial.print("\t");
      Serial.print("|");
    }
    Serial.println();
    Serial.print("|");
    for (int j = 0; j < 8; j++){
      Serial.print("\t");
      Serial.print(readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]));
      Serial.print(" ");
      for (int k=0; k<32; k++){
        if (piecesPosition[k][0] == i && piecesPosition[k][1] == j){
          Serial.print(piecesNames[k]);
        }
        // if (blackPiecesPosition[k][0] == j && blackPiecesPosition[k][1] == (7 - i)){
        //   Serial.print(piecesNames[k]);
        // }
        // else
        //   Serial.print(piecesNames[32]);
      }
      // Serial.print(chessFieldState[i][j]);
      // Serial.print(i);
      // Serial.print(j);
      Serial.print("\t");
      Serial.print("|");
    }
    Serial.print(" ");
    Serial.print(8-i);
    Serial.println();
    Serial.print("|");
    for (int j = 0; j < 8; j++){
      Serial.print("\t");
      Serial.print("\t");
      Serial.print("|");
    }
    Serial.println("\n---------------------------------------------------------------------------------------------------------------------------------");
  }
  for (int i = 0; i < 8; i++){
    Serial.print("\t");
    Serial.write(65 + i);
    Serial.print("\t");
  }
  Serial.println();
}

// funkcja odczytująca wartość odpowiedniego czujnika halla
int readMux(int SIG_pin, int channel){
  int controlPin[] = {S3, S2, S1, S0};

  int muxChannel[16][4]={
    {0,0,0,0}, // kanał 0
    {1,0,0,0}, // kanał 1
    {0,1,0,0}, // kanał 2
    {1,1,0,0}, // kanał 3
    {0,0,1,0}, // kanał 4
    {1,0,1,0}, // kanał 5
    {0,1,1,0}, // kanał 6
    {1,1,1,0}, // kanał 7
    {0,0,0,1}, // kanał 8
    {1,0,0,1}, // kanał 9
    {0,1,0,1}, // kanał 10
    {1,1,0,1}, // kanał 11
    {0,0,1,1}, // kanał 12
    {1,0,1,1}, // kanał 13
    {0,1,1,1}, // kanał 14
    {1,1,1,1}  // kanał 15
  };

  // pętla ustawiająca 4 piny sygnałowe
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }
  int val;
  if (SIG_pin == SIG_PIN1 && channel == 1){
    val = analogRead(15);
  }
  else
  // odczytanie wartości z czujnika halla
  	val = analogRead(SIG_pin);

  return val;
}

// funkcja wywołująca odpowiednią funkcję w zależności od podniesionego bierka
void allowedMoves(int piece, int x, int y){
  // warunek wielokrotnego wyboru, przyjmujący numer bierki (piece)
  switch(piece){
    case 1:   // black rook 1
    case 8:   // black rook 2
    case 25:  // white rook 1
    case 32:  // white rook 2
      queenCheck(piece, x, y, rookMoves);
      break;
    case 2:   // black knight 1
    case 7:   // black knight 2
    case 26:  // white knight 1
    case 31:  // white knight 2
      knightCheck(piece, x, y);
      break;
    case 3:   // black bishop 1
    case 6:   // black bishop 2  
    case 27:  // white bishop 1
    case 30:  // white bishop 2
			queenCheck(piece, x, y, bishopMoves);
      break;
    case 4:   // black queen  
    case 28:  // white queen
    	queenCheck(piece, x, y, bishopMoves);
      queenCheck(piece, x, y, rookMoves);
      break;
    case 5:   // black king
    case 29:  // white king
      kingCheck(piece, x, y);
      break;
    case 9: // black pawns
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16: 
    case 17: //white pawns
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
      if (!pawnsPromotions[piece - 9]) pawnCheck(piece, x, y);
      else if (pawnsPromotions[piece - 9] == 1) queenCheck(piece, x, y, rookMoves);
			else if (pawnsPromotions[piece - 9] == 2) knightCheck(piece, x, y);
      else if (pawnsPromotions[piece - 9] == 3) queenCheck(piece, x, y, bishopMoves);
      else if (pawnsPromotions[piece - 9] == 4){
        queenCheck(piece, x, y, bishopMoves);
        queenCheck(piece, x, y, rookMoves);
      } 
      break;
  }
  setLed(x, y, CRGB(255,255,0));
  FastLED.show();
}

// funkcja podświetlająca pole o współrzędnych x,y
void setLed(int x, int y, CRGB c)
{
  if((x >= 0) && (x < 8) && (y >= 0) && (y < 8)){
    // if (raisedPieceName < 16 && chessFieldState[x][y])
    for (int i = 0; i < 6; i++){
      leds[ledMapArray[x][7-y][i]] = c; 
    }
  }
}

// funkcja uzupełniająca tablicę przechowującą pozycję bierek
void fillChessFieldState(){
  for (int i = 0; i < 32; i++){
    if (piecesPosition[i][0] < 8)
      chessFieldState[piecesPosition[i][0]][piecesPosition[i][1]] = i + 1;
  }
}

// funkcja mapująca 6 ledów jako pojedyncze pole szachownicy
void mapLeds(){
  for (int y = 0; y < 8; y++){
    for (int x = 0; x < 8; x++){
      ledMapArray[x][y][0] = 0+48*y+x*3;
      ledMapArray[x][y][1] = 1+48*y+x*3;
      ledMapArray[x][y][2] = 2+48*y+x*3;
      ledMapArray[x][y][3] = 45+48*y-x*3;
      ledMapArray[x][y][4] = 46+48*y-x*3;
      ledMapArray[x][y][5] = 47+48*y-x*3;
    }
  } 
}

// funkcja sprawdzająca czy ruch z pola x,y na pole x1,y1 jest dozwolnoy dla wieży
int rook(int x, int y, int x1, int y1)
{ // x,y współrzędne początkowe, x1,y1 współrzędne końcowe
  // jeśli a == 1 - ruch dozwolony, a == 0 ruch NIE dozwolony
  int allowed = 1;
  if (y == y1){
    for (int i = x + 1; i < x1; i++){
      if (chessFieldState[i][y] != 0){
        allowed = 0;
        break;
      }
    }
    for (int i = x - 1; i > x1; i--){
      if (chessFieldState[i][y] != 0){
        allowed = 0;
        break;
      }
    }
  }
  else if (x == x1){
    for (int i = y + 1; i < y1; i++){
      if (chessFieldState[x][i] != 0){
        allowed = 0;
        break;
      }
    }
    for (int i = y - 1; i > y1; i--){
      if (chessFieldState[x][i] != 0){
        allowed = 0;
        break;
      }
    }
  }
  else{
    allowed = 0;
  }
  return allowed;
}

// funkcja sprawdzająca czy ruch z pola x,y na pole x1,y1 jest dozwolnoy dla gońca
int bishop(int x, int y, int x1, int y1)
{
  int allowed = 1, i;
  if (abs(x - x1) != abs(y - y1)){
    allowed = 0;
  }
  if ((x < x1) && (y < y1)){
    for (i = 1; (x + i) < x1; i++){
      if (chessFieldState[x + i][y + i] != 0)
        allowed = 0;
    }
  }
  else if ((x > x1) && (y > y1)){
    for (i = 1; (x - i) > x1; i++){
      if (chessFieldState[x - i][y - i] != 0)
        allowed = 0;
    }
  }
  else if ((x > x1) && (y < y1)){
    for (i = 1; (x - i) > x1; i++){
      if (chessFieldState[x - i][y + i] != 0)
        allowed = 0;
    }
  } 
  else if ((x < x1) && (y > y1)){
    for (i = 1; (x + i) < x1; i++){
      if (chessFieldState[x + i][y - i] != 0)
        allowed = 0;
    }
  }
  return allowed;
}

// funkcja sprawdzająca czy ruch z pola x,y na pole x1,y1 jest dozwolnoy dla hetmana
int queen(int x, int y, int x1, int y1)
{
  // rych w pionie lub poziomie
  if (x == x1 || y == y1)
  {
    return rook(x, y, x1, y1);
  }
  // ruch po skosie
  else if (abs(x - x1) == abs(y - y1))
  {
    return bishop(x, y, x1, y1);
  }
  else
    return 0;
}

// funkcja sprawdzająca czy ruch z pola x,y na pole x1,y1 jest dozwolnoy dla konia
int horse(int x, int y, int x1, int y1)
{
  if ((y1 == y + 2 && x1 == x + 1) || (y1 == y + 2 && x1 == x - 1) || (y1 == y + 1 && x1 == x + 2) || \
      (y1 == y + 1 && x1 == x - 2) || (y1 == y - 1 && x1 == x + 2) || (y1 == y - 1 && x1 == x - 2) || \
      (y1 == y - 2 && x1 == x + 1) || (y1 == y - 2 && x1 == x - 1))
  {
    return 1;
  }
  return 0;
}

// funkcja sprawdzająca czy ruch z pola x,y na pole x1,y1 jest dozwolnoy dla pionka
int pawn(int x, int y, int x1, int y1)
{
  int allowed = 0;
  if (chessFieldState[x][y] >= 9 && chessFieldState[x][y] <= 16){
    if (x == 1){
      if (x1 == (x + 2) && y1 == y){
        if (chessFieldState[x1][y1] == 0 && chessFieldState[x + 1][y] == 0){
          allowed = 1;
        }
      }
    }
    if (x1 == x + 1 && y1 == y){
      if (chessFieldState[x1][y1] == 0){
        allowed = 1;
      }
    }
    else if (x1 == (x + 1) && (y1 == (y + 1) || y1 == (y - 1))){
      if (chessFieldState[x1][y1] > 16) {
        allowed = 1;
      }
    }
  }
  else if (chessFieldState[x][y] >= 17 && chessFieldState[x][y] <= 24){
    if (x == 6){
      if (x1 == (x - 2) && y1 == y){
        if (chessFieldState[x1][y1] == 0 && chessFieldState[x - 1][y] == 0){
          allowed = 1;
        }
      }
    }
    if (x1 == (x - 1) && y1 == y){
      if (chessFieldState[x1][y1] == 0){
        allowed = 1;
      }
    }
    else if (x1 == (x - 1) && (y1 == (y - 1) || y1 == (y + 1))){
      if (chessFieldState[x1][y1] < 17 && !chessFieldState[x1][y1]){
        allowed = 1;
      }
    }
  }
  // if (allowed == 1){
  //     if (piecesPosition[x][y] >= 9 && piecesPosition[x][y] <= 16){
  //         if (x1 == 7)
  //             return 2;
  //     }
  //     else if (piecesPosition[x][y] >= 17 && piecesPosition[x][y] <= 24){
  //         if (x1 == 0)
  //             return 2;
  //     }
  // }
  return allowed;
}

// funkcja sprawdzająca, czy którać bierka szachuje króla
bool checkIfCheck(int def, int x, int y){
  // return : 0 - brak szacha; 1 - szach
  // turn: 0 - ruch bialych; 1 - ruch czarnych
  int t = 0, i, j, xKing, yKing;
  //tablica pieces zawiera kolejne numery bierek, na pierwszym miejscu są bierki białe (zero w drugim wymiarze tablicy od turn = 0), a na drugi miejscu analogicznie bierki czarne
  int pieces [9][2] = {{9,17}, {16, 24}, {2, 26}, {7, 31}, {4, 28}, {1, 25}, {8, 32}, {3, 27}, {6, 30}};
		if(!turn && def){
			xKing = piecesPosition[28][0];
			yKing = piecesPosition[28][1];
		}
  	else if (turn == 1 && def){
      xKing = piecesPosition[4][0];
			yKing = piecesPosition[4][1];
    }
    else {
      xKing = x;
			yKing = y;
    }
  
    for (i = 0; i <= 7; i++)
    {
      for (j = 0; j <= 7; j++)
      {
        if (t == 1)
        {
					// check
          Serial.println("############################################################## szach ##############################################################\n");
          return 1;
        }

        if (chessFieldState[i][j] >= pieces[0][turn] && chessFieldState[i][j] <= pieces[1][turn]) //B: 9 16 //W: 17 24
          t = pawn(i, j, xKing, yKing);
        else if (chessFieldState[i][j] == pieces[2][turn] || chessFieldState[i][j] == pieces[3][turn]) //B: 2 7 //W: 26 31
          t = horse(i, j, xKing, yKing);
        else if (chessFieldState[i][j] == pieces[4][turn]) //B: 4 //W: 28
          t = queen(i, j, xKing, yKing);
        else if (chessFieldState[i][j] == pieces[5][turn] || chessFieldState[i][j] == pieces[6][turn]) //B: 1 3 //W: 25 32
          t = rook(i, j, xKing, yKing);
        else if (chessFieldState[i][j] == pieces[7][turn] || chessFieldState[i][j] == pieces[8][turn]) //B: 3 6 //W: 27 30
          t = bishop(i, j, xKing, yKing);
      } // for
    }   // for
  return 0;
}

// funkcja podświetlająca pola, na które może ruszyć się koń
void knightCheck(int knight, int x, int y){
  int tmp, check;
	for (int i = 0; i < 8; i++){
    tmp = chessFieldState[x + knightMoves[i][0]][y + knightMoves[i][1]];
    chessFieldState[x + knightMoves[i][0]][y + knightMoves[i][1]] = knight;
    chessFieldState[x][y] = 0;
    check = checkIfCheck();
    chessFieldState[x + knightMoves[i][0]][y + knightMoves[i][1]] = tmp;
    if (!check){
      if(!tmp) setLed(x + knightMoves[i][0], y + knightMoves[i][1], CRGB(0,255,0));
      else if(tmp > 16) setLed(x + knightMoves[i][0], y + knightMoves[i][1], CRGB(255,0,0));
    }
  }
}
// funkcja podświetlająca pola, na które może ruszyć się pionek
void pawnCheck(int pawn, int x, int y){
  int tmp[4], check[6], pawnLine;
  if (!turn)
    pawnLine = 6;
  else
    pawnLine = 1;

  //sprawdzenie czy po ruchu pionka jest szach, petla wykonuje sie 4 razy dla 4 roznych mozliwych ruchow pionkiem
  for (int i = 0; i < 8; i+= 2){
    tmp[i / 2] = chessFieldState[x + pawnMoves[i][turn]][y + pawnMoves[i + 1][turn]];
    chessFieldState[x + pawnMoves[i][turn]][y + pawnMoves[i + 1][turn]] = pawn;
    chessFieldState[x][y] = 0;
    check[i / 2] = checkIfCheck();
    chessFieldState[x + pawnMoves[i][turn]][y + pawnMoves[i + 1][turn]] = tmp[i / 2];
  }
  //sprawdzenie czy po zbiciu w przelocie wystąpi szach (uwzględnienie zniknięcia z szachownicy bitego pionka)
  if (pawnMoved[!turn] != 0){
    for (int j = 0; j < 2; j++){
      chessFieldState[x + pawnMoves[4 + 2 * j][turn]][y + pawnMoves[5 + 2 * j][turn]] = pawn;
      chessFieldState[x][y] = 0;
      chessFieldState[x][y - 1 + 2 * j] = 0; // pierwsza iteracja: y - 1 - znika pionek po lewej, druga iteracja: y + 1 - znika pionek po prawej
      check[4 + j] = checkIfCheck();
      chessFieldState[x + pawnMoves[4 + 2 * j][turn]][y + pawnMoves[5 + 2 * j][turn]] = tmp[2 + j];
    }
  }

  if (!check[0] && !tmp[0]){
    setLed(x + pawnMoves[0][turn], y, CRGB(0,255,0));
    if (!check[1] && x == pawnLine && !tmp[1])
      setLed(x + pawnMoves[2][turn], y, CRGB(0,255,0)); 
  }
  if (!check[2] && tmp[2] > turn * 16 && tmp[2] <= (turn + 1) * 16) 
    setLed(x + pawnMoves[4][turn], y + pawnMoves[5][turn], CRGB(255,0,0));
  if (!check[3] && tmp[3] > turn * 16 && tmp[3] <= (turn + 1) * 16) 
    setLed(x + pawnMoves[6][turn], y + pawnMoves[7][turn], CRGB(255,0,0));
  // bicie w przelocie
  //+-7 - pionek po prawej +-9 pionek po lewej
  if (!check[4] && !turn && x == 3 && piecesPosition[pawnMoved[!turn] - 1][1] == y + 1){
    Serial.println("beka1");
    setLed(x + pawnMoves[4][turn], y + 1, CRGB(255,0,0));
  }
	else if (!check[4] && turn && x == 4 && piecesPosition[pawnMoved[!turn] - 1][1] == y + 1){
    Serial.println("beka2");
    setLed(x + pawnMoves[4][turn], y + 1, CRGB(255,0,0));
  }
  if(!check[5] && !turn && x == 3 && piecesPosition[pawnMoved[!turn] - 1][1] == y - 1){
    Serial.println("beka3");
    setLed(x + pawnMoves[6][turn], y - 1, CRGB(255,0,0));
  }
  else if (!check[5] && turn && x == 4 && piecesPosition[pawnMoved[!turn] - 1][1] == y - 1){
    Serial.println("beka4");
    setLed(x + pawnMoves[6][turn], y - 1, CRGB(255,0,0));
  }
} 

// funkcja podświetlająca pola, na które może ruszyć się wieża, goniec oraz hetman, w zależności od agrumentu moves
void queenCheck(int queen, int x, int y, int moves[][2]){
  int tmp, check, allowed = 1;
  for (int j = 0; j < 4; j++)
  {
    for (int i = 1; x + i * moves[j][0] < 8 && y + i * moves[j][1] < 8 && x + i * moves[j][0] >= 0 && y + i * moves[j][1] >= 0; i++){
      tmp = chessFieldState[x + i * moves[j][0]][y + i * moves[j][1]];
      chessFieldState[x + i * moves[j][0]][y + i * moves[j][1]] = queen;
      chessFieldState[x][y] = 0;
      check = checkIfCheck();
      chessFieldState[x + i * moves[j][0]][y + i * moves[j][1]] = tmp;
      // jeśli brak szacha
      if (!check){
        // jeśli pole na ktore potencjalnie chcemy postawic figure jest puste zaświeć na zielono
        if(!tmp) 
          setLed(x + i * moves[j][0], y + i * moves[j][1], CRGB(0,255,0));
        // jeśli na polu jest biała bierka i ruch czarnych to zaświeć pole na czerowno
        else if(tmp > 16 && turn){
          setLed(x + i * moves[j][0], y + i * moves[j][1], CRGB(255,0,0));
          break;
        } // czarna bierka i ruch czarnych
        else if(tmp <= 16 && turn){
          break;
        } // czarna bierka i ruch białych 
        else if(tmp <= 16 && !turn){
          setLed(x + i * moves[j][0], y + i * moves[j][1], CRGB(255,0,0));
          break;
        } // biała bierka i ruch białych
        else {
          break;
        }
      }
      else if(tmp){
        break;
      }
    } 
  }
}

// funkcja podświetlająca pola, na które może ruszyć się król
void kingCheck(int king, int x, int y){
  for (int i = 0; i < 8; i++){
    if (!checkIfCheck(0, x + kingMoves[i][0], y + kingMoves[i][1])){
      if(!chessFieldState[x + kingMoves[i][0]][y + kingMoves[i][1]]) setLed(x + kingMoves[i][0], y + kingMoves[i][1], CRGB(0,255,0));
      else if(!turn && chessFieldState[x + kingMoves[i][0]][y + kingMoves[i][1]] <= 16) setLed(x + kingMoves[i][0], y + kingMoves[i][1], CRGB(255,0,0));
      else if(turn && chessFieldState[x + kingMoves[i][0]][y + kingMoves[i][1]] > 16) setLed(x + kingMoves[i][0], y + kingMoves[i][1], CRGB(255,0,0));
    }
  }
  // sprawdzanie czy dostępna jest roszada
  // 0 białe, 1 czarne
  if (!moved[0][turn] && !moved[1][turn] && !chessFieldState[x][y - 1] && !checkIfCheck(0, x, y - 1) && !chessFieldState[x][y - 2] && \
      !checkIfCheck(0, x, y - 2) && !chessFieldState[x][y - 3] && chessFieldState[x][y - 4] == (25 - (24 * turn)))
    setLed(x, y - 2, CRGB(0,255,0));
    // todo wymusić ruch wieży jako jedyny możliwy??
    // przy globalnym turnie nie można go zmienić i trzeba czekać na ruch wieży i guess
  if (!moved[0][turn] && !moved[2][turn] && !chessFieldState[x][y + 1] && !checkIfCheck(0, x, y + 1) && !chessFieldState[x][y + 2] && \
      !checkIfCheck(0, x, y + 2) && chessFieldState[x][y + 3] == (32 - (24 * turn)))
    setLed(x, y + 2, CRGB(0,255,0));
    // todo wymusić ruch wieży jako jedyny możliwy??
    // przy globalnym turnie nie można go zmienić i trzeba czekać na ruch wieży i guess
}
                     
//sprawdzamy czy dany pionek znalazł się na jego ostaniej linii i czy nie został wcześniej zpromowany (zapobiega sprawdzaniu promocji dla bierki, która została zpromowana)
//ustawiana jest tablica promocji od 1 do 4 w zależności od figury, na którą chcemy zamienić pionka
//w tablicy chessFieldState na miejscu pionka zmieniany jest numer który identyfikuje bierke (będący pionkiem) na numer wybranej figury 
void pawnPromotion(int pawn, int x, int y){
  if (turn){
    if (!pawnsPromotions[pawn - 9] && piecesPosition[pawn - 1][0] == 7){
      //promocja na hetmana
      pawnsPromotions[pawn - 9] = 4;
      chessFieldState[x][y] = 4;
      //promocja na gońca
      // pawnsPromotions[i][0] = 3;
      // chessFieldState[x][y] = 3;
      //promocja na konia
      // pawnsPromotions[i][0] = 2;
      // chessFieldState[x][y] = 2;
      //promocja na wieżę
      // pawnsPromotions[i][0] = 1;
      // chessFieldState[x][y] = 1;
      
    }
  }
  else {
    if (!pawnsPromotions[pawn - 9] && piecesPosition[pawn - 1][0] == 0){
      //promocja na hetmana
      pawnsPromotions[pawn - 9] = 4;
      chessFieldState[x][y] = 4;
      //promocja na gońca
      // pawnsPromotions[i][0] = 3;
      // chessFieldState[x][y] = 3;
      //promocja na konia
      // pawnsPromotions[i][0] = 2;
      // chessFieldState[x][y] = 2;
      //promocja na wieżę
      // pawnsPromotions[i][0] = 1;
      // chessFieldState[x][y] = 1;
    }
  }
}

// funkcja wymuszjąca dokończenie roszady po ruchu królem
void castle(int x, int y, int y1, int rook){
  setLed(x, y, CRGB(255,255,0));
  setLed(x, y1, CRGB(0,255,0));
  FastLED.show();
  while (readMux(muxMapArray[x][y1][0], muxMapArray[x][y1][1]) < 2500){}
  piecesPosition[rook - 1][1] = y1;
  chessFieldState[x][y] = 0;
  chessFieldState[x][y1] = rook;
}