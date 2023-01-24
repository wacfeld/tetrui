#ifndef BOT_H
#define BOT_H

std::vector<struct piece> possible(const struct piece &p);
std::vector<struct piece> calc(std::vector<struct piece> method(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye));
std::vector<struct piece> greedy(const struct piece &o1, const struct piece &o2, const enum type *queue, int qi);
std::vector<struct piece> ninezero(const struct piece &o1, const struct piece &o2, const enum type *queue, int qeye);

#endif
