#include "defs.h"
#include "drawer.h"

SDL_Window *gwin;
SDL_Surface *gsurf;
std::array<SDL_Surface *, 10> sprites;

enum type gscreen[tot_width][tot_height];

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
