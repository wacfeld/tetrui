// -*- mode: c++ -*-

#include "defs.h"
#include "drawer.h"
#include "movement.h"
#include "turn.h"
#include "score.h"


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

// random number generator distribution
std::uniform_int_distribution<int> dist(0,6);

enum mode gmode = VERSUS;

struct player *gplayers = NULL;
int cur_player = 0; // index of current player

// hold slot. NONE means empty (start of game)
// enum type ghold = NONE;

// the queue, which is always full, determines what pieces come next
// it itself is supplied using 7bag(), fullrand(), etc.
// enum type gqueue[queue_len];

// 0 = empty
// L, J, S, Z, I, O, T = respective colored mino
// enum type gboard[tot_width][tot_height+1];
// guideline: there is a 10x20 buffer area above visible play area
// the +1 is there so that the line clear code can operate (it stays as NONE the whole time)

// bool gchanged[tot_width][tot_height]; // keeps track of which things need changing

// enum state {PLAYING, LOST};
// enum state gstate = PLAYING;

// // update both gboard and gchanged
// void changeboard(int x, int y, enum type t)
// {
//   gboard[x][y] = t;
//   gchanged[x][y] = 1;
// }

// returns true if piece touching ground
bool stuck(struct piece &p, int dx, int dy)
{
  // save copy and then delete piece from board
  enum type saved[4];
  int i = 0;
  for(auto &m : p.p)
  {
    saved[i++] = gplayers[cur_player].board[m[0]][m[1]];
    gplayers[cur_player].board[m[0]][m[1]] = NONE;
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
    gplayers[cur_player].board[m[0]][m[1]] = saved[i++];
    // gboard[m[0]][m[1]] = p.t;
    // changeboard(m[0], m[1], p.t);
  }

  return g;
}

bool grounded(struct piece &p)
{
  return stuck(p, 0, -1);
}

// set state variables for next turn
#define nextturn() do {\
    canhold = true;\
    locking = false;\
    lastgrav = curtime;\
  } while(0)

int main(int argc, char **args)
{
  // initialize players
  gplayers = new struct player[gmode];

  // init RNG
  // srand(time(NULL));

  // seed all RNGs with the same seed
  {
    auto t = time(NULL);
    for(int i = 0; i < gmode; i++)
    {
      gplayers[i].gen.seed(t);
    }
  }
  
  // initialize window
  init("tetrui", SCREEN_WIDTH, SCREEN_HEIGHT);
  SDL_FillRect( gsurf, NULL, SDL_MapRGB( gsurf->format, 0xFF, 0xFF, 0xFF ) );

  // load sprites and put them in sprites[]
  initsprites();

  // draw hold, board, queue
  initscreen();

  enum type (*qmeth)(bool reset) = bag7;
  bool (*tspinmeth)(struct piece &) = threecornerT;

  bool dosplash = true;
  if(dosplash)
  {
    splash(qmeth, 20, 300);
  }

  // reset queue
  // qmeth(true);
  
  // abbreviation for convenience
  int &pl = cur_player;

  // fill & draw the queue
  for(pl = 0; pl < gmode; pl++)
  {
    for(int i = 0; i < queue_len; i++)
    {
      gplayers[pl].queue[i] = qmeth(false);
    }
  }
  pl = 0;
  
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

  // todo list:
  // DONE ghost piece
  // TODO proper move reset
  // TODO DAS/ARR
  // TODO edit mode
  // sync queues


  for(pl = 0; pl < gmode; pl++)
  {
    // grab first piece from queue, draw everything
    enum type t = queuenext(qmeth);
    gplayers[pl].p = spawnpiece(t);
    drawpiece(gplayers[pl].p);
    drawqueue();

    gplayers[pl].ghost = drawghost(gplayers[pl].p);
  }
  pl = 0;
  
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
      movepiece(gplayers[pl].p, 0, -1, 0);

      // update screen
      SDL_UpdateWindowSurface( gwin );
    }

    // if touching the ground, check for lockdown
    if(grounded(gplayers[pl].p) && doground)
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
        gplayers[pl].p = nextpiece(gplayers[pl].p, qmeth, tspinmeth);
        undrawghost(gplayers[pl].ghost);
        gplayers[pl].ghost = drawghost(gplayers[pl].p);
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
          moved = movepiece(gplayers[pl].p, 0, -1, 0);
        }
        else if(sym == binds.l)
        {
          moved = movepiece(gplayers[pl].p, -1, 0, 0);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.r)
        {
          moved = movepiece(gplayers[pl].p, 1, 0, 0);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.ccw)
        {
          moved = rotatepiece(gplayers[pl].p, CCW, srs);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.f)
        {
          moved = rotatepiece(gplayers[pl].p, FLIP, srs);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.cw)
        {
          moved = rotatepiece(gplayers[pl].p, CW, srs);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }

        else if(sym == binds.h)
        {
          if(canhold)
          {
            // perform hold
            gplayers[pl].p = swaphold(gplayers[pl].p, qmeth);

            // update ghost
            undrawghost(gplayers[pl].ghost);
            gplayers[pl].ghost = drawghost(gplayers[pl].p);

            // draw
            drawpiece(gplayers[pl].p);
            drawqueue(); // queue might have been updated if ghold == NONE originally
            drawholdpiece(gplayers[cur_player].hold);
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
          movepiece(gplayers[pl].p, 0, -1, true); // move down repeatedly

          gplayers[pl].p = nextpiece(gplayers[pl].p, qmeth, tspinmeth);

          // update ghost for new piece
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);

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
            gplayers[pl].p.lastrot = true;
          else
            gplayers[pl].p.lastrot = false;
        }

        SDL_UpdateWindowSurface( gwin );
      }
    }
  }

  close();
  return 0;
}

// guideline https://tetris.wiki/Tetris_Guideline
