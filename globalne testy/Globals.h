#include <vector>
#ifndef MICROMAX_H
#define MICROMAX_H

std::vector<std::vector<char>> chessEngine();
void setupEngine();
unsigned short myrand(void);
short D(short q, short l, short e, unsigned char E, unsigned char z, unsigned char n);
void gameOver();
void serialBoard();
void cfsToBoard(short chessFieldState[][8], int moved[][2], bool engineColor);

#endif