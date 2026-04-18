#include "vga.h"

void vga_vline(uint8_t *vmem, int x, int y, int len, uint8_t color)
{
	vmem += y * vga_pitch + (x >> 2);
	vga_planemask(1 << (x & 3));

	while(len-- > 0) {
		*vmem = color;
		vmem += vga_pitch;
	}
}

void vga_hline(uint8_t *vmem, int x, int y, int len, uint8_t color)
{
	uint32_t c4, *ptr32;
	unsigned int align;
	vmem += y * vga_pitch + (x >> 2);

	align = x & 3;
	if(align != 0) {
		vga_planemask(0xf << align);
		*vmem++ = color;
		len -= 4 - align;
	}
	vga_planemask(0xf);

	if(len >= 16) {
		c4 = color | ((uint32_t)color << 8) | ((uint32_t)color << 16) |
			((uint32_t)color << 24);
		ptr32 = (uint32_t*)vmem;

		while(len >= 16) {
			*ptr32++ = c4;
			len -= 16;
		}
		vmem = (uint8_t*)ptr32;
	}

	while(len >= 4) {
		*vmem++ = color;
		len -= 4;
	}

	if(len) {
		vga_planemask(0xf >> (4 - len));
		*vmem = color;
	}
}

void vga_rect_outline(uint8_t *vmem, int x, int y, int w, int h, uint8_t color)
{
	vga_vline(vmem, x, y, h, color);
	vga_vline(vmem, x + w - 1, y, h, color);
	vga_hline(vmem, x + 1, y, w - 2, color);
	vga_hline(vmem, x + 1, y + h - 1, w - 2, color);
}

void vga_fillrect(uint8_t *vmem, int x, int y, int w, int h, uint8_t color)
{
	int i, j;
	uint32_t c4, *ptr32;
	unsigned int align;
	uint8_t *vptr;
	vmem += y * vga_pitch + (x >> 2);

	align = x & 3;
	if(align != 0) {
		vga_planemask(0xf << align);
		vptr = vmem;
		for(i=0; i<h; i++) {
			*vptr = color;
			vptr += vga_pitch;
		}
		vmem++;
		w -= 4 - align;
	}
	vga_planemask(0xf);

	if(w >= 16) {
		c4 = color | ((uint32_t)color << 8) | ((uint32_t)color << 16) |
			((uint32_t)color << 24);

		ptr32 = (uint32_t*)vmem;
		for(i=0; i<h; i++) {
			for(j=0; j<w/16; j++) {
				ptr32[j] = c4;
			}
			ptr32 += vga_pitch / 4;
		}
		vmem += w / 4;
	}

	while(w >= 4) {
		vptr = vmem;
		for(i=0; i<h; i++) {
			*vptr = color;
			vptr += vga_pitch;
		}
		vmem++;
		w -= 4;
	}

	if(w) {
		vga_planemask(0xf >> (4 - w));
		for(i=0; i<h; i++) {
			*vmem = color;
			vmem += vga_pitch;
		}
	}
}
