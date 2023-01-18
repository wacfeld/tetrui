#ifndef TURN_H
#define TURN_H

#include "defs.h"
#include "drawer.h"

// pieces spawn in some or all of columns 4, 5, 6, 7 (rounded left)
// and rows 21, 22 (rounded down)
// this is the bottom-left corner of that box
#define SX 3
#define SY 20

// #define SY1 (SY0+1)
// #define SX1 (SX0+1)
// #define SX2 (SX0+2)
// #define SX3 (SX0+3)

// spawn center for J, L, S, Z, T
// (4, 20) doubled
// #define CENT {8, 40}
const int CENT[] = {2, 0};

// spawn center for O
// (4.5, 20.5) doubled
const int OCENT[] = {3, 1};

// spawn center for I
// (4.5, 19.5) doubled
const int ICENT[] = {3, -1};

// starting shapes & orientations in terms of offsets from bottom-left corner
// used to spawn pieces, populate the queue, hold, etc.
#define ISHAPE {{0, 0}, {1, 0}, {2, 0}, {3, 0}}
#define JSHAPE {{0, 1}, {0, 0}, {1, 0}, {2, 0}}
#define LSHAPE {{0, 0}, {1, 0}, {2, 0}, {2, 1}}
#define SSHAPE {{0, 0}, {1, 0}, {1, 1}, {2, 1}}
#define ZSHAPE {{0, 1}, {1, 1}, {1, 0}, {2, 0}}
#define OSHAPE {{1, 0}, {1, 1}, {2, 0}, {2, 1}}
#define TSHAPE {{0, 0}, {1, 0}, {1, 1}, {2, 0}}


bool topout(struct piece &p);
void lose();
struct piece placepiece(int x, int y, enum type t);
struct piece spawnpiece(enum type t);

void swap(enum type &a, enum type &b);
enum type bag7(bool reset);
enum type fullrand();
enum type queuenext(enum type (*qmeth)(bool reset));

struct piece swaphold(struct piece &p, enum type (*qmeth)(bool reset));

struct piece nextpiece(struct piece &old, enum type (*qmeth)(bool reset), bool (*tspinmeth)(struct piece &));

#endif
