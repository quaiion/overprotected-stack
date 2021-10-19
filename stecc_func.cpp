#include "stecc.hpp"

#define dev_stack_dump(stack_ptr, oper_code, verif_code)                                                                        \
    do {                                                                                                                        \
        dev_stack_dump_ (stack_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__, oper_code, verif_code);                            \
    } while (0)

static void dev_stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,               \
                             OPER_CODE oper_code, unsigned short verif_code);
                  
static unsigned char* stack_resize_up (stack_t* stack);
static unsigned char* stack_resize_down (stack_t* stack);

static unsigned int hash (const unsigned char* data, size_t size);
static void stack_hash (stack_t* stack);

OPER_CODE stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity /* = 0 */) {

    OPER_CODE oper_code = UNDEFINED;
    unsigned short verif_code = NOT_CONSTRUCTED_YET;

    if ((stack_verify (stack) & NO_STK) == NO_STK) {

        verif_code |= NO_STK;
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
    stack -> left_canary = CANARY;
    stack -> right_canary = CANARY;

    size_t datasize = mincapacity * typesize + 2 * sizeof (CANARY);
    stack -> data = (unsigned char*) malloc (datasize);

    verif_code = stack_verify (stack);
    if  (verif_code != STK_OK &&                                                                                          \
        (verif_code & MEMDMG_DATAHASH) != MEMDMG_DATAHASH &&                                                              \
        (verif_code & MEMDMG_PARAMHASH) != MEMDMG_PARAMHASH &&                                                            \
        (verif_code & LEFTDEAD_DATACAN) != LEFTDEAD_DATACAN &&                                                            \
        (verif_code & RIGHTDEAD_DATACAN) != RIGHTDEAD_DATACAN) {

        if ((verif_code & MEM_ERR) == MEM_ERR) {

            oper_code = MEMERROR;
        }

        dev_stack_dump (stack, oper_code, verif_code);
    }

    memset (stack -> data, 0, datasize);
    *((unsigned long long*) stack -> data) = CANARY;
    *((unsigned long long*) (stack -> data + datasize - sizeof (CANARY))) = CANARY;
    stack -> data += sizeof (CANARY);

    stack_hash (stack);

    return oper_code;
}

OPER_CODE stack_dtor (stack_t* stack) {

    OPER_CODE oper_code = UNDEFINED;
    unsigned short verif_code = stack_verify (stack);

    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    oper_code = SUCCESS;

    memset (stack -> data, BROKEN_BYTE, stack -> typesize * stack -> capacity);
    free (stack -> data - sizeof (CANARY));

    stack -> data = (unsigned char*) OS_RESERVED_ADDRESS;
    stack -> size = SIZE_MAX;
    stack -> capacity = SIZE_MAX;
    stack -> mincapacity = SIZE_MAX;

    return oper_code;
}

OPER_CODE stack_push (stack_t* stack, void* value) {

    OPER_CODE oper_code = UNDEFINED;
    unsigned short verif_code = stack_verify (stack);
    
    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (!value) {

        val_dump (value);
    }

    oper_code = SUCCESS;

    if (stack -> size == stack -> capacity) {

        unsigned char* real_buffer = stack_resize_up (stack);

        if (real_buffer) {

            oper_code = RESIZED;

            stack -> data = real_buffer + sizeof (CANARY);
            memset (stack -> data + stack -> size * stack -> typesize, 0, (stack -> capacity - stack -> size) * stack -> typesize);
            *((unsigned long long*) (stack -> data + stack -> capacity * stack -> typesize)) = CANARY;

        } else {

            oper_code = MEMERROR;
        }
    }

    verif_code = stack_verify (stack);
    if  (verif_code != STK_OK &&                                           \
        (verif_code & MEMDMG_DATAHASH) != MEMDMG_DATAHASH &&               \
        (verif_code & MEMDMG_PARAMHASH) != MEMDMG_PARAMHASH) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    memcpy (stack -> data + stack -> size ++ * stack -> typesize, (unsigned char*) value, stack -> typesize);

    stack_hash (stack);

    verif_code = stack_verify (stack);
    if  (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    return oper_code;
}

OPER_CODE stack_pop (stack_t* stack, void* value) {

    OPER_CODE oper_code = UNDEFINED;
    unsigned short verif_code = stack_verify (stack);
    
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

    if (stack -> size == (stack -> capacity - 3) / 4 && stack -> size >= stack -> mincapacity) {

        unsigned char* real_buffer = stack_resize_down (stack);

        if (real_buffer) {

            oper_code = RESIZED;

            stack -> data = real_buffer + sizeof (CANARY);
            *((unsigned long long*) (stack -> data + stack -> capacity * stack -> typesize)) = CANARY;

        } else {

            oper_code = MEMERROR;
        }
    }

    stack_hash (stack);

    verif_code = stack_verify (stack);
    if (verif_code != STK_OK || (oper_code & POPPED_EMPTY) == POPPED_EMPTY) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    return oper_code;
}

unsigned char* stack_resize_up (stack_t* stack) {

    unsigned char* real_buffer = (unsigned char*) realloc (stack -> data - sizeof (CANARY),                                      \
    (stack -> capacity = stack -> capacity * 2 + 1) * stack -> typesize + 2 * sizeof (CANARY));

    return real_buffer;
}

unsigned char* stack_resize_down (stack_t* stack) {

    unsigned char* real_buffer = (unsigned char*) realloc (stack -> data - sizeof (CANARY),                                      \
    (stack -> capacity = (stack -> capacity - 1) / 2) * stack -> typesize + 2 * sizeof (CANARY));

    return real_buffer;
}

unsigned short stack_verify (stack_t* stack) {

    unsigned short verif_code = STK_OK;

    if (! stack) {
        
        verif_code |= NO_STK;
        return verif_code;
    }

    if (stack -> size > stack -> capacity) {

        verif_code |= OVERFLOW;
    }

    if (stack -> typesize == 0) {

        verif_code |= TYPESIZE_ERR;
    }

    if (stack -> capacity < stack -> mincapacity) {

        verif_code |= CAP_DRAIN;
    }

    if (stack -> data == NULL) {

        verif_code |= MEM_ERR;
        return verif_code;
    }

    if (*((unsigned long long*) (stack -> data - sizeof (CANARY))) != CANARY) {

        verif_code |= LEFTDEAD_DATACAN;
    }

    if (*((unsigned long long*) (stack -> data + stack -> capacity * stack -> typesize)) != CANARY) {

        verif_code |= RIGHTDEAD_DATACAN;
    }

    if (stack -> left_canary != CANARY) {

        verif_code |= LEFTDEAD_PARAMCAN;
    }

    if (stack -> right_canary != CANARY) {

        verif_code |= RIGHTDEAD_PARAMCAN;
    }

    if (hash (stack -> data, stack -> capacity * stack -> typesize + 2 * sizeof (CANARY)) != stack -> data_hash) {

        verif_code |= MEMDMG_DATAHASH;
    }

    unsigned int temp_hash = stack -> param_hash;
    stack -> param_hash = 0;
    stack -> param_hash = hash ((unsigned char*) stack, sizeof (stack_t));

    if (stack -> param_hash != temp_hash) {

        verif_code |= MEMDMG_PARAMHASH;
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

void dev_stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,                      \
                      OPER_CODE oper_code, unsigned short verif_code) {

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

        case MEMERROR:
            oper_res = "the attempt to reallocate memory for actual stack's data failed";
            break;

        default:
            oper_res = "ERROR: operation code is not valid";
            break;
    }

    fprintf (logfile, "\n\nWARNING: if you see this message, described further part of your progaram works some kind incorrectly\n\n");
    fprintf (logfile, "internal dump called from file %s; function %s; line %d\noperation code: %d (%s)",                               \
             filename, funcname, linenum, oper_code, oper_res);
    fprintf (logfile, "\n\nverification codes:");
    
    if ((verif_code & (~NOT_CONSTRUCTED_YET)) == STK_OK) {

        if (verif_code == NOT_CONSTRUCTED_YET) {

            fputs ("\nstate 1: nothing to verify; the actual stack was not constructed yet", logfile);

        } else {

            fputs ("\nstate 0: no flaws detected", logfile);
        }
    } else {

        if ((verif_code & NO_STK) == NO_STK) {

            fputs ("\nerror -1: stack pointer is null", logfile);
        }

        if ((verif_code & MEM_ERR) == MEM_ERR) {

            fputs("\nerror -2: actual stack's DATA pointer is null", logfile);
        }

        if ((verif_code & OVERFLOW) == OVERFLOW) {

            fputs ("\nerror -3: SIZE parameter of actual stack exceeds its CAPACITY parameter", logfile);
        }

        if ((verif_code & TYPESIZE_ERR) == TYPESIZE_ERR) {

            fputs ("\nerror -4: TYPESIZE parameter of actual stack has invalid value", logfile);
        }

        if ((verif_code & CAP_DRAIN) == CAP_DRAIN) {

            fputs ("\nerror -5: actual stack's CAPACITY value ran below MINCAPACITY value", logfile);
        }

        if ((verif_code & MEMDMG_DATAHASH) == MEMDMG_DATAHASH) {

            fputs ("\nerror -6: external memory damage was detected (hash sum changed): actual stack's data is no longer valid", logfile);
        }

        if ((verif_code & MEMDMG_PARAMHASH) == MEMDMG_PARAMHASH) {

            fputs ("\nerror -7: external memory damage was detected (hash sum changed): actual stack's parameters are no longer valid", logfile);
        }

        if ((verif_code & LEFTDEAD_DATACAN) == LEFTDEAD_DATACAN) {

            fputs ("\nerror -8: left data canary value has changed: actual stack's data is no longer valid", logfile);
        }

        if ((verif_code & RIGHTDEAD_DATACAN) == RIGHTDEAD_DATACAN) {

            fputs ("\nerror -9: right data canary value has changed: actual stack's data is no longer valid", logfile);
        }

        if ((verif_code & LEFTDEAD_PARAMCAN) == LEFTDEAD_PARAMCAN) {

            fputs ("\nerror -10: left structure canary value has changed: actual stack's parameters are no longer valid", logfile);
        }

        if ((verif_code & RIGHTDEAD_PARAMCAN) == RIGHTDEAD_PARAMCAN) {

            fputs ("\nerror -11: right structure canary value has changed: actual stack's parameters are no longer valid", logfile);
        }
    }

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

    stack -> data_hash = hash (stack -> data, stack -> capacity * stack -> typesize + 2 * sizeof (CANARY));
    stack -> param_hash = 0;
    stack -> param_hash = hash ((unsigned char*) stack, sizeof (stack_t));
}
