#include <Arduino.h>
#include <FastLED.h>
#include <SPI.h>
#include "micromax.h"

// konfiguracja paska LEDowego
#define LED_PIN         23
#define NUM_LEDS        384
#define BRIGHTNESS      20 // 255
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

//polaryzacja magnesow: ok 1000 dla czarnych ok 3000 dla bialych
int blackMagFieldStr = 1500, whiteMagFieldStr = 2500;

// pierwsza współrzędna to wiersz, druga to kolumna
// A8 - 0.0 B8 - 0.1 C8 - 0.2 D8 - 3.3 E8 - 0.4  F8 - 0.5 G8 - 0.6 H8 - 0.7  
// A7 - 1.0 B7 - 1.1 C7 - 1.2 D7 - 3.3 E7 - 1.4  F7 - 1.5 G7 - 1.6 H7 - 1.7  
// A6 - 2.0 B6 - 2.1 C6 - 2.2 D6 - 3.3 E6 - 2.4  F6 - 2.5 G6 - 2.6 H6 - 2.7  
// A5 - 3.0 B5 - 3.1 C5 - 3.2 D5 - 3.3 E5 - 3.4  F5 - 3.5 G5 - 3.6 H5 - 3.7  
// A4 - 4.0 B4 - 4.1 C4 - 4.2 D4 - 4.3 E4 - 4.4  F4 - 4.5 G4 - 4.6 H4 - 4.7  
// A3 - 5.0 B3 - 5.1 C3 - 5.2 D3 - 5.3 E3 - 5.4  F3 - 5.5 G3 - 5.6 H3 - 5.7 
// A2 - 6.0 B2 - 6.1 C2 - 6.2 D2 - 6.3 E2 - 6.4  F2 - 6.5 G2 - 6.6 H2 - 6.7  
// A1 - 7.0 B1 - 7.1 C1 - 7.2 D1 - 7.3 E1 - 7.4  F1 - 7.5 G1 - 7.6 H1 - 7.7 

const char *piecesNames[13] = {"","r","n","b","q","k","p",
                                  "R","N","B","Q","K","P"};

// 0 - empty square
// 1 - black rook     // 7  - white rook
// 2 - black knight   // 8  - white knight
// 3 - black bishop   // 9  - white bishop
// 4 - black queen    // 10 - white queen
// 5 - black king     // 11 - white king
// 6 - black pawn     // 12 - white pawn

short chessFieldState[8][8] = {{4, 0, 0, 0, 5, 0, 0, 1},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 11, 0, 0, 0}};

short allowedMovesArr[8][8] = {{0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0},
                               {0, 0, 0, 0, 0, 0, 0, 0}};

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

int hall;

int prevPosFlag = 0;
bool turn = 0; // 0 ruch białych, 1 ruch czarnych
short raisedPieceName = 0;
short loweredPieceName = 0;
bool notAllowedMoveFlag = true;
bool isEngineOn = true;
bool engineColor = 1; // jeśli 0 to silnik zaczyna białymi, 1 zaczyna czarnymi

// tablice ruchów dla bierek
int knightMoves[8][2] = {{-1, -2}, {1, -2}, {-1, 2}, {1, 2}, {2, -1}, {-2, -1}, {2, 1}, {-2, 1}};
int pawnMoves[8][2] = {{-1, 1}, {0, 0}, {-2, 2}, {0, 0}, {-1, 1}, {1, 1}, {-1, 1}, {-1, -1}};  
int kingMoves[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
int rookMoves[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; 
int bishopMoves[4][2] = {{1 ,1}, {-1, -1}, {1, -1}, {-1, 1}};

// tablica przechowująca flagi sprawdzające pierwszy ruch króli oraz wież po lewej i wież po prawej
int moved[3][2] = {{0, 0}, {0, 0}, {0, 0}};

int pawnMoved[2] = {8, 8};

char chessEngineReturn[5];
int asciiToMove[8] = {7, 6, 5, 4, 3, 2, 1, 0}; 

// deklaracje funkcji
void lightChessboard();
void printChessboard();
int readMux(int x, int y);
int allowedMoves(int piece, int x, int y, int checkOnly = 0);
void setLed(int x, int y, CRGB c);
void mapLeds();

int rook(int x, int y, int x1, int y1);
int bishop(int x, int y, int x1, int y1);
int queen(int x, int y, int x1, int y1);
int knight(int x, int y, int x1, int y1);
int pawn(int x, int y, int x1, int y1);

int knightCheck(int knight, int x, int y, int checkOnly); 
int pawnCheck(int pawn, int x, int y, int checkOnly);
int queenCheck(int queen, int x, int y, int moves[][2], int checkOnly);
int kingCheck(int king, int x, int y, int checkOnly);

bool checkIfCheck(int def = 1, int x = 9, int y = 9);
void pawnPromotion(int x, int y);
void castle(int x, int y, int y1);
void checkIfCastle(int piece, int i, int j, int m);
int checkStaleMate();
void makeEngineMove();
void isGameOver();

// ######################## WIFI @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// #include <WiFi.h>
// #include <HTTPClient.h> 
// #include "ESPAsyncWebServer.h"

// #include <Preferences.h>

// String ssid;
// String password;

// int notConnectedCounter = 0;

// // Set your access point network credentials
// const char* APssid = "Chessboard-AP";

// const char* PARAM_INPUT_1 = "ssid";
// const char* PARAM_INPUT_2 = "password";

// IPAddress local_IP(192, 168, 1, 1);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(1, 1, 1, 1);   //optional
// IPAddress secondaryDNS(4, 4, 4, 4); //optional

// AsyncWebServer server(2391);

// Preferences preferences;

// void startAP();
// void setupWifi();

// ######################## WIFI @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
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

  printChessboard();
  // funkcja mapująca ledy
  mapLeds();

  turn ? Serial.println("Ruch czarnych") : Serial.println("Ruch białych");

  // ######################################################################

  // preferences.begin("credentials", false);

  // ssid = preferences.getString("ssid", ""); 
  // password = preferences.getString("password", "");

  // // jeśli dane wifi nie są zapisane, utwórz access point
  // Serial.println();
  // if (ssid == "" || password == ""){
  //   Serial.println("Brak zapisanych wartości dla ssid lub password");
  //   startAP();
  // }
  // else{
  //   setupWifi();
  // }

}

// void loop(){

// }

// void startAP(){
//   //WiFi.softAP(APssid, APpassword);
//   WiFi.softAP(APssid);

//   Serial.println("Tworzenie Access Pointu o naziwe: Chessboard-AP");

//   if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
//     Serial.println("AP Failed to configure");
//   }
//   else { 
//     Serial.println("AP configured successfully.");
//   }

//   Serial.print("IP adres: ");
//   Serial.println(WiFi.softAPIP());

//   server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request){
//     String inputMessage1;
//     String inputMessage2;
//     // String inputMessage3;

//     if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
//       inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
//       inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
//       //inputMessage3 = request->getParam(PARAM_INPUT_3)->value();

//       // ssid = inputMessage1;
//       // password = inputMessage2;
//       // token = inputMessage3;

//       preferences.putString("ssid", inputMessage1); 
//       preferences.putString("password", inputMessage2);
//       //preferences.putString("token", inputMessage3);

//       Serial.println("Network Credentials Saved using Preferences");
//     }
//     else {
//     inputMessage1 = "No message sent";
//     inputMessage2 = "No message sent";
//     //inputMessage3 = "No message sent";
//   }
//     Serial.println("Dane: ");
//     //Serial.println("ssid: " + inputMessage1 + ", password: " + inputMessage2 + ", token: " + inputMessage3);
//     Serial.println("ssid: " + inputMessage1 + ", password: " + inputMessage2);
//     if (inputMessage1 == "No message sent")
//       request->send(500, "text/plain", "NOT OK"); 
//     else{
//       request->send(200, "text/plain", "OK");
//       delay(1000);
//       server.end();
//       //setup_wifi();
//       ESP.restart();
//     }
      
//   });

//   DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

//   server.begin();
// }

// void setupWifi(){
//   // connecting wifi
//   WiFi.mode(WIFI_OFF);        // Prevents reconnection issue (taking too long to connect)
//   delay(1000);
//   WiFi.mode(WIFI_STA);        // This line hides the viewing of ESP as wifi hotspot
 
//   Serial.println();

//   Serial.print("Łączenie z wifi");
//   // Wait for connection

//   notConnectedCounter = 0;
//   WiFi.begin(ssid.c_str(), password.c_str());     // Connect to your WiFi router
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//     delay(500);
//     if (notConnectedCounter == 10 || notConnectedCounter == 20) 
//       WiFi.begin(ssid.c_str(), password.c_str());  
//     else if (notConnectedCounter == 30){
//       Serial.println();
//       Serial.println("Coś poszło nie tak!");
//       Serial.println("Szachownica zostanie zrestartowana.");
//       preferences.clear();
//       ESP.restart();
//     }
//     notConnectedCounter++;
//   }

//   Serial.println("");
//   Serial.print("WiFi connected to: ");
//   Serial.println(WiFi.SSID());
//   Serial.print("IP address: ");
//   Serial.println(WiFi.localIP());
//   Serial.print("GW address: ");
//   Serial.println(WiFi.gatewayIP());
//   Serial.print("Mask: ");
//   Serial.println(WiFi.subnetMask());

//   server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request){
//     IPAddress tmpIP = WiFi.localIP();
//     request->send(200, "text/plain", String(tmpIP[3]));
//     Serial.println("Znaleziono z aplikacji");
//   });

//   server.on("/disconnect", HTTP_GET, [](AsyncWebServerRequest *request){
//     IPAddress tmpIP = WiFi.localIP();
//     request->send(200, "text/plain", "disconnected");
//     Serial.println("Rozłączam WiFi i wymazuję dane logowania");
//     preferences.clear();
//     ESP.restart();
//   });

//   DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

//   server.begin();
// }

void loop(){
  // zawsze na start w loop() będzie rysowana szachownica w konsoli portu szeregowego
  lightChessboard();
  memset(allowedMovesArr, 0, sizeof(allowedMovesArr));
  
  if (isEngineOn && (engineColor & turn)){
    makeEngineMove();
    turn ? turn = 0 : turn = 1;
  }
  
  // przejście po szachownicy
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      
      hall = readMux(i, j);
      // odczytujemy czy pole szachowe jest puste
      if (hall < whiteMagFieldStr && hall > blackMagFieldStr){
        // wiemy, że pole jest puste, sprawdzamy czy coś na tym polu stało wcześniej (w tablicy pozycji pionków)
          // jeśli na polu coś wcześniej stało a teraz tego nie ma to:
          // kordy podniesionego pionka i - kord x, j - kord y
        if (chessFieldState[i][j]){
          // zapisujemy jaki to był pionek 
          raisedPieceName = chessFieldState[i][j];
          // podniesienie swojej bierki w turze przeciwnika
          // czeka aż ta bierka zostanie odłożona na podświetlone pole
          if ((!turn && raisedPieceName <= 6) || (turn && raisedPieceName > 6)){
            setLed(i, j, CRGB(255,0,0));
            FastLED.show();
            hall = readMux(i, j);
            while (hall < whiteMagFieldStr && hall > blackMagFieldStr){ hall = readMux(i, j); }
            allowedMovesArr[i][j] = 0;
            break;
          }
          chessFieldState[i][j] = 0;
          allowedMoves(raisedPieceName, i, j);
          for (int l = 0; l < 8; l++){
              for (int m = 0; m < 8; m++){
                Serial.print(allowedMovesArr[l][m]);
              }
            Serial.println();
          }
          
          printChessboard();
          // zaczynamy szukać pola, na które pionek został odłożnoy
          while (raisedPieceName > 0){
            // przejście po wszystkich polach szachownicy
            for (int l = 0; l < 8; l++){
              for (int m = 0; m < 8; m++){
                // opuszczenie flagi służącej do sprawdzenia czy pionek wcześniej stał już na danym polu
                prevPosFlag = 0;
                // odczytanie czy na polu szachowym znajduje się pionek
                hall = readMux(l, m);
                //#########################################################################################################################################################################################################
                // tura czarnych i czarny bierek lub tura białych i biały bierek, oznacza, że został on odłożony w inne miejsce
                if (hall > whiteMagFieldStr || hall < blackMagFieldStr){
                  loweredPieceName = chessFieldState[l][m];
                  // sprawdzenie czy pionek wcześniej znajdował się na danym polu
                  // jeśli tak, to podnosimy pepeFlage
                  if (loweredPieceName){
                    prevPosFlag = 1;
                    // bicie bierki przeciwnika
                    if ((loweredPieceName > 6 && hall < blackMagFieldStr) || (loweredPieceName <= 6 && hall > whiteMagFieldStr)){
                      chessFieldState[l][m] = raisedPieceName;
                      Serial.println("wychodze z pierwszego ifa");
                      raisedPieceName = 0;
                      turn ? turn = 0 : turn = 1;
                      isGameOver();
                      printChessboard();
                      turn ? Serial.println("Ruch czarnych") : Serial.println("Ruch białych");
                    }
                  }
                  // jeśli flaga jest opuszczona, oznacza to że sprawdzane pole wcześniej było puste
                  if (!prevPosFlag){
                    // wykonanie niedozwolonego ruchu
                    if (allowedMovesArr[l][m] == 0 && (i != l || j != m)){
                      lightChessboard();
                      setLed(l, m, CRGB(255,0,0));
                      FastLED.show();
                      long tim = millis();
                      short color;
                      hall = readMux(l, m);
                      while (hall > whiteMagFieldStr || hall < blackMagFieldStr){
                        if (millis() - tim > 500){
                          color ? (color = 0, setLed(l, m, CRGB(255,0,0))) : (color = 1, ((l + m) % 2 ? setLed(l, m, CRGB(0,0,0)) : setLed(l, m, CRGB(255,255,255))));
                          FastLED.show();
                          tim = millis();
                        }
                        hall = readMux(l, m);
                      }
                      (l + m) % 2 ? setLed(l, m, CRGB(0,0,0)) : setLed(l, m, CRGB(255,255,255));
                      allowedMovesArr[l][m] = 0;
                      allowedMoves(raisedPieceName, i, j);
                      break;
                    }
                    // wpisuje do chessfieldstate daną bierke w odpowiednim miejscy na szachownicy
                    chessFieldState[l][m] = raisedPieceName;
                    
                    // promocja pionka
                    if (raisedPieceName == 6 || raisedPieceName == 12){
                      pawnPromotion(l, m);
                      // flaga wykonania ruchu o 2 pola przez pionka
                      // po tym ruchu zapisujemy jego kord y do późniejszego sprawdzenia możliwości bicia w przelocie
                      if (abs(i - l) == 2) // czarne
                        turn ? pawnMoved[1] = j : pawnMoved[0] = j;
                      else if (abs(i - l) == 1)
                        turn ? pawnMoved[1] = 8 : pawnMoved[0] = 8;
                      // sprawdzam czy po postawieniu pionka stoi obok niego pionek, ktory wykonal ruch o dwa pola, czyli sprawdzam flage
                      // bicie w przelocie
                      if (pawnMoved[!turn] < 8){
                        // bije czarnym pionkiem białego pionka
                        if (turn && ((j == m + 1 && m == pawnMoved[!turn]) || (j == m - 1 && m == pawnMoved[!turn]))){
                          lightChessboard();
                          setLed(l - 1, m, CRGB(255,0,0));
                          FastLED.show();
                          while (readMux(l - 1, m) > whiteMagFieldStr){}
                          chessFieldState[l - 1][m] = 0;
                        }
                        // bije białym pionkiem czarnego pionka
                        else if (!turn && ((j == m + 1 && m == pawnMoved[!turn]) || (j == m - 1 && m == pawnMoved[!turn]))){
                          lightChessboard();
                          setLed(l + 1, m, CRGB(255,0,0));
                          FastLED.show();
                          while (readMux(l + 1, m) < blackMagFieldStr){}
                          chessFieldState[l + 1][m] = 0;
                        }
                      }
                    }
                    else if (i != l || j != m) 
                      turn ? pawnMoved[1] = 8 : pawnMoved[0] = 8;
                    //wywolanie funkcji, ktora sprawdzi czy król jest w trakcie roszady
                    checkIfCastle(raisedPieceName, i, j, m);

                    //ustawianie flagi, gdy ruszona zostanie wieża lub król w celu późniejszego sprawdzenia możliwości wykonania roszady
                    //warunek if sprawdza czy nie odłożyliśmy bierki w to samo miejsce
                    if(i != l || j != m){
                      switch (raisedPieceName){
                        case 5:   moved[0][1] = 1; break;
                        case 11:  moved[0][0] = 1; break;
                        case 1:   if (i == 0 && j == 0) moved[1][1] = 1;  else if (i == 0 && j == 7) moved[2][1] = 1; break;
                        case 7:  	if (i == 7 && j == 0) moved[1][0] = 1;  else if (i == 7 && j == 7) moved[2][0] = 1; break;
                      }
                      turn ? turn = 0 : turn = 1;
                      isGameOver();
                    }
                    
                    raisedPieceName = 0;
                    printChessboard();
                    // po odłożeniu bierki następuje zmiana kolejki zawodnika
                    turn ? Serial.println("Ruch czarnych") : Serial.println("Ruch białych");
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  FastLED.show();
}

void lightChessboard(){
  // startujemy od 0 (A1), inkrementujemy o 2
  for (int i = 0; i < 8; i += 2){
    for (int j = 0; j < 8; j++){
      for (int k = 0; k < 6; k++){
        if (j % 2 == 1){
          leds[ledMapArray[i][j][k]] = CRGB(255, 0, 255);
          leds[ledMapArray[i + 1][j][k]] = CRGB(0, 0, 0);
        }
        else {
          leds[ledMapArray[i][j][k]] = CRGB(0, 0, 0);
          leds[ledMapArray[i + 1][j][k]] = CRGB(255, 0, 255);
        }
      }
    }
  }
  FastLED.show();
}

void printChessboard(){
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
      Serial.print(readMux(i, j));
      Serial.print(" ");
      Serial.print(piecesNames[chessFieldState[i][j]]);
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
int readMux(int x, int y){
  int SIG_pin = muxMapArray[x][y][0];
  int channel = muxMapArray[x][y][1];
  // pętla ustawiająca 4 piny sygnałowe
  for(int i = 0; i < 4; i ++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }
  int val;
  if (SIG_pin == SIG_PIN1 && channel == 1){
    val = analogRead(15);
  }
  // odczytanie wartości z czujnika halla
  else
  	val = analogRead(SIG_pin);
  return val;
}

// funkcja wywołująca odpowiednią funkcję w zależności od podniesionej bierki
int allowedMoves(int piece, int x, int y, int checkOnly){
  int breakFlag;
  // warunek wielokrotnego wyboru, przyjmujący numer bierki (piece)
  switch(piece){
    case 1:  // black rook 
    case 7:  // white rook 
      breakFlag = queenCheck(piece, x, y, rookMoves, checkOnly);
      break;
    case 2:  // black knight 
    case 8:  // white knight 
      breakFlag = knightCheck(piece, x, y, checkOnly);
      break;
    case 3:  // black bishop  
    case 9:  // white bishop
			breakFlag = queenCheck(piece, x, y, bishopMoves, checkOnly);
      break;
    case 4:   // black queen  
    case 10:  // white queen
    	breakFlag = queenCheck(piece, x, y, bishopMoves, checkOnly);
      !breakFlag ? breakFlag = queenCheck(piece, x, y, rookMoves, checkOnly) : queenCheck(piece, x, y, rookMoves, checkOnly);
      break;
    case 5:   // black king
    case 11:  // white king
      breakFlag = kingCheck(piece, x, y, checkOnly);
      break;
    case 6:  // black pawns
    case 12: // white pawns
      breakFlag = pawnCheck(piece, x, y, checkOnly);
      break;
  }
  if (checkOnly){
    return breakFlag;
  }
  else{
    setLed(x, y, CRGB(255,255,0));
    FastLED.show();
    return 0;
  }
}

// funkcja podświetlająca pole o współrzędnych x,y
void setLed(int x, int y, CRGB c){
  if((x >= 0) && (x < 8) && (y >= 0) && (y < 8)){
    allowedMovesArr[x][y] = 1;
    // if (raisedPieceName < 6 && chessFieldState[x][y])
    for (int i = 0; i < 6; i++){
      leds[ledMapArray[x][7-y][i]] = c; 
    }
  }
}

// funkcja mapująca 6 ledów jako pojedyncze pole szachownicy
// TODO odpalić na połowie ledów
void mapLeds(){
  for (int y = 0; y < 8; y++){
    for (int x = 0; x < 8; x++){
      ledMapArray[x][y][0] = 0 + 48 * y + x * 3;
      ledMapArray[x][y][1] = 1 + 48 * y + x * 3;
      ledMapArray[x][y][2] = 2 + 48 * y + x * 3;
      ledMapArray[x][y][3] = 45 + 48 * y - x * 3;
      ledMapArray[x][y][4] = 46 + 48 * y - x * 3;
      ledMapArray[x][y][5] = 47 + 48 * y - x * 3;
      // ledMapArray[x][y][0] = 1 + 48 * y + x * 3;
      // ledMapArray[x][y][1] = 1 + 48 * y + x * 3;
      // ledMapArray[x][y][2] = 1 + 48 * y + x * 3;
      // ledMapArray[x][y][3] = 46 + 48 * y - x * 3;
      // ledMapArray[x][y][4] = 46 + 48 * y - x * 3;
      // ledMapArray[x][y][5] = 46 + 48 * y - x * 3;
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
int knight(int x, int y, int x1, int y1)
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
  if (chessFieldState[x][y] == 6){
    if (x1 == (x + 1) && (y1 == (y + 1) || y1 == (y - 1)) && !turn)
      allowed = 1;
    }
  else if (chessFieldState[x][y] == 12){
    if (x1 == (x - 1) && (y1 == (y - 1) || y1 == (y + 1)) && turn)
      allowed = 1;
  }
  return allowed;
}

// funkcja sprawdzająca, czy która bierka szachuje króla
bool checkIfCheck(int def, int x, int y){
  // return : 0 - brak szacha; 1 - szach
  // turn: 0 - ruch bialych; 1 - ruch czarnych
  int t = 0, i, j, xKing, yKing;
  // tablica pieces zawiera kolejne numery bierek, na pierwszym miejscu są bierki czarne,
  // a na drugi miejscu analogicznie bierki białe
  int pieces [5][2] = {{6, 12}, {2, 8}, {4, 10}, {1, 7}, {3, 9}};
  if(!turn && def){
    for (i = 0; i < 8; i++){
      for (j = 0; j < 8; j++){
        if (chessFieldState[i][j] == 11){
          xKing = i;
          yKing = j;
        }
      }
    }
  }
  else if (turn == 1 && def){
    for (i = 0; i < 8; i++){
      for (j = 0; j < 8; j++){
        if (chessFieldState[i][j] == 5){
          xKing = i;
          yKing = j;
        }
      }
    }
  }
  else {
    xKing = x;
    yKing = y;
  }
  for (i = 0; i <= 7; i++){
    for (j = 0; j <= 7; j++){
      // w turze białych sprawdzamy czy czarna bierka ma dostęp do białego króla (analogicznie w turze czarnych)
      // stąd uzależnienie tablicy pieces od turna
      if (chessFieldState[i][j] == pieces[0][turn]) //B: 6 //W: 12
        t = pawn(i, j, xKing, yKing);
      else if (chessFieldState[i][j] == pieces[1][turn]) //B: 2 //W: 8
        t = knight(i, j, xKing, yKing);
      else if (chessFieldState[i][j] == pieces[2][turn]) //B: 4 //W: 10
        t = queen(i, j, xKing, yKing);
      else if (chessFieldState[i][j] == pieces[3][turn]) //B: 1 //W: 7
        t = rook(i, j, xKing, yKing);
      else if (chessFieldState[i][j] == pieces[4][turn]) //B: 3 //W: 9
        t = bishop(i, j, xKing, yKing);
      if (t == 1){
        // check
        Serial.println("############################################################## szach ##############################################################\n");
        return 1;
      }
    } // for
  }   // for
  return 0;
}

// funkcja podświetlająca pola, na które może ruszyć się koń
int knightCheck(int knight, int x, int y, int checkOnly){
  int tmp, check;
	for (int i = 0; i < 8; i++){
    tmp = chessFieldState[x + knightMoves[i][0]][y + knightMoves[i][1]];
    chessFieldState[x + knightMoves[i][0]][y + knightMoves[i][1]] = knight;
    chessFieldState[x][y] = 0;
    check = checkIfCheck();
    chessFieldState[x + knightMoves[i][0]][y + knightMoves[i][1]] = tmp;
    if (!check && checkOnly){
      return 1;
    }
    else if (!check && !checkOnly){
      if(!tmp) setLed(x + knightMoves[i][0], y + knightMoves[i][1], CRGB(0,255,0));
      else if(turn && tmp > 6) setLed(x + knightMoves[i][0], y + knightMoves[i][1], CRGB(255,0,0));
      else if(!turn && tmp < 7) setLed(x + knightMoves[i][0], y + knightMoves[i][1], CRGB(255,0,0));
    }
  }
  return 0;
}

// funkcja podświetlająca pola, na które może ruszyć się pionek
int pawnCheck(int pawn, int x, int y, int checkOnly){
  int tmp[4], tmpOposon, check[6], pawnLine;
  turn ? pawnLine = 1 : pawnLine = 6;

  //sprawdzenie czy po ruchu pionka jest szach, petla wykonuje sie 4 razy dla 4 roznych mozliwych ruchow pionkiem
  for (int i = 0; i < 8; i+= 2){
    tmp[i / 2] = chessFieldState[x + pawnMoves[i][turn]][y + pawnMoves[i + 1][turn]];
    chessFieldState[x + pawnMoves[i][turn]][y + pawnMoves[i + 1][turn]] = pawn;
    chessFieldState[x][y] = 0;
    check[i / 2] = checkIfCheck();
    chessFieldState[x + pawnMoves[i][turn]][y + pawnMoves[i + 1][turn]] = tmp[i / 2];
    if (!check[i / 2] && checkOnly) return 1;
  }
  //sprawdzenie czy po zbiciu w przelocie wystąpi szach (uwzględnienie zniknięcia z szachownicy bitego pionka)
  if (pawnMoved[!turn] < 8){
    for (int j = 0; j < 2; j++){
      chessFieldState[x + pawnMoves[4 + 2 * j][turn]][y + pawnMoves[5 + 2 * j][turn]] = pawn;
      chessFieldState[x][y] = 0;
      tmpOposon = chessFieldState[x][y - 1 + 2 * j]; 
      chessFieldState[x][y - 1 + 2 * j] = 0; // pierwsza iteracja: y - 1 - znika pionek po lewej, druga iteracja: y + 1 - znika pionek po prawej
      check[5 - j] = checkIfCheck();
      chessFieldState[x + pawnMoves[4 + 2 * j][turn]][y + pawnMoves[5 + 2 * j][turn]] = tmp[2 + j];
      chessFieldState[x][y - 1 + 2 * j] = tmpOposon;
      if (!check[5 - j] && checkOnly) 
        return 1;
    }
  }
  if (!checkOnly){
    if (!check[0] && !tmp[0]){
    setLed(x + pawnMoves[0][turn], y, CRGB(0,255,0));
    if (!check[1] && x == pawnLine && !tmp[1])
      setLed(x + pawnMoves[2][turn], y, CRGB(0,255,0)); 
  }
  if (!check[2] && tmp[2] > turn * 6 && tmp[2] <= (turn + 1) * 6) 
    setLed(x + pawnMoves[4][turn], y + pawnMoves[5][turn], CRGB(255,0,0));
  if (!check[3] && tmp[3] > turn * 6 && tmp[3] <= (turn + 1) * 6) 
    setLed(x + pawnMoves[6][turn], y + pawnMoves[7][turn], CRGB(255,0,0));
  // bicie w przelocie
  if (!check[4] && !turn && x == 3 && pawnMoved[!turn] == y + 1)
    setLed(x - 1, y + 1, CRGB(255,0,0));
	else if (!check[4] && turn && x == 4 && pawnMoved[!turn] == y + 1)
    setLed(x + 1, y + 1, CRGB(255,0,0));
  if(!check[5] && !turn && x == 3 && pawnMoved[!turn] == y - 1)
    setLed(x - 1, y - 1, CRGB(255,0,0));
  else if (!check[5] && turn && x == 4 && pawnMoved[!turn] == y - 1)
    setLed(x + 1, y - 1, CRGB(255,0,0));
  }
	return 0;
}  

// funkcja podświetlająca pola, na które może ruszyć się wieża, goniec oraz hetman, w zależności od agrumentu moves
int queenCheck(int queen, int x, int y, int moves[][2], int checkOnly){
  int tmp, check, allowed = 1;
  for (int j = 0; j < 4; j++){
    for (int i = 1; x + i * moves[j][0] < 8 && y + i * moves[j][1] < 8 && x + i * moves[j][0] >= 0 && y + i * moves[j][1] >= 0; i++){
      tmp = chessFieldState[x + i * moves[j][0]][y + i * moves[j][1]];
      chessFieldState[x + i * moves[j][0]][y + i * moves[j][1]] = queen;
      chessFieldState[x][y] = 0;
      check = checkIfCheck();
      chessFieldState[x + i * moves[j][0]][y + i * moves[j][1]] = tmp;
      // if (!check && checkOnly) return 1;
      // jeśli brak szacha
      if (!check){
        if (checkOnly)
          return 1;
        // jeśli pole na ktore potencjalnie chcemy postawic figure jest puste zaświeć na zielono
        if(!tmp)
          setLed(x + i * moves[j][0], y + i * moves[j][1], CRGB(0,255,0));
        // jeśli na polu jest biała bierka i ruch czarnych to zaświeć pole na czerowno
        else if(tmp > 6 && turn){
          setLed(x + i * moves[j][0], y + i * moves[j][1], CRGB(255,0,0));
          break;
        } // czarna bierka i ruch czarnych
        else if(tmp <= 6 && turn)
          break;
         // czarna bierka i ruch białych 
        else if(tmp <= 6 && !turn){
          setLed(x + i * moves[j][0], y + i * moves[j][1], CRGB(255,0,0));
          break;
        } // biała bierka i ruch białych
        else 
          break;
      // jeśli na drodze stoi bierka to nie podświetlaj pól za nią (wieża nie może przeskakiwać figur)
      }
      else if(tmp)
        break;
    } 
  }
  return 0;
}

// funkcja podświetlająca pola, na które może ruszyć się król
int kingCheck(int king, int x, int y, int checkOnly){
  for (int i = 0; i < 8; i++){
    if((x + kingMoves[i][0]) < 8 && (x + kingMoves[i][0]) >= 0 && (y + kingMoves[i][1]) < 8 && (y + kingMoves[i][1]) >= 0){
      if (!checkOnly && !checkIfCheck(0, x + kingMoves[i][0], y + kingMoves[i][1])){
        if(!chessFieldState[x + kingMoves[i][0]][y + kingMoves[i][1]]){
          setLed(x + kingMoves[i][0], y + kingMoves[i][1], CRGB(0,255,0));
        } 
        else if(!turn && chessFieldState[x + kingMoves[i][0]][y + kingMoves[i][1]] <= 6) setLed(x + kingMoves[i][0], y + kingMoves[i][1], CRGB(255,0,0));
        else if(turn && chessFieldState[x + kingMoves[i][0]][y + kingMoves[i][1]] > 6) setLed(x + kingMoves[i][0], y + kingMoves[i][1], CRGB(255,0,0));
      }
      else if(!checkIfCheck(0, x + kingMoves[i][0], y + kingMoves[i][1]) && checkOnly) return 1;
    }
  }
  // sprawdzanie czy dostępna jest roszada
  // 0 białe, 1 czarne
  if (!checkOnly){
    if (!moved[0][turn] && !moved[1][turn] && !chessFieldState[x][y - 1] && !checkIfCheck(0, x, y - 1) && !chessFieldState[x][y - 2] && \
        !checkIfCheck(0, x, y - 2) && !chessFieldState[x][y - 3] && chessFieldState[x][y - 4] == (7 - (6 * turn)))
      setLed(x, y - 2, CRGB(0,255,0));
    if (!moved[0][turn] && !moved[2][turn] && !chessFieldState[x][y + 1] && !checkIfCheck(0, x, y + 1) && !chessFieldState[x][y + 2] && \
        !checkIfCheck(0, x, y + 2) && chessFieldState[x][y + 3] == (7 - (6 * turn)))
      setLed(x, y + 2, CRGB(0,255,0));
  }
  return 0;
}
                     
void pawnPromotion(int x, int y){
  if (turn){
    if (x == 7){
      //promocja na hetmana
      chessFieldState[x][y] = 4;
      //promocja na gońca
      // chessFieldState[x][y] = 3;
      //promocja na konia
      // chessFieldState[x][y] = 2;
      //promocja na wieżę
      // chessFieldState[x][y] = 1;
    }
  }
  else {
    if (x == 0){
      //promocja na hetmana
      chessFieldState[x][y] = 10;
      //promocja na gońca
      // chessFieldState[x][y] = 9;
      //promocja na konia
      // chessFieldState[x][y] = 8;
      //promocja na wieżę
      // chessFieldState[x][y] = 7;
    }
  }
}

// funkcja sprawdzająca czy roszada została rozpoczęta
void checkIfCastle(int piece, int i, int j, int m){
  if (piece == 11 - turn * 6){
    lightChessboard();
    if (j == 4 && i == 7 - turn * 7){
      if (m == 2 || m == 6)
        castle(7 - turn * 7, (m - 2) * 7/4, m / 2 + 2);
    }
  }
}

// funkcja wymuszjąca dokończenie roszady po ruchu królem
void castle(int x, int y, int y1){
  setLed(x, y, CRGB(255,255,0));
  setLed(x, y1, CRGB(0,255,0));
  FastLED.show();
  hall = readMux(x, y1);
  while (hall < whiteMagFieldStr && hall > blackMagFieldStr){ hall = readMux(x, y1); }
  chessFieldState[x][y] = 0;
  chessFieldState[x][y1] = 7  - turn * 6;
}
// koniec tury - sprawdzamy czy nie ma pata lub mata,
// gdy jest koniec tury czarnych sprawdzamy jakie są możliwe ruchy dla białych i odwrotnie
int checkStaleMate(){
  int tmp;
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++){
      tmp = chessFieldState[i][j];
      if (tmp && ((tmp < 7 && turn) || (tmp >= 7 && !turn))){
        chessFieldState[i][j] = 0;
        // sprawdzanie po ruchu bialych czy jakakolwiek czarna bierka ma jakikolwiek możliwy ruch 
        // (analogicznie po ruchu czarnych).
        if (allowedMoves(tmp, i, j, 1)){
          chessFieldState[i][j] = tmp;
          // funkcja zwraca 0 gdy istnieje możliwy ruch
          return 0;
        }
        else
          chessFieldState[i][j] = tmp;
      }
    }
  }
  // jeśli nie ma możliwego ruchu i chechIfCheck = 1 (król jest pod szachem) -> MAT return zwraca 1
  // jeśli nie ma możliwego ruchu i chechIfCheck = 0 (król nie jest pod szachem) -> PAT return zwraca 2
  return checkIfCheck() ? 1 : 2;
}

void makeEngineMove(){
  int xStart, yStart, xEnd, yEnd;
  // zapisywanie ruchu silnika do zmiennej
  strcpy(chessEngineReturn, chessEngine(turn, chessFieldState, moved));
  // translacja ruchu na współrzędne
  yStart = int(chessEngineReturn[0] - 97);
  xStart = asciiToMove[int(chessEngineReturn[1] - 49)];
  yEnd = int(chessEngineReturn[2] - 97);
  xEnd = asciiToMove[int(chessEngineReturn[3] - 49)];
  // wyświetlenie szachownicy
  serialBoard();
  // wykonanie ruchu silnika
  // podświetlenie ruchu
  setLed(xStart, yStart, CRGB(255,255,0));
  setLed(xEnd, yEnd, CRGB(0,255,0));
  FastLED.show();
  // oczekiwanie na podniesienie bierki z pola startowego i ogłożenie jej na pole końcowe
  hall = readMux(xStart, yStart);
  while ((!turn && hall > whiteMagFieldStr) || (turn && hall < blackMagFieldStr)){ hall = readMux(xStart, yStart); }
  hall = readMux(xEnd, yEnd);
  while ((!turn && hall < whiteMagFieldStr) || (turn && hall > blackMagFieldStr)){ hall = readMux(xEnd, yEnd); }
  // wykonanie ruchu w tablic
  chessFieldState[xEnd][yEnd] = chessFieldState[xStart][yStart];
  chessFieldState[xStart][yStart] = 0;
}

void isGameOver(){
  switch (checkStaleMate()){
    case 0: Serial.println("GRAMY DALEJ"); break;
    case 1: Serial.println("MAT"); break;
    case 2: Serial.println("PAT"); break; 
  }
}