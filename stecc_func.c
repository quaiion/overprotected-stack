#include "stecc.h"

CTOR_RES stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity) {         //  Минкап нужен, т.к. если просто давать стартовый размер, первый же поп его порежет, да еще и криво

    assert (stack);
    assert (mincapacity >= 0);

    CTOR_RES res_code = CTOR_SUCCESS;           //  Нужно ли инициализировать enum-переменные?
    
    stack -> typesize = typesize;
    stack -> data = (char*) calloc (mincapacity, typesize);
    stack -> size = 0;
    
    if (stack -> data == NULL && mincapacity != 0) {

        stack -> capacity = 0;

        res_code = CTOR_MEMERROR;
        return res_code;
    }

    stack -> capacity = mincapacity;

    return res_code;
}

DTOR_RES stack_dtor (stack_t* stack) {

    assert (stack);

    DTOR_RES res_code = DTOR_SUCCESS;

    size_t stk_cap = stack -> capacity;         //  Чтобы даже в случае memerror убить capacity

    stack -> size = SIZE_MAX;           //  Безоппасно, т.к. при обращении в буфер после деструкта прога все равно рухнет, а это - для случая холостого вытягивания размеров
    stack -> capacity = SIZE_MAX;           //  Надеемся, что пользователь не полезет рукми менять буфер, потому что тогда он себе навернет живую память
    stack -> mincapacity = SIZE_MAX;

    if (stack -> data == NULL && stk_cap != 0) {

        res_code = DTOR_MEMERROR;
        return res_code;
    }

    if (stack -> data) {

        memset (stack -> data, BROKEN_BYTE, stack -> typesize * stk_cap);
        free (stack -> data);
    }

    stack -> data = (char*) OS_RESERVED_ADDRESS;

    return res_code;
}

PUSH_RES stack_push (stack_t* stack, void* value) {

    assert (stack);
    assert (value);

    PUSH_RES res_code = PUSH_SUCCESS;

    if (stack -> data == NULL && stack -> capacity != 0) {

        res_code = PUSH_MEMERROR;
        return res_code;
    }

    if (stack -> size == stack -> capacity) {

        if (stack_resize_up (stack)) {

            res_code = PUSH_RESIZED;

        } else {

            res_code = PUSH_MEMERROR;
        }
    }

    memcpy (stack -> data + stack -> size ++ * stack -> typesize, (char*) value, stack -> typesize);

    return res_code;
}

POP_RES stack_pop (stack_t* stack, void* value) {

    assert (stack);
    assert (value);

    POP_RES res_code = POP_SUCCESS;

    if (stack -> data == NULL && stack -> capacity != 0) {

        res_code = POP_MEMERROR;
        return res_code;
    }

    if (stack -> size == 0) {

        res_code = POP_STKEMPTY;
        return res_code;
    }

    if (stack -> size == stack -> capacity / 4) {

        if (stack -> size >= stack -> mincapacity) {

            if (stack_resize_down (stack)) {

                res_code = POP_RESIZED;

            } else {

                res_code = POP_MEMERROR;
            }
        } else {

            res_code = POP_MINCAP;
        }
    }

    if (value == NULL) {

        value = calloc (1, stack -> typesize);
    }

    memcpy ((char*) value, stack -> data + -- stack -> size * stack -> typesize, stack -> typesize);

    return res_code;
}

char* stack_resize_up (stack_t* stack) {

    if (stack -> capacity == 0) {

        stack -> data = (char*) calloc (++ stack -> capacity, stack -> typesize);

    } else {

        stack -> data = (char*) realloc (stack -> data, (stack -> capacity *= 2) * stack -> typesize);
    }

    return stack -> data;
}

char* stack_resize_down (stack_t* stack) {

    stack -> data = (char*) realloc (stack -> data, (stack -> capacity /= 2) * stack -> typesize);

    return stack -> data;
}


/*

Пропиши демпфер ресайза вверх, если будет время
Хорошо бы кодстайл названий расшарить
Что будет если не char'ить, а оставлять войды? Однобайтовая указательная арифметика с ними тоже не работает? Воспримут ли войды memset, free, calloc&

*/