#ifndef STACK_ACTIVE
#define STACK_ACTIVE

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <limits.h>
#include <string.h>
#include "stack_type.cpp"

#define stack_dump(stack_ptr, elem_fprint)                                                                                          \
    do {                                                                                                                            \
        stack_dump_ (stack_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__, elem_fprint);                                              \
    } while (0)

#define val_dump(value_ptr)                                                                                                         \
    do {                                                                                                                            \
        val_dump_ (value_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__);                                                             \
    } while (0)

enum VERIF_CODE {STK_OK = 0b0000000000000000, NOT_CONSTRUCTED_YET = 0b1000000000000000, NO_STK = 0b0000000000000010,                \
                 OVERFLOW = 0b0000000000000100, TYPESIZE_ERR = 0b0000000000001000, MEM_ERR = 0b0000000000010000,                    \
                 CAP_DRAIN = 0b0000000000100000, MEMDMG_DATAHASH = 0b0000000001000000, MEMDMG_PARAMHASH = 0b0000000010000000,       \
                 LEFTDEAD_DATACAN = 0b0000000100000000, RIGHTDEAD_DATACAN = 0b0000001000000000,                                     \
                 LEFTDEAD_PARAMCAN = 0b0000010000000000, RIGHTDEAD_PARAMCAN = 0b0000100000000000};

enum OPER_CODE {SUCCESS = 0, RECONSTRUCT = -1, MEMERROR = -2, RESIZED = 1, POPPED_EMPTY = 2, UNDEFINED = 3};

constexpr unsigned char BROKEN_BYTE = 0xFF;
constexpr int OS_RESERVED_ADDRESS = 1;
constexpr unsigned long long CANARY = 0x5AFE5AFE5AFE5AFE;

OPER_CODE stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity = 0);
OPER_CODE stack_dtor (stack_t* stack);

OPER_CODE stack_push (stack_t* stack, void* value);
OPER_CODE stack_pop (stack_t* stack, void* value);

unsigned short stack_verify (stack_t* stack);
void stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,
                  void (*elem_fprint) (FILE*, const void*));

void val_dump_ (void* value, const int linenum, const char* funcname, const char* filename);

#endif

/*

Реализовать условную компиляцию с полной верификацией и без нее
Сегфолтнется ли попытка прибавить значение к нулевому поинтеру?

*/