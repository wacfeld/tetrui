// -*- mode: c++ -*-

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <time.h>
#include <set>

#define putd(x) do{ printf(#x ": %d\n", x); } while(0)
#define error(fmt, ...) do { fprintf(stderr, "%s: %d: %s: " fmt, __FILE__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); close(); exit(1); } while(0)

typedef unsigned char uchar;
typedef unsigned int uint;

//Screen dimension constants
// my screen is 1366x768

const int MINO_LEN = 32;

// total (including buffer) and visible dimensions of board
const int tot_height = 40;
const int tot_width = 10;

const int vis_height = 20;
// const int vis_height = 23;

const int vis_width = 10;

// rotation directions
enum rot {CW, FLIP, CCW}; // FLIP means 180

// NONE specifies that a square is empty
enum type {NONE=0, I=1, J=2, L=3, S=4, Z=5, O=6, T=7, QBG=8};

// a piece is a type, a center of rotation, and 4 minoes
struct piece
{
  enum type t;
  // center of rotation coords may be in-between minoes (for I and O)
  // hence c[2] coords are stored as double their actual value
  std::array<uchar, 2> c;
  std::array<std::array<uchar, 2>, 4> p;
};

// struct winsurf
// {
//   SDL_Window *win;
//   SDL_Surface *surf;
// };

// struct winsurf ws = {NULL, NULL};

SDL_Window *gwin;
SDL_Surface *gsurf;
std::array<SDL_Surface *, 9> sprites;

// graphical board top-left corner
const int bX = 0;
const int bY = 0;

// queue (to the right of board)
const int queue_width = 6; // number of minoes
const int queue_height = vis_height;
const int qX = MINO_LEN * vis_width;
const int qY = 0;

// board is 10x20
const int SCREEN_WIDTH = MINO_LEN*vis_width + queue_width * MINO_LEN;
const int SCREEN_HEIGHT = MINO_LEN*vis_height;

// spawn center for J, L, S, Z, T
// remember coords are doubled
#define CENT {8, 40}
// spawn center for O
#define OCENT {9, 41}
// spawn center for I
#define ICENT {9, 39}

// rows 21, 22
#define SY0 20
#define SY1 (SY0+1)

// columns 4, 5, 6, 7
#define SX0 3
#define SX1 (SX0+1)
#define SX2 (SX0+2)
#define SX3 (SX0+3)

// shapes in terms of offsets
#define ISHAPE

// 0 = empty
// L, J, S, Z, I, O, T = respective colored mino
enum type gboard[tot_width][tot_height+1];
// guideline: there is a 10x20 buffer area above visible play area
// the +1 is there so that the line clear code can operate (it stays as NONE the whole time)

// gscreen is kept synchronized with physical screen
// reblitmino() consults this to see what needs updating
enum type gscreen[tot_width][tot_height];

// bool gchanged[tot_width][tot_height]; // keeps track of which things need changing

enum state {PLAYING, LOST};
enum state gstate = PLAYING;

// // update both gboard and gchanged
// void changeboard(int x, int y, enum type t)
// {
//   gboard[x][y] = t;
//   gchanged[x][y] = 1;
// }

/* void close(struct winsurf ws, std::vector<SDL_Surface *> surfs) */
void close()
{
  return;
  SDL_DestroyWindow(gwin);
  gwin = NULL;
  gsurf = NULL;

  for(SDL_Surface *s : sprites)
  {
    SDL_FreeSurface(s);
    s = NULL;
  }

  //Quit SDL subsystems
  SDL_Quit();
}

SDL_Surface *loadBMP(const char *name)
{
  SDL_Surface *bmp = SDL_LoadBMP(name);
  if(!bmp)
  {
    error( "Unable to load image %s! SDL Error: %s\n", name, SDL_GetError());
  }

  SDL_Surface *opt = SDL_ConvertSurface( bmp, gsurf->format, 0 );
  if( opt == NULL )
  {
    error( "Unable to optimize image %s! SDL Error: %s\n", name, SDL_GetError() );
  }

  SDL_FreeSurface( bmp );
  // sprites.push_back(opt);
  return opt;
}

void init(const char *title, int w, int h)
{

  //Initialize SDL
  if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
  {
    error( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
  }
  else
  {
    //Create window
    gwin = SDL_CreateWindow( title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN );
    if( gwin == NULL )
    {
      error( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
    }

    gsurf = SDL_GetWindowSurface( gwin );
  }
}

// given top-left corner of board, surface and coords on tetris board, scale and place mino (does not update surface)
void blitmino(int X, int Y, enum type t, int col, int row)
{
  // puts("hi");
  // printf("%d %d\n", col, row);
  // check bounds
  if(row >= vis_height || col >= vis_width || row < 0 || col < 0)
  {
    // fprintf(stderr, "out of bounds: %d %d\n", row, col);
    return;
  }

  // convert bottom-left-based coords to top-left-based for graphics
  int srow = vis_height - row - 1;
  // printf("%d %d\n", srow, col);

  SDL_Rect dest;
  dest.w = MINO_LEN;
  dest.h = MINO_LEN;
  dest.x = X+col * MINO_LEN;
  dest.y = Y+srow * MINO_LEN;

  // printf("!!! %d %d\n", dest.x, dest.y);

  // update gscreen alongside actual screen
  gscreen[col][row] = t;
  SDL_BlitScaled(sprites[t], NULL, gsurf, &dest);
}

// after the initial covering of the board in NONE,
// we can be efficient by only blitting things that have changed on gboard
void reblitmino(int X, int Y, enum type t, int col, int row)
{
  // only blit if contents of cell have changed
  // useful for being efficient during line clears
  if(gscreen[col][row] != gboard[col][row])
  {
    // puts("blitting");
    blitmino(X, Y, t, col, row);
  }
  else
  {
    // puts("skipping blit");
  }

  // changed[col][row] = 0;
}

// given a piece, draw the parts of it that are on the visible play area
// (don't draw any parts in the buffer area)
// (don't update screen)
void drawpiece(struct piece &p)
{
  for(auto &m : p.p)
  {
    // puts("hey");
    reblitmino(bX, bY, p.t, m[0], m[1]);
  }
}

void undrawpiece(struct piece &p)
{
  for(auto &m : p.p)
  {
    reblitmino(bX, bY, NONE, m[0], m[1]);
  }
}

// return false if out of bounds, or collides with existing mino
bool goodcoords(int x, int y)
{
  if(x < 0 || y < 0 || x >= tot_width || y >= tot_height
     || gboard[x][y] != NONE)
  {
    // puts("these coords are NOT good");
    return false;
  }

  return true;
}

// rotate piece
// TODO: kicks, etc.
// currently only rotates around center
// returns true if successfully rotated (rotating O always returns true)
bool rotatepiece(struct piece &p, enum rot r)
{
  // delete piece from board
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = NONE;
    // changeboard(m[0], m[1], NONE);
  }

  // copy the coords and double them to match up with center
  auto cp = p.p;
  for(auto &m : cp)
  {
    m[0] *= 2;
    m[1] *= 2;
  }

  // grab center coords
  int cx = p.c[0];
  int cy = p.c[1];

  // rotate all minos around the center
  for(auto &m : cp)
  {
    // diff := mino - center
    int dx = m[0] - cx;
    int dy = m[1] - cy;

    // rotate difference vector
    if(r == CCW) // swap then negate x coord
    {
      int temp = dx;
      dx = -dy;
      dy = temp;
    }
    if(r == CW) // swap then negate y coord
    {
      int temp = dy;
      dy = -dx;
      dx = temp;
    }
    if(r == FLIP) // negate both coords
    {
      // puts("flipping");
      dy = -dy;
      dx = -dx;
    }

    // mino := center + newdiff
    m[0] = cx + dx;
    m[1] = cy + dy;

    // halve coords
    m[0] /= 2;
    m[1] /= 2;
  }

  // check for collisions/out of bounds
  for(auto &m : cp)
  {
    if(!goodcoords(m[0], m[1]))
    {
      // TODO deal with kicks, etc.
      // in the meantime, simply return failure
      return false;
    }
  }

  // undraw piece
  undrawpiece(p);

  // update piece & board
  p.p = cp;
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = p.t;
    // changeboard(m[0], m[1], p.t);
  }

  // redraw piece
  drawpiece(p);

  return true;
}

// move piece according to delta (do not draw)
// if rep is true, move until it hits a barrier
// returns false if failed to move piece at all
bool movepiece(struct piece &p, int dx, int dy, bool rep)
{
  // delete piece from board (otherwise it will 'collide' with itself)
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = NONE;
    // changeboard(m[0], m[1], NONE);
  }

  // make a copy of the coords & center
  auto cp = p.p;
  auto cc = p.c;

  bool moved = false;
  do
  {
    for(auto &m : cp) // update position
    {
      // printf("%d %d\n", m[0], m[1]);
      m[0] += dx;
      m[1] += dy;

      // check for out of bounds/collisions
      if(!goodcoords(m[0], m[1]))
      {
        goto stopmoving;
      }
    }

    // update position of center
    cc[0] += 2*dx;
    cc[1] += 2*dy;

    if(!moved) // on first successful move, undraw piece
    {
      undrawpiece(p);
      moved = true;
    }

    p.p = cp;
    p.c = cc;

  } while(rep);
stopmoving:

  // update board, redraw piece
  for(auto &m : p.p)
  {
    // printf("%d %d\n", m[0], m[1]);
    gboard[m[0]][m[1]] = p.t;
    // changeboard(m[0], m[1], p.t);
  }

  if(moved)
  {
    drawpiece(p);
  }

  return moved;
}

// game is over if buffer exceeded or piece spawn creates collision
bool topout(struct piece &p)
{
  // TODO check for exceeding buffer (only possible in competitive)

  for(auto &m : p.p)
  {
    if(gboard[m[0]][m[1]] != NONE) // spawn mino collides with existing mino
    {
      return 1;
    }
  }

  return 0;
}

// run until told to quit by user, no other input is processed
void lose()
{
  SDL_Event e;

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

// spawn a piece above the playing field according to guideline
// see https://tetris.fandom.com/wiki/SRS?file=SRS-pieces.png
struct piece spawnpiece(enum type t)
{
  struct piece p;
  p.t = t;

  // spawn on 21st row (0-indexed)
  // center of rotation below piece (or as low as possible)

  if(t == I)
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX2, SY0}, {SX3, SY0}}};
    p.c = ICENT; // (4.5, 19.5) doubled
  }
  else if(t == J)
  {
    p.p = {{{SX0, SY1}, {SX0, SY0}, {SX1,SY0}, {SX2,SY0}}};
    p.c = CENT;
  }
  else if(t == L)
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX2, SY0}, {SX2, SY1}}};
    p.c = CENT;
  }
  else if(t == S)
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX1, SY1}, {SX2, SY1}}};
    p.c = CENT;
  }
  else if(t == Z)
  {
    p.p = {{{SX0, SY1}, {SX1, SY1}, {SX1, SY0}, {SX2, SY0}}};
    p.c = CENT;
  }
  else if(t == O)
  {
    p.p = {{{SX1, SY0}, {SX1, SY1}, {SX2, SY0}, {SX2, SY1}}};
    p.c = OCENT;
  }
  else if(t == T)
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX1, SY1}, {SX2, SY0}}};
    p.c = CENT;
  }
  else // error
  {
    error("could not spawn mino %d\n", t);
  }

  // process garbage
  // TODO

  // check for topout
  if(topout(p))
  {
    // indicate lost and return
    gstate = LOST;
    return p;
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
bool grounded(struct piece &p)
{
  // delete piece from board temporarily
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = NONE;
    // changeboard(m[0], m[1], NONE);
  }

  bool g = false;

  // inspect the cell just below every mino
  for(auto &m : p.p)
  {
    if(!goodcoords(m[0], m[1]-1))
    {
      g = true;
    }
  }

  // reinstate piece
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = p.t;
    // changeboard(m[0], m[1], p.t);
    
  }

  return g;
}

// whatever method (7-bag, etc.) selects the next piece
struct piece pickpiece()
{
  // spawn new piece
  static enum type pieces[] = {I, J, L, S, Z, O, T};
  static int npieces = sizeof(pieces) / sizeof(*pieces);

  struct piece p = spawnpiece(pieces[rand() % npieces]);
  return p;
}

// check for line clears, spawn new piece and draw
// spawn next piece (using whatever selection process) and draw
struct piece nextpiece(struct piece &old)
{
  // TODO implement 7-bag
  // TODO check for tetrises, tspins, etc.

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
      reblitmino(bX, bY, gboard[i][j], i, j);
    }
  }

  // pick next piece, draw, and return
  struct piece p = pickpiece();
  drawpiece(p);
  
  return p;
}

// // at start of game, no old piece, so this version does not do any clears
// struct piece nextpiece()
// {
//   puts("hi");
//   // pick next piece, draw, and return
//   struct piece p = pickpiece();
//   drawpiece(p);

//   printf("%d\n", p.t);
  
//   return p;
// }

void initsprites()
{
  // load sprites
  SDL_Surface *ispr = loadBMP("sprites/I.bmp");
  SDL_Surface *jspr = loadBMP("sprites/J.bmp");
  SDL_Surface *lspr = loadBMP("sprites/L.bmp");
  SDL_Surface *sspr = loadBMP("sprites/S.bmp");
  SDL_Surface *zspr = loadBMP("sprites/Z.bmp");
  SDL_Surface *ospr = loadBMP("sprites/O.bmp");
  SDL_Surface *tspr = loadBMP("sprites/T.bmp");
  SDL_Surface *bgspr = loadBMP("sprites/bg.bmp"); // board background
  SDL_Surface *qbgspr = loadBMP("sprites/qbg.bmp"); // queue background

  sprites[NONE] = bgspr;
  sprites[I] = ispr;
  sprites[J] = jspr;
  sprites[L] = lspr;
  sprites[S] = sspr;
  sprites[Z] = zspr;
  sprites[T] = tspr;
  sprites[O] = ospr;

  sprites[QBG] = qbgspr;
}

void initscreen()
{
  // draw board
  for(int i = 0; i < 10; i++)
  {
    for(int j = 0; j < 20; j++)
    {
      blitmino(bX, bY, NONE, i, j);
    }
  }

  // draw queue
  for(int i = 0; i < queue_width; i++)
  {
    for(int j = 0; j < queue_height; j++)
    {
      blitmino(qX, qY, QBG, i, j);
    }
  }
}

int main(int argc, char **args)
{
  // initialize window
  init("tetrui", SCREEN_WIDTH, SCREEN_HEIGHT);
  SDL_FillRect( gsurf, NULL, SDL_MapRGB( gsurf->format, 0xFF, 0xFF, 0xFF ) );

  // load sprites and put them in sprites[]
  initsprites();

  // draw hold, board, queue
  initscreen();

  // init RNG
  srand(time(NULL));

  // get random piece to start & update
  // TODO replace with 7-bag
  // struct piece p = spawnpiece(pieces[rand() % npieces]);
  struct piece p = pickpiece();

  SDL_UpdateWindowSurface( gwin );

  // gravity
  uint lastgrav = 0; // ms since last gravity tick
  uint gravdelay = 1000; // ms between gravity ticks
  uint curtime = 0; // current time in ms

  // lock down
  // int movecount = 0;
  uint lockdelay = 500; // ms before piece locks
  uint lastreset = 0; // when locktimer >= lockdelay, piece locks
  bool locking = false; // becomes true whenever piece is touching the ground

  bool quit = false;
  SDL_Event e;

  while(!quit)
  {
    // update time
    curtime = SDL_GetTicks();

    // check gravity timing
    if(curtime - lastgrav >= gravdelay)
    {
      // update timer
      lastgrav = curtime;

      // move down once
      movepiece(p, 0, -1, 0);

      // update screen
      SDL_UpdateWindowSurface( gwin );
    }

    // if touching the ground, check for lockdown
    if(grounded(p))
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
        p = nextpiece(p);
        SDL_UpdateWindowSurface(gwin);

        locking = false;
        lastgrav = curtime;

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
        auto sym = e.key.keysym.sym;

        bool moved = false;

        // translation
        if(sym == SDLK_UP)
        {
          moved = movepiece(p, 0, 1, 0);
        }
        else if(sym == SDLK_DOWN)
        {
          moved = movepiece(p, 0, -1, 0);
        }
        else if(sym == SDLK_LEFT)
        {
          moved = movepiece(p, -1, 0, 0);
        }
        else if(sym == SDLK_RIGHT)
        {
          moved = movepiece(p, 1, 0, 0);
        }

        // rotation (currently dvorak keyboard)
        else if(sym == SDLK_a) // CCW
        {
          moved = rotatepiece(p, CCW);
        }
        else if(sym == SDLK_o) // 180
        {
          moved = rotatepiece(p, FLIP);
        }
        else if(sym == SDLK_e) // CW
        {
          moved = rotatepiece(p, CW);
        }

        // reset lock timer
        if(moved && locking)
        {
          lastreset = curtime;
        }

        SDL_UpdateWindowSurface( gwin );
      }
    }
  }

  close();
  return 0;
}

// keycodes: https://wiki.libsdl.org/SDL2/SDL_Keycode
// guideline https://tetris.wiki/Tetris_Guideline
