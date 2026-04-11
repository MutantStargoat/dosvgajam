#include "vga.h"

void vga_vline(uint8_t *vmem, int x, int y, int len, uint8_t color)
{
	vmem += y * VGA_PITCH + (x >> 2);
	vga_planemask(1 << (x & 3));

	while(len-- > 0) {
		*vmem = color;
		vmem += VGA_PITCH;
	}
}

void vga_hline(uint8_t *vmem, int x, int y, int len, uint8_t color)
{
	uint32_t c4, *ptr32;
	unsigned int align;
	vmem += y * VGA_PITCH + (x >> 2);

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
