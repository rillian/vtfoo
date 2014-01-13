# This Source Code Form is subject to the terms of the Mozilla Public
# # License, v. 2.0. If a copy of the MPL was not distributed with this
# # file, You can obtain one at http://mozilla.org/MPL/2.0/.

all: vtfoo

CFLAGS ?= -g -Wall -O2
FRAMEWORKS := VideoToolbox CoreMedia CoreFoundation

vtfoo_SRCS := vtfoo.c
vtfoo_OBJS := $(vtfoo_SRCS:.c=.o)
vtfoo: $(vtfoo_OBJS)
	$(CC) $(LDFLAGS) $^ $(addprefix -framework ,$(FRAMEWORKS)) -o $@

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $^

clean:
	$(RM) $(vtfoo_OBJS)
	$(RM) vtfoo

check: all
	@echo No tests.

dist:
	@echo Not implemented.

.PHONY: all clean check dist
