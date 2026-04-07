#ifndef REND_H_
#define REND_H_

struct level;
struct level_cell;

void draw_level_cell(struct level *lvl, struct level_cell *cell, int layer);

#endif	/* REND_H_ */
