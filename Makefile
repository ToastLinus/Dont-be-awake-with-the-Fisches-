# Nintendo 3DS homebrew Makefile
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. Try installing devkitARM.")
endif

include $(DEVKITARM)/3ds_rules

TARGET      := fish-night
BUILD       := build
SOURCES     := source
DATA        :=
INCLUDES    := include
GRAPHICS    :=

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard
CFLAGS      = -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS    = $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS        := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS     := $(DEVKITPRO)/libctru $(DEVKITPRO)/portlibs/3ds

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES     := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES     := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CC)
export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)

.PHONY: all clean
all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf

else
.PHONY: all
DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)
endif
