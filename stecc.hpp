#ifndef STACK_ACTIVE
#define STACK_ACTIVE

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <limits.h>
#include <string.h>
#include <typeinfo>

#define stack_dump(stack_ptr, elem_fprint)                                                                                          \
    do {                                                                                                                            \
        stack_dump_ (stack_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__, elem_fprint);                                              \
    } while (0)

#define val_dump(value_ptr)                                                                                                         \
    do {                                                                                                                            \
        val_dump_ (value_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__);                                                             \
    } while (0)

enum VERIF_CODE {STK_OK = 0, NOT_CONSTRUCTED_YET = 1, NO_STK = -1, OVERFLOW = -2, TYPESIZE_ERR = -3, MEM_ERR = -4, CAP_DRAIN = -5,  \
                 MEMDMG_DATAHASH = -6, MEMDMG_PARAMHASH = -7,                                                                       \
                 LEFTDEAD_DATACAN = -8, RIGHTDEAD_DATACAN = -9, LEFTDEAD_PARAMCAN = -10, RIGHTDEAD_PARAMCAN = -11};

enum OPER_CODE {SUCCESS = 0, RECONSTRUCT = -1, MEMERROR = -2, RESIZED = 1, POPPED_EMPTY = 2, MINCAP_REACHED = 3, UNDEFINED = 4};

constexpr unsigned char BROKEN_BYTE = 0xFF;
constexpr int OS_RESERVED_ADDRESS = 1;
constexpr unsigned long long CANARY = 0x5AFE5AFE5AFE5AFE;

struct stack_t {

    unsigned long long left_canary;
    unsigned char* data;         //  Раз этим указателем пользуются только внутренние функции, зачем его войдовать и потом миллиард раз явно переприсваивать char'у?
    unsigned int typesize;
    size_t capacity;
    size_t mincapacity;
    size_t size;
    unsigned int data_hash;
    unsigned int param_hash;
    unsigned long long right_canary;
};

OPER_CODE stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity = 0);        //  Надо бы прописать в доке необходимость занулять стек перед конструкцией
OPER_CODE stack_dtor (stack_t* stack);

OPER_CODE stack_push (stack_t* stack, void* value);
OPER_CODE stack_pop (stack_t* stack, void* value);

VERIF_CODE stack_verify (stack_t* stack);
void stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,
                  void (*elem_fprint) (FILE*, const void*));

void val_dump_ (void* value, const int linenum, const char* funcname, const char* filename);

#endif

/*

Расшарить %m, * и [] вместе с ним
Надо бы как-то запретить пользователю пользоваться стеком напрямую (или войдовать data, но тогда возникает вопрос, как видит войды calloc, memset, memcpy и free)
Прописать демпфер ресайза вверх, если будет время
Хорошо бы кодстайл названий до конца расшарить
Что такое реактор, handler, exception?
В чем преимущество pretty?
Зачем списывать pretty_function, если есть funcsig?
Как адекватно вчертить в dump имя используемой стек-переменной? Тип данных в data? (#variable? как пользоваться typeid и классом type_info, чтобы не косячить?)
Могут ли в макросах бать параметры по умолчанию?
Как конфликтуют два параметра по умолчанию?
Гарантированное объявление в свиче с дефолтом - харам?
Дописать интегрируемость пользовательского верификатора value
Убрать обнуление новой части стека при ресайзе вверх (или обнуление ячейки при попе)? Может и в конструкторе нулить не надо?

*/