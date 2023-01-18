#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "defs.h"

bool goodcoords(int x, int y);
const char *suffix(unsigned long long n);
bool srs(struct piece &p, enum rot r);
bool rotatepiece(struct piece &p, enum rot r, bool (*rmeth)(struct piece &p, enum rot r));
bool movepiece(struct piece &p, int dx, int dy, bool rep);

#endif
