#include <Arduino.h>
#include <micromax.h>
#include <vector>

#define EMPTY 0
#define WHITE 8
#define BLACK 16

#define STATE 64

/* make unique integer from engine move representation */
#define PACK_MOVE 256 * K + L;

#define W while
#define K(A, B) *(int *)(T + A + (B & 8) + S * (B & 7))
#define J(A) K(y + A, b[y]) - K(x + A, u) - K(H + A, t)

/* convert intger argument back to engine move representation */
#define UNPACK_MOVE(A)  \
    K = (A) >> 8 & 255; \
    L = (A)&255;

#define msTOsFactor 1000

/* Global variables visible to engine. Normally they */
/* would be replaced by the names under which these  */
/* are known to your engine, so that they can be     */
/* manipulated directly by the interface.            */

int StartKey;

int Side;
int Move;
int PromPiece;
int Result;
int TimeLeft;
int MovesLeft;
int MaxDepth;
int MaxMoves;
int Post = 1;
int Fifty;
int UnderProm;
char engineMove[4];

int Computer, MaxTime, m;

unsigned long long Ticks, tlim;
int TimeInc;

int GameHistory[1024];
char HistoryBoards[1024][STATE];
int GamePtr, HistPtr;

int U = (1 << 23) - 1;
struct _
{
    int K, V;
    char X, Y, D;
} *A; /* hash table, 16M+8 entries*/

int M = 136, S = 128, I = 8e3, Q, O, K, N, j, R, J, Z; /* M=0x88                   */

signed char L,
    w[] = {0, 2, 2, 7, -1, 8, 12, 23},                                      /* relative piece values    */
    o[] = {-16, -15, -17, 0, 1, 16, 0, 1, 16, 15, 17, 0, 14, 18, 31, 33, 0, /* step-vector lists */
           7, -1, 11, 6, 8, 3, 6,                                           /* 1st dir. in o[] per piece*/
           6, 3, 5, 7, 4, 5, 3, 6},                                         /* initial piece setup      */
    T[1035],                                                                /* hash translation table   */

    n[] = ".?pnkbrq?P?NKBRQ"; /* piece symbols on printout*/

char b[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 28, 21, 16, 13, 12, 13, 16, 21,
    0, 0, 0, 0, 0, 0, 0, 0, 22, 15, 10, 7, 6, 7, 10, 15,
    0, 0, 0, 0, 0, 0, 0, 0, 18, 11, 6, 3, 2, 3, 6, 11,
    0, 0, 0, 0, 0, 0, 0, 0, 16, 9, 4, 1, 0, 1, 4, 9,
    0, 0, 0, 0, 0, 0, 0, 0, 16, 9, 4, 1, 0, 1, 4, 9,
    0, 0, 0, 0, 0, 0, 0, 0, 18, 11, 6, 3, 2, 3, 6, 11,
    0, 0, 0, 0, 0, 0, 0, 0, 22, 15, 10, 7, 6, 7, 10, 15,
    0, 0, 0, 0, 0, 0, 0, 0, 28, 21, 16, 13, 12, 13, 16, 21, 0};

void cfsToBoard(short chessFieldState[][8], int moved[][2], bool engineColor)
{
    char cfsMap[] = {0, 22, 19, 21, 23, 20, 18, 14, 11, 13, 15, 12, 9};
    char piece;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            piece = cfsMap[chessFieldState[i][j]];
            if (piece == 0)
                b[j + 16 * i] = piece;
            else if ((piece == 18 && i == 1) || (piece == 9 && i == 6))
                b[j + 16 * i] = piece;
            // król, wieża, wieża
            else if ((piece == 20 && !moved[0][1]) || (piece == 22 && !moved[1][1] && i == 0) || (piece == 22 && !moved[2][1] && i == 7) ||
                     (piece == 12 && !moved[0][0]) || (piece == 14 && !moved[1][0] && i == 0) || (piece == 14 && !moved[2][0] && i == 7))
            {
                b[j + 16 * i] = piece;
            }
            else
                b[j + 16 * i] = piece | 32;
        }
    }
    engineColor ? Side = BLACK : Side = WHITE;
}

int D(int k, int q, int l, int e, int E, int z, int n) /* recursive minimax search, k=moving side, n=depth*/
{                                                      /* e=score, z=prev.dest; J,Z=hashkeys; return score*/
    int j, r, m, v, d, h, i, F, G, P, V, f = J, g = Z, C, s;
    char t, p, u, x, y, X, Y, H, B;
    struct _ *a = A + (J + k * E & U - 1); /* lookup pos. in hash table*/
    q -= q < e;
    l -= l <= e; /* adj. window: delay bonus */
    d = a->D;
    m = a->V;
    X = a->X;
    Y = a->Y;                                                                   /* resume at stored depth   */
    if (a->K - Z | z & 8 |                                                      /* miss: other pos. or empty*/
        !(m <= q | X & 8 && m >= l | X & S))                                    /*   or window incompatible */
        d = Y = 0;                                                              /* start iter. from scratch */
    X &= ~M;                                                                    /* start at best-move hint  */
    W(d++ < n || d < 3 ||                                                       /*** min depth = 2   iterative deepening loop */
      z == 8 & K == I && (millis() - Ticks < tlim & d <= MaxDepth & m < 7998 ||   /* root: deepen upto time   */
                          (K = X, L = Y & S - 9, d = 3)))                       /* time's up: go do best    */
    {
        x = B = X;                                                      /* start scan at prev. best */
        h = Y & S;                                                      /* request try noncastl. 1st*/
        P = d > 2 && l + I ? D(24 - k, -l, 1 - l, -e, S, S, d - 3) : I; /* search null move         */
        m = -P<l | R> 35 ? d - 2 ? -I : e : -P;                         /*** prune if > beta  unconsidered:static eval */
        N++;                                                            /* node count (for timing)  */
        do
        {
            u = b[x];  /* scan board looking for   */
            if (u & k) /*  own piece (inefficient!)*/
            {
                r = p = u & 7;                      /* p = piece type (set r>0) */
                j = o[p + 16];                      /* first step vector f.piece*/
                W(r = p > 2 & r < 0 ? -r : -o[++j]) /* loop over directions o[] */
                {
                A: /* resume normal after best */
                    y = x;
                    F = G = S; /* (x,y)=move, (F,G)=castl.R*/
                    do
                    {                              /* y traverses ray, or:     */
                        H = y = h ? Y ^ h : y + r; /* sneak in prev. best move */
                        if (y & M)
                            break;                                         /* board edge hit           */
                        m = E - S & b[E] && y - E < 2 & E - y < 2 ? I : m; /* bad castling  */
                        if (p < 3 & y == E)
                            H ^= 16; /* shift capt.sqr. H if e.p.*/
                        t = b[H];
                        if (t & k | p < 3 & !(y - x & 7) - !t)
                            break;                     /* capt. own, bad pawn mode */
                        i = 37 * w[t & 7] + (t & 192); /* value of capt. piece t   */
                        if (i < 0)
                            m = I, d = 98; /* K capture                */
                        if (m >= l & d > 1)
                            goto C; /* abort on fail high       */

                        v = d - 1 ? e : i - p; /*** MVV/LVA scoring if d=1**/
                        if (d - !t > 1)        /*** all captures if d=2  ***/
                        {
                            v = p < 6 ? b[x + 8] - b[y + 8] : 0; /* center positional pts.   */
                            b[G] = b[H] = b[x] = 0;
                            b[y] = u | 32; /* do move, set non-virgin  */
                            if (!(G & M))
                                b[F] = k + 6, v += 50;    /* castling: put R & score  */
                            v -= p - 4 | R > 30 ? 0 : 20; /*** freeze K in mid-game ***/
                            if (p < 3)                    /* pawns:                   */
                            {
                                v -= 9 * ((x - 2 & M || b[x - 2] - u) +                   /* structure, undefended    */
                                          (x + 2 & M || b[x + 2] - u) - 1                 /*        squares plus bias */
                                          + (b[x ^ 16] == k + 36))                        /*** cling to magnetic K ***/
                                     - (R >> 2);                                          /* end-game Pawn-push bonus */
                                i += V = y + r + 1 & S ? 647 - p : 2 * (u & y + 16 & 32); /* promotion / passer bonus */
                                b[y] += V;                                                /* upgrade P or convert to Q*/
                            }
                            J += J(0);
                            Z += J(8) + G - S;
                            v += e + i;
                            V = m > q ? m : q;                                /*** new eval & alpha    ****/
                            C = d - 1 - (d > 5 & p > 2 & !t & !h);            /* nw depth, reduce non-cpt.*/
                            C = R > 30 | P - I | d < 3 || t && p - 4 ? C : d; /* extend 1 ply if in-check */
                            do
                                s = C > 2 | v > V ? -D(24 - k, -l, -V, -v, /*** futility, recursive eval. of reply */
                                                       F, y, C)
                                                  : v;
                            W(s > q & ++C < d);
                            v = s;              /* no fail:re-srch unreduced*/
                            if (z & 8 && K - I) /* move pending: check legal*/
                            {
                                if (v + I && x == K & y == L) /*   if move found          */
                                {
                                    Q = -e - i;
                                    O = F;
                                    a->D = 99;
                                    a->V = 0;    /* lock game in hash as draw*/
                                    R += i >> 7; /*** total captd material ***/
                                    if ((b[y] & 7) != p && PromPiece == 'n')
                                        UnderProm = y;
                                    if ((b[y] & 7) != p)
                                    {
                                        Serial.println("tellics kibitz promotion\n");
                                        // fflush(stdout);
                                    };
                                    Fifty = t | p < 3 ? 0 : Fifty + 1;
                                    return l;
                                }      /*   & not in check, signal */
                                v = m; /* (prevent fail-lows on    */
                            }          /*   K-capt. replies)       */
                            J = f;
                            Z = g;
                            b[G] = k + 6;
                            b[F] = b[y] = 0;
                            b[x] = u;
                            b[H] = t;                    /* undo move,G can be dummy */
                        }                                /*          if non-castling */
                        if (v > m)                       /* new best, update max,best*/
                            m = v, X = x, Y = y | S & F; /* mark non-double with S   */
                        if (h)
                        {
                            h = 0;
                            goto A;
                        }                                              /* redo after doing old best*/
                        if (x + r - y | u & 32 |                       /* not 1st step,moved before*/
                            p > 2 & (p - 4 | j - 7 ||                  /* no P & no lateral K move,*/
                                     b[G = x + 3 ^ r >> 1 & 7] - k - 6 /* no virgin R in corner G, */
                                     || b[G ^ 1] | b[G ^ 2])           /* no 2 empty sq. next to R */
                        )
                            t += p < 5; /* fake capt. for nonsliding*/
                        else
                            F = y; /* enable e.p.              */
                    }
                    W(!t); /* if not capt. continue ray*/
                }
            }
        }
        W((x = x + 9 & ~M) - B); /* next sqr. of board, wrap */
    C:
        m = m + I | P == I ? m : 0;                             /*** check test thru NM  best loses K: (stale)mate*/
        if (a->D < 99)                                          /* protect game history     */
            a->K = Z, a->V = m, a->D = d,                       /* always store in hash tab */
                a->X = X | 8 * (m > q) | S * (m < l), a->Y = Y; /* move, type (bound/exact),*/
        if (z == 8 & Post)
        {
            // printf("%2d ", d - 2);
            // printf("%6d ", m);
            // printf("%8d %10d %c%c%c%c\n", (millis() - Ticks) / 10, N,
            //        'a' + (X & 7), '8' - (X >> 4), 'a' + (Y & 7), '8' - (Y >> 4 & 7)),
            //     fflush(stdout);

            *engineMove = 'a' + (X & 7);
            engineMove[1] = '8' - (X >> 4);
            engineMove[2] = 'a' + (Y & 7);
            engineMove[3] = '8' - (Y >> 4 & 7);

            Serial.println();
            Serial.print(d - 2);
            Serial.print("\t");
            Serial.print(m);
            Serial.print("\t");
            Serial.print(N);
            Serial.print("\t");
            Serial.print(millis() - Ticks);
            Serial.print("\t");
            Serial.print(*engineMove);
            Serial.print(engineMove[1]);
            Serial.print(engineMove[2]);
            Serial.println(engineMove[3]);
        }
    } /*    encoded in X S,8 bits */
    if (z == S + 1)
        K = X, L = Y & ~M;
    return m += m < e; /* delayed-loss bonus       */
}

void gameOver()
{
    Serial.println("MAT");
}

void serialBoard()
{
    Serial.println("\n  +-----------------+");
    for (int i = 0; i < 8; i++)
    {
        Serial.print(' ');
        Serial.print(8 - i);
        Serial.print("| ");
        for (int j = 0; j < 8; j++)
        {
            char c = n[b[16 * i + j] & 15];
            Serial.print(c);
            Serial.print(' ');
        }
        Serial.println('|');
    }
    Serial.println("  +-----------------+");
    Serial.println("    a b c d e f g h");
}

void setupEngine()
{
    // MAIN SETUP
    // jeśli funkcja main() zostanie wywołana z więcej niż 1 parametrem (if (argc > 1))
    // oraz uda się ten parametr przypisać do zmiennej m
    // to wykonywane jest polecenie: U = (1 << m) - 1;
    // czyli trzzeba ustawić m na jakąś wartość przed wykonianiem polecenia.
    // if (argc > 1 && sscanf(argv[1], "%d", &m) == 1)
    // przesunięcie bitowe 1 w lewo o m miejsc i odjęcie 1
    // domyślnie U jest zadeklarowane na: int U = (1 << 23) - 1;
    m = 8; // m = 0 -> U = 0; m = 1 => U = 1; m = 2 => U = 3; m = 3 => U = 7; ...
    U = (1 << m) - 1;
    Serial.print("\nU: ");
    Serial.println(U);
    A = (struct _ *)calloc(U + 1, sizeof(struct _));

    // INIT ENGINE
    N = 1035;
    W(N-- > M)
    T[N] = rand() >> 9;

    // INIT GAME
    // Side = WHITE;
    Q = 0;
    O = S;
    Fifty = R = 0;
    UnderProm = -1;

    Computer = EMPTY;
    MaxDepth = 30;   /* maximum depth of your search */

    m = MovesLeft <= 0 ? 40 : MovesLeft;

    tlim = 0.6 * (TimeLeft + (m - 1) * TimeInc) / (m + 7);
    // tlim = 3000;
    PromPiece = 'q';
    N = 0;
    K = I;

    // MaxMoves, MaxTime, TimeInc ustawia użytkownik
    MaxMoves = 0;
    MaxTime = 1;
    TimeInc = 2;
    
    MovesLeft = MaxMoves = 0;
    TimeLeft = MaxTime = 60000 * MaxTime + 1000 ;
    TimeInc *= msTOsFactor;

    // U = (1 << m) - 1;
    // Serial.print("\nU: ");
    // Serial.println(U);
    // A = (struct _ *)calloc(U + 1, sizeof(struct _));
}

char *chessEngine()
{   
    m = MovesLeft <= 0 ? 40 : MovesLeft;
    tlim = 0.6 * (TimeLeft + (m - 1) * TimeInc) / (m + 7);
    Serial.print("Time limit: ");
    Serial.println(tlim);
    Serial.print("Moves left: ");
    Serial.println(MovesLeft);
    // tlim = 3000;
    PromPiece = 'q';
    N = 0;
    K = I;

    
    Serial.print("\ndepth");
    Serial.print("\t");
    Serial.print("score");
    Serial.print("\t");
    Serial.print("nodes");
    Serial.print("\t");
    Serial.print("time");
    Serial.print("\t");
    Serial.println("move");


    Ticks = millis();
    D(Side, -I, I, Q, O, 8, 3);

    m = millis() - Ticks;

    /* time-control accounting */
    TimeLeft -= m;
    TimeLeft += TimeInc;

    if (--MovesLeft == 0)
    {
        MovesLeft = MaxMoves;
        if (MaxMoves == 1)
            TimeLeft = MaxTime;
        else
            TimeLeft += MaxTime;
    }

    Serial.print("Time left: ");
    Serial.print(TimeLeft / 1000);
    Serial.println(" s.");

    // return {engineMove, b};
    serialBoard();
    return engineMove;
}

// void boardToCfs(){
//   int boardMap[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 8, 11, 9, 7, 10, 0, 0, 6, 2, 5, 3, 1, 4};
//   for (int i = 0; i < 8; i++){
//     for (int j = 0; j < 8; j++){
//         Serial.print(int(b[16 * i + j] & 15));
//         Serial.print(" ");
//         chessFieldState[i][j] = boardMap[int(b[16 * i + j] & 31)];
//     }
//     Serial.println();
//   }
// }
