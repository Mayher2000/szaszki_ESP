#include <Arduino.h>
#include <FastLED.h>
#include <SPI.h>

// LEDs config
#define LED_PIN         23
#define NUM_LEDS        200
#define BRIGHTNESS      20 // 255
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB

// Mux control pins
#define S0              26
#define S1              25
#define S2              33
#define S3              32

// Mux in "SIG" pin
#define SIG_PIN1        35
#define SIG_PIN2        34
#define SIG_PIN3        39
#define SIG_PIN4        36

// tablica ledów
CRGB leds[NUM_LEDS];

// tablica do odczytywania sygnałów z multiplekserów, uporządkowana jak pola szachownicy tj. A1 - (mux1, ch0), B1 - (mux1, ch1)... A2 - (mux1, ch8) ... A3 - (mux2, ch0) ...
const short muxMapArray[8][8][2] = {{{SIG_PIN1, 0}, {SIG_PIN1, 1}, {SIG_PIN1, 2},  {SIG_PIN1, 3},  {SIG_PIN1, 4},  {SIG_PIN1, 5},  {SIG_PIN1, 6},  {SIG_PIN1, 7}},
                                    {{SIG_PIN1, 8}, {SIG_PIN1, 9}, {SIG_PIN1, 10}, {SIG_PIN1, 11}, {SIG_PIN1, 12}, {SIG_PIN1, 13}, {SIG_PIN1, 14}, {SIG_PIN1, 15}},
                                    {{SIG_PIN2, 0}, {SIG_PIN2, 1}, {SIG_PIN2, 2},  {SIG_PIN2, 3},  {SIG_PIN2, 4},  {SIG_PIN2, 5},  {SIG_PIN2, 6},  {SIG_PIN2, 7}},
                                    {{SIG_PIN2, 8}, {SIG_PIN2, 9}, {SIG_PIN2, 10}, {SIG_PIN2, 11}, {SIG_PIN2, 12}, {SIG_PIN2, 13}, {SIG_PIN2, 14}, {SIG_PIN2, 15}},
                                    {{SIG_PIN3, 0}, {SIG_PIN3, 1}, {SIG_PIN3, 2},  {SIG_PIN3, 3},  {SIG_PIN3, 4},  {SIG_PIN3, 5},  {SIG_PIN3, 6},  {SIG_PIN3, 7}},
                                    {{SIG_PIN3, 8}, {SIG_PIN3, 9}, {SIG_PIN3, 10}, {SIG_PIN3, 11}, {SIG_PIN3, 12}, {SIG_PIN3, 13}, {SIG_PIN3, 14}, {SIG_PIN3, 15}},
                                    {{SIG_PIN4, 0}, {SIG_PIN4, 1}, {SIG_PIN4, 2},  {SIG_PIN4, 3},  {SIG_PIN4, 4},  {SIG_PIN4, 5},  {SIG_PIN4, 6},  {SIG_PIN4, 7}},
                                    {{SIG_PIN4, 8}, {SIG_PIN4, 9}, {SIG_PIN4, 10}, {SIG_PIN4, 11}, {SIG_PIN4, 12}, {SIG_PIN4, 13}, {SIG_PIN4, 14}, {SIG_PIN4, 15}}};

// tablica do odczytywania numeru ledów, uporządkowana tak jak biegnął ledy pod szachownicą
const short ledMapArray[8][8] = {{0,  1,  2,  3,  4,  5,  6,  7},
                                 {15, 14, 13, 12, 11, 10, 9,  8},
                                 {16, 17, 18, 19, 20, 21, 22, 23},
                                 {31, 30, 29, 28, 27, 26, 25, 24},
                                 {32, 33, 34, 35, 36, 37, 38, 39},
                                 {47, 46, 45, 44, 43, 42, 41, 40},
                                 {48, 49, 50, 51, 52, 53, 54, 55},   
                                 {63, 62, 61, 60, 59, 58, 57, 56}};












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

short piecesPosition[32][2] = {{0,0},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},
                               {8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},
                               {8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},
                               {8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8},{8,8}};

// short blackPiecesPosition[16][2] = {{0,7},{1,7},{2,7},{3,7},{4,7},{5,7},{6,7},{7,7},
//                                     {0,6},{1,6},{2,6},{3,6},{4,6},{5,6},{6,6},{7,6}};

const char *piecesNames[33] = {"BR","BN","BB","BQ","BK","BB","BN","BR",
                               "BP","BP","BP","BP","BP","BP","BP","BP",
                               "WP","WP","WP","WP","WP","WP","WP","WP",
                               "WR","WN","WB","WQ","WK","WB","WN","WR",
                               "E"};

// short chessFieldState[8][8] = {{-1, -1, -1, -1, -1, -1, -1, -1},
//                                {-1, -1, -1, -1, -1, -1, -1, -1},
//                                { 0,  0,  0,  0,  0,  0,  0,  0},
//                                { 0,  0,  0,  0,  0,  0,  0,  0},
//                                { 0,  0,  0,  0,  0,  0,  0,  0},
//                                { 0,  0,  0,  0,  0,  0,  0,  0},
//                                { 1,  1,  1,  1,  1,  1,  1,  1},
//                                { 1,  1,  1,  1,  1,  1,  1,  1}};

short chessFieldState[8][8] = {{0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0}};

// short chessFieldState[8][8] = {{1, 1, 1, 1, 1, 1, 1, 1},
//                                {1, 1, 1, 1, 1, 1, 1, 1},
//                                {0, 0, 0, 0, 0, 0, 0, 0},
//                                {0, 0, 0, 0, 0, 0, 0, 0},
//                                {0, 0, 0, 0, 0, 0, 0, 0},
//                                {0, 0, 0, 0, 0, 0, 0, 0},
//                                {1, 1, 1, 1, 1, 1, 1, 1},
//                                {1, 1, 1, 1, 1, 1, 1, 1}};


bool ppflag = false;
bool colorFlag = true;
short raisedCords[2] = {8,8};
short raisedPieceName = 33;
bool notAllowedMoveFlag = false;

void lightChessBoard();
void printChessBoard();
int readMux(int SIG_pin, int channel);
void allowedMoves(int pice, int x, int y);
void setLed(int x, int y, CRGB c);
void fillChessFieldState();

void setup(){
  // inicjalizacja portu szeregowego (wysoki baudrate, żeby "konsolowa animacja" była bardzje przejrzysta)
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
}


void loop(){
  // zawsze na start w loop() będzie rysowana szachownica
  lightChessBoard();

//  przejście po szachownicy
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      
      // odczytujemy czy pole szachowe jest puste
      if (readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) < 2500){
        // wiemy, że pole jest puste, sprawdzamy czy coś na tym polu stało wcześniej (w tablicy pozycji pionków)
        for (int k = 0; k < 32; k++){
          // jeśli na polu coś wcześniej stało a teraz tego nie ma to:
          if (piecesPosition[k][0] == i && piecesPosition[k][1] == j){
            // zapisujemy kordy podniesionego pionka
            raisedCords[0] = piecesPosition[k][0];
            raisedCords[1] = piecesPosition[k][1];
            // chessFieldState[raisedCords[0]][raisedCords[1]] = 0;
            // zapisujemy jaki to był pionek (0 - czarna wieża itd.)
            raisedPieceName = k;
            // zmieniamy kordy pionka na podniesione (9,9)
            piecesPosition[k][0] = 9;
            piecesPosition[k][1] = 9;
            allowedMoves(raisedPieceName, raisedCords[0], raisedCords[1]);
            printChessBoard();
            // zaczynamy szukać pola, na które pionek został odłożnoy
            while (piecesPosition[raisedPieceName][0] == 9){
              // przejście po wszystkich polach szachownicy
              for (int l = 0; l < 8; l++){
                for (int m = 0; m < 8; m++){
                  // opuszczenie flagi służącej do sprawdzenia czy pionek wcześniej stał już na danym polu
                  ppflag = false;
                  // odczytanie czy na polu szachowym znajduje się pionek
                  if (readMux(muxMapArray[l][m][0], muxMapArray[l][m][1]) > 2500){
                    // sprawdzenie czy pionek wcześniej znajdował się na danym polu
                    for (int n = 0; n < 32; n++){
                      // jeśli tak, to podnosimy pepeFlage
                      if (ppflag == false && piecesPosition[n][0] == l && piecesPosition[n][1] == m){
                        ppflag = true;
                        break;
                      }
                    }
                    // jeśli flaga jest opuszczona, oznacza to że sprawdzane pole wcześniej było puste
                    if (!ppflag){
                      // zmiana kordów opuszczonego pionka (umożliwia wyjście z pętli while)
                      piecesPosition[raisedPieceName][0] = l;
                      piecesPosition[raisedPieceName][1] = m;
                      
                      // chessFieldState[l][m] = 1; // trzeba zmienić potem
                      printChessBoard();
                    }
                    
                  }
                }
              }
              // printChessBoard();
              // Serial.println(readMux(muxMapArray[0][0][0], muxMapArray[0][0][1]));
              // delay(1000);
            }
          }
        }
      }
    }
  }
  FastLED.show();

  // wyświetlanie szachownicy w serial monitorze
  // printChessBoard();
  // Serial.println(readMux(muxMapArray[0][0][0], muxMapArray[0][0][1]));
  // delay(1000);
}

void lightChessBoard(){
  // startujemy od 0 (A1), inkrementujemy o 2
  for (int i = 0; i < NUM_LEDS; i += 2){
    // zaczyanmy od A1 - czarne
    leds[i] = CRGB(0, 0, 0);
    // potem A2 - białe
    leds[i + 1] = CRGB(255, 255, 255);
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
      // Serial.print(readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]));
      // Serial.print(" ");
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
      Serial.print(i);
      Serial.print(j);
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


int readMux(int SIG_pin, int channel){
  int controlPin[] = {S3, S2, S1, S0};

  int muxChannel[16][4]={
    {0,0,0,0}, //channel 0
    {1,0,0,0}, //channel 1
    {0,1,0,0}, //channel 2
    {1,1,0,0}, //channel 3
    {0,0,1,0}, //channel 4
    {1,0,1,0}, //channel 5
    {0,1,1,0}, //channel 6
    {1,1,1,0}, //channel 7
    {0,0,0,1}, //channel 8
    {1,0,0,1}, //channel 9
    {0,1,0,1}, //channel 10
    {1,1,0,1}, //channel 11
    {0,0,1,1}, //channel 12
    {1,0,1,1}, //channel 13
    {0,1,1,1}, //channel 14
    {1,1,1,1}  //channel 15
  };

  //loop through the 4 sig
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  //read the value at the SIG pin
  int val = analogRead(SIG_pin);

  //return the value
  return val;
}

// "BR","BN","BB","BQ","BK","BB","BN","BR",
// "BP","BP","BP","BP","BP","BP","BP","BP",
// "WP","WP","WP","WP","WP","WP","WP","WP",
// "WR","WN","WB","WQ","WK","WB","WN","WR",


void allowedMoves(int piece, int x, int y){
  switch(piece){
    case 0:   // black rook 1
    case 7:   // black rook 2
    case 24:  // white rook 1
    case 31:  // white rook 2
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      for (int i = 0; i < 8; i++){
        setLed(x, i, CRGB(0,255,0));
        setLed(i, y, CRGB(0,255,0));
      }
      break;
    case 1:   // black knight 1
    case 6:   // black knight 2
    case 25:  // white knight 1
    case 30:  // white knight 2
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      setLed(x - 1, y - 2, CRGB(0,255,0));
      setLed(x + 1, y - 2, CRGB(0,255,0));
      setLed(x - 1, y + 2, CRGB(0,255,0));
      setLed(x + 1, y + 2, CRGB(0,255,0));
      setLed(x + 2, y - 1, CRGB(0,255,0));
      setLed(x - 2, y - 1, CRGB(0,255,0));
      setLed(x + 2, y + 1, CRGB(0,255,0));
      setLed(x - 2, y + 1, CRGB(0,255,0));
      break;
    case 2:   // black bishop 1
    case 5:   // black bishop 2
    case 26:  // white bishop 1
    case 29:  // white bishop 2
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      for (int i = 0; i < 8; i++){
        setLed(x + i, y + i, CRGB(0,255,0));
        setLed(x - i, y - i, CRGB(0,255,0));
        setLed(x + i, y - i, CRGB(0,255,0));
        setLed(x - i, y + i, CRGB(0,255,0));
      }
      if (((x % 2 == 0) && (y % 2 == 0)) || ((x % 2 == 1) && (y % 2 == 1))) setLed(x, y, CRGB(0,0,0));
      else setLed(x, y, CRGB(255,255,255));
      break;
    case 3:   // black queen  
    case 27:  // white queen
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      for (int i = 0; i < 8; i++){
        setLed(x, i, CRGB(0,255,0));
        setLed(i, y, CRGB(0,255,0));
        setLed(x + i, y + i, CRGB(0,255,0));
        setLed(x - i, y - i, CRGB(0,255,0));
        setLed(x + i, y - i, CRGB(0,255,0));
        setLed(x - i, y + i, CRGB(0,255,0));
      }
      if (((x % 2 == 0) && (y % 2 == 0)) || ((x % 2 == 1) && (y % 2 == 1))) setLed(x, y, CRGB(0,0,0));
      else setLed(x, y, CRGB(255,255,255));
      break;
    case 4:   // black king
    case 28:  // white king
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      setLed(x + 1, y, CRGB(0,255,0));
      setLed(x - 1, y, CRGB(0,255,0));
      setLed(x, y - 1, CRGB(0,255,0));
      setLed(x, y + 1, CRGB(0,255,0));
      setLed(x + 1, y + 1, CRGB(0,255,0));
      setLed(x - 1, y - 1, CRGB(0,255,0));
      setLed(x + 1, y - 1, CRGB(0,255,0));
      setLed(x - 1, y + 1, CRGB(0,255,0));   
      break;
    case 8: // black pawns
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: 
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      setLed(x + 1, y, CRGB(0,255,0));   
      break;
    case 16: //white pawns
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
      if (notAllowedMoveFlag) { notAllowedMoveFlag = false; break;}
      setLed(x - 1, y, CRGB(0,255,0));    
      break;
  }
  FastLED.show();
}

void setLed(int x, int y, CRGB c)
{
  if((x >= 0) && (x < 8) && (y >= 0) && (y < 8))
  {
    if (raisedPieceName < 16 && chessFieldState[x][y])
    leds[ledMapArray[x][y]] = c; 
  }
}

void fillChessFieldState(){
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      if (readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) > 2500){
        chessFieldState[i][j] = 1;
      }
      // else if(readMux(muxMapArray[i][j][0], muxMapArray[i][j][1]) < 700){
      //   chessFieldState[i][j] = -1;
      // }
      else {
        chessFieldState[i][j] = 0;
      }
    }
  }
}