#ifndef XMATH_H_
#define XMATH_H_

#include "szint.h"

#define SINLUT_SIZE		512
#define SINLUT_MASK		(SINLUT_SIZE - 1)
#define SINLUT_SCALE	8192

extern int32_t sintab[];

#define XPI		(SINLUT_SIZE >> 1)
#define XHPI	(SINLUT_SIZE >> 2)
#define XQPI	(SINLUT_SIZE >> 3)
#define XTWOPI	SINLUT_SIZE

/* sin(x) lookup -> 16.16 fixed point, pi is half of SINLUT_SIZE */
#define XSIN(x)	(sintab[(x) & SINLUT_MASK] << 3)
#define XCOS(x)	(sintab[((x) + XHPI) & SINLUT_MASK] << 3)

void vec3_xform(int32_t *v, const int32_t *m);
void vec4_xform(int32_t *v, const int32_t *m);

void mat_identity(int32_t *m);

void mat_trans(int32_t *m, int32_t x, int32_t y, int32_t z);
void mat_rotx(int32_t *m, int32_t theta);
void mat_roty(int32_t *m, int32_t theta);
void mat_rotz(int32_t *m, int32_t theta);

void mat_mul_trans(int32_t *m, int32_t x, int32_t y, int32_t z);
void mat_mul_rotx(int32_t *m, int32_t theta);
void mat_mul_roty(int32_t *m, int32_t theta);
void mat_mul_rotz(int32_t *m, int32_t theta);

void mat_perspective(int32_t *m, int vfov, int32_t aspect, int32_t znear, int32_t zfar);

/* NOTE: prefer mat_mult, it's *slightly* faster than _pre */
void mat_mult(int32_t *ma, const int32_t *mb);
void mat_mult_pre(int32_t *ma, const int32_t *mb);

#endif	/* XMATH_H_ */
