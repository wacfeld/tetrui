// -*- mode: c++ -*-

#include "defs.h"
#include "drawer.h"
#include "movement.h"
#include "turn.h"
#include "score.h"
#include "bot.h"


// arrows move + WASD + shift + space
struct keybinds arr_wasd =
{
  // keycodes: https://wiki.libsdl.org/SDL2/SDL_Keycode
  .cmd = SDLK_SLASH,

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

const int garb_batch = 8; // maximum amount of pending garbage a player can receive at once

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
  // // save copy and then delete piece from board
  // enum type saved[4];
  // int i = 0;
  // for(auto &m : p.p)
  // {
  //   saved[i++] = gplayers[cur_player].board[m[0]][m[1]];
  //   gplayers[cur_player].board[m[0]][m[1]] = NONE;
  //   // changeboard(m[0], m[1], NONE);
  // }

  bool g = false;

  // inspect the cell just below every mino
  for(auto &m : p.p)
  {
    if(!goodcoords(m[0] + dx, m[1] + dy))
    {
      g = true;
    }
  }

  // // reinstate piece, if it ever existed
  // i = 0;
  // for(auto &m : p.p)
  // {
  //   gplayers[cur_player].board[m[0]][m[1]] = saved[i++];
  //   // gboard[m[0]][m[1]] = p.t;
  //   // changeboard(m[0], m[1], p.t);
  // }

  // if(g)
  //   puts("stuck!");
  return g;
}

bool grounded(struct piece &p)
{
  return stuck(p, 0, -1);
}

// set state variables for next turn
#define nextturn() do {\
    gplayers[pl].canhold = true;\
    gplayers[pl].locking = false;\
    gplayers[pl].lastgrav = curtime;\
  } while(0)

int main(int argc, char **args)
{
  // initialize players
  gplayers = new struct player[gmode];

  // general purpose random generator (e.x. for garbage holes)
  srand(time(NULL));

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

  
  // struct piece p = spawnpiece(I);
  // SDL_UpdateWindowSurface( gwin );
  // auto poss = possible(p);
  // putd(poss.size());
  // for(auto &q : poss)
  // {
  //   fprintf(stderr, "x:%2d y:%2d r:%d lr:%d lk:%d\n", q.c[0], q.c[1], q.r, q.lastrot, q.lastkick);
  //   printf("%d\n", q.t);
  //   drawpiece(q);
  // SDL_UpdateWindowSurface( gwin );
    
  //   getchar();
  //   undrawpiece(q);
  // SDL_UpdateWindowSurface( gwin );
  // }
  // getchar();
  // return 0;

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
  // DONE sync queues


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
  // uint lastgrav = SDL_GetTicks(); // time of last gravity tick
  for(pl = 0; pl < gmode; pl++)
  {
    gplayers[pl].lastgrav = SDL_GetTicks();
  }
  pl = 0;

  uint gravdelay = 1000; // ms between gravity ticks
  uint curtime; // current time in ms
  bool dograv = true;

  // lock down
  // int movecount = 0;
  uint lockdelay = 500; // ms before piece locks
  // uint lastreset = 0; // when locktimer >= lockdelay, piece locks
  // bool locking = false; // becomes true whenever piece is touching the ground
  bool doground = true;

  // after hold is used once, set to false until lock down
  // bool canhold = true;

  bool quit = false;
  SDL_Event e;

  // different from cur_player. player who is currently being controlled by keyboard
  int cont_player = 0;

  while(!quit)
  {
    // update time
    curtime = SDL_GetTicks();

    for(pl = 0; pl < gmode; pl++)
    {
      // skip lost players
      if(gplayers[pl].lost)
        continue;

// check gravity timing
      if(curtime - gplayers[pl].lastgrav >= gravdelay && dograv)
      {
        // update timer
        gplayers[pl].lastgrav = curtime;

        // move down once
        undrawpiece(gplayers[pl].p);
        if(movepiece(gplayers[pl].p, 0, -1, 0))
        {
          // gplayers[pl].p.lastrot = false;
          // gplayers[pl].p.lastkick = false;
        }
        drawpiece(gplayers[pl].p);
        
        // update screen
        SDL_UpdateWindowSurface( gwin );
      }

      // if touching the ground, check for lockdown
      if(grounded(gplayers[pl].p) && doground)
      {
        // currently infinity
        // TODO change to move reset

        if(!gplayers[pl].locking) // start timer
        {
          // puts("grounded");
          gplayers[pl].locking = true;
          gplayers[pl].lastreset = curtime;
        }

        // timer exceeded lock delay, lock down
        if(curtime - gplayers[pl].lastreset >= lockdelay)
        {
          // write piece to board
          for(auto &m : gplayers[pl].p.p)
          {
            gplayers[pl].board[m[0]][m[1]] = gplayers[pl].p.t;
          }
          
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
        gplayers[pl].locking = false;
      }
    }
    pl = 0;

    pl = cont_player;
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

        // enter a command
        if(sym == binds.cmd)
        {
  auto poss = possible(gplayers[pl].p);
  putd(poss.size());
  for(auto &q : poss)
  {
    fprintf(stderr, "x:%2d y:%2d r:%d lr:%d lk:%d\n", q.c[0], q.c[1], q.r, q.lastrot, q.lastkick);
    printf("%d\n", q.t);
    drawpiece(q);
  SDL_UpdateWindowSurface( gwin );
    
    getchar();
    undrawpiece(q);
  SDL_UpdateWindowSurface( gwin );
  }
  getchar();
          
          // char cmd[1000];
          // fprintf(stderr, "> ");
          // fgets(cmd, 1000, stdin);

          // // remove trailing newline
          // cmd[strlen(cmd)-1] = 0;

          // if(!strcmp(cmd, "next"))
          // {
            // cont_player++;
            // cont_player %= gmode;
            // fprintf(stderr, "now controlling player %d\n", cont_player);
          // }

          continue;
        }

        // skip lost players
        if(gplayers[pl].lost)
          continue;

        bool moved = false;

        // translation
        // if(sym == SDLK_UP)
        // {
        //   moved = movepiece(p, 0, 1, 0);
        // }
        if(sym == binds.sd)
        {
          undrawpiece(gplayers[pl].p);
          moved = movepiece(gplayers[pl].p, 0, -1, 0);
          drawpiece(gplayers[pl].p);
        }
        else if(sym == binds.l)
        {
          undrawpiece(gplayers[pl].p);
          moved = movepiece(gplayers[pl].p, -1, 0, 0);
          drawpiece(gplayers[pl].p);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.r)
        {
          undrawpiece(gplayers[pl].p);
          moved = movepiece(gplayers[pl].p, 1, 0, 0);
          drawpiece(gplayers[pl].p);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.ccw)
        {
          undrawpiece(gplayers[pl].p);
          moved = rotatepiece(gplayers[pl].p, CCW, srs);
          drawpiece(gplayers[pl].p);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.f)
        {
          undrawpiece(gplayers[pl].p);
          moved = rotatepiece(gplayers[pl].p, FLIP, srs);
          drawpiece(gplayers[pl].p);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }
        else if(sym == binds.cw)
        {
          undrawpiece(gplayers[pl].p);
          moved = rotatepiece(gplayers[pl].p, CW, srs);
          drawpiece(gplayers[pl].p);
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);
        }

        else if(sym == binds.h)
        {
          if(gplayers[pl].canhold)
          {
            // perform hold
            undrawpiece(gplayers[pl].p);
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
            gplayers[pl].canhold = false; // but don't reset this one

            continue;
          }
          else
          {
            fprintf(stderr, "you have already used the hold once this turn\n");
          }
        }

        else if(sym == binds.hd)
        {
          undrawpiece(gplayers[pl].p);
          if(movepiece(gplayers[pl].p, 0, -1, true)) // move down repeatedly
          {
            // gplayers[pl].p.lastrot = false;
            // gplayers[pl].p.lastkick = false;
          }
          drawpiece(gplayers[pl].p);

          // write piece to board
          for(auto &m : gplayers[pl].p.p)
          {
            gplayers[pl].board[m[0]][m[1]] = gplayers[pl].p.t;
          }

          gplayers[pl].p = nextpiece(gplayers[pl].p, qmeth, tspinmeth);

          // update ghost for new piece
          undrawghost(gplayers[pl].ghost);
          gplayers[pl].ghost = drawghost(gplayers[pl].p);

          SDL_UpdateWindowSurface(gwin);

          nextturn();
          continue;
        }

        // reset lock timer
        if(moved && gplayers[pl].locking)
        {
          gplayers[pl].lastreset = curtime;
        }

        // // update whether last move was rotation or not
        // if(moved)
        // {
        //   if(sym == binds.cw || sym == binds.ccw || sym == binds.f)
        //     gplayers[pl].p.lastrot = true;
        //   else
        //     gplayers[pl].p.lastrot = false;
        // }

        SDL_UpdateWindowSurface( gwin );
      }
    }
  }

  close();
  return 0;
}

// guideline https://tetris.wiki/Tetris_Guideline
