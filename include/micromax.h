#ifndef MICROMAX_H
#define MICROMAX_H

char *chessEngine();
void setupEngine();
unsigned short myrand(void);
short D(short q, short l, short e, unsigned char E, unsigned char z, unsigned char n);
void gameOver();
void serialBoard();
void cfsToBoard(short chessFieldState[][8], int moved[][2], bool engineColor);
void boardToCfs();

extern char b[129];

#endif