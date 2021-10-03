#ifndef STEC
#define STEC

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <limits.h>
#include <string.h>

enum CTOR_RES   {CTOR_SUCCESS = 0, CTOR_MEMERROR = -1, CTOR_RECNSTR = 1};
enum DTOR_RES   {DTOR_SUCCESS = 0, DTOR_MEMERROR = -1, DTOR_REDCNSTR = 1};
enum PUSH_RES   {PUSH_SUCCESS = 0, PUSH_MEMERROR = -1, PUSH_RESIZED = 1};
enum POP_RES    {POP_SUCCESS  = 0, POP_MEMERROR = -1, POP_RESIZED = 1, POP_STKEMPTY = 2, POP_MINCAP = 3};

constexpr int BROKEN_BYTE = 0xF000000;
constexpr int OS_RESERVED_ADDRESS = 1;

struct stack_t {

    char* data;         //  Раз этим указателем пользуются только внутренние функции, зачем его войдовать и потом миллиард раз явно переприсваивать char'у?
    size_t capacity;
    size_t mincapacity;
    size_t size;
    unsigned int typesize;
};

CTOR_RES stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity = 0);
DTOR_RES stack_dtor (stack_t* stack);

PUSH_RES stack_push (stack_t* stack, void* value);
POP_RES stack_pop (stack_t* stack, void* value);

char* stack_resize_up (stack_t* stack);
char* stack_resize_down (stack_t* stack);

#endif