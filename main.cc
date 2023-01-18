// -*- mode: c++ -*-

#include "defs.h"
#include "drawer.h"
#include "movement.h"


// arrows move + WASD + shift + space
struct keybinds arr_wasd =
{
  .hd = SDLK_SPACE,
  .h  = SDLK_LSHIFT,

  .l  = SDLK_LEFT,
  .r  = SDLK_RIGHT,
  .sd = SDLK_DOWN,

  .ccw = SDLK_a,
  .cw = SDLK_d,
  .f  = SDLK_s
};

// hold slot. NONE means empty (start of game)
enum type ghold = NONE;

// the queue, which is always full, determines what pieces come next
// it itself is supplied using 7bag(), fullrand(), etc.
enum type gqueue[queue_len];

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

// pieces spawn in some or all of columns 4, 5, 6, 7 (rounded left)
// and rows 21, 22 (rounded down)
// this is the bottom-left corner of that box
#define SX 3
#define SY 20

// #define SY1 (SY0+1)
// #define SX1 (SX0+1)
// #define SX2 (SX0+2)
// #define SX3 (SX0+3)

// starting shapes & orientations in terms of offsets from bottom-left corner
// used to spawn pieces, populate the queue, hold, etc.
#define ISHAPE {{0, 0}, {1, 0}, {2, 0}, {3, 0}}
#define JSHAPE {{0, 1}, {0, 0}, {1, 0}, {2, 0}}
#define LSHAPE {{0, 0}, {1, 0}, {2, 0}, {2, 1}}
#define SSHAPE {{0, 0}, {1, 0}, {1, 1}, {2, 1}}
#define ZSHAPE {{0, 1}, {1, 1}, {1, 0}, {2, 0}}
#define OSHAPE {{1, 0}, {1, 1}, {2, 0}, {2, 1}}
#define TSHAPE {{0, 0}, {1, 0}, {1, 1}, {2, 0}}

// 0 = empty
// L, J, S, Z, I, O, T = respective colored mino
enum type gboard[tot_width][tot_height+1];
// guideline: there is a 10x20 buffer area above visible play area
// the +1 is there so that the line clear code can operate (it stays as NONE the whole time)

// bool gchanged[tot_width][tot_height]; // keeps track of which things need changing

enum state {PLAYING, LOST};
enum state gstate = PLAYING;

// // update both gboard and gchanged
// void changeboard(int x, int y, enum type t)
// {
//   gboard[x][y] = t;
//   gchanged[x][y] = 1;
// }

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
    gstate = LOST;
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

// returns true if piece touching ground
bool stuck(struct piece &p, int dx, int dy)
{
  // save copy and then delete piece from board
  enum type saved[4];
  int i = 0;
  for(auto &m : p.p)
  {
    saved[i++] = gboard[m[0]][m[1]];
    gboard[m[0]][m[1]] = NONE;
    // changeboard(m[0], m[1], NONE);
  }

  bool g = false;

  // inspect the cell just below every mino
  for(auto &m : p.p)
  {
    if(!goodcoords(m[0] + dx, m[1] + dy))
    {
      g = true;
    }
  }

  // reinstate piece, if it ever existed
  i = 0;
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = saved[i++];
    // gboard[m[0]][m[1]] = p.t;
    // changeboard(m[0], m[1], p.t);
  }

  return g;
}

bool grounded(struct piece &p)
{
  return stuck(p, 0, -1);
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

void undrawghost(struct piece &p)
{
  for(auto &m : p.p)
  {
    boardmino(bX, bY, gboard[m[0]][m[1]], m[0], m[1]);
  }
}

// given current piece, draw a ghost piece for it
struct piece drawghost(struct piece &p)
{
  // make copy
  struct piece q = p;

  // delete piece from board temporarily
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = NONE;
  }

  // move the copy down until it's grounded
  while(!grounded(q))
  {
    // decrement all y coords
    for(auto &m : q.p)
    {
      m[1]--;
    }
  }

  // replace piece on board
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = p.t;
  }

  // draw ghost piece wherever not occupied already
  for(auto &m : q.p)
  {
    if(gboard[m[0]][m[1]] == NONE)
    {
      boardmino(bX, bY, GHOST, m[0], m[1]);
    }
  }

  // return for later undrawing
  return q;
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
struct piece nextpiece(struct piece &old, enum type (*qmeth)(bool reset))
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
  cl.tspin = immobile(old);
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

// set state variables for next turn
#define nextturn() do {\
    canhold = true;\
    locking = false;\
    lastgrav = curtime;\
  } while(0)

int main(int argc, char **args)
{
  // init RNG
  srand(time(NULL));

  // initialize window
  init("tetrui", SCREEN_WIDTH, SCREEN_HEIGHT);
  SDL_FillRect( gsurf, NULL, SDL_MapRGB( gsurf->format, 0xFF, 0xFF, 0xFF ) );

  // load sprites and put them in sprites[]
  initsprites();

  // draw hold, board, queue
  initscreen();

  enum type (*qmeth)(bool reset) = bag7;

  bool dosplash = true;
  if(dosplash)
  {
    splash(qmeth, 20, 300);
  }

  // reset queue
  qmeth(true);
  
  // fill & draw the queue
  for(int i = 0; i < queue_len; i++)
  {
    gqueue[i] = qmeth(false);
  }

  // for(int i = 0; i < 10; i++)
  // {
  //   boardmino(bX, bY, I, i, 0);
  //   boardmino(bX, bY, I, i, 1);
  //   gboard[i][0] = I;
  //   gboard[i][1] = I;
  // }
  // gboard[5][0] = NONE;
  // gboard[6][0] = NONE;
  // gboard[6][1] = NONE;
  // gboard[7][1] = NONE;
  // boardmino(bX, bY, NONE, 5, 0);
  // boardmino(bX, bY, NONE, 6, 0);
  // boardmino(bX, bY, NONE, 6, 1);
  // boardmino(bX, bY, NONE, 7, 1);
  // gqueue[0] = S;

  // bugs:
  // - [X] trying to twist S into above setup sometimes deletes blocks
  // - [ ] key inputs are cancelling each other

  // todo:
  // - ghost piece
  // - proper move reset
  // - DAS/ARR


  // grab first piece from queue, draw everything
  enum type t = queuenext(qmeth);
  struct piece p = spawnpiece(t);
  drawpiece(p);
  drawqueue();

  struct piece ghost = drawghost(p);

  // boardmino(bX, bY, GHOST, 0, 0);

  SDL_UpdateWindowSurface( gwin );

  struct keybinds binds = arr_wasd;

  // start the game

  // gravity
  uint lastgrav = SDL_GetTicks(); // time of last gravity tick
  uint gravdelay = 1000; // ms between gravity ticks
  uint curtime; // current time in ms
  bool dograv = true;

  // lock down
  // int movecount = 0;
  uint lockdelay = 500; // ms before piece locks
  uint lastreset = 0; // when locktimer >= lockdelay, piece locks
  bool locking = false; // becomes true whenever piece is touching the ground
  bool doground = true;

  // after hold is used once, set to false until lock down
  bool canhold = true;

  bool quit = false;
  SDL_Event e;

  while(!quit)
  {
    // update time
    curtime = SDL_GetTicks();

    // check gravity timing
    if(curtime - lastgrav >= gravdelay && dograv)
    {
      // update timer
      lastgrav = curtime;

      // move down once
      movepiece(p, 0, -1, 0);

      // update screen
      SDL_UpdateWindowSurface( gwin );
    }

    // if touching the ground, check for lockdown
    if(grounded(p) && doground)
    {
      // currently infinity
      // TODO change to move reset

      if(!locking) // start timer
      {
      // puts("grounded");
        locking = true;
        lastreset = curtime;
      }

      // timer exceeded lock delay, lock down
      if(curtime - lastreset >= lockdelay)
      {
        // spawn new piece, reset variables, and continue
        p = nextpiece(p, qmeth);
        undrawghost(ghost);
        ghost = drawghost(p);
        SDL_UpdateWindowSurface(gwin);

        nextturn();
        // canhold = true;
        // locking = false;
        // lastgrav = curtime;

        continue;
      }
    }
    else
    {
      locking = false;
    }

    while( SDL_PollEvent( &e ) != 0 )
    {
      //User requests quit
      if( e.type == SDL_QUIT )
      {
        quit = true;
        fprintf(stderr, "quitting\n");
      }

      else if( e.type == SDL_KEYDOWN )
      {
        // keycodes: https://wiki.libsdl.org/SDL2/SDL_Keycode
        auto sym = e.key.keysym.sym;

        bool moved = false;

        // translation
        // if(sym == SDLK_UP)
        // {
        //   moved = movepiece(p, 0, 1, 0);
        // }
        if(sym == binds.sd)
        {
          moved = movepiece(p, 0, -1, 0);
        }
        else if(sym == binds.l)
        {
          moved = movepiece(p, -1, 0, 0);
          undrawghost(ghost);
          ghost = drawghost(p);
        }
        else if(sym == binds.r)
        {
          moved = movepiece(p, 1, 0, 0);
          undrawghost(ghost);
          ghost = drawghost(p);
        }
        else if(sym == binds.ccw)
        {
          moved = rotatepiece(p, CCW, srs);
          undrawghost(ghost);
          ghost = drawghost(p);
        }
        else if(sym == binds.f)
        {
          moved = rotatepiece(p, FLIP, srs);
          undrawghost(ghost);
          ghost = drawghost(p);
        }
        else if(sym == binds.cw)
        {
          moved = rotatepiece(p, CW, srs);
          undrawghost(ghost);
          ghost = drawghost(p);
        }

        else if(sym == binds.h)
        {
          if(canhold)
          {
            // perform hold
            p = swaphold(p, qmeth);

            // update ghost
            undrawghost(ghost);
            ghost = drawghost(p);

            // draw
            drawpiece(p);
            drawqueue(); // queue might have been updated if ghold == NONE originally
            drawholdpiece(ghold);
            SDL_UpdateWindowSurface(gwin);

            // reset state variables
            nextturn();
            canhold = false; // but don't reset this one

            continue;
          }
          else
          {
            fprintf(stderr, "you have already used the hold once this turn\n");
          }
        }

        else if(sym == binds.hd)
        {
          movepiece(p, 0, -1, true); // move down repeatedly

          p = nextpiece(p, qmeth);

          // update ghost for new piece
          undrawghost(ghost);
          ghost = drawghost(p);

          SDL_UpdateWindowSurface(gwin);

          nextturn();
          continue;
        }

        // reset lock timer
        if(moved && locking)
        {
          lastreset = curtime;
        }

        // update whether last move was rotation or not
        if(moved)
        {
          if(sym == binds.cw || sym == binds.ccw || sym == binds.f)
            p.lastrot = true;
          else
            p.lastrot = false;
        }

        SDL_UpdateWindowSurface( gwin );
      }
    }
  }

  close();
  return 0;
}

// guideline https://tetris.wiki/Tetris_Guideline
