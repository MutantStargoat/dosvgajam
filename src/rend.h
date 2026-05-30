#ifndef REND_H_
#define REND_H_

#include "util.h"

struct level;
struct level_cell;

void draw_level_cell(struct level *lvl, struct level_cell *cell, int layer, int destx, int desty);

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax);
void draw_line(int x0, int y0, int x1, int y1, uint8_t color);

#endif	/* REND_H_ */
