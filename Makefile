FASTROM=1

ifeq ($(strip $(PVSNESLIB_HOME)),)
$(error "Please create an environment variable PVSNESLIB_HOME by following this guide: https://github.com/alekmaul/pvsneslib/wiki/Installation")
endif

AUDIOFILES := effectssfx.it
export SOUNDBANK := soundbank

include ${PVSNESLIB_HOME}/devkitsnes/snes_rules

#---------------------------------------------------------------------------------
# ROMNAME is used in snes_rules file
export ROMNAME := JUMPGAME

SMCONVFLAGS	:= -s -o $(SOUNDBANK) -V -b 5
musics: $(SOUNDBANK).obj

all: musics bitmaps $(ROMNAME).sfc

clean: cleanBuildRes cleanRom cleanGfx cleanAudio

resource_bg.pic: resource_bg.bmp
	$(GFXCONV) -s 8 -o 16 -u 16 -e 2 -p -t bmp -m -i $<

resource_bg2.pic: resource_bg2.bmp
	$(GFXCONV) -s 8 -o 4 -u 4 -e 0 -p -t bmp -m -i $<

resource_sp.pic: resource_sp.bmp
	$(GFXCONV) -s 8 -o 16 -u 16 -p -t bmp  -i $<

resource_font.pic: resource_font.bmp
	$(GFXCONV) -s 8 -o 16 -u 16 -p -t bmp  -i $<

bitmaps : resource_bg.pic resource_bg2.pic resource_sp.pic resource_font.pic
