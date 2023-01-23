#include "defs.h"
#include "bot.h"
#include "movement.h"
#include "turn.h"

#include <queue>


// #define mark(p) do { board[p.c[0]][p.c[1]][p.rotstate] = true; } while(0)
// #define ismarked(p)

// #define marked(p) (board[p.c[0]+1][p.c[1]+1][p.r][p.lastrot][p.lastkick])
#define marked(p) (board[p.c[0]+1][p.c[1]+1][p.r][p.lastrot][p.lastkick])

// determine all possible ways a piece could lock down
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

// executes the fastest achievable line clear
std::vector<struct piece> greedy()
{
  int &pl = cur_player;

  // when considering possible piece placement orders, the hold/current/queue system is not very useful or easy to implement
  // it's better to look at it as option 1 option 2/queue
  // say hold/current/queue looks like [empty]/T/IJLSZ
  // then option 1/option 2/queue looks like TI/JLSZ
  // and O/T/IJLSZ corresponds to OT/IJLSZ
  // this means that we don't have to think about a hold operation, and can simply consider 2 options at every step
  // this function converts hold/current/queue to o1 o2/queue and then passes those values to a recursive helper function

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
  
  // pass to helper function
  return greedy_h(o1, o2, queue, qi);
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

std::vector<struct piece> greedy_h(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye)
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
    }
    
    // check of clear has occurred
    int cl = findclears(

    Q.pop();
  }
}
