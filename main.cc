// -*- mode: c -*-

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

//Screen dimension constants
// my screen is 1366x768

const int MINO_LEN = 30;

// board is 10x20
const int SCREEN_WIDTH = MINO_LEN*10;
const int SCREEN_HEIGHT = MINO_LEN*20;

struct winsurf
{
  SDL_Window *win;
  SDL_Surface *surf;
};

struct winsurf ws = {NULL, NULL};

int board[20][10];

SDL_Surface *loadBMP(const char *name)
{
  SDL_Surface *bmp = SDL_LoadBMP(name);
  if(!bmp)
  {
    printf( "Unable to load image %s! SDL Error: %s\n", name, SDL_GetError() );
    exit(1);
  }

  SDL_Surface *opt = SDL_ConvertSurface( bmp, ws.surf->format, 0 );
  if( opt == NULL )
  {
    printf( "Unable to optimize image %s! SDL Error: %s\n", name, SDL_GetError() );
  }

  return bmp;
}

struct winsurf init(const char *title, int w, int h)
{

  //Initialize SDL
  if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
  {
    printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
    exit(1);
  }
  else
  {
    //Create window
    ws.win = SDL_CreateWindow( title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN );
    if( ws.win == NULL )
    {
      printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
      exit(1);
    }

    ws.surf = SDL_GetWindowSurface( ws.win );
  }

  return ws;
}

void close(struct winsurf ws, std::vector<SDL_Surface *> surfs)
{
  SDL_DestroyWindow(ws.win);
  ws = {NULL, NULL};

  for(SDL_Surface *s : surfs)
  {
    SDL_FreeSurface(s);
    s = NULL;
  }

  //Quit SDL subsystems
  SDL_Quit();
}

// given surface and coords on tetris board, scale and place mino (does not update surface)
void BlitMino(SDL_Surface *surf, int row, int col)
{
  SDL_Rect dest;
  dest.w = MINO_LEN;
  dest.h = MINO_LEN;
  dest.x = col * MINO_LEN;
  dest.y = row * MINO_LEN;

  SDL_BlitScaled(surf, NULL, ws.surf, &dest);
}

int main(int argc, char **args)
{
  struct winsurf ws = init("test", SCREEN_WIDTH, SCREEN_HEIGHT);

  SDL_FillRect( ws.surf, NULL, SDL_MapRGB( ws.surf->format, 0xFF, 0xFF, 0xFF ) );

  SDL_Surface *mino = loadBMP("mino.bmp");

  for(int i = 0; i < 9; i++)
  {
    for(int j = 0; j < 19; j++)
    {
      BlitMino(mino, j, i);
    }
  }

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
  SDL_UpdateWindowSurface( ws.win );

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
        printf("quitting\n");
      }
      else if( e.type == SDL_KEYDOWN )
      {
        printf("%d\n", e.key.keysym.sym);
      }
    }
  }

  close(ws, {mino});

  return 0;
}

// keycodes: https://wiki.libsdl.org/SDL2/SDL_Keycode
