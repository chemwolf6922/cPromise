LIBPROMISE_DIR:=$(shell pwd)
BUILD_DIR=$(LIBPROMISE_DIR)/build/

CFLAGS?=-O3
override CFLAGS+=-MMD -MP
LDFLAGS?=

STATIC_LIB=libpromise.a

LIB_SRC=promise.c
PACK_LIBS=libmap.a

.PHONY:all
all:lib

.PHONY:lib
lib:$(STATIC_LIB)

$(STATIC_LIB):$(patsubst %.c,$(BUILD_DIR)%.o,$(LIB_SRC)) $(patsubst %.a,$(BUILD_DIR)%,$(PACK_LIBS))
	$(AR) -rcs $@ $(patsubst %.c,$(BUILD_DIR)%.o,$(LIB_SRC))
	$(AR) -qcs $@ $(patsubst %.a,$(BUILD_DIR)%/*,$(PACK_LIBS))

$(BUILD_DIR)libmap.a:
	$(MAKE) -C map lib
	cp map/libmap.a $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)lib%:$(BUILD_DIR)lib%.a
	mkdir -p $@
	cd $@; ar -x $<

$(BUILD_DIR)%.o:%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

-include $(patsubst %.c,$(BUILD_DIR)/%.d,$(SRC))

.PHONY:clean
clean:
	$(MAKE) -C map clean
	rm -rf $(BUILD_DIR)
	rm -f $(STATIC_LIB) 
