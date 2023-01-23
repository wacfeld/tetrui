#ifndef BOT_H
#define BOT_H

std::vector<struct piece> possible(const struct piece &p);
std::vector<struct piece> greedy();
std::vector<struct piece> greedy_h(const struct piece &o1, const struct piece &o2, const enum type *queue, int qi);

#endif
