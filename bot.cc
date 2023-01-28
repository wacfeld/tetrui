#include "defs.h"
#include "bot.h"
#include "movement.h"
#include "turn.h"
#include "score.h"

#include <queue>
#include <algorithm>


// #define mark(p) do { board[p.c[0]][p.c[1]][p.rotstate] = true; } while(0)
// #define ismarked(p)

#define marked(p) (board[p.c[0]+1][p.c[1]+1][p.r][0][0])
// #define marked(p) (board[p.c[0]+1][p.c[1]+1][p.r][p.lastrot][p.lastkick])

// determine all possible ways a piece could lock down
std::vector<struct piece> possible(const struct piece &p)
{
  // TODO optimize this as it seems to be responsible for a lot of slowdown
  // poss consistss of all encountered grounded pieces
  std::vector<struct piece> poss;

  // all possible locations/states of pieces (of the current type) on a board, represented by 2 center coords, a rotation state, lastrot and lastkick
  bool board[tot_width*2+2][tot_height*2+2][4][2][2] = {0}; // initialize to all false
  
  // create queue
  std::queue<struct piece> Q;

  // push copy of p
  struct piece q = p;
  Q.push(q);

  // mark p on board
  marked(q) = true;
  // board[q.c[0]][q.c[1]][q.r][q.lastrot][q.lastkick] = true;
  if(grounded(q))
  {
    poss.push_back(q);
  }

  while(!Q.empty())
  {
    // consider all possible moves (down, left, right, cw, ccw, flip)
    
    struct piece cand[6] = {Q.front(), Q.front(), Q.front(), Q.front(), Q.front(), Q.front()}; // candidates
    Q.pop();

    bool success[sizeof(cand)/sizeof(*cand)] = {0};
    success[0] = movepiece(cand[0], 0, -1, false); // down
    success[1] = movepiece(cand[1], -1, 0, false); // left
    success[2] = movepiece(cand[2], 1, 0, false); // right
    success[3] = rotatepiece(cand[3], CW, srs); // CW
    success[4] = rotatepiece(cand[4], CCW, srs); // CCW
    success[5] = rotatepiece(cand[5], FLIP, srs); // FLIP

    for(int i = 0; i < (int) (sizeof(cand)/sizeof(*cand)); i++)
    {
      // if a move is successful and isn't already marked, then mark and append to Q
      if(success[i] && !marked(cand[i]))
      {
        marked(cand[i]) = true;
        Q.push(cand[i]);
        
        // if grounded, append to poss as well
        if(grounded(cand[i]))
        {
          poss.push_back(cand[i]);
        }
      }
    }
  }

  return poss;
}

std::vector<struct piece> calc(std::vector<struct piece> method(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye))
{
  int &pl = cur_player;

  // when considering possible piece placement orders, the hold/current/queue system is not very useful or easy to implement
  // it's better to look at it as option 1 option 2/queue
  // say hold/current/queue looks like [empty]/T/IJLSZ
  // then option 1/option 2/queue looks like TI/JLSZ
  // and O/T/IJLSZ corresponds to OT/IJLSZ
  // this means that we don't have to think about a hold operation, and can simply consider 2 options at every step
  // this function converts hold/current/queue to o1 o2/queue and then passes those values to a helper function

  // get queue
  const enum type *queue = gplayers[pl].queue;
  int qi = 0; // queue index

  // option 1 is current piece
  const struct piece &o1 = gplayers[pl].p;
  
  // option 2 is the hold, or if the hold is empty, the front of queue
  enum type temp;
  if(gplayers[pl].hold == NONE)
    temp = queue[qi++];
  else
    temp = gplayers[pl].hold;
  struct piece o2 = spawnpiece(temp);
  
  // pass on
  return method(o1, o2, queue, qi);
}

void advance(enum type choice, enum type &t1, enum type &t2, const enum type *queue, int &qi)
{
  if(choice == t1)
  {
    if(qi < queue_len)
    {
      t1 = queue[qi++];
    }
    else
      t1 = NONE;
  }
  else
  {
    if(qi < queue_len)
    {
      t2 = queue[qi++];
    }
    else
      t2 = NONE;
  }
}

// go through entire board, count how many minoes how air beneath them
int countgaps()
{
  int c = 0;
  
  auto &board = gplayers[cur_player].board;
  for(int i = 0; i < tot_width; i++)
  {
    for(int j = 0; j < tot_height; j++)
    {
      if(board[i][j] != NONE && goodcoords(i, j-1))
        c++;
    }
  }

  return c;
}

// go through entire board, count how many air cells have minoes vertically above them (at any distance)
int counttallgaps()
{
  int c = 0;
  
  auto &board = gplayers[cur_player].board;
  for(int i = 0; i < tot_width; i++)
  {
    int d = 0;
    for(int j = tot_height-1; j >= 0; j--)
    {
      if(board[i][j] != NONE)
        d = 1;
      
      if(board[i][j] == NONE)
        c += d;
    }
  }

  return c;
}

// return true if air gap exists directly below any of the piece's minoes
// function assumes piece is on board already
bool gap(struct piece &p)
{
  // puts("hi");
  // enum type back[4];
  // for(int i = 0; i < 4; i++)
  // {
  //   back[i] = gplayers[cur_player].board[p.p[i][0]][p.p[i][1]];
  // }

  // boardpiece(p);

  bool g = false;

  for(auto &m : p.p)
  {
    if(goodcoords(m[0], m[1]-1))
      g = true;
  }

  // for(int i = 0; i < 4; i++)
  // {
  //   gplayers[cur_player].board[p.p[i][0]][p.p[i][1]] = back[i];
  // }

  return g;
}

// executes the fastest achievable line clear. uses BFS
// int qc = 0;
std::vector<struct piece> greedy(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye)
{
  // create exploration queue
  // each vector in the queue is a possible sequence of placements
  std::queue<std::vector<struct piece>> Q;
  
  // put initial options in exploration queue
  auto v1 = possible(o1);
  auto v2 = possible(o2);

  for(auto &p : v1)
    Q.push({p});
  for(auto &p : v2)
    Q.push({p});
  

  enum type t1;
  enum type t2;
  int qi;

  while(!Q.empty())
  {
    t1 = o1.t;
    t2 = o2.t;
    qi = qeye;
    
    // get vector
    auto &v = Q.front();
    
    // run through the vector (place pieces on board and advance state variables)
    for(auto &p : v)
    {
      advance(p.t, t1, t2, queue, qi);
      boardpiece(p); // don't have to check for clears since this algorithm terminates after finding one clear
      drawpiece(p);
    }
    
    // check if clear has occurred after placing last piece on board
    int cl = findclears(v.back()).size();
    if(cl != 0) // clears have occurred; return this vector
    {
      return v;
    }
    else if(gap(v.back())) // otheriwse, if gap, ignore
    {
      // puts("gap");
      goto undo;
    }
    else
    {
      // puts("gap!");
    }
    SDL_UpdateWindowSurface( gwin );

    if(t1 == NONE && t2 == NONE) // ran out of pieces; don't do anything
    { }
    else
    {
      if(t1 != NONE) // try selecting t1
      {
        struct piece q = spawnpiece(t1);
        auto poss = possible(q);
        
        // append them all to queue
        for(struct piece &r : poss)
        {
          auto newv = v;
          newv.push_back(r);
          // putd(qc++);
          Q.push(newv);
        }
      }
      
      if(t2 != NONE) // try selecting t2
      {
        struct piece q = spawnpiece(t2);
        auto poss = possible(q);
        
        // append them all to queue
        for(struct piece &r : poss)
        {
          auto newv = v;
          newv.push_back(r);
          // putd(qc++);
          Q.push(newv);
        }
      }
    }

undo:
    // remove pieces from board
    for(auto &p : v)
    {
      unboardpiece(p);
      undrawpiece(p);
    }
    SDL_UpdateWindowSurface( gwin );

    Q.pop();
  }
  
  // 
  fprintf(stderr, "greedy: failed to find solution\n");
  auto ret = {v1[0]};
  return ret;
}

int getelev(struct piece &p)
{
  // get maximum y coordinate of 4 minoes
  int max = -3;
  for(auto &m : p.p)
  {
    max = mymax(max, m[1]);
  }

  // putd(max);
  return max;
}

// returns score depending on how many tetrominoes the current board state accepts (has no gaps)
// rules being, accepting more tetromino types is always better
// ties broken by how many placements are accepted by those types
long permscore()
{
  // current shortcomings:
  // does not consider possible clears which eliminate gaps
  // does not consider the differences between gaps (some are less bad)
  
  long score = 0;
  
  // each accepted tetromino contributes at least this much, to guarantee that more tetrominoes is better that fewer tetrominoes with many placements
  const static int maxplace = (tot_width*2+2)*(tot_height*2+2)*4*2*2;
  
  for(enum type t : {I,J,L,S,Z,T,O})
  {
    // spawn in piece
    struct piece p = spawnpiece(t);
    
    // see all possible ways to place it down
    std::vector<struct piece> poss = possible(p);
    
    // count how many placements are gapless
    int numaccepts = 0;
    for(struct piece &q : poss)
    {
      boardpiece(q);
      if(!gap(q))
      // if(!countgaps())
      {
        numaccepts++;
      }
      unboardpiece(q);
    }

    // score += numaccepts;
    if(numaccepts)
      score += maxplace;
  }

  return score;
}

// std::vector<struct piece> rank()
// {
  
// }

// filter out all piece placements that don't satisfy pred()
// std::vector<struct piece> filter(bool pred(), std::vector<struct piece> v)
// {
  
// }

int holedepth(int col)
{
  int dep = 0;
  int add = 0;
  for(int i = tot_height - 1; i <= 0; i--)
  {
    if(goodcoords(col-1, i) && goodcoords(col+1, i)) // minoes left and right occupied
    {
      add = 1;
    }
    if(goodcoords(col, i)) // mino below
    {
      break;
    }
    dep += add;
  }

  return dep;
}

// a hole of depth d is defined as a column of d cells where the top cell has minoes immediately left and right and no minoes above, and the bottom cell has a mino immediately below
// drawbacks: does not recognize hole when covered
bool ishole(int col, int mindepth)
{
  // auto &board = gplayers[cur_player].board;

  int dep = holedepth(col);
  return dep >= mindepth;
}

bool permelev_comp(struct piece &p, struct piece &q)
{
  boardpiece(p);
  int pperm = permscore();
  unboardpiece(p);

  boardpiece(q);
  int qperm = permscore();
  unboardpiece(q);

  if(pperm < qperm)
    return true;

  else if(pperm == qperm)
  {
    int pe = getelev(p);
    int qe = getelev(q);
    return pe > qe;
  }

  return false;
}

bool tgappermelev_comp(struct piece &p, struct piece &q)
{
  boardpiece(p);
  int ptg = counttallgaps();
  unboardpiece(p);

  boardpiece(q);
  int qtg = counttallgaps();
  unboardpiece(q);

  if(ptg > qtg)
    return true;
  else if(ptg == qtg)
  {
    return permelev_comp(p, q);
  }

  return false;
}

// 9-0 stacking, where the goal is create a robust 9-wide stack on the left and use I pieces to do quads on the right (ideally back to back)
std::vector<struct piece> ninezero(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye)
{
  // putd(counttallgaps());
  // for the moment this method is very shortsighted
  // it only considers the current two options, without looking ahead
  // it compensates for that by trying to accommodate for all possible upcoming pieces

  // if have to create gap, pick the placement that makes the fewest/smallest gaps
  // if gaps exist and a placement can eliminate it, do that

  // TODO try to keep the range of heights on the stack minimal
  // TODO take into account the gap counts of the options themselves
  // TODO avoid creating picky zones
  // TODO penalize creating inaccessible gaps
  // TODO behave differently when near topping out
  // TODO recognize and discourage creating dependencies

  // column where I pieces are dropped (for 9-0, will either be leftmost or rightmost column)
  // const static int Icol = 0;

  auto v1 = possible(o1);
  auto v2 = possible(o2);

  // // find the highest-scoring possibility (fewest possible gaps)
  // int maxscore = 0;
  // struct piece *maxpiece = &v1[0];

  // 1. if I piece and quad possible, do quad
  // 2. if zero-gap options, pick highest-permitting lowest-elevation option
  // 3. if current piece can leave no gaps immediately below, pick highest-permitting lowest-elevation option
  // 4. if no zero-gap options, pick lowest tall gap count highest-permitting lowest elevation option

  // int zmaxperm = -1;{{{
  // int zminelev = tot_height*3+10;
  // struct piece *zmaxpermpiece = 0;

  // int gmaxperm = -1;
  // int gminelev = tot_height*3+10;
  // struct piece *gbestpiece = 0;

  // int nmintgap = tot_height*tot_width + 10;
  // int nmaxperm = -1;
  // int nminelev = tot_height*3+10;
  // struct piece *nmaxpermpiece = 0;}}}

  // concatenate vectors
  v1.insert(v1.end(), v2.begin(), v2.end());

  std::vector<struct piece> nogaps;
  std::vector<struct piece> nonewgaps;
  std::vector<struct piece> hasgaps;
  std::vector<struct piece> quads;

  // check if any undesired holes exist, remember the tallest one
  // int depths[tot_width] = {0};

  // for(int i = 0; i < tot_width; i++)
  // {
  //   if(i == Icol)
  //     continue;

  //   // 3-tall or taller hole
  //   if(ishole(i, 3))
  // }

  for (struct piece &p : v1) {

    boardpiece(p);

    // 1. I piece and quad possible
    if(p.t == I && findclears(p).size() == 4)
    {
      quads.push_back(p);
      // unboardpiece(p);
      // return {p};
    }

    int tgaps = counttallgaps();

    // 3. zero gaps
    if(!tgaps)
    {
      nogaps.push_back(p);
    }

    else if(!gap(p))
    {
      nonewgaps.push_back(p);
    }

    // 2. has gaps
    else
    {
      hasgaps.push_back(p);
    }

    unboardpiece(p);
  }

  fprintf(stderr, "nogaps %ld; nonewgaps %ld; hasgaps %ld\n", nogaps.size(), nonewgaps.size(), hasgaps.size());

  if(!quads.empty())
  {
  }

  if(!nogaps.empty())
  {
    return {*std::max_element(nogaps.begin(), nogaps.end(), permelev_comp)};
  }

  if(!nonewgaps.empty())
  {
    return {*std::max_element(nonewgaps.begin(), nonewgaps.end(), permelev_comp)};
  }

  if(!hasgaps.empty())
  {
    return {*std::max_element(hasgaps.begin(), hasgaps.end(), tgappermelev_comp)};
  }

  error("no possible pieces found");
  exit(1);

}

bool piecebelow(struct piece &p, int maxy)
{
  for(auto &m : p.p)
    if(m[1] > maxy)
      return false;
  return true;
}

// solve a perfect clear
// for example, solve a PCO setup
std::vector<struct piece> pc(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye)
{
  int maxy = 3; // going to assume for now that all pieces must be placed within the first 4 rows
  long unsigned maxdep = 4;
  
  // make copy of grid
  enum type board[ARRLEN(gplayers[cur_player].board)][ARRLEN(*gplayers[cur_player].board)];
  memcpy(board, gplayers[cur_player].board, sizeof(enum type) * ARRLEN(gplayers[cur_player].board) * ARRLEN(*gplayers[cur_player].board));
  
  std::queue<std::vector<struct piece>> Q;
  auto v1 = possible(o1);
  auto v2 = possible(o2);

  for(auto &p : v1)
  {
    if(piecebelow(p, maxy))
      Q.push({p});
  }
  for(auto &p : v2)
  {
    if(piecebelow(p, maxy))
      Q.push({p});
  }
  
  enum type t1;
  enum type t2;
  int qi;

  for(; !Q.empty(); Q.pop())
  {
    int my = maxy;
    // wait(50);
    redrawboard(bX, bY);
    SDL_UpdateWindowSurface( gwin );
    t1 = o1.t;
    t2 = o2.t;
    qi = qeye;
    
    // get vector
    auto &v = Q.front();

    if(v.size() > maxdep)
    {
      continue;
    }
    
    // run through the vector (place pieces on board and advance state variables)
    for(auto &p : v)
    {
      advance(p.t, t1, t2, queue, qi);
      boardpiece(p);
      redrawboard(bX,bY);

      // do any clears
      auto rows = findclears(p);
      doclears(rows);
      my -= rows.size();
    }
    
    SDL_UpdateWindowSurface( gwin );
    
    if(ispc())
    {
      memcpy(gplayers[cur_player].board, board, sizeof(enum type) * ARRLEN(gplayers[cur_player].board) * ARRLEN(*gplayers[cur_player].board));
      redrawboard(bX, bY);
      SDL_UpdateWindowSurface( gwin );
      return v;
    }


    if(t1 != NONE) // try selecting t1
    {
      struct piece q = spawnpiece(t1);
      auto poss = possible(q);

      // append them all to queue
      for(struct piece &r : poss)
      {
        if(piecebelow(r, my))
        {
            auto newv = v;
            newv.push_back(r);
            // putd(qc++);
            Q.push(newv);
        }
      }
    }

    if(t2 != NONE) // try selecting t2
    {
      struct piece q = spawnpiece(t2);
      auto poss = possible(q);

      // append them all to queue
      for(struct piece &r : poss)
      {
        if(piecebelow(r, my))
        {
          auto newv = v;
          newv.push_back(r);
          // putd(qc++);
          Q.push(newv);
        }
      }
    }


    // reset board
    memcpy(gplayers[cur_player].board, board, sizeof(enum type) * ARRLEN(gplayers[cur_player].board) * ARRLEN(*gplayers[cur_player].board));
  }


  redrawboard(bX, bY);
  SDL_UpdateWindowSurface( gwin );
  fprintf(stderr, "pc: failed to find perfect clear\n");
  auto ret = {v1[0]};
  return ret;
}
