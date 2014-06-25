#pragma once

enum log_level { LOG_LEVEL_DEBUG = 0, LOG_LEVEL_INFO, LOG_LEVEL_ERROR };

extern void log_init(void);
extern void log_quiet(void);
extern void log_noisy(void);
extern void log_shader_errors(const char *fn, unsigned object);
extern void log_msg(const char *fn, enum log_level level, const char *fmt, ...);
extern void log_lib_errors(const char *fn);


#define LOG_DEBUG(...) log_msg(__func__, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG(...) log_msg(__func__, LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_ERROR(...) log_msg(__func__, LOG_LEVEL_ERROR, __VA_ARGS__)
