#include "turn.h"
#include "defs.h"
#include "movement.h"
#include "score.h"

// game is over if buffer exceeded or piece spawn creates collision
bool topout(struct piece &p)
{
  // TODO check for exceeding buffer (only possible in competitive)

  // puts("topout?");
  for(auto &m : p.p)
  {
    if(gboard[m[0]][m[1]] != NONE) // spawn mino collides with existing mino
    {
      // puts("yes!");
      return 1;
    }
  }

  // puts("no!");
  return 0;
}

// run until told to quit by user, no other input is processed
void lose()
{
  fprintf(stderr, "game over\n");

  SDL_Event e;

  while(1)
  {
    while( SDL_PollEvent( &e ) != 0 )
    {
      //User requests quit
      if( e.type == SDL_QUIT )
      {
        close();
        fprintf(stderr, "quitting\n");
        exit(0);
      }
    }
  }
}

// given top-left coords, return piece with correct type and mino/center coords
struct piece placepiece(int x, int y, enum type t)
{
  struct piece p;
  p.r = ZERO; // initially no rotation
  p.t = t;

  // spawn on 21st row (0-indexed)
  // center of rotation below piece (or as low as possible)
  p.c = {2*x, 2*y};

  if(t == I)
  {
    p.p = {ISHAPE};
    p.c[0] += ICENT[0];
    p.c[1] += ICENT[1];
    // p.c = ICENT;
  }
  else if(t == J)
  {
    p.p = {JSHAPE};
    p.c[0] += CENT[0];
    p.c[1] += CENT[1];
    // p.c = CENT;
  }
  else if(t == L)
  {
    p.p = {LSHAPE};
    p.c[0] += CENT[0];
    p.c[1] += CENT[1];
    // p.c = CENT;
  }
  else if(t == S)
  {
    p.p = {SSHAPE};
    p.c[0] += CENT[0];
    p.c[1] += CENT[1];
    // p.c = CENT;
  }
  else if(t == Z)
  {
    p.p = {ZSHAPE};
    p.c[0] += CENT[0];
    p.c[1] += CENT[1];
    // p.c = CENT;
  }
  else if(t == O)
  {
    p.p = {OSHAPE};
    p.c[0] = OCENT[0];
    p.c[0] = OCENT[1];
    // p.c = OCENT;
  }
  else if(t == T)
  {
    p.p = {TSHAPE};
    p.c[0] += CENT[0];
    p.c[1] += CENT[1];
    // p.c = CENT;
  }
  else // error
  {
    error("mino type %d does not exist\n", t);
  }

  // add x, y to mino coords
  for(auto &m : p.p)
  {
    m[0] += x;
    m[1] += y;
  }

  return p;
}

// spawn a piece above the playing field according to guideline
// see https://tetris.fandom.com/wiki/SRS?file=SRS-pieces.png
struct piece spawnpiece(enum type t)
{
  struct piece p = placepiece(SX, SY, t);

  // process garbage
  // TODO

  // check for topout
  if(topout(p))
  {
    // indicate lost and return
    // gstate = LOST;
    lose();
    // return p;
  }

  // write piece onto board
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = t;
    // changeboard(m[0], m[1], t);
  }

  // move piece down
  movepiece(p, 0, -1, false);

  return p;
}

// used to shuffle bags
void swap(enum type &a, enum type &b)
{
  enum type temp = a;
  a = b;
  b = temp;
}

// get a single piece type using the 7-bag method
// uses static variables to maintain state
enum type bag7(bool reset)
{
  static enum type bag[7];
  static int siz = 0;

  if(reset)
  {
    siz = 0;
    return NONE; // return type doesn't matter
  }
  
  // bag is empty
  if(siz == 0)
  {
    // fill with 1 of each piece
    bag[siz++] = I;
    bag[siz++] = J;
    bag[siz++] = L;
    bag[siz++] = S;
    bag[siz++] = Z;
    bag[siz++] = T;
    bag[siz++] = O;

    // shuffle
    for(int i = 0; i < 200; i++)
    {
      int j = rand() % 7;
      int k = rand() % 7;
      swap(bag[j], bag[k]);
    }
  }

  // dish out 1 piece
  siz--;
  return bag[siz];
}

// most primitive method (randomly select pieces independently)
enum type fullrand()
{
  static const enum type pieces[] = {I, J, L, S, Z, O, T};
  static const int npieces = sizeof(pieces) / sizeof(*pieces);
  return pieces[rand() % npieces];
}

// return next piece from gqueue, shift queue forward
enum type queuenext(enum type (*qmeth)(bool reset))
{
  // grab next piece type
  enum type next = gqueue[0];

  // shift rest of queue forward
  for(int i = 0; i < queue_len - 1; i++)
  {
    gqueue[i] = gqueue[i+1];
  }

  // put new piece at back of queue using given method
  gqueue[queue_len - 1] = qmeth(false);

  return next;
}

struct piece swaphold(struct piece &p, enum type (*qmeth)(bool reset))
{
  enum type t; // type of new piece

  if(ghold == NONE) // empty hold, grab from queue
  {
    t = queuenext(qmeth);
    ghold = p.t;
  }
  else // swap with hold
  {
    t = ghold;
    ghold = p.t;
  }

  // delete old piece and undraw
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = NONE;
  }
  undrawpiece(p);

  // spawn new piece & return it
  struct piece q = spawnpiece(t);
  return q;
}

// check for line clears, spawn new piece and draw
// spawn next piece (using whatever selection process) and draw
struct piece nextpiece(struct piece &old, enum type (*qmeth)(bool reset), bool (*tspinmeth)(struct piece &))
{
  // check for line clears
  std::set<int, std::greater<int>> rows; // rows to clear
  // std::greater puts the ints in descending order, which makes clearing the lines easire later
  for(auto &m : old.p) // loop through 4 minoes and check their rows
  {
    if(!rows.count(m[1]))
    {
      // check if line is full
      bool full = true;
      for(int i = 0; i < tot_width; i++)
      {
        if(gboard[i][m[1]] == NONE)
        {
          full = false;
        }
      }

      // if full, add to rows
      if(full)
      {
        rows.insert(m[1]);
      }
    }
  }

  // calculate clear
  struct clear cl = {0, false, false};
  cl.lines = rows.size();
  cl.tspin = threecornerT(old);
  // cl.pc will be set after clears are executed

  // execute the clears & redraw
  // perhaps placing the column loop outside improves locality of reference
  for(int i = 0; i < tot_width; i++)
  {
    // clear this column
    // this isn't the most efficient way to clear multiple lines at a time but it shouldn't be a big deal
    for(int r : rows) // iterate in descending order (thanks to std::greater)
    {
      // gboard contains an extra row at the very top so this works
      memmove(&gboard[i][r], &gboard[i][r+1], sizeof(**gboard) * (tot_height - r));
    }

    // redraw column
    for(int j = 0; j < tot_height; j++)
    {
      // gchanged[i][j] = 1;
      reboardmino(bX, bY, gboard[i][j], i, j);
    }
  }

  // if bottom row empty, then perfect clear has occurred
  bool pc = true;
  for(int i = 0; i < tot_width; i++)
  {
    if(gboard[i][0] != NONE)
      pc = false;
  }
  cl.pc = pc;
  putclear(cl);

  // get next piece, spawn, draw, draw queue, and return
  enum type t = queuenext(qmeth);
  struct piece p = spawnpiece(t);
  
  drawpiece(p);
  drawqueue();
  
  return p;
}
