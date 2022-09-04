################################################################################
# @file   makefile
# @author Lucas Jadilo
# @brief  Build automation for multi-target projects using GNU Make.
# @details
# The variables `n_TARGET` and `n_SRC_FILES` must be defined for each target.
# `n_TARGET` must contain the name of the nth target, and `n_SRC_FILES` must
# contain a list of the source files necessary to build the nth target. Each
# source file string must contain its path relative to makefile's directory.
#
# The variable `ENABLE_TARGETS` must contain the indexes of the targets that
# will be expanded in the makefile. Any target outside this list will be ignored.
#
# The variable `INC_DIRS` must contain a list of directories where the compiler
# should look for header files. This list applies to every target.
#
# The command `make` or `make all` will build (but not execute) all the targets.
#
# The command `make clean` will delete everything that was created during build
# (i.e., the directory defined in `BUILD_DIR` and everything in it).
#
# For each target, a phony target will be created named the same way as defined
# in `n_TARGET`. The command `make <target>` (<target> = phony target) will
# build and execute the corresponding target.
################################################################################

ENABLE_TARGETS := 1 2 3 4 5

INC_DIRS := src test/unity

1_TARGET    := test
1_SRC_FILES := src/snap.c test/test_snap.c test/unity/unity.c test/unity/unity_fixture.c

2_TARGET    := example1
2_SRC_FILES := src/snap.c src/examples/example1.c

3_TARGET    := example2
3_SRC_FILES := src/snap.c src/examples/example2.c

4_TARGET    := example3
4_SRC_FILES := src/snap.c src/examples/example3.c

5_TARGET    := example4
5_SRC_FILES := src/snap.c src/examples/example4.c

BUILD_DIR := build
BIN_DIR   := $(BUILD_DIR)/bin
OBJ_DIR   := $(BUILD_DIR)/obj

DOC_DIR  := doc/doxygen
HTML_DIR := $(DOC_DIR)/html
DOXYFILE := $(DOC_DIR)/doxyfile

CC := gcc

CPPFLAGS  = $$(addprefix -I ,$(INC_DIRS))
CPPFLAGS += -MMD -MP -MF $$(patsubst $(OBJ_DIR)/%.o,$(OBJ_DIR)/%.d,$$@) -MT $$@
CPPFLAGS += -D UNITY_FIXTURE_NO_EXTRAS
CPPFLAGS += -D SNAP_SIZE_USER_HASH=3
CPPFLAGS += -D SNAP_CRC8_TABLE
CPPFLAGS += -D SNAP_CRC16_TABLE
CPPFLAGS += -D SNAP_CRC32_TABLE

CFLAGS  = -c
CFLAGS += -std=c99
CFLAGS += -O2
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpedantic
CFLAGS += -Wcast-align
CFLAGS += -Wpointer-arith
CFLAGS += -Wswitch-default
CFLAGS += -Wshadow
CFLAGS += -Wdouble-promotion
CFLAGS += -Wformat=2
CFLAGS += -Wformat-truncation
CFLAGS += -Wconversion
CFLAGS += -Wundef
CFLAGS += -Wunused-macros
CFLAGS += -Wunreachable-code
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wold-style-definition
CFLAGS += -Wstrict-prototypes
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror

LDFLAGS := -Wl,--gc-sections

all:

ifeq ($(OS),Windows_NT)
    TARGET_EXTENSION := .exe
    ifeq ($(shell uname -s 2>/dev/null),)
        SHELL_SYNTAX := Command Prompt
    else
        SHELL_SYNTAX := Bash
    endif
else
    SHELL_SYNTAX := Bash
endif

ifeq ($(SHELL_SYNTAX),Command Prompt)
    NEWLINE  := @echo.
    MKDIR     = mkdir $(subst /,\,$(1))
    RMDIR     = rmdir /s /q $(subst /,\,$(1)) 1>nul 2>nul || rem
    RUN       = $(subst /,\,$(1))
    OPENDOC  := cd $(HTML_DIR) & start index.html
    CLEANDOC := del /q $(subst /,\,$(DOC_DIR)/*.chm) 1>nul 2>nul & rmdir /s /q $(subst /,\,$(HTML_DIR)) 1>nul 2>nul || rem
else ifeq ($(SHELL_SYNTAX),Bash)
    NEWLINE  := @echo
    MKDIR     = mkdir -p $(1)
    RMDIR     = rm -rf $(1)
    RUN       = $(1)
    OPENDOC  := firefox ${PWD}/$(HTML_DIR)/index.html
    CLEANDOC := rm -rf $(DOC_DIR)/*.chm ; rm -rf $(HTML_DIR)
else
    $(error Error: Shell syntax not supported)
endif

define SETUP_SRCS
SRC_FILES += $$($(1)_SRC_FILES)
endef

define SETUP_OBJS
OBJ_DIRS  := $$(sort $$(patsubst %/,$(OBJ_DIR)/%,$$(dir $(SRC_FILES))))
OBJ_FILES := $$(sort $$(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES)))
DEP_FILES := $$(patsubst %.o,%.d,$$(OBJ_FILES))

$$(OBJ_FILES): $(OBJ_DIR)/%.o: %.c $(OBJ_DIR)/%.d | $$(OBJ_DIRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $$@ $$<
	$(NEWLINE)

$$(DEP_FILES):
-include $$(DEP_FILES)
endef

define SETUP_BINS
$(1)_OBJ_FILES := $$(patsubst %.c,$(OBJ_DIR)/%.o,$$($(1)_SRC_FILES))
$(1)_BIN_FILE   = $(BIN_DIR)/$$($(1)_TARGET)$(TARGET_EXTENSION)
BIN_FILES += $$($(1)_BIN_FILE)

.PHONY: $$($(1)_TARGET)
$$($(1)_TARGET): $$($(1)_BIN_FILE)
	$$(call RUN,$$($(1)_BIN_FILE))

$$($(1)_BIN_FILE): $$($(1)_OBJ_FILES) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $$@ $$^
	$(NEWLINE)
endef

$(foreach target,$(ENABLE_TARGETS),$(eval $(call SETUP_SRCS,$(target))))
$(eval $(SETUP_OBJS))
$(foreach target,$(ENABLE_TARGETS),$(eval $(call SETUP_BINS,$(target))))

.PHONY: all
all: $(BIN_FILES)

.PHONY: clean
clean:
	$(call RMDIR,$(BUILD_DIR))

$(OBJ_DIRS) $(BIN_DIR):
	$(call MKDIR,$@)

.PHONY: doc
doc:
	doxygen $(DOXYFILE)

.PHONY: open-doc
open-doc:
	$(OPENDOC)

.PHONY: clean-doc
clean-doc:
	$(CLEANDOC)

# Debug
#$(file > debug.txt,$(foreach target,$(ENABLE_TARGETS),$(call SETUP_SRCS,$(target))))
#$(file >> debug.txt,)
#$(file >> debug.txt,$(SETUP_OBJS))
#$(file >> debug.txt,)
#$(file >> debug.txt,$(foreach target,$(ENABLE_TARGETS),$(call SETUP_BINS,$(target))))

################################# END OF FILE ##################################
