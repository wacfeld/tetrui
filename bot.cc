#include "defs.h"
#include "bot.h"
#include "movement.h"

#include <queue>


// #define mark(p) do { board[p.c[0]][p.c[1]][p.rotstate] = true; } while(0)
// #define ismarked(p)

#define marked(p) (board[p.c[0]+1][p.c[1]+1][p.r][p.lastrot][p.lastkick])

std::vector<struct piece> possible(struct piece &p)
{
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
