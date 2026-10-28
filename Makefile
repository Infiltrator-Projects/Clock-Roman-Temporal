# SPDX-License-Identifier: GPL-3.0-or-later
# Clock-specific logic stays local; Common owns generic code and ABI.
.DEFAULT_GOAL := all
CC ?= cc
PKG_CONFIG ?= pkg-config
PYTHON ?= python3
BUILD_DIR ?= build
RESOURCE_NAME := Ancient Roman Temporal Clock
COMMON_DIR := common
COMMON_REVISION := dc4aac326964ca4400b6e21851d16eea674040c8
COMMON_VERSION := 1.19.80
COMMON_BUILD_DIR := $(abspath $(BUILD_DIR))/common
COMMON_ARCHIVE := $(COMMON_BUILD_DIR)/libinfiltratr-common.a
DEPENDENCIES := 
SOURCES := $(shell find src -type f -name '*.c' | LC_ALL=C sort)
TARGET := $(BUILD_DIR)/$(RESOURCE_NAME).so
ifneq ($(strip $(DEPENDENCIES)),)
DEP_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(DEPENDENCIES))
endif
ifneq ($(filter glib-2.0,$(DEPENDENCIES)),)
DEP_LIBS := $(shell $(PKG_CONFIG) --libs glib-2.0)
endif
CPPFLAGS += -I$(COMMON_DIR)/include -Iinclude -Isrc -Isrc/support -Isrc/calendars -Isrc/calendars/engine -Isrc/clocks \
    -include include/provider-config.h -DGETTEXT_PACKAGE=\"infiltrator-temporal-resources\" \
    -DU_DISABLE_RENAMING=1 -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_74 \
    -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_74
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections \
    -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wshadow -Wstrict-prototypes \
    -Wmissing-prototypes -fstack-protector-strong -fno-common \
    -ffile-prefix-map=$(CURDIR)=. -fdebug-prefix-map=$(CURDIR)=.
LDLIBS += $(DEP_LIBS) -lm -ldl -pthread
.PHONY: common-check
common-check:
	@if test ! -f common/VERSION; then git submodule update --init --depth 1 -- common; fi
	@grep -qx "$(COMMON_VERSION)" common/VERSION
	@if test -e common/.git; then git -C common rev-parse HEAD | grep -qx "$(COMMON_REVISION)"; fi
	@test ! -d include/infiltratr || { echo "Copied Common headers forbidden" >&2; exit 1; }
$(COMMON_ARCHIVE): common-check
	@mkdir -p "$(COMMON_BUILD_DIR)"
	$(MAKE) -C common CC="$(CC)" BUILD_DIR="$(COMMON_BUILD_DIR)" CFLAGS="$(CFLAGS) -Wno-missing-profile" all
.PHONY: all check audit-authorities clean
all: $(COMMON_ARCHIVE)
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEP_CFLAGS) -shared $(SOURCES) $(COMMON_ARCHIVE) \
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
