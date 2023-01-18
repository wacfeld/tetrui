#include "movement.h"
#include "drawer.h"
#include "defs.h"

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

// assumes n >= 1
const char *suffix(unsigned long long n)
{
  if(n % 100 == 11 || n % 100 == 12 || n % 100 == 13)
    return "th";
  if(n % 10 == 1)
    return "st";
  if(n % 10 == 2)
    return "nd";
  if(n % 10 == 3)
    return "rd";
  else
    return "th";
}

// super rotation system
// rotates around center, then tries certain hardcoded offsets before giving up
bool srs(struct piece &p, enum rot r)
{
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

  // rotate all minos around the center and halve again
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


  // decide on kick table
  std::array<std::array<int, 2>, 5> kt;


  // O has no kick table
  if(p.t == O)
  {
    static unsigned long long ORC = 0;
    ORC++;
    fprintf(stderr, "why are you rotating an O piece? this is the %Lu%s time you've done this\n", ORC, suffix(ORC));
    return true;
  }

  // if 180 rotation, no kicks
  else if(r == FLIP)
  {
    kt = {{{0,0}}};
    // p.p = cp;
    // return true;
  }

  // JLSTZ kick table
  else if(p.t == J || p.t == L || p.t == S || p.t == Z || p.t == T)
  {
    if(p.r == ZERO && r == CW) // 0->R
      kt = {{{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}}};

    if(p.r == RIGHT && r == CCW) // R->0
      kt = {{{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}};

    if(p.r == RIGHT && r == CW) // R->2
      kt = {{{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}};

    if(p.r == TWO && r == CCW) // 2->R
      kt = {{{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}}};

    if(p.r == TWO && r == CW) // 2->L
      kt = {{{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}}};

    if(p.r == LEFT && r == CCW) // L->2
      kt = {{{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}};

    if(p.r == LEFT && r == CW) // L->0
      kt = {{{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}};

    if(p.r == ZERO && r == CCW) // 0->L
      kt = {{{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}}};
  }

  // I kick table
  else if(p.t == I)
  {
    if(p.r == ZERO && r == CW) // 0->R
      kt = {{{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}}};

    if(p.r == RIGHT && r == CCW) // R->0
      kt = {{{0,0}, {2,0}, {-1,0}, {2,1}, {-1,-2}}};

    if(p.r == RIGHT && r == CW) // R->2
      kt = {{{0,0}, {-1,0}, {2,0}, {-1,2}, {2,-1}}};

    if(p.r == TWO && r == CCW) // 2->R
      kt = {{{0,0}, {1,0}, {-2,0}, {1,-2}, {-2,1}}};

    if(p.r == TWO && r == CW) // 2->L
      kt = {{{0,0}, {2,0}, {-1,0}, {2,1}, {-1,-2}}};

    if(p.r == LEFT && r == CCW) // L->2
      kt = {{{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}}};

    if(p.r == LEFT && r == CW) // L->0
      kt = {{{0,0}, {1,0}, {-2,0}, {1,-2}, {-2,1}}};

    if(p.r == ZERO && r == CCW) // 0->L
      kt = {{{0,0}, {-1,0}, {2,0}, {-1,2}, {2,-1}}};
  }

  else
  {
    error("invalid piece type %d\n", p.t);
  }

  // try each of the 5 offsets (including (0,0)) one at a time
  for(auto off : kt)
  {
    // does it collide?
    bool collides = false;
    for(auto &m : cp)
    {
      // if(gboard[m[0]+off[0]][m[1]+off[1]] != NONE)
      if(!goodcoords(m[0]+off[0], m[1]+off[1]))
      {
        collides = true;
      }
    }

    // if not, then that's the one
    if(!collides)
    {
      // apply offset
      for(auto &m : cp)
      {
        m[0] += off[0];
        m[1] += off[1];
      }

      // write back
      p.p = cp;

      // apply to center too
      p.c[0] += 2 * off[0];
      p.c[1] += 2 * off[1];

      // check if kick occurred
      if(off[0] == 0 && off[1] == 0)
        p.lastkick = false;
      else
        p.lastkick = true;

      return true;
    }
  }

  // all of them collided; return failure
  return false;
}

// rotate piece
// rmeth (e.x. SRS) attempts to rotate the piece and returns true on success
// (rotating O returns true always)
bool rotatepiece(struct piece &p, enum rot r, bool (*rmeth)(struct piece &p, enum rot r))
{
  // printf("attempting to rotate %d\n", r);
  // delete piece from board
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = NONE;
    // changeboard(m[0], m[1], NONE);
  }

  // save copy of piece
  auto temp = p;

  // attempt to rotate
  bool rotated = rmeth(p, r);

  // putd(rotated);

  if(rotated)
  {
    // update rotstate on success
    {
      // 0, 1, 2, 3 in clockwise order
      int st = p.r == ZERO ? 0 : (p.r == RIGHT ? 1 : (p.r == TWO ? 2 : 3));
      int change = r == CW ? 1 : (r == FLIP ? 2 : 3);

      st += change;
      st %= 4;

      p.r = st == 0 ? ZERO : (st == 1 ? RIGHT : st == 2 ? TWO : LEFT);
    }
  }

  // put piece back on board, rotated or not
  for(auto &m : p.p)
  {
    gboard[m[0]][m[1]] = p.t;
    // changeboard(m[0], m[1], p.t);
  }


  // failure
  if(!rotated)
  {
    return false;
  }

  // success

  // undraw piece
  undrawpiece(temp);
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
