# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

SOURCES += $(wildcard modules/*.cpp)
SOURCES += $(wildcard modules/**/*.cpp)
DISTRIBUTABLES += $(wildcard modules/**/*.svg)
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)
include $(RACK_DIR)/plugin.mk