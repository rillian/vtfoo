# This Source Code Form is subject to the terms of the Mozilla Public
# # License, v. 2.0. If a copy of the MPL was not distributed with this
# # file, You can obtain one at http://mozilla.org/MPL/2.0/.

PACKAGE := vtfoo
VERSION := $(or $(shell git describe --tags --dirty --always),unknown)

PROGRAMS := vtfoo

CFLAGS ?= -g -Wall -O2
FRAMEWORKS := VideoToolbox CoreMedia CoreFoundation

vtfoo_SRCS := vtfoo.c

## Below this point is boilerplate

all: $(PROGRAMS)

clean:
	$(RM) $(ALL_OBJS)
	$(RM) $(PROGRAMS)

check: all
	for prog in $(PROGRAMS); do echo ./$$(prog) && ./$$(prog); done

dist:
	@echo Not implemented.

.PHONY: all clean check dist

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $^

define PROGRAM_template =
$(1)_OBJS := $$($(1)_SRCS:%.c=%.o)
ALL_OBJS += $$($(1)_OBJS)
$(1) : $$($(1)_OBJS)
	$$(CC) $$(LDFLAGS) $$^ $$(addprefix -framework ,$$(FRAMEWORKS)) -o $$@
endef

$(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))
