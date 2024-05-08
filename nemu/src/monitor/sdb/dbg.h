#ifndef __dbg_h__
#define __dbg_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>


//通过实现一系列转换来处理错误
//任何时候发生了错误，你的函数都会跳到执行清理和返回错误代码的“error:”区域。
//你可以使用check宏来检查错误代码，打印错误信息，然后跳到清理区域。
//你也可以使用一系列日志函数来打印出有用的调试信息。

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d:%s: " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d:%s: errno: %s) " M "\n", __FILE__, __LINE__, __func__, clean_errno(), ##__VA_ARGS__)

#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d:%s: errno: %s) " M "\n", __FILE__, __LINE__, __func__, clean_errno(), ##__VA_ARGS__)

#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d:%s) " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif
