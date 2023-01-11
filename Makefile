CFLAGS?=-O3
override CFLAGS+=-MMD -MP
LDFLAGS?=
STATIC_LIB=libpromise.a
LIB_SRC=promise.c promise_all.c promise_any.c
TEST_SRC=test.c
ALL_SRC=$(TEST_SRC) $(LIB_SRC)

LIB_MAP=map/libmap.a
LIBS=$(LIB_MAP)

.PHONY:all
all:test lib

test:$(patsubst %.c,%.oo,$(TEST_SRC)) $(patsubst %.c,%.o,$(LIB_SRC)) $(LIBS)
	$(CC) $(LDFLAGS) -o $@ $^

.PHONY:lib
lib:$(STATIC_LIB)

$(STATIC_LIB):$(patsubst %.c,%.o,$(LIB_SRC)) $(LIBS)
	for lib in $(LIBS); do \
		$(AR) -x $$lib; \
	done
	$(AR) -rcs $@ *.o

$(LIB_MAP):
	$(MAKE) -C map lib

%.oo:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o:%.c
	$(CC) $(CFLAGS) -c $<

-include $(TEST_SRC:.c=.d)

clean:
	$(MAKE) -C map clean
	rm -f *.oo *.o *.d test $(STATIC_LIB) 
