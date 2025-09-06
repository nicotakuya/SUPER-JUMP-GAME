.include "hdr.asm"

.section ".rodata1" superfree

gfxpsrite:
.incbin "resource_sp.pic"
gfxpsrite_end:

palsprite:
.incbin "resource_sp.pal"
palsprite_end:

map:
.incbin "resource_bg.map"
map_end:

palette:
.incbin "resource_bg.pal"
palette_end:

.ends

.section ".rodata2" superfree

patterns:
.incbin "resource_bg.pic"
patterns_end:

tilfont:
.incbin "resource_font.pic"

palfont:
.incbin "resource_font.pal"

.ends

