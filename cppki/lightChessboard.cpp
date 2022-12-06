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