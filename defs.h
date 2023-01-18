#ifndef DEFS_H
#define DEFS_H

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <time.h>
#include <set>
#include <random>

#define putd(x) do{ printf(#x ": %d\n", x); } while(0)

// print error message to stderr & exit
#define error(fmt, ...) do { fprintf(stderr, "%s: %d: %s: " fmt, __FILE__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__); close(); exit(1); } while(0)

typedef unsigned char uchar;
typedef unsigned int uint;

//Screen dimension constants
// my screen is 1366x768

// total (including buffer) and visible dimensions of board
const int tot_height = 40;
const int tot_width = 10;

enum mode {SINGLE=1, VERSUS=2, WHAT=3};
extern enum mode gmode;

extern std::uniform_int_distribution<int> dist;

// rotation directions
enum rot {CW, FLIP, CCW}; // FLIP means 180

// rotation states that a piece can be in
enum rotstate {ZERO, RIGHT, LEFT, TWO};

// NONE specifies that a square is empty
enum type {NONE=0, I=1, J=2, L=3, S=4, Z=5, O=6, T=7, QBG=8, GHOST=9};

struct keybinds
{
  SDL_Keycode cmd; // command
  SDL_Keycode hd; // hard drop
  SDL_Keycode h; // hold
  SDL_Keycode l; // move left
  SDL_Keycode r; // move right
  SDL_Keycode sd; // soft drop
  SDL_Keycode ccw; // counterclockwise
  SDL_Keycode cw; // clockwise
  SDL_Keycode f; // 180
};

// properties that a clear can have
struct clear
{
  int lines; // none, single, double, triple, quad (0, 1, 2, 3, 4)
  bool tspin;
  bool mini; // whether tspin is mini or not (only for singles and doubles)
  bool pc; // perfect clear
};

// a piece is a type, a center of rotation, and 4 minoes
struct piece
{
  enum type t;
  enum rotstate r;

  bool lastrot;
  bool lastkick;

  // center of rotation coords may be in-between minoes (for I and O)
  // hence c[2] coords are stored as double their actual value
  std::array<int, 2> c;
  std::array<std::array<int, 2>, 4> p;

  piece()
  {
    t = NONE;
    r = ZERO;
    lastrot = false;
    lastkick = false;
  }
};

// struct winsurf
// {
//   SDL_Window *win;
//   SDL_Surface *surf;
// };

// struct winsurf ws = {NULL, NULL};

const int queue_len = 5; // number of pieces to display

struct player
{
  bool lost;

  enum type board[tot_width][tot_height+1];
  enum type queue[queue_len];
  enum type screen[tot_width][tot_height];
  enum type hold;
  
  struct piece p;
  struct piece ghost;

  enum type *bag;
  int siz;
  std::mt19937 gen; // random number generator for queue (all generators get seeded with the same seed so queues are identical)

  uint lastgrav;
  uint lastreset;
  bool locking;
  bool canhold;

  player()
  {
    lost = false;
    
    // clear board
    for(int i = 0; i < tot_width; i++)
    {
      for(int j = 0; j <= tot_height; j++)
      {
        board[i][j] = NONE;
      }
    }

    // clear screen
    for(int i = 0; i < tot_width; i++)
    {
      for(int j = 0; j < tot_height; j++)
      {
        screen[i][j] = NONE;
      }
    }

    // clear queue
    for(int i = 0; i < queue_len; i++)
    {
      queue[i] = NONE;
    }

    bag = NULL;
    siz = 0;

    lastgrav = 0;
    lastreset = 0;
    locking = false;
    canhold = true;
  }
};

extern struct player *gplayers;
extern int cur_player;

/* extern enum type gboard[tot_width][tot_height+1]; */
/* extern enum type gqueue[queue_len]; */
/* extern enum type ghold; */

struct piece placepiece(int x, int y, enum type t);

bool stuck(struct piece &p, int dx, int dy);
bool grounded(struct piece &p);
#endif
