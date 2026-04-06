#ifndef VGA_H_
#define VGA_H_

#include "szint.h"

#ifdef MSDOS
#include "vgaregs.h"
#include "dosutil.h"

#define VGA_MODEX
/* TODO: change for horiz. scrolling */
#define VGA_PITCH	80

#else

#define VGA_LFB
#define VGA_PITCH	320

#endif

#define VGA_VMEM	((uint8_t*)0xa0000)
extern uint8_t *vga_backbuf;

int vga_setmodex(void);
void vga_cleanup(void);

/* idx -1 for repeat calls to skip setting the DAC address. r,g,b in [0,255] */
void vga_setpal(int16_t idx, uint8_t r, uint8_t g, uint8_t b);

void vga_clearfb(unsigned int color);
void vga_blitfb(void *vmem, const void *img);

void vga_pgflip(int wait_vblank);


#ifdef MSDOS
#define vga_planemask(mask)		vga_sc_write(VGA_SC_MAPMASK_REG, mask)

#define vga_sc_write(reg, data) \
	outpw(VGA_SC_ADDR_PORT, (uint16_t)(reg) | ((uint16_t)(data) << 8))
#define vga_sc_read(reg) \
	(outp(VGA_SC_ADDR_PORT, reg), inp(VGA_SC_DATA_PORT))
#define vga_crtc_write(reg, data) \
	outpw(VGA_CRTC_PORT, (uint16_t)(reg) | ((uint16_t)(data) << 8))
#define vga_crtc_read(reg) \
	(outp(VGA_CRTC_ADDR_PORT, reg), inp(VGA_CRTC_DATA_PORT))
#define vga_crtc_wrmask(reg, data, mask) \
	outp(VGA_CRTC_DATA_PORT, (crtc_read(reg) & ~(mask)) | (data))
#endif	/* defined(MSDOS) */

#endif	/* VGA_H_ */
