// -*- mode: c++ -*-

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>

#define error(fmt, ...) do { fprintf(stderr, "%s: %d: %s: " fmt, __FILE__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); close(); exit(1); } while(0)

typedef unsigned char uchar;

//Screen dimension constants
// my screen is 1366x768

const int MINO_LEN = 32;

// board is 10x20
const int SCREEN_WIDTH = MINO_LEN*10;
const int SCREEN_HEIGHT = MINO_LEN*20;

// enum type {I, J, L, S, Z, O, T};

// a piece is a center of rotation along with 4 minoes
struct piece
{
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
std::vector<SDL_Surface *> sprites;

// total (including buffer) and visible dimensions of board
const int tot_height = 40;
const int tot_width = 10;
const int vis_height = 20;
const int vis_width = 10;

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
int gboard[tot_height][20];
// guideline: there is a 20x10 buffer area above visible play area

enum state {PLAYING, LOST};
enum state gstate;

/* void close(struct winsurf ws, std::vector<SDL_Surface *> surfs) */
void close()
{
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
  // check bounds
  if(row >= vis_height || col >= vis_width || row < 0 || col < 0)
  {
    fprintf(stderr, "out of bounds: %d %d\n", row, col);
    return;
  }

  // convert bottom-left-based coords to top-left-based for graphics
  row = vis_height - row - 1;
  
  SDL_Rect dest;
  dest.w = MINO_LEN;
  dest.h = MINO_LEN;
  dest.x = X+col * MINO_LEN;
  dest.y = Y+row * MINO_LEN;

  SDL_BlitScaled(surf, NULL, gsurf, &dest);
}

// given a piece, draw the parts of it that are on the visible play area
// (don't draw any parts in the buffer area)
// (don't update screen)
void drawpiece(struct piece p)
{
  for(auto m : p.p)
  {
    blitmino(boardX, boardY, gsurf, m[0], m[1]);
  }
}

// game is over if 
int topout(struct piece p)
{
  // TODO check for exceeding buffer (only possible in competitive)

  for(auto m : p.p)
  {
    if(gboard[m[0]][m[1]] != 0) // spawn mino collides with existing mino
    {
      return 1;
    }
  }

  return 0;
}

// spawn a piece above the playing field according to guideline
// see https://tetris.fandom.com/wiki/SRS?file=SRS-pieces.png
struct piece spawnpiece(char t)
{
  struct piece p;

  // spawn on 21st row (0-indexed)
  // center of rotation below piece (or as low as possible)
  

  if(t == 'i')
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX2, SY0}, {SX3, SY0}}};
    p.c = ICENT; // (4.5, 19.5) doubled
  }
  else if(t == 'j')
  {
    p.p = {{{SX0, SY1}, {SX0, SY0}, {SX1,SY0}, {SX2,SY0}}};
    p.c = CENT;
  }
  else if(t == 'l')
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX2, SY0}, {SX2, SY1}}};
    p.c = CENT;
  }
  else if(t == 's')
  {
    p.p = {{{SX0, SY0}, {SX1, SY0}, {SX1, SY1}, {SX2, SY1}}};
    p.c = CENT;
  }
  else if(t == 'z')
  {
    p.p = {{{SX0, SY1}, {SX1, SY1}, {SX1, SY0}, {SX2, SY0}}};
    p.c = CENT;
  }
  else if(t == 'o')
  {
    p.p = {{{SX1, SY0}, {SX1, SY1}, {SX2, SY0}, {SX2, SY1}}};
    p.c = OCENT;
  }
  else if(t == 't')
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
  for(auto m : p.p)
  {
    gboard[m[0]][m[1]] = t;
  }

  // move piece down
  // TODO
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

  // draw background
  for(int i = 0; i < 10; i++)
  {
    for(int j = 0; j < 20; j++)
    {
      blitmino(0, 0, bgspr, i, j);
    }
  }

  blitmino(0, 0, ispr, 1, 1);
  blitmino(0, 0, tspr, 9, 19);
  blitmino(0, 0, tspr, 10, 19);

  /* SDL_Rect dest; */
  /* dest.x = 50; */
  /* dest.y = 50; */
  // sch.w = SCREEN_WIDTH;
  // sch.h = SCREEN_HEIGHT;
  /* SDL_BlitSurface( hey, NULL, ws.surf, &dest ); */

  // tetris is 10x20
  // guideline https://tetris.wiki/Tetris_Guideline

  /* SDL_BlitSurface( hey, NULL, ws.surf, NULL ); */

  //Update the surface
  SDL_UpdateWindowSurface( gwin );

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
