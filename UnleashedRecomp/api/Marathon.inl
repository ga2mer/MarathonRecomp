#pragma once

#include <cpu/guest_stack_var.h>
#include <kernel/function.h>

#define MARATHON_CONCAT2(x, y) x##y
#define MARATHON_CONCAT(x, y) MARATHON_CONCAT2(x, y)

#define MARATHON_INSERT_PADDING(length) \
    uint8_t MARATHON_CONCAT(pad, __LINE__)[length]

#define MARATHON_ASSERT_OFFSETOF(type, field, offset) \
    static_assert(offsetof(type, field) == offset)

#define MARATHON_ASSERT_SIZEOF(type, size) \
    static_assert(sizeof(type) == size)

#define MARATHON_VIRTUAL_FUNCTION(returnType, virtualIndex, ...) \
    GuestToHostFunction<returnType>(*(be<uint32_t>*)(g_memory.Translate(*(be<uint32_t>*)(this) + (4 * virtualIndex))), __VA_ARGS__)

struct marathon_null_ctor {};
