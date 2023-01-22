#include "defs.h"
#include "drawer.h"

SDL_Window *gwin;
SDL_Surface *gsurf;
std::array<SDL_Surface *, 12> sprites;

// enum type gscreen[tot_width][tot_height];

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

// scale and place mino
// origin is top-left
void blitmino(int X, int Y, enum type t, int col, int row)
{
  SDL_Rect dest;
  dest.w = MINO_LEN;
  dest.h = MINO_LEN;
  dest.x = X+col * MINO_LEN;
  dest.y = Y+row * MINO_LEN;

  // printf("!!! %d %d\n", dest.x, dest.y);
  SDL_BlitScaled(sprites[t], NULL, gsurf, &dest);
}

// given top-left corner of board, surface and coords on tetris board, draw mino
// origin is top-right
void boardmino(int X, int Y, enum type t, int col, int row)
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

  blitmino(playerX(cur_player)+X, Y, t, col, srow);

  // update gscreen alongside actual screen
  gplayers[cur_player].screen[col][row] = t;
}

// after the initial covering of the board in NONE,
// we can be efficient by only blitting things that have changed on gboard
void reboardmino(int X, int Y, enum type t, int col, int row)
{
  // check bounds
  if(row >= vis_height || col >= vis_width || row < 0 || col < 0)
  {
    // fprintf(stderr, "out of bounds: %d %d\n", row, col);
    return;
  }

  // only blit if contents of cell have changed
  // useful for being efficient during line clears
  // if(gscreen[col][row] != gboard[col][row])
  if(gplayers[cur_player].screen[col][row] != t)
  {
    // puts("blitting");
    boardmino(X, Y, t, col, row);
  }
  else
  {
    // puts("skipping blit");
  }

  // changed[col][row] = 0;
}

void queuemino(int X, int Y, enum type t, int col, int row)
{
  // check bounds
  if(row >= queue_height || col >= queue_width || row < 0 || col < 0)
  {
    fprintf(stderr, "queue out of bounds: (%d, %d)\n", col, row);
    return;
  }

  int srow = queue_height - row - 1;
  blitmino(playerX(cur_player)+X, Y, t, col, srow);

  // don't bother with checking for changes with the queue, just redraw always
}

// given a piece, draw the parts of it that are on the visible play area
// (don't draw any parts in the buffer area)
// (don't update screen)
void drawpiece(struct piece &p)
{
  for(auto &m : p.p)
  {
    // puts("hey");
    reboardmino(bX, bY, p.t, m[0], m[1]);
  }
}

void undrawpiece(struct piece &p)
{
  for(auto &m : p.p)
  {
    reboardmino(bX, bY, NONE, m[0], m[1]);
  }
}

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
  SDL_Surface *gspr = loadBMP("sprites/ghost.bmp"); // ghost piece
  SDL_Surface *garbspr = loadBMP("sprites/garbage.bmp"); // garbage piece
  SDL_Surface *skullspr = loadBMP("sprites/skull.bmp"); // skull

  sprites[NONE] = bgspr;
  sprites[I] = ispr;
  sprites[J] = jspr;
  sprites[L] = lspr;
  sprites[S] = sspr;
  sprites[Z] = zspr;
  sprites[T] = tspr;
  sprites[O] = ospr;
  sprites[GHOST] = gspr;
  sprites[QBG] = qbgspr;
  sprites[GARB] = garbspr;
  sprites[SKULL] = skullspr;
}

void initscreen()
{
  for(cur_player = 0; cur_player < gmode; cur_player++)
  {
    int px = playerX(cur_player);

    // draw spacing
    for(int i = 0; i < hold_height; i++)
    {
      blitmino(px-MINO_LEN, 0, NONE, 0, i);
    }

    // draw hold
    for(int i = 0; i < hold_width; i++)
    {
      for(int j = 0; j < hold_height; j++)
      {
        // reuse queue background here
        boardmino(hX, hY, QBG, i, j);
      }
    }

    // draw board
    for(int i = 0; i < vis_width; i++)
    {
      for(int j = 0; j < vis_height; j++)
      {
        boardmino(bX, bY, NONE, i, j);
        // boardmino(bX, bY, bag7(), i, j);
      }
    }

    // draw queue
    for(int i = 0; i < queue_width; i++)
    {
      for(int j = 0; j < queue_height; j++)
      {
        queuemino(qX, qY, QBG, i, j);
      }
    }
  }
  cur_player = 0;
}

void wait(uint ms)
{
  uint oldtime = SDL_GetTicks();
  uint curtime = oldtime;
  SDL_Event e;
  while(true)
  {
    while( SDL_PollEvent( &e ) != 0 )
    {
      if( e.type == SDL_QUIT )
      {
        close();
        fprintf(stderr, "quitting\n");
        exit(0);
      }
    }
    curtime = SDL_GetTicks();
    if(curtime - oldtime >= ms)
    {
      // fprintf(stderr, "breaking; delta is %u\n", curtime - oldtime);
      break;
    }
  }

}

void splash(enum type (*qmeth)(bool reset), uint d1, uint d2)
{
  for(int j = 0; j < vis_height; j++)
  {
    for(int i = 0; i < vis_width; i++)
    {
      for(cur_player = 0; cur_player < gmode; cur_player++)
      {
        boardmino(bX, bY, qmeth(false), i, cur_player%2 ? j : vis_height-1-j);
        blitmino(playerX(cur_player)-MINO_LEN, 0, SKULL, 0, !cur_player%2 ? j : vis_height-1-j);
      }
    }
    SDL_UpdateWindowSurface( gwin );
    wait(d1);
  }

  // getchar();
  wait(d2);

  for(int j = 0; j < vis_height; j++)
  {
    for(int i = 0; i < vis_width; i++)
    {
      for(cur_player = 0; cur_player < gmode; cur_player++)
      {
        boardmino(bX, bY, NONE, i, cur_player%2 ? j : vis_height-1-j);
        blitmino(playerX(cur_player)-MINO_LEN, 0, NONE, 0, !cur_player%2 ? j : vis_height-1-j);
      }
    }
    SDL_UpdateWindowSurface( gwin );
    wait(d1);
  }

  // reset queues
  for(cur_player = 0; cur_player < gmode; cur_player++)
  {
    qmeth(true);
  }

  cur_player = 0;
  wait(d2);
}

void drawholdpiece(enum type t)
{
  struct piece p = placepiece(HCORN[0], HCORN[1], t);

  // draw blanks
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 2; j++)
    {
      queuemino(hX, hY, QBG, HCORN[0]+i, HCORN[1]+j);
    }
  }

  // draw minoes
  for(auto &m : p.p)
  {
    queuemino(hX, hY, t, m[0], m[1]);
  }
}

// place a whole piece
void drawqueuepiece(int place, enum type t)
{
  // place: 0 is next piece, 1 is next-next, etc.
  // used to calculate where to place minoes

  if(place >= queue_len || place < 0)
  {
    fprintf(stderr, "queue placement: %d is not between [1, %d]\n", place+1, queue_len);
  }

  // calculate coords of bottom-left
  int x = QCORN[0]; // always aligned to the left
  int y = QCORN[1] - place * queue_diff; // moves down as place increases

  // get coords
  struct piece p = placepiece(x, y, t);

  // draw blanks over the 4x2 area
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 2; j++)
    {
      queuemino(qX, qY, QBG, x+i, y+j);
    }
  }

  // draw minoes
  for(auto &m : p.p)
  {
    queuemino(qX, qY, t, m[0], m[1]);
  }
}

// draw the entire queue
void drawqueue()
{
  for(int i = 0; i < queue_len; i++)
  {
    drawqueuepiece(i, gplayers[cur_player].queue[i]);
  }
}

void undrawghost(struct piece &p)
{
  for(auto &m : p.p)
  {
    boardmino(bX, bY, gplayers[cur_player].board[m[0]][m[1]], m[0], m[1]);
  }
}

// given current piece, draw a ghost piece for it
struct piece drawghost(struct piece &p)
{
  // make copy
  struct piece q = p;

  // // delete piece from board temporarily
  // for(auto &m : p.p)
  // {
  //   gplayers[cur_player].board[m[0]][m[1]] = NONE;
  // }

  // move the copy down until it's grounded
  while(!grounded(q))
  {
    // decrement all y coords
    for(auto &m : q.p)
    {
      m[1]--;
    }
  }

  // // replace piece on board
  // for(auto &m : p.p)
  // {
  //   gplayers[cur_player].board[m[0]][m[1]] = p.t;
  // }

  // draw ghost piece wherever not occupied already, then draw original piece (piece is in front of ghost)
  for(auto &m : q.p)
  {
    // if(gplayers[cur_player].board[m[0]][m[1]] == NONE)
    // {
      boardmino(bX, bY, GHOST, m[0], m[1]);
    // }
  }

  for(auto &m : p.p)
  {
    boardmino(bX, bY, p.t, m[0], m[1]);
  }

  // return for later undrawing
  return q;
}

void skullmeter(int pl, int c)
{
  for(int i = 0; i < vis_height; i++)
  {
    if(i < c)
    {
      blitmino(playerX(pl)-MINO_LEN, 0, SKULL, 0, vis_height-1-i);
    }
    else
    {
      blitmino(playerX(pl)-MINO_LEN, 0, NONE, 0, vis_height-1-i);
    }
  }
}
