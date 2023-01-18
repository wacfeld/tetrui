#include "score.h"
#include "defs.h"
#include "drawer.h"
#include "movement.h"

// print clear attributes to screen
void putclear(struct clear &c)
{
  if(c.tspin)
    fprintf(stderr, "tspin ");

  if(c.lines == 0)
    return;
  else if(c.lines == 1)
    fprintf(stderr, "single!");
  else if(c.lines == 2)
    fprintf(stderr, "double!");
  else if(c.lines == 3)
    fprintf(stderr, "triple!");
  else if(c.lines == 4)
    fprintf(stderr, "quad!");
  else
    error("c.lines is %d\n", c.lines);

  if(c.pc)
    printf(" perfect clear!");

  putchar('\n');
}

// a twist is recognized (for any piece, not just T) if the piece cannot move down, left, right, or up
// does not require rotation
// line clear is not handlede here
bool immobile(struct piece &p)
{
  // the 0,-1 case is equivalent to being grounded
  return stuck(p, 1, 0) && stuck(p, -1, 0) && stuck(p, 0, -1) && stuck(p, 0, 1);
}

// if a T piece was rotated and has 3 full corners, a twist is recognized
bool threecornerT(struct piece &p)
{
  if(p.t != T) return false;

  if(p.lastrot == false) return false;

  // get center coords
  int x = p.c[0]/2;
  int y = p.c[0]/2;

  // check 4 corners
  int corns = 0; // number of occupied corners
  const static int offs[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
  for(int i = 0; i < 4; i++)
  {
    if(!goodcoords(x+offs[i][0], y+offs[i][1]))
    {
      corns++;
    }
  }

  if(corns >= 3)
    return true;

  return false;
}

// TODO implement tspin mini recognition
// tspin mini
// if a T piece is rotated & kicked into a tight space but can still move up, then it's a tspin mini
// bool miniT(struct piece &p)
// {
//   if(p.t != T)
//     return false;

//   if(!p.lastrot)
//     return false;

//   if(!p.lastkick)
//     return false;

//   // grounded, stuck left and right, but not up
//   if(stuck(p, -1, 0) && stuck(p, 1, 0) && stuck(p, 0, -1))
//   {
//     return true;
//   }

//   return false;
// }

int guidelinecombo(struct clear &cl)
{
  const static int combotable[13] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5};
  const static int combotablelen = sizeof(combotable) / sizeof(*combotable);
  const static int combomax = 5; // maxes out at 5

  static int combo = -1;

  if(cl.lines == 0) // nothing was cleared; reset combo & exit
  {
    combo = -1;
    putd(combo);
    return 0;
  }

  // something was cleared; increment combo and calculate combo garbage
  combo++;

  putd(combo);
  if(combo < combotablelen)
    return combotable[combo];
  else
    return combomax;
}

int guidelinebtb(struct clear &cl)
{
  static bool btb = false; // whether the last clear was a 'difficult clear'

  int lines = 0;

  if(cl.lines == 0) // non-clears don't affect btb
  {
    putd(btb);
    return 0;
  }
  
  else if(cl.tspin && cl.mini) // any tspin mini -> +1
  {
    if(btb)
      lines = 1;
    btb = true;
  }

  else if(cl.tspin && cl.lines == 1) // tspin single -> +1
  {
    if(btb)
      lines = 1;
    btb = true;
  }

  else if(cl.tspin && cl.lines == 2) // tspin double -> +2
  {
    if(btb)
      lines = 2;
    btb = true;
  }

  else if(cl.lines == 4) // quad -> +2
  {
    if(btb)
      lines = 2;
    btb = true;
  }

  else if(cl.tspin && cl.lines == 3) // tspin triple -> +3
  {
    if(btb)
      lines = 3;
    btb = true;
  }

  else // non-difficult line clear, reset btb
  {
    lines = 0;
    btb = false;
  }

  putd(btb);
  return lines;
}

// garbage is called on every lock down (even if nothing is cleared), so that it can keep track of combo
int garbage(struct clear &cl, int (*combometh)(struct clear &), int (*btbmeth)(struct clear &))
{
  int lines = 0;

  if(cl.pc) // perfect clear -> +10 lines
    lines += 10;

  // no tspin
  if(!cl.tspin)
  {
    if(cl.lines == 0) // no clear -> 0
      ;
    else if(cl.lines == 1) // single -> 0
      ;
    else if(cl.lines == 2) // double -> 1
      lines += 1;
    else if(cl.lines == 3) // triple -> 2
      lines += 2;
    else if(cl.lines == 4) // quad -> 4
      lines += 4;
    else
      error("invalid line count: %d\n", cl.lines);
  }

  // tspin
  else
  {
    if(cl.mini)
    {
      fprintf(stderr, "tspin mini has not been implemented yet\n");
    }
    else
    {
      if(cl.lines == 1) // tspin single -> 2
        lines += 2;
      else if(cl.lines == 2) // tspin double -> 4
        lines += 4;
      else if(cl.lines == 3) // tspin triple -> 6
        lines += 6;
      else
        error("invalid tspin line count: %d\n", cl.lines);
    }
  }

  // add combo garbage
  lines += combometh(cl);

  // add btb garbage
  lines += btbmeth(cl);

  return lines;
}
