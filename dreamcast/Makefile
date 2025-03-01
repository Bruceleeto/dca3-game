#
# Basic KallistiOS skeleton / test program
# Copyright (C)2001-2004 Megan Potter
#   

# Put the filename of the output binary here
PROJECT_NAME = dca3
TARGET ?= dca3.elf

HAVE_CDI4DC := $(shell which cdi4dc > /dev/null 2>&1 && echo "yes" || echo "no")
IS_MAC := $(shell uname -s | grep -i "darwin" > /dev/null && echo "yes" || echo "no")

MOD_NAME?=

GTA_DIR?=../../miami
GTA_MOD_DIR?=../../miami_mod$(MOD_NAME)
GTA_MOD_IMG_DIR?=$(GTA_MOD_DIR)/img
GTA_MOD_SFX_DIR?=$(GTA_MOD_DIR)/sfx
GTA_MOD_LOOSE_DIR?=$(GTA_MOD_DIR)/loose

REPACK_DIR?=repack-data
REPACK_GTA_DIR?=$(REPACK_DIR)/miami
REPACK_IMG_ORIG_DIR?=$(REPACK_DIR)/miami-img-orig
REPACK_IMG_DC_DIR?=$(REPACK_DIR)/miami-img-dc
REPACK_SFX_ORIG_DIR?=$(REPACK_DIR)/miami-sfx-orig
REPACK_SFX_DC_DIR?=$(REPACK_DIR)/miami-sfx-dc
REPACK_STREAM_DECODED_DIR?=$(REPACK_DIR)/miami-stream-decoded

LIBS :=
TEXCONV_FLAGS :=

ifeq ($(IS_MAC), yes)
TEXCONV_FLAGS += -DMACOS64
endif

AUDIO_STREAM_OPTION=-t
MKDCDISC_PAD_OPTION=-N
ifeq ($(FOR_DISC),1)
AUDIO_STREAM_OPTION=-q
MKDCDISC_PAD_OPTION=
else ifeq ($(FOR_DISC),2)
AUDIO_STREAM_OPTION=-m
MKDCDISC_PAD_OPTION=
endif

all: $(TARGET)

include common.mk

OBJS = $(RE3_OBJS) $(RW_OBJS) \
	../miami/audio/sampman_dc.o \
	../src/prof/profiler.o

OBJS_TEXCONV = $(RW_OBJS:.o=.texconv.o)
OBJS_TEXCONV += \
	../vendor/koshle/hlekos.texconv.o \
	../vendor/koshle/hlepvr_mem.texconv.o \
	../vendor/koshle/hlepvr_prim.texconv.o \
	../vendor/koshle/hlepvr_scene.texconv.o \
	../vendor/koshle/hlepvr_misc.texconv.o \
	../vendor/koshle/hlepvr_init_term.texconv.o \
	../vendor/koshle/hlepvr_buffers.texconv.o \
	../vendor/koshle/hlepvr_irq.texconv.o \
	../vendor/koshle/hlematrix3d.texconv.o \
	../vendor/librw/src/dc/vq.texconv.o \
	../miami/fakerw/fake.texconv.o \
	../miami/skel/crossplatform.texconv.o \
	../miami/rw/TxdStore.texconv.o \
	../miami/rw/RwHelper.texconv.o \
	texconv.texconv.o \
	../vendor/TriStripper/src/connectivity_graph.texconv.o \
	../vendor/TriStripper/src/policy.texconv.o \
	../vendor/TriStripper/src/tri_stripper.texconv.o \
	../miami/rw/VisibilityPlugins.texconv.o \
	../miami/rw/NodeName.texconv.o \
	../miami/animation/RpAnimBlend.texconv.o \
	../miami/animation/Bones.texconv.o \
	../miami/animation/AnimBlendAssociation.texconv.o \
	../miami/animation/AnimBlendNode.texconv.o \
	../miami/animation/AnimBlendClumpData.texconv.o \
	../miami/rw/MemoryMgr.texconv.o \
	../miami/math/Quaternion.texconv.o \
	../vendor/librw/src/ps2-x/ps2.texconv.o \
	../vendor/librw/src/ps2-x/ps2device.texconv.o \
	../vendor/librw/src/ps2-x/ps2raster.texconv.o \
	../vendor/librw/src/ps2-x/ps2skin.texconv.o \
	../vendor/librw/src/d3d-x/d3d.texconv.o \
	../vendor/librw/src/d3d-x/d3d8.texconv.o \
	../vendor/librw/src/d3d-x/d3d8render.texconv.o \
	../vendor/librw/src/bmp.texconv.o \
	../vendor/librw/src/png.texconv.o \
	../vendor/librw/src/lodepng/lodepng.texconv.o

# Add compilation units to this list to explicity compile them with
# -O3 optimizations, while the rest get the default (-Os) treatment
# to conserve RAM.
OBJS_O3 = \
	../vendor/librw/src/dc/rwdc.o \
	../miami/core/World.o \
	../miami/collision/Collision.o \
	../miami/math/math.o \
	../miami/math/Matrix.o \
	../miami/math/Quaternion.o \
	../miami/math/Rect.o \
	../miami/math/Vector.o \
	../vendor/librw/src/base.o \
	../miami/renderer/Shadows.o

OBJS_NO_FAST_MATH = \
	../miami/core/Cam.o \
	../miami/core/Camera.o \
	../miami/vehicles/Bike.o \
	../miami/renderer/Particle.o

KOS_CPPFLAGS += -fbuiltin -ffast-math -ffp-contract=fast \
	-mfsrra -mfsca

ifdef KOS_BASE
include $(KOS_BASE)/Makefile.rules
else
$(warning "KOS_BASE is not set. Only prebuilt targets will work.")
endif

DEPS = $(OBJS:.o=.d) $(OBJS_TEXCONV:.o:.d)

CXXFLAGS += $(if $(WITH_32MB),-O3,-Os) \
	$(if $(WITH_IDE),-DWITH_IDE) \
	$(if $(WITH_PROF),-DWITH_PROF=\"$(WITH_PROF)\") \
	-MMD -MP -ffunction-sections -fdata-sections -ffast-math \
	-fmerge-all-constants -fomit-frame-pointer -ml -std=gnu++20 \
	-fno-exceptions -fno-rtti -fipa-pta -Wno-write-strings \
	-Wno-deprecated-enum-enum-conversion -Wno-deprecated-enum-float-conversion \
	-Wno-multichar -Wno-unused-value -Wno-char-subscripts -Wno-reorder \
	-Wno-unused-function -Wno-class-memaccess -fno-permissive \
	-fno-asynchronous-unwind-tables -fno-enforce-eh-specs -fno-non-call-exceptions \
	-fno-strict-aliasing -fwrapv

clean-texconv:
	-rm -f $(OBJS_TEXCONV)
	-rm -f texconv

clean-pvrtex:
	$(MAKE) -C ./pvrtex clean
	-rm -f ./pvrtex/pvrtex

clean-objs:
	-rm -f $(OBJS)
	
clean:
	-rm -f $(OBJS)
	-rm -f $(OBJS_TEXCONV)
	-rm -f $(TARGET)
	-rm -f $(TARGET).bin
	-rm -f 1ST_READ.BIN
	-rm -f IP.BIN
	-rm -f $(PROJECT_NAME).iso
	-rm -f $(PROJECT_NAME).ds.iso
	-rm -f $(PROJECT_NAME).cdi
	-rm -f $(DEPS)
	-rm -rf $(REPACK_DIR)
	-rm -rf analyze-profile

$(OBJS_O3): %.o: %.cpp
	kos-c++ $(CXXFLAGS) $(CPPFLAGS) -O3 -c $< -o $@

$(OBJS_NO_FAST_MATH): %.o: %.cpp
	kos-c++ $(CXXFLAGS) $(CPPFLAGS) -O3 -c $< -o $@ -fno-fast-math

$(TARGET): $(OBJS)
	kos-c++ -o $(TARGET) $(OBJS) -Wl,--gc-sections -Wl,--as-needed -Wl,-Map,output.map \
		$(if $(WITH_IDE),-lkosfat) $(if $(WITH_SD),-lkosfat) -Wl,--build-id=sha1 
	@echo && echo && echo "*** Build Completed Successfully ***" && echo && echo

run: $(TARGET)
	$(KOS_LOADER) $(TARGET)

$(REPACK_GTA_DIR)/GTA3SF8.b: GTA3SF8.b
	mkdir -p $(@D)
	cp $< $@

1ST_READ.BIN: $(TARGET)
	rm -f $(TARGET).bin
	rm -f 1ST_READ.BIN
	kos-objcopy -R .stack -O binary $(TARGET) $(TARGET).bin
	$(KOS_BASE)/utils/scramble/scramble $(TARGET).bin 1ST_READ.BIN

.PHONY: pvrtex

pvrtex:
	$(MAKE) -C ./pvrtex

IP.BIN:
	rm -f IP.BIN
	$(KOS_BASE)/utils/makeip/makeip ip.txt IP.BIN

$(PROJECT_NAME).iso: IP.BIN 1ST_READ.BIN $(REPACK_DIR)/repacked $(REPACK_GTA_DIR)/GTA3SF8.b
	rm -f $(PROJECT_NAME).iso
	rm -f $(REPACK_GTA_DIR)/1ST_READ.BIN
	cp 1ST_READ.BIN $(REPACK_GTA_DIR)
	mkisofs -C 0,11702 -V $(PROJECT_NAME) -G IP.BIN -r -J -l -o $(PROJECT_NAME).iso $(REPACK_GTA_DIR)

$(PROJECT_NAME)-no-repack.iso: IP.BIN 1ST_READ.BIN $(REPACK_GTA_DIR)/GTA3SF8.b
	rm -f $(PROJECT_NAME)-no-repack.iso
	rm -f $(REPACK_GTA_DIR)/1ST_READ.BIN
	cp 1ST_READ.BIN $(REPACK_GTA_DIR)
	mkisofs -C 0,11702 -V $(PROJECT_NAME) -G IP.BIN -r -J -l -o $(PROJECT_NAME)-no-repack.iso $(REPACK_GTA_DIR)


$(PROJECT_NAME).ds.iso: IP.BIN 1ST_READ.BIN $(REPACK_DIR)/repacked $(REPACK_GTA_DIR)/GTA3SF8.b
	rm -f $(PROJECT_NAME).ds.iso
	rm -f $(REPACK_GTA_DIR)/1ST_READ.BIN
	cp $(TARGET).bin $(REPACK_GTA_DIR)/1ST_READ.BIN
	mkisofs -V $(PROJECT_NAME) -G IP.BIN -r -J -l -o $(PROJECT_NAME).ds.iso $(REPACK_GTA_DIR)

1ST_READ_PREBUILT.BIN:
	kos-objcopy -R .stack -O binary $(TARGET) $(TARGET)-prebuilt.bin 
	$(KOS_BASE)/utils/scramble/scramble $(TARGET)-prebuilt.bin 1ST_READ_PREBUILT.BIN
	mkdir -p $(REPACK_GTA_DIR)

$(PROJECT_NAME)-prebuilt.iso: IP.BIN 1ST_READ_PREBUILT.BIN $(REPACK_DIR)/repacked $(REPACK_GTA_DIR)/GTA3SF8.b
	rm -f $(REPACK_GTA_DIR)/1ST_READ.BIN
	cp 1ST_READ_PREBUILT.BIN $(REPACK_GTA_DIR)/1ST_READ.BIN
	mkisofs -C 0,11702 -V $(PROJECT_NAME) -G IP.BIN -r -J -l -o $(PROJECT_NAME).iso $(REPACK_GTA_DIR)

ifeq ($(HAVE_CDI4DC), yes)
$(PROJECT_NAME).cdi: $(PROJECT_NAME).iso
	cdi4dc $(PROJECT_NAME).iso $(PROJECT_NAME).cdi > cdi.log
	@echo && echo && echo "*** CDI Baked Successfully ***" && echo && echo

$(PROJECT_NAME)-no-repack.cdi: $(PROJECT_NAME)-no-repack.iso
	cdi4dc $(PROJECT_NAME)-no-repack.iso $(PROJECT_NAME)-no-repack.cdi > cdi.log
	@echo && echo && echo "*** CDI Baked Successfully ***" && echo && echo

$(PROJECT_NAME)-prebuilt.cdi: $(PROJECT_NAME)-prebuilt.iso
	cdi4dc $(PROJECT_NAME).iso $(PROJECT_NAME).cdi > cdi.log
	rm 1ST_READ_PREBUILT.BIN
	@echo && echo && echo "*** CDI Baked Successfully ***" && echo && echo
else
$(PROJECT_NAME).cdi: $(TARGET) $(REPACK_DIR)/repacked $(REPACK_GTA_DIR)/GTA3SF8.b
	mkdcdisc -e $(TARGET) -o $(PROJECT_NAME).cdi -d $(REPACK_GTA_DIR)/ $(MKDCDISC_PAD_OPTION) -n DCA3 -a "the gang"
	@echo && echo && echo "*** CDI Baked Successfully ***" && echo && echo

$(PROJECT_NAME)-no-repack.cdi: $(TARGET) $(REPACK_GTA_DIR)/GTA3SF8.b
	mkdcdisc -e $(TARGET) -o $(PROJECT_NAME)-no-repack.cdi -d $(REPACK_GTA_DIR)/ $(MKDCDISC_PAD_OPTION) -n DCA3 -a "the gang"
	@echo && echo && echo "*** CDI Baked Successfully ***" && echo && echo

$(PROJECT_NAME)-prebuilt.cdi: $(REPACK_DIR)/repacked $(REPACK_GTA_DIR)/GTA3SF8.b
	mkdcdisc -e $(TARGET) -o $(PROJECT_NAME).cdi -d $(REPACK_GTA_DIR)/ $(MKDCDISC_PAD_OPTION) -n DCA3 -a "the gang"
	@echo && echo && echo "*** CDI Baked Successfully ***" && echo && echo
endif

cdi: $(PROJECT_NAME).cdi

cdi-no-repack: $(PROJECT_NAME)-no-repack.cdi

dsiso: $(PROJECT_NAME).ds.iso

cdi-prebuilt: $(PROJECT_NAME)-prebuilt.cdi

sim: $(REPACK_DIR)/repacked
	$(MAKE) -f sim.mk

run-flycast: 
	flycast ./$(PROJECT_NAME).cdi

repack-dc: $(REPACK_DIR)/repacked

gprof:
	@sh-elf-gprof $(TARGET) $(REPACK_DIR)/kernel_gmon_1.out > gprof.out
	@cat gprof.out | gprof2dot | dot -Tpng -o $(TARGET)-kernel.png
	@-rm -rf gprof.out
	@echo "\033[42m Profiling data saved to $(TARGET)-kernel.png \033[0m"

# tools
imgtool: imgtool.cpp
	$(CXX) -std=c++17 -o $@ -Og $<

extract-sfx: extract-sfx.cpp
	$(CXX) -std=c++17 -o $@ -Og $<

pack-sfx: pack-sfx.cpp
	$(CXX) -std=c++17 -o $@ -Og $<

adf2mp3: adf2mp3.cpp
	$(CXX) -o $@ -O3 -g $<

aud2adpcm: aud2adpcm.c
	$(CC) -o $@ -O3 -g $< -I../vendor/minimp3

texconv: $(OBJS_TEXCONV) | pvrtex # You'll have to rebuild pvrtex manually if you change it
	$(CXX) -o $@ $(OBJS_TEXCONV)

%.texconv.o: %.cpp
	$(CXX) -std=c++2a -c -O0 -g -MMD -MP -o $@ -I../vendor/koshle -I../vendor/librw/src $(INCLUDE) -I../vendor/emu -I../vendor/crypto -I../vendor/TriStripper/include $(DEFINES) -DDC_TEXCONV -DDC_SIM $(TEXCONV_FLAGS) $<

-include $(DEPS)

#### Repacking ####

TXD_OPTS_fonts = 256 256
TXD_OPTS_fronten1 = 512 512
TXD_OPTS_fronten2 = 512 512
# TXD_OPTS_generic = 512 512
# TXD_OPTS_wheels = 512 512
TXD_OPTS_hud = 128 128
TXD_OPTS_INTRO = 512 512
# TXD_OPTS_MISC = 512 512
# TXD_OPTS_particle = 512 512
TXD_OPTS_intro1 = 512 512
TXD_OPTS_intro2 = 512 512
TXD_OPTS_INTRO3 = 512 512
TXD_OPTS_intro4 = 512 512
TXD_OPTS_LOADSC0 = 512 512
TXD_OPTS_LOADSC1 = 512 512
TXD_OPTS_LOADSC10 = 512 512
TXD_OPTS_LOADSC11 = 512 512
TXD_OPTS_LOADSC12 = 512 512
TXD_OPTS_LOADSC13 = 512 512
TXD_OPTS_LOADSC3 = 512 512
TXD_OPTS_LOADSC4 = 512 512
TXD_OPTS_LOADSC5 = 512 512
TXD_OPTS_LOADSC6 = 512 512
TXD_OPTS_LOADSC7 = 512 512
TXD_OPTS_LOADSC8 = 512 512
TXD_OPTS_LOADSC9 = 512 512
TXD_OPTS_NEWS = 128 128
TXD_OPTS_outro = 512 512
TXD_OPTS_SPLASH1 = 512 512
TXD_OPTS_SPLASH2 = 512 512
TXD_OPTS_SPLASH3 = 512 512

DEFAULT_RES = 512

PVR_ENCODER ?= PVRTEX
TEXTURE_DOWNSAMPLE_TXD ?= NONE
TEXTURE_DOWNSAMPLE_IMG ?= HALF

-include texlist.mk
-include modlist.mk
-include imgmisc.mk
-include gta3files.mk
-include sfxlist.mk
-include sfxlooplist.mk
-include wavlist.mk
-include mp3list.mk

IMG_TEXTURES_DC = $(addprefix $(REPACK_IMG_DC_DIR)/, $(IMG_TEXTURES))
IMG_MODELS_DC = $(addprefix $(REPACK_IMG_DC_DIR)/, $(IMG_MODELS))
IMG_MISC_DC = $(addprefix $(REPACK_IMG_DC_DIR)/, $(IMG_MISC))
LOOSE_FILES_DC = $(addprefix $(REPACK_GTA_DIR)/, $(MISC_FILES))
SFX_DC_DIR = $(REPACK_GTA_DIR)/sfx
SFX_DC_RAW = $(SFX_DC_DIR)/sfx_all.raw
SFX_DC_DSC = $(SFX_DC_DIR)/sfx_all.dsc
STREAM_ADPCM_DC = $(addprefix $(REPACK_GTA_DIR)/stream/, $(STREAM_WAV:.wav=.APM)) \
				  $(addprefix $(REPACK_GTA_DIR)/stream/, $(STREAM_MP3:.mp3=.APM)) \
				  $(addprefix $(REPACK_GTA_DIR)/stream/, $(STREAM_ADF:.adf=.APM))

IMG_TEXTURES_ORIG = $(addprefix $(REPACK_IMG_ORIG_DIR)/, $(IMG_TEXTURES))
IMG_MODELS_ORIG = $(addprefix $(REPACK_IMG_ORIG_DIR)/, $(IMG_MODELS))
IMG_MISC_ORIG = $(addprefix $(REPACK_IMG_ORIG_DIR)/, $(IMG_MISC))

SFX_ORIG = $(addprefix $(REPACK_SFX_ORIG_DIR)/, $(SFX_WAV))
SFX_ORIG_LOOP = $(addprefix $(REPACK_SFX_ORIG_DIR)/, $(SFX_LOOP_WAV))
SFX_REPACK_DC_WAV = $(addprefix $(REPACK_SFX_DC_DIR)/, $(SFX_WAV) $(SFX_LOOP_WAV))
SFX_REPACK_DC = $(SFX_REPACK_DC_WAV:.wav=.pcm)
STREAM_ADF_DECODED = $(addprefix $(REPACK_STREAM_DECODED_DIR)/, $(STREAM_ADF:.adf=.mp3))

.PRECIOUS: $(SFX_ORIG) $(SFX_REPACK_DC) $(STREAM_ADF_DECODED)

$(REPACK_DIR)/repacked: $(REPACK_GTA_DIR)/models/gta3.img $(REPACK_GTA_DIR)/models/gta3.dir $(LOOSE_FILES_DC) $(STREAM_ADPCM_DC) $(SFX_DC_RAW) $(SFX_DC_DSC)
	mkdir -p $(@D)
	@git archive --format zip --output "$(REPACK_GTA_DIR)/DCA3-$(GIT_VERSION).zip" HEAD
	@touch $@
	@echo && echo && echo "*** Repack Completed Successfully ***" && echo && echo
 
$(REPACK_DIR)/unpacked: imgtool $(GTA_DIR)/models/gta3.img $(GTA_DIR)/models/gta3.dir
	mkdir -p $(@D)
	./imgtool unpack "$(GTA_DIR)/models/gta3" "$(REPACK_IMG_ORIG_DIR)"
	@touch $@

$(IMG_TEXTURES_ORIG) $(IMG_MODELS_ORIG) $(IMG_MISC_ORIG): $(REPACK_DIR)/unpacked
	@test -f $@
	@touch $@

# First try the mods img directory
$(REPACK_IMG_DC_DIR)/%.dff: $(GTA_MOD_IMG_DIR)/%.dff texconv
	@mkdir -p $(@D)
	./texconv $< $@
$(REPACK_IMG_DC_DIR)/%.DFF: $(GTA_MOD_IMG_DIR)/%.DFF texconv
	@mkdir -p $(@D)
	./texconv $< $@

# if not, the extracted img directory
$(REPACK_IMG_DC_DIR)/%.dff: $(REPACK_IMG_ORIG_DIR)/%.dff texconv
	@mkdir -p $(@D)
	./texconv $< $@
$(REPACK_IMG_DC_DIR)/%.DFF: $(REPACK_IMG_ORIG_DIR)/%.DFF texconv
	@mkdir -p $(@D)
	./texconv $< $@

# first try the mods img directory. NB, the textures are not resized here, unlike normal .img textures
$(REPACK_IMG_DC_DIR)/%.txd: $(GTA_MOD_IMG_DIR)/%.txd texconv
	@mkdir -p $(@D)
	./texconv $< $@ 1024 1024 -e $(PVR_ENCODER) -d NONE
$(REPACK_IMG_DC_DIR)/%.TXD: $(GTA_MOD_IMG_DIR)/%.TXD texconv
	@mkdir -p $(@D)
	./texconv $< $@ 1024 1024 -e $(PVR_ENCODER) -d NONE

# if not, the extracted img directory
$(REPACK_IMG_DC_DIR)/%.txd: $(REPACK_IMG_ORIG_DIR)/%.txd texconv
	@mkdir -p $(@D)
	./texconv $< $@ $(DEFAULT_RES) $(DEFAULT_RES) -e $(PVR_ENCODER) -d $(TEXTURE_DOWNSAMPLE_IMG)
$(REPACK_IMG_DC_DIR)/%.TXD: $(REPACK_IMG_ORIG_DIR)/%.TXD texconv
	@mkdir -p $(@D)
	./texconv $< $@ $(DEFAULT_RES) $(DEFAULT_RES) -e $(PVR_ENCODER) -d $(TEXTURE_DOWNSAMPLE_IMG)

# First try the mods img directory
$(REPACK_IMG_DC_DIR)/%.col: $(GTA_MOD_IMG_DIR)/%.col
	@mkdir -p $(@D)
	cp $< $@
$(REPACK_IMG_DC_DIR)/%.ifp: $(GTA_MOD_IMG_DIR)/%.ifp
	@mkdir -p $(@D)
	cp $< $@

# if not, the extracted img directory
$(REPACK_IMG_DC_DIR)/%.col: $(REPACK_IMG_ORIG_DIR)/%.col
	@mkdir -p $(@D)
	cp $< $@
$(REPACK_IMG_DC_DIR)/%.ifp: $(REPACK_IMG_ORIG_DIR)/%.ifp
	@mkdir -p $(@D)
	cp $< $@

# first try the mods loose directory
$(REPACK_GTA_DIR)/%.dff: $(GTA_MOD_LOOSE_DIR)/%.dff texconv
	@mkdir -p $(@D)
	./texconv $< $@
$(REPACK_GTA_DIR)/%.DFF: $(GTA_MOD_LOOSE_DIR)/%.DFF texconv
	@mkdir -p $(@D)
	./texconv $< $@

#if not, the original files
$(REPACK_GTA_DIR)/%.dff: $(GTA_DIR)/%.dff texconv
	@mkdir -p $(@D)
	./texconv $< $@
$(REPACK_GTA_DIR)/%.DFF: $(GTA_DIR)/%.DFF texconv
	@mkdir -p $(@D)
	./texconv $< $@

# first try the mods loose directory
# Note the mods loose directory is not resized, unlike the normal .txd textures
$(REPACK_GTA_DIR)/%.txd: $(GTA_MOD_LOOSE_DIR)/%.txd texconv
	@mkdir -p $(@D)
	./texconv $< $@ 1024 1024 -e $(PVR_ENCODER) -d NONE
$(REPACK_GTA_DIR)/%.TXD: $(GTA_MOD_LOOSE_DIR)/%.TXD texconv
	@mkdir -p $(@D)
	./texconv $< $@ 1024 1024 -e $(PVR_ENCODER) -d NONE

# if not, the original files
$(REPACK_GTA_DIR)/%.txd: $(GTA_DIR)/%.txd texconv
	@mkdir -p $(@D)
	./texconv $< $@ $(TXD_OPTS_$(notdir $*)) -e $(PVR_ENCODER) -d $(TEXTURE_DOWNSAMPLE_TXD)
$(REPACK_GTA_DIR)/%.TXD: $(GTA_DIR)/%.TXD texconv
	@mkdir -p $(@D)
	./texconv $< $@ $(TXD_OPTS_$(notdir $*)) -e $(PVR_ENCODER) -d $(TEXTURE_DOWNSAMPLE_TXD)

$(REPACK_DIR)/packed: $(IMG_TEXTURES_DC) $(IMG_MODELS_DC) $(IMG_MISC_DC)
	mkdir -p $(@D)
	mkdir -p "$(REPACK_GTA_DIR)/models/gta3"
	./imgtool pack "$(REPACK_GTA_DIR)/models/gta3" "$(REPACK_IMG_DC_DIR)"
	@touch $@

$(REPACK_GTA_DIR)/models/gta3.img $(REPACK_GTA_DIR)/models/gta3.dir: $(REPACK_DIR)/packed
	@touch $@

# sfx processing
$(REPACK_DIR)/unpacked-sfx: extract-sfx $(GTA_DIR)/Audio/sfx.SDT $(GTA_DIR)/Audio/sfx.RAW
	mkdir -p $(@D)
	mkdir -p "$(REPACK_SFX_ORIG_DIR)"
	./extract-sfx "$(GTA_DIR)/Audio/sfx.SDT" "$(GTA_DIR)/Audio/sfx.RAW" "$(REPACK_SFX_ORIG_DIR)"
	@touch $@

$(SFX_ORIG): $(REPACK_DIR)/unpacked-sfx
	@test -f $@
	@touch $@

$(SFX_ORIG_LOOP): $(SFX_ORIG)
	@test -f $@
	@touch $@

# try first mods sfx directory
$(REPACK_SFX_DC_DIR)/%.pcm: $(GTA_MOD_SFX_DIR)/%.wav aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm -raw $< $@

# then original (extracted) sfx directory
$(REPACK_SFX_DC_DIR)/%.pcm: $(REPACK_SFX_ORIG_DIR)/%.wav aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm -raw $< $@

# stream processing

# first try the mods loose directory
$(REPACK_GTA_DIR)/stream/%.APM: $(GTA_MOD_LOOSE_DIR)/Audio/%.wav aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm $(AUDIO_STREAM_OPTION) $< $@

$(REPACK_STREAM_DECODED_DIR)/%.mp3: $(GTA_MOD_LOOSE_DIR)/Audio/%.adf adf2mp3
	@mkdir -p $(@D)
	./adf2mp3 $< $@

$(REPACK_GTA_DIR)/stream/%.APM: $(GTA_MOD_LOOSE_DIR)/Audio/%.mp3 aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm $(AUDIO_STREAM_OPTION) $< $@

$(REPACK_GTA_DIR)/stream/%.APM: $(REPACK_STREAM_DECODED_DIR)/%.mp3 aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm $(AUDIO_STREAM_OPTION) $< $@

# then original folder
$(REPACK_GTA_DIR)/stream/%.APM: $(GTA_DIR)/Audio/%.wav aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm $(AUDIO_STREAM_OPTION) $< $@

$(REPACK_STREAM_DECODED_DIR)/%.mp3: $(GTA_DIR)/Audio/%.adf adf2mp3
	@mkdir -p $(@D)
	./adf2mp3 $< $@

$(REPACK_GTA_DIR)/stream/%.APM: $(GTA_DIR)/Audio/%.mp3 aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm $(AUDIO_STREAM_OPTION) $< $@

$(REPACK_GTA_DIR)/stream/%.APM: $(REPACK_STREAM_DECODED_DIR)/%.mp3 aud2adpcm
	@mkdir -p $(@D)
	./aud2adpcm $(AUDIO_STREAM_OPTION) $< $@


# Note: This is last so it has least priority, files should be processed if possible
$(REPACK_GTA_DIR)/%: $(GTA_DIR)/%
	@mkdir -p $(@D)
	cp $< $@

$(SFX_DC_RAW): pack-sfx $(SFX_REPACK_DC)
	mkdir -p $(@D)
	./pack-sfx "$(GTA_DIR)/Audio/sfx.SDT" $(SFX_DC_RAW) $(SFX_DC_DSC) $(REPACK_SFX_DC_DIR)

 $(SFX_DC_DSC): $(SFX_DC_RAW)
	ls -l $@

analyze-profile: analyze-profile.cpp
	$(CXX) -std=c++17 -O3 analyze-profile.cpp -o analyze-profile
