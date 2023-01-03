// -*- mode: c++ -*-

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>

#define putd(x) do{ printf(#x ": %d\n", x); } while(0)
#define error(fmt, ...) do { fprintf(stderr, "%s: %d: %s: " fmt, __FILE__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); close(); exit(1); } while(0)

typedef unsigned char uchar;

//Screen dimension constants
// my screen is 1366x768

const int MINO_LEN = 32;

// total (including buffer) and visible dimensions of board
const int tot_height = 40;
const int tot_width = 10;
const int vis_height = 20;
const int vis_width = 10;

// board is 10x20
const int SCREEN_WIDTH = MINO_LEN*vis_width;
const int SCREEN_HEIGHT = MINO_LEN*vis_height;

// NONE specifies that a square is empty
enum type {NONE, I, J, L, S, Z, O, T};

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
std::vector<SDL_Surface *> sprites(7);

// graphical board top-left corner
const int boardX = 0;
const int boardY = 0;

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

// 0 = empty
// L, J, S, Z, I, O, T = respective colored mino
enum type gboard[tot_width][tot_height];
// guideline: there is a 10x20 buffer area above visible play area

enum state {PLAYING, LOST};
enum state gstate;

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
  sprites.push_back(opt);
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
void blitmino(int X, int Y, SDL_Surface *surf, int col, int row)
{
  // puts("hi");
  // printf("%d %d\n", col, row);
  // check bounds
  if(row >= vis_height || col >= vis_width || row < 0 || col < 0)
  {
    fprintf(stderr, "out of bounds: %d %d\n", row, col);
    return;
  }

  // convert bottom-left-based coords to top-left-based for graphics
  row = vis_height - row - 1;
  // printf("%d %d\n", row, col);
  
  SDL_Rect dest;
  dest.w = MINO_LEN;
  dest.h = MINO_LEN;
  dest.x = X+col * MINO_LEN;
  dest.y = Y+row * MINO_LEN;

  // printf("!!! %d %d\n", dest.x, dest.y);

  SDL_BlitScaled(surf, NULL, gsurf, &dest);
}

// given a piece, draw the parts of it that are on the visible play area
// (don't draw any parts in the buffer area)
// (don't update screen)
void drawpiece(struct piece p)
{
  for(auto &m : p.p)
  {
    // puts("hey");
    blitmino(boardX, boardY, sprites[p.t], m[0], m[1]);
  }
}

// return false if out of bounds, or collides with existing mino
bool goodcoords(int x, int y)
{
  if(x < 0 || y < 0 || x >= tot_width || y >= tot_height
     || gboard[x][y] != NONE)
  {
    return false;
  }

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
  }
  
  // make a copy of the coords
  auto copy = p.p;

  bool moved = false;
  do
  {
    for(auto &m : copy) // update position of copy
    {
      // printf("%d %d\n", m[0], m[1]);
      m[0] += dx;
      m[1] += dy;
      // printf("%d %d\n", m[0], m[1]);
    }

    for(auto &m : copy) // check for out of bounds/collisions
    {
      if(!goodcoords(m[0], m[1]))
      {
        goto stopmoving;
      }
    }

    // if all coords valid, update piece
    p.p = copy;
    moved = true;

  } while(rep);
stopmoving:
  
  // update board
  for(auto &m : p.p)
  {
    // printf("%d %d\n", m[0], m[1]);
    gboard[m[0]][m[1]] = p.t;
  }

  return moved;
}

// game is over if 
bool topout(struct piece p)
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
  }

  // move piece down
  movepiece(p, 0, -1, false);

  return p;
}

int main(int argc, char **args)
{
  // initialize window
  init("test", SCREEN_WIDTH, SCREEN_HEIGHT);
  SDL_FillRect( gsurf, NULL, SDL_MapRGB( gsurf->format, 0xFF, 0xFF, 0xFF ) );

  // load sprites
  SDL_Surface *ispr = loadBMP("sprites/I.bmp");
  SDL_Surface *jspr = loadBMP("sprites/J.bmp");
  SDL_Surface *lspr = loadBMP("sprites/L.bmp");
  SDL_Surface *sspr = loadBMP("sprites/S.bmp");
  SDL_Surface *zspr = loadBMP("sprites/Z.bmp");
  SDL_Surface *ospr = loadBMP("sprites/O.bmp");
  SDL_Surface *tspr = loadBMP("sprites/T.bmp");
  SDL_Surface *bgspr = loadBMP("sprites/bg.bmp");

  sprites[I] = ispr;
  sprites[J] = jspr;
  sprites[L] = lspr;
  sprites[S] = sspr;
  sprites[Z] = zspr;
  sprites[T] = tspr;
  sprites[O] = ospr;
  sprites.push_back(bgspr);

  for(int i = 0; i < 10; i++)
  {
    for(int j = 0; j < 20; j++)
    {
      blitmino(0, 0, bgspr, i, j);
    }
  }

  struct piece p,q,r,s,t,u,v;
  p = spawnpiece(I);
  movepiece(p, 0, -1, 1);
  drawpiece(p);

  q = spawnpiece(T);
  movepiece(q, 0, -1, 1);
  drawpiece(q);

  r = spawnpiece(O);
  movepiece(r, 0, -1, 1);
  drawpiece(r);

  s = spawnpiece(S);
  movepiece(s, 0, -1, 1);
  drawpiece(s);

  t = spawnpiece(Z);
  movepiece(t, 0, -1, 1);
  drawpiece(t);

  u = spawnpiece(J);
  movepiece(u, 0, -1, 1);
  drawpiece(u);

  v = spawnpiece(L);
  movepiece(v, 0, -1, 1);
  drawpiece(v);

  

  //Update the surface
  SDL_UpdateWindowSurface( gwin );

  // blitmino(0, 0, ispr, 1, 1);
  // blitmino(0, 0, tspr, 9, 19);
  // blitmino(0, 0, tspr, 10, 19);

  /* SDL_Rect dest; */
  /* dest.x = 50; */
  /* dest.y = 50; */
  // sch.w = SCREEN_WIDTH;
  // sch.h = SCREEN_HEIGHT;
  /* SDL_BlitSurface( hey, NULL, ws.surf, &dest ); */

  // tetris is 10x20
  // guideline https://tetris.wiki/Tetris_Guideline

  /* SDL_BlitSurface( hey, NULL, ws.surf, NULL ); */


  bool quit = false;
  SDL_Event e;
  
  while(!quit)
  {
    while( SDL_PollEvent( &e ) != 0 )
    {
      //User requests quit
      if( e.type == SDL_QUIT )
      {
        quit = true;
        fprintf(stderr, "quitting\n");
      }
      /* else if( e.type == SDL_KEYDOWN ) */
      /* { */
      /*   printf("%d\n", e.key.keysym.sym); */
      /* } */
    }
  }

  close();

  return 0;
}

// keycodes: https://wiki.libsdl.org/SDL2/SDL_Keycode
