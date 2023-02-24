PROJECT_PATH=$(shell pwd)
BUILD_DIR=$(PROJECT_PATH)/build/


CFLAGS?=-g
override CFLAGS+=-MMD -MP
override CFLAGS+=-I..
LDFLAGS?=

TEST_PROMISE=test_promise
TEST_PROMISE_SRC=test_promise.c promise.c
TEST_PROMISE_STATIC_LIBS=libmap.a
TEST_PROMISE_SHARED_LIBS=

TEST_ASYNC=test_async
TEST_ASYNC_SRC=test_async.c promise.c
TEST_ASYNC_STATIC_LIBS=libmap.a
TEST_ASYNC_SHARED_LIBS=


.PHONY:all
all:$(TEST_PROMISE) $(TEST_ASYNC)

$(TEST_PROMISE):$(patsubst %.c,$(BUILD_DIR)%.o,$(TEST_PROMISE_SRC)) $(patsubst %,$(BUILD_DIR)%,$(TEST_PROMISE_STATIC_LIBS))
	$(CC) $(LDFLAGS) -o $@ $^ $(patsubst %,-l%,$(TEST_PROMISE_SHARED_LIBS))

$(TEST_ASYNC):$(patsubst %.c,$(BUILD_DIR)%.o,$(TEST_ASYNC_SRC)) $(patsubst %,$(BUILD_DIR)%,$(TEST_ASYNC_STATIC_LIBS))
	$(CC) $(LDFLAGS) -o $@ $^ $(patsubst %,-l%,$(TEST_ASYNC_SHARED_LIBS))

$(BUILD_DIR)%.o:../%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)%.o:%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)libmap.a:
	$(MAKE) -C ../map lib
	cp ../map/libmap.a $@

-include $(patsubst %.c,$(BUILD_DIR)%.d,$(SRC))

.PHONY:clean
clean:
	$(MAKE) -C ../map clean
	rm -rf $(BUILD_DIR)
	rm -f $(TEST_PROMISE)
	rm -f $(TEST_ASYNC)

