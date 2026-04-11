Game for the VGA+Adlib jam
==========================
Not done yet, check back later.

License
-------
Copyright (C) 2026 John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software. Feel free to use, modify, and/or redistribute,
under the terms of the GNU General Public License v3, or at your option any
later version published by the Free Software Foundation.
See COPYING for details.

Datafiles
---------
The game datafiles will be in their own subversion repo. Checkout with:

    svn co svn://mutantstargoat.com/datadirs/vgajam data

Notes
-----
The isometric coordinate system has its origin at the top corner. X axis
increases down and to the right, Y axis down and to the left.

Calling conventions cheatsheet
------------------------------
### DJGPP

 - All symbols are *prefixed* with an underscore.
 - Preserve registers: ebx, esi, edi, ebp.
 - Arguments on the stack, on entry: 0:retaddr, 4:first argument (typically at 8
   after saving ebp).
 - Return in eax, edx:eax, or st0.

### Watcom

 - Functions have an underscore *suffix*.
 - Preserve all registers except eax, and any argument registers.
 - Arguments on regs: eax, edx, ebx, ecx.
 - Return in eax
