#ifndef SCORE_H
#define SCORE_H
#include "defs.h"

void putclear(struct clear &c);
bool immobile(struct piece &p);
bool threecornerT(struct piece &p);


int guidelinecombo(struct clear &cl);
int guidelinebtb(struct clear &cl);
int garbage(struct clear &cl, int (*combometh)(struct clear &), int (*btbmeth)(struct clear &));

bool ispc();
#endif
