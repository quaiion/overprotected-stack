#include "stecc.hpp"

#define dev_stack_dump(stack_ptr, oper_code, verif_code)                                                                        \
    do {                                                                                                                        \
        dev_stack_dump_ (stack_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__, oper_code, verif_code);                            \
    } while (0)

void dev_stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,                      \
                      OPER_CODE oper_code, VERIF_CODE verif_code);
                  
unsigned char* stack_resize_up (stack_t* stack);
unsigned char* stack_resize_down (stack_t* stack);

unsigned int hash (const unsigned char* data, size_t size);
void stack_hash (stack_t* stack);

OPER_CODE stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity) {         //  Минкап нужен, т.к. если просто давать стартовый размер, первый же поп его порежет, да еще и криво

    OPER_CODE oper_code = UNDEFINED;           //  Нужно ли инициализировать enum-переменные в общем случае?
    VERIF_CODE verif_code = NOT_CONSTRUCTED_YET;

    if (stack_verify (stack) == NO_STK) {       //  Сработает из-за приоритета NO_STK в верификаторе

        verif_code = NO_STK;
        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (stack -> data || stack -> size || stack -> capacity || stack -> mincapacity || stack -> typesize) {

        oper_code = RECONSTRUCT;
        dev_stack_dump (stack, oper_code, verif_code);

        return oper_code;
    }

    oper_code = SUCCESS;
    
    stack -> typesize = typesize;
    stack -> size = 0;
    stack -> mincapacity = mincapacity;
    stack -> capacity = mincapacity;
    stack -> left_canary = CANARY;          //  Надо как-то закладываться на вариант, что стек будут юзать на аппаратуре с НЕ 16-байтным лонг-лонгом? Cейчас я этого не делаю
    stack -> right_canary = CANARY;

    size_t datasize = mincapacity * typesize + 2 * sizeof (unsigned long long);
    stack -> data = (unsigned char*) malloc (datasize);

    verif_code = stack_verify (stack);
    if (verif_code != STK_OK && verif_code != MEMDMG_DATAHASH && verif_code != MEMDMG_PARAMHASH &&      \
        verif_code != LEFTDEAD_DATACAN && verif_code != RIGHTDEAD_DATACAN) {

        if (verif_code == MEM_ERR) {              //  Чтобы не возникло диссонанса, когда верификатор дает ошибку, код операции чистый (в верификаторе ошибка даты стоит на (почти) высшем приоритете, а значит ориентироваться на его код ошибки можно) - после дампа только инциализация памяти, которую верификатор все равно не чекает

            oper_code = MEMERROR;
        }

        dev_stack_dump (stack, oper_code, verif_code);
    }

    memset (stack -> data, 0, datasize);
    *((unsigned long long*) stack -> data) = CANARY;
    *((unsigned long long*) (stack -> data + datasize - sizeof (unsigned long long))) = CANARY;
    stack -> data += sizeof (unsigned long long);

    stack_hash (stack);

    return oper_code;
}

OPER_CODE stack_dtor (stack_t* stack) {

    OPER_CODE oper_code = UNDEFINED;
    VERIF_CODE verif_code = stack_verify (stack);

    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    oper_code = SUCCESS;

    memset (stack -> data, BROKEN_BYTE, stack -> typesize * stack -> capacity);
    free (stack -> data - sizeof (unsigned long long));

    stack -> data = (unsigned char*) OS_RESERVED_ADDRESS;
    stack -> size = SIZE_MAX;           //  Безоппасно, т.к. при обращении в буфер после деструкта прога все равно рухнет, а это - для случая холостого вытягивания размеров
    stack -> capacity = SIZE_MAX;           //  Надеемся, что пользователь не полезет рукми менять буфер, потому что тогда он себе навернет живую память
    stack -> mincapacity = SIZE_MAX;

    return oper_code;
}

OPER_CODE stack_push (stack_t* stack, void* value) {

    OPER_CODE oper_code = UNDEFINED;
    VERIF_CODE verif_code = stack_verify (stack);
    
    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (!value) {

        val_dump (value);
    }

    oper_code = SUCCESS;

    if (stack -> size == stack -> capacity) {

        if (stack_resize_up (stack)) {

            oper_code = RESIZED;

        } else {

            oper_code = MEMERROR;
        }
    }

    verif_code = stack_verify (stack);

    if (verif_code != STK_OK && verif_code != MEMDMG_DATAHASH && verif_code != MEMDMG_PARAMHASH) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    memcpy (stack -> data + stack -> size ++ * stack -> typesize, (unsigned char*) value, stack -> typesize);       //  После верификатора во избежание немого сегфолта

    stack_hash (stack);

    return oper_code;
}

OPER_CODE stack_pop (stack_t* stack, void* value) {

    OPER_CODE oper_code = UNDEFINED;
    VERIF_CODE verif_code = stack_verify (stack);
    
    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (!value) {

        val_dump (value);
    }

    oper_code = SUCCESS;

    if (stack -> size == 0) {

        oper_code = POPPED_EMPTY;
        return oper_code;
    }

    memcpy ((unsigned char*) value, stack -> data + -- stack -> size * stack -> typesize, stack -> typesize);

    if (stack -> size == (stack -> capacity - 3) / 4) {         //  Технически оно может ложно сработать по причине унсигнед, но только тогда size априори больше capacity, а значит его срежет верификатор

        if (stack -> size >= stack -> mincapacity) {

            if (stack_resize_down (stack)) {

                oper_code = RESIZED;

            } else {

                oper_code = MEMERROR;
            }
        } else {

            oper_code = MINCAP_REACHED;
        }
    }

    stack_hash (stack);

    verif_code = stack_verify (stack);
    
    if (verif_code != STK_OK || oper_code == POPPED_EMPTY) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    return oper_code;
}

unsigned char* stack_resize_up (stack_t* stack) {

    stack -> data = (unsigned char*) realloc (stack -> data - sizeof (unsigned long long),                          \
    (stack -> capacity = stack -> capacity * 2 + 1) * stack -> typesize + 2 * sizeof (unsigned long long));

    memset (stack -> data + stack -> size * stack -> typesize, 0, stack -> capacity - stack -> size + sizeof (unsigned long long));
    *((unsigned long long*) (stack -> data + stack -> capacity)) = CANARY;

    return stack -> data;
}

unsigned char* stack_resize_down (stack_t* stack) {

    stack -> data = (unsigned char*) realloc (stack -> data - sizeof (unsigned long long),                          \
    (stack -> capacity = (stack -> capacity - 1) / 2) * stack -> typesize + 2 * sizeof (unsigned long long));

    *((unsigned long long*) (stack -> data + stack -> capacity)) = CANARY;

    return stack -> data;
}

VERIF_CODE stack_verify (stack_t* stack) {

    VERIF_CODE verif_code = STK_OK;

    if (! stack) {
        
        verif_code = NO_STK;
        return verif_code;
    }

    if (stack -> data == NULL) {

        verif_code = MEM_ERR;
        return verif_code;
    }

    if (stack -> size > stack -> capacity) {

        verif_code = OVERFLOW;
        return verif_code;
    }

    if (stack -> typesize == 0) {

        verif_code = TYPESIZE_ERR;
        return verif_code;
    }

    if (stack -> capacity < stack -> mincapacity) {

        verif_code = CAP_DRAIN;
        return verif_code;
    }

    if (*((unsigned long long*) (stack -> data - sizeof (unsigned long long))) != CANARY) {

        verif_code = LEFTDEAD_DATACAN;
        return verif_code;
    }

    if (*((unsigned long long*) (stack -> data + stack -> capacity * stack -> typesize)) != CANARY) {

        verif_code = RIGHTDEAD_DATACAN;
        return verif_code;
    }

    if (stack -> left_canary != CANARY) {

        verif_code = LEFTDEAD_PARAMCAN;
        return verif_code;
    }

    if (stack -> right_canary != CANARY) {

        verif_code = RIGHTDEAD_PARAMCAN;
        return verif_code;
    }

    if (hash (stack -> data, stack -> capacity * stack -> typesize + 2 * sizeof (unsigned long long)) != stack -> data_hash) {

        verif_code = MEMDMG_DATAHASH;
        return verif_code;
    }

    unsigned int temp_hash = stack -> param_hash;
    stack -> param_hash = 0;
    stack -> param_hash = hash ((unsigned char*) stack, sizeof (stack_t));

    if (stack -> param_hash != temp_hash) {

        verif_code = MEMDMG_PARAMHASH;
        return verif_code;
    }

    return verif_code;
}

void stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,
                  void (*elem_fprint) (FILE*, const void*)) {

    FILE* logfile = fopen ("LOG.txt", "a");

    fprintf (logfile, "\n\ndump called from file %s; function %s; line %d", filename, funcname, linenum);

    if (stack) {

        fprintf (logfile, "\n\nstack address [%p]", stack);
        fprintf (logfile, "\n\nsize %llu\ncapacity %llu\nmincapacity %llu\ntypesize %d\n\ndata [%p]\n", stack -> size, stack -> capacity, \
                 stack -> mincapacity, stack -> typesize, stack -> data);

        size_t sizesize = stack -> size * stack -> typesize;
        size_t i = 0;
        for ( ; i < sizesize; i += stack -> typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack -> typesize);
            elem_fprint (logfile, stack -> data + i);
        }
        fprintf (logfile, " ____ SIZE EDGE");

        size_t capsize = stack -> capacity * stack -> typesize;
        for ( ; i < capsize; i += stack -> typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack -> typesize);
            elem_fprint (logfile, stack -> data + i);
        }

    } else {

        fprintf (logfile, "\n-------------------------------------\n\nWARNING: dump function found no stack address");
    }

    fprintf (logfile, "\n\n__________________________________\n");

    fclose (logfile);
}

void dev_stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,                         \
                      OPER_CODE oper_code, VERIF_CODE verif_code) {

    FILE* logfile = fopen ("LOG.txt", "a");

    const char* oper_res = NULL;
    switch (oper_code) {

        case SUCCESS:
            oper_res = "the operation was fully successful";
            break;

        case RECONSTRUCT:
            oper_res = "constructor function was applied to actual stack more than once";
            break;

        case RESIZED:
            oper_res = "actual stack's buffer was resized up or down during PUSH or POP operation";
            break;

        case POPPED_EMPTY:
            oper_res = "POP operation was applied to the actual stack while its buffer is already empty (size = 0)";
            break;

        case UNDEFINED:
            oper_res = "operation code is undefined: the operation was not done yet";
            break;

        case MINCAP_REACHED:
            oper_res = "minimal capacity value is reached: resizing down will no longer be performed";
            break;

        case MEMERROR:
            oper_res = "the attempt to reallocate memory for actual stack's data failed";
            break;

        default:
            oper_res = "ERROR: operation code is not valid";
            break;
    }

    const char* verif_res = NULL;
    switch (verif_code) {

        case STK_OK:
            verif_res = "no flaws detected";
            break;

        case NOT_CONSTRUCTED_YET:
            verif_res = "nothing to verify: the actual stack was not constructed yet";
            break;

        case NO_STK:
            verif_res = "stack pointer is null";
            break;

        case OVERFLOW:
            verif_res = "SIZE parameter of actual stack exceeds its CAPACITY parameter";
            break;

        case TYPESIZE_ERR:
            verif_res = "TYPESIZE parameter of actual stack has invalid value";
            break;

        case MEM_ERR:
            verif_res = "actual stack's DATA pointer is null";
            break;

        case CAP_DRAIN:
            verif_res = "actual stack's CAPACITY value ran below MINCAPACITY value";
            break;

        case MEMDMG_DATAHASH:
            verif_res = "external memory damage was detected (hash sum changed): actual stack's data is no longer valid";
            break;

        case MEMDMG_PARAMHASH:
            verif_res = "external memory damage was detected (hash sum changed): actual stack's parameters are no longer valid";
            break;

        case LEFTDEAD_DATACAN:
            verif_res = "left data canary value has changed: actual stack's data is no longer valid";
            break;

        case RIGHTDEAD_DATACAN:
            verif_res = "right data canary value has changed: actual stack's data is no longer valid";
            break;

        case LEFTDEAD_PARAMCAN:
            verif_res = "left structure canary value has changed: actual stack's parameters are no longer valid";
            break;

        case RIGHTDEAD_PARAMCAN:
            verif_res = "right structure canary value has changed: actual stack's parameters are no longer valid";
            break;

        default:
            verif_res = "ERROR: verification code is not valid";
            break;
    }

    fprintf (logfile, "\n\nWARNING: if you see this message, described further part of your progaram works some kind incorrectly\n\n");
    fprintf (logfile, "internal dump called from file %s; function %s; line %d\noperation code: %d (%s)\nverification code: %d (%s)",       \
             filename, funcname, linenum, oper_code, oper_res, verif_code, verif_res);

    if (stack) {

        fprintf (logfile, "\n\nstack address [%p]", stack);
        fprintf (logfile, "\n\nsize %llu\ncapacity %llu\nmincapacity %llu\ntypesize %d\n\ndata [%p]\n", stack -> size, stack -> capacity,   \
                 stack -> mincapacity, stack -> typesize, stack -> data);

        size_t sizesize = stack -> size * stack -> typesize;
        size_t i = 0;
        for ( ; i < sizesize; i += stack -> typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack -> typesize);
                
            for (unsigned int k = 0; k < stack -> typesize; k++) {

                fprintf (logfile, "|%.3d| ", *(stack -> data + i + k));
            }  
        }
        fprintf (logfile, " ____ SIZE EDGE");

        size_t capsize = stack -> capacity * stack -> typesize;
        for ( ; i < capsize; i += stack -> typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack -> typesize);

            for (unsigned int k = 0; k < stack -> typesize; k++) {

                fprintf (logfile, "|%.3d| ", *(stack -> data + i + k));
            }
        }

    } else {

        fprintf (logfile, "\n-------------------------------------\n\nWARNING: dump function found no stack address");
    }

    fprintf (logfile, "\n\n__________________________________\n");

    fclose (logfile);
}

void val_dump_ (void* value, const int linenum, const char* funcname, const char* filename) {

    FILE* logfile = fopen ("LOG.txt", "a");

    fprintf (logfile, "\n\nWARNING: if you see this message, described further part of your progaram works some kind incorrectly\n\n");
    fprintf (logfile, "internal dump called from file %s; function %s; line %d", filename, funcname, linenum);

    if (value) {

        fprintf (logfile, "\n\nvalue address [%p]", value);

    } else {

        fprintf (logfile, "\n-------------------------------------\n\nWARNING: dump function found no value address");
    }

    fprintf (logfile, "\n\n__________________________________\n");

    fclose (logfile);
}

unsigned int hash (const unsigned char* data, size_t size) {

    const unsigned char* data_end = data + size;
    unsigned int hash = 0;

    for ( ; data < data_end; data++) {

        hash += (unsigned char) (*data);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

void stack_hash (stack_t* stack) {

    stack -> data_hash = hash (stack -> data, stack -> capacity * stack -> typesize + 2 * sizeof (unsigned long long));
    stack -> param_hash = 0;
    stack -> param_hash = hash ((unsigned char*) stack, sizeof (stack_t));
}
