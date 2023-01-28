#ifndef DRAWER_H
#define DRAWER_H

#include "defs.h"

const int vis_height = 20;
// const int vis_height = 23;
const int vis_width = 10;

extern SDL_Window *gwin;
extern SDL_Surface *gsurf;
extern std::array<SDL_Surface *, 12> sprites;

const int MINO_LEN = 16;

// hold area top-left corner
const int hX = 0;
const int hY = 0;
const int hold_width = 6;
const int hold_height = 20;

// graphical board top-left corner
const int bX = hX + MINO_LEN * hold_width;
const int bY = 0;

// queue (to the right of board)
const int qX = bX + MINO_LEN * vis_width;
const int qY = 0;

const int queue_width = 6; // number of minoes
const int queue_height = vis_height;
const int queue_diff = 3; // number of minoes between pieces in hold

// bottom-left corner of top queue piece
const int QCORN[] = {1,17};

// bottom-left corner of hold piece
const int HCORN[] = {1,17};

// board is 10x20
const int spacing = 1;
const int player_width = MINO_LEN*(hold_width + vis_width + queue_width);
const int SCREEN_WIDTH = gmode * player_width + (gmode) * spacing * MINO_LEN;
const int SCREEN_HEIGHT = MINO_LEN*vis_height;

#define playerX(num) ((num+1) * spacing * MINO_LEN + num * (player_width))

// gscreen is kept synchronized with physical screen
// reboardmino() consults this to see what needs updating
/* extern enum type gscreen[tot_width][tot_height]; */

void close();

SDL_Surface *loadBMP(const char *name);
void init(const char *title, int w, int h);

void blitmino(int X, int Y, enum type t, int col, int row);
void boardmino(int X, int Y, enum type t, int col, int row);
void reboardmino(int X, int Y, enum type t, int col, int row);
void queuemino(int X, int Y, enum type t, int col, int row);
void drawpiece(struct piece &p);
void undrawpiece(struct piece &p);

void initsprites();
void initscreen();
void wait(uint ms);
void splash(enum type (*qmeth)(bool reset), uint d1, uint d2);


void drawholdpiece(enum type t);
void drawqueuepiece(int place, enum type t);
void drawqueue();

void undrawghost(struct piece &p);
struct piece drawghost(struct piece &p);

void skullmeter(int pl, int c);

void redrawboard(int X, int Y);
#endif
