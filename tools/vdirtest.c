#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

enum { DIR8_N, DIR8_NW, DIR8_W, DIR8_SW, DIR8_S, DIR8_SE, DIR8_E, DIR8_NE };

#define SZ	256

int gridvec_to_dir8(int32_t dx, int32_t dy);

int main(void)
{
	int i, j, dir;
	int32_t x, y, dx, dy;

	printf("P6\n%d %d\n255\n", SZ, SZ);

	for(i=0; i<SZ; i++) {
		y = (i << 8) - (SZ << 8) / 2;
		for(j=0; j<SZ; j++) {
			x = (j << 8) - (SZ << 8) / 2;

			dir = gridvec_to_dir8(x, y);

			putchar(dir & 1 ? 255 : 0);
			putchar(dir & 2 ? 255 : 0);
			putchar(dir & 4 ? 255 : 0);
		}
	}
	fflush(stdout);

	return 0;
}

int gridvec_to_dir8(int32_t dx, int32_t dy)
{
	static int diag[] = {DIR8_S, DIR8_W, DIR8_E, DIR8_N};
	unsigned int flip = 0;

	if(dx < 0) {
		dx = -dx;
		flip = 1;
	}
	if(dy < 0) {
		dy = -dy;
		flip |= 2;
	}

	if((dy << 7) < dx * 53) {
		/* slope less than 0.4142 -> 22.5 deg */
		return flip & 1 ? DIR8_NW : DIR8_SE;
	}
	if((dx << 7) < dy * 53) {
		/* slope above 2.4142 -> between 67.5 and 90 deg */
		return flip & 2 ? DIR8_NE : DIR8_SW;
	}
	/* slope between 0.4142 and 2.4142 -> between 22.5 and 67.5 deg */
	return diag[flip];
}
