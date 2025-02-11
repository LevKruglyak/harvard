#pragma once

#include <stdbool.h>
#include <stddef.h>

struct tpool;
typedef struct tpool thread_pool;

typedef void (*thread_func_t)(void *arg);

thread_pool *new_thread_pool(size_t num);
void delete_thread_pool(thread_pool *tm);

bool add_task_thread_pool(thread_pool *tm, thread_func_t func, void *arg);
void wait_thread_pool(thread_pool *tm);
