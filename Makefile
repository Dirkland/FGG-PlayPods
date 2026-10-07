# FGG-PlayPods
#
#   export PS5_PAYLOAD_SDK=/opt/ps5-payload-sdk
#   make

PS5_HOST ?= ps5
PS5_PORT ?= 9021

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    ifneq ($(MAKECMDGOALS),check-offline)
    $(error PS5_PAYLOAD_SDK is undefined)
    endif
endif

ELF   := fgg-playpods.elf
BUILD := build

CFLAGS     := -DHCI_OUT_WAIT_REARM=0 -DSTREAM_LIMIT_MS=0 -std=c11 -Wall -Wextra -Werror -O2 -Isrc -Ithird_party/sbc
# Vendored code is built as upstream wrote it, without this project's
# warning policy.
SBC_CFLAGS := -std=gnu11 -O2 -w -Ithird_party/sbc

SRCS     := src/main.c src/capture.c src/hci.c src/bt.c src/sdp.c src/a2dp.c src/log.c src/session_lock.c
SBC_SRCS := third_party/sbc/sbc.c third_party/sbc/sbc_primitives.c
OBJS     := $(patsubst %.c,$(BUILD)/%.o,$(SRCS) $(SBC_SRCS))

.PHONY: all clean test check-offline

all: $(ELF)

$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/third_party/%.o: third_party/%.c
	@mkdir -p $(dir $@)
	$(CC) $(SBC_CFLAGS) -c -o $@ $<

$(BUILD)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^

# Linux-only fixtures; mocks every console operation and USB call.
check-offline:
	python3 tools/check-resident.py

clean:
	rm -rf $(BUILD) $(ELF)
