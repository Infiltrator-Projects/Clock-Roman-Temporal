# SPDX-License-Identifier: GPL-3.0-or-later
# This repository builds exactly one provider from its own copied sources.
.DEFAULT_GOAL := all
CC ?= cc
PKG_CONFIG ?= pkg-config
PYTHON ?= python3
BUILD_DIR ?= build
RESOURCE_NAME := Ancient Roman Temporal Clock
DEPENDENCIES := 
SOURCES := $(shell find src -type f -name '*.c' | LC_ALL=C sort)
TARGET := $(BUILD_DIR)/$(RESOURCE_NAME).so
ifneq ($(strip $(DEPENDENCIES)),)
DEP_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(DEPENDENCIES))
endif
ifneq ($(filter glib-2.0,$(DEPENDENCIES)),)
DEP_LIBS := $(shell $(PKG_CONFIG) --libs glib-2.0)
endif
CPPFLAGS += -Iinclude -Isrc -Isrc/support -Isrc/calendars -Isrc/calendars/engine -Isrc/clocks \
    -include include/provider-config.h -DGETTEXT_PACKAGE=\"infiltrator-temporal-resources\" \
    -DU_DISABLE_RENAMING=1 -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_74 \
    -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_74
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections \
    -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wshadow -Wstrict-prototypes \
    -Wmissing-prototypes -fstack-protector-strong -fno-common \
    -ffile-prefix-map=$(CURDIR)=. -fdebug-prefix-map=$(CURDIR)=.
LDLIBS += $(DEP_LIBS) -lm -ldl -pthread
.PHONY: all check audit-authorities clean
all:
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEP_CFLAGS) -shared $(SOURCES) \
	    -Wl,--version-script=exports.map -Wl,-z,defs -Wl,--gc-sections \
	    $(LDFLAGS) -o "$(TARGET)" $(LDLIBS)
check: all
	$(PYTHON) tests/check_provider.py "$(TARGET)"
	LANGUAGE=en_US LC_ALL=C.UTF-8 $(PYTHON) tests/check_accuracy.py "$(TARGET)"
audit-authorities: all
	LANGUAGE=en_US LC_ALL=C.UTF-8 $(PYTHON) tests/check_accuracy.py "$(TARGET)" --authorities
clean:
	rm -rf "$(BUILD_DIR)"

.PHONY: release deb native-deb
release:
	$(PYTHON) packaging/release.py release
deb:
	$(PYTHON) packaging/release.py deb --profile portable
native-deb:
	$(PYTHON) packaging/release.py deb --profile aggressive
