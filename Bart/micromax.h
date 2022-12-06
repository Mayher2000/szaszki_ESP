#ifndef MICROMAX_H
#define MICROMAX_H

char* chessEngine(bool turn, short chessFieldState[][8], int moved[][2]);
unsigned short myrand(void);
short D(short q, short l, short e, unsigned char E, unsigned char z, unsigned char n);
void gameOver();
void serialBoard();

#endif