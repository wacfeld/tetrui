#ifndef DEFS_H
#define DEFS_H

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <time.h>
#include <set>

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

// rotation directions
enum rot {CW, FLIP, CCW}; // FLIP means 180

// rotation states that a piece can be in
enum rotstate {ZERO, RIGHT, LEFT, TWO};

// NONE specifies that a square is empty
enum type {NONE=0, I=1, J=2, L=3, S=4, Z=5, O=6, T=7, QBG=8, GHOST=9};

struct keybinds
{
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

#endif
