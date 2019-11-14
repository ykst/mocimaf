#pragma once
#include "logger.h"
#include "md5.h"
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//#define NESTEST

#ifndef swap
#define swap(l, r)                                                             \
    ({                                                                         \
        typeof(l) __tmp = (l);                                                 \
        (l) = (r);                                                             \
        (r) = __tmp;                                                           \
    })
#endif

#ifndef negate
#define negate(l) ({ *(l) = !(*(l)); })
#endif

#ifndef sign
#define sign(x) ((x) >= 0 ? 1 : -1)
#endif

#ifndef max
#define max(a, b)                                                              \
    ({                                                                         \
        typeof(a) __ma = (a);                                                  \
        typeof(b) __mb = (b);                                                  \
        __ma > __mb ? __ma : __mb;                                             \
    })
#endif

#ifndef min
#define min(a, b)                                                              \
    ({                                                                         \
        typeof(a) __ma = (a);                                                  \
        typeof(b) __mb = (b);                                                  \
        __ma < __mb ? __ma : __mb;                                             \
    })
#endif

#ifndef range
#define range(l, x, r) (min(max(l, x), r))
#endif

#ifndef likely
#ifdef __builtin_expect
#define likely(x) __builtin_expect((x), 1)
#else
#define likely(x) (x)
#endif
#endif

#ifndef unlikely
#ifdef __builtin_expect
#define unlikely(x) __builtin_expect((x), 0)
#else
#define unlikely(x) (x)
#endif
#endif

#ifndef container_of
#define container_of(type, ptr, member)                                        \
    ({                                                                         \
        const typeof(((type *)0)->member) *__mptr = (ptr);                     \
        (type *)((char *)__mptr - offsetof(type, member));                     \
    })
#endif
#define LOGRAW(level, fmt, ...)                                                \
    if (global_logger) {                                                       \
        logger_append(global_logger, LOGLEVEL_##level, fmt, ##__VA_ARGS__);    \
    }

#define LOG(level, fmt, ...)                                                   \
    if (global_logger) {                                                       \
        logger_append(global_logger, LOGLEVEL_##level,                         \
                      #level " :%s:%s:%d: " fmt, __BASE_FILE__, __FUNCTION__,  \
                      __LINE__, ##__VA_ARGS__);                                \
    }

#define PRINT(fmt, ...) LOGRAW(ALWAYS, fmt, ##__VA_ARGS__)
#define TRACE(fmt, ...) LOGRAW(TRACE, fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) LOG(WARN, fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) LOG(INFO, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) LOG(ERROR, fmt, ##__VA_ARGS__)

#define MGUARD(b, action, fmt, ...)                                            \
    ({                                                                         \
        bool __b = (bool)(b);                                                  \
        if (unlikely(!(__b))) {                                                \
            ERROR("failed (%s): " fmt, #b, ##__VA_ARGS__);                     \
            action;                                                            \
        }                                                                      \
    })
#define MEXPECT(b, action, fmt, ...)                                           \
    ({                                                                         \
        bool __b = (bool)(b);                                                  \
        if (unlikely(!(__b))) {                                                \
            WARN("failed (%s): " fmt, #b, ##__VA_ARGS__);                      \
            action;                                                            \
        }                                                                      \
        __b;                                                                   \
    })

#define DIE(msg, ...)                                                          \
    ({                                                                         \
        ERROR(msg "\n", ##__VA_ARGS__);                                        \
        goto error;                                                            \
    })
#define GUARD(b) MGUARD(b, goto error, "\n")
// #define GUARD(b, action) MGUARD(b, action, "\n")
#define EXPECT(b, action) MEXPECT(b, action, "\n")

#ifdef DEBUG
#define ASSERT(b, action) MGUARD(b, action, "(debug)\n")
#define DBG(fmt, ...) LOG(DEBUG, fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, args...)
#define ASSERT(b, action)
//#define ASSERT(b)
#endif

#define DUMPD(x) TRACE("%s = %d\n", #x, (int)(x))
#define DUMPZ(x) TRACE("%s = %zu\n", #x, (size_t)(x))
#define DUMPP(x) TRACE("%s = %p\n", #x, (void *)(x))
#define DUMPS(x) TRACE("%s = %s\n", #x, (char *)(x))
#define DUMPC(x) TRACE("%s = %c\n", #x, (char)(x))
#define DUMPL(x) TRACE("%s = %llu\n", #x, (uint64_t)(x))
#define DUMP8(x) TRACE("%s = 0x%02x\n", #x, (uint8_t)(x))
#define DUMP16(x) TRACE("%s = 0x%04x\n", #x, (uint16_t)(x))
#define DUMP32(x) TRACE("%s = 0x%08x\n", #x, (uint32_t)(x))
#define DUMP64(x) TRACE("%s = 0x%016llx\n", #x, (uint64_t)(x))

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CLEARS(a, sz)                                                          \
    ({                                                                         \
        typeof(sz) __sz = (sz);                                                \
        memset(a, 0x00, (sizeof(*(a))) * __sz);                                \
    })

#define ALLOC(t) (t = (typeof(t))calloc(1, sizeof(typeof(*(t)))))
#define TALLOC(h)                                                              \
    do {                                                                       \
        h = NULL;                                                              \
        if (!(ALLOC(h))) {                                                     \
            ERROR("Cannot alloc %zuB\n", sizeof(*(h)));                        \
            goto error;                                                        \
        }                                                                      \
    } while (0)
#define TALLOCS(h, num)                                                        \
    ({                                                                         \
        typeof(num) __num = (num);                                             \
        if (!((h) = (typeof(h))(calloc((sizeof(*(h))), __num)))) {             \
            ERROR("Cannot alloc %zuB\n", (size_t)(sizeof(*(h)) * __num));      \
            goto error;                                                        \
        }                                                                      \
    })

#define REALLOC(t, num)                                                        \
    ({                                                                         \
        typeof(t) __t = (t);                                                   \
        typeof(num) __num = (num);                                             \
        size_t __sz = (__num) * sizeof(*(t));                                  \
        if (unlikely(!(__t = (typeof(t))realloc((t), __sz)))) {                \
            ERROR("Cannot realloc %zuB\n", __sz);                              \
            goto error;                                                        \
        }                                                                      \
        (t) = __t;                                                             \
    })
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CLOSE(x)                                                               \
    do {                                                                       \
        if ((x) >= 0) {                                                        \
            close(x);                                                          \
            (x) = -1;                                                          \
        }                                                                      \
    } while (0)
#define FREE(h)                                                                \
    do {                                                                       \
        if (h) {                                                               \
            free(h);                                                           \
            h = NULL;                                                          \
        }                                                                      \
    } while (0);
#define SUCCESS (1)
#define NG (0)

// worse is better, you know.
extern jmp_buf global_jump;
extern logger_ref global_logger;

#define THROW(msg, ...)                                                        \
    do {                                                                       \
        ERROR("*** exception: " msg "\n", ##__VA_ARGS__);                      \
        longjmp(global_jump, 1);                                               \
    } while (0)

#define TRY                                                                    \
    for (int _ = setjmp(global_jump) ? ({                                      \
             goto error;                                                       \
             0;                                                                \
         })                                                                    \
                                     : 1;                                      \
         _; _ = 0)

static inline void md5_hex(MD5_CTX *ctx, char dst[33])
{
    uint8_t *bin = (uint8_t *)(uint32_t[]){ctx->a, ctx->b, ctx->c, ctx->d};
    for (int i = 0; i < 16; ++i) {
        uint8_t h = *bin >> 4;
        uint8_t l = *bin & 0xF;
        *dst++ = h > 9 ? h + 87 : h + 48;
        *dst++ = l > 9 ? l + 87 : l + 48;
        ++bin;
    }
    *dst = '\0';
}
