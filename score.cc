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
