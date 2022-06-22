#include "stecc.hpp"

#ifdef STACK_VERIFICATION_ON
#define dev_stack_dump(stack_ptr, oper_code, verif_code)                                                                        \
    do {                                                                                                                        \
        dev_stack_dump_ (stack_ptr, __LINE__, __PRETTY_FUNCTION__, __FILE__, oper_code, verif_code);                            \
    } while (0)

static void dev_stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,               \
                             OPER_CODE oper_code, unsigned short verif_code);
#endif
                  
static unsigned char* stack_resize_up (stack_t* stack);
static unsigned char* stack_resize_down (stack_t* stack);

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
static unsigned int hash (const unsigned char* data, size_t size);
static void stack_hash (stack_t* stack);
#endif

OPER_CODE stack_ctor (stack_t* stack, unsigned int typesize, size_t mincapacity /* = 0 */) {

    OPER_CODE oper_code = UNDEFINED;

#ifdef STACK_VERIFICATION_ON
    unsigned short verif_code = NOT_CONSTRUCTED_YET;

    if ((stack_verify (stack) & NO_STK) == NO_STK) {

        verif_code |= NO_STK;
        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (stack->data || stack->size || stack->capacity || stack->mincapacity || stack->typesize) {

        oper_code = RECONSTRUCT;
        dev_stack_dump (stack, oper_code, verif_code);

        return oper_code;
    }
#endif

    oper_code = SUCCESS;
    
    stack->typesize = typesize;
    stack->size = 0;
    stack->mincapacity = mincapacity;
    stack->capacity = mincapacity;

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    stack->left_canary = CANARY;
    stack->right_canary = CANARY;
#endif

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    size_t datasize = mincapacity * typesize + 2 * sizeof (CANARY);
#else
    size_t datasize = mincapacity * typesize;
#endif

    stack->data = (unsigned char*) malloc (datasize);

#ifdef STACK_VERIFICATION_ON
    verif_code = stack_verify (stack);
    verif_code &= ~MEMDMG_DATAHASH;
    verif_code &= ~MEMDMG_PARAMHASH;
    verif_code &= ~LEFTDEAD_DATACAN;
    verif_code &= ~RIGHTDEAD_DATACAN;
    if  (verif_code != STK_OK) {

        if ((verif_code & MEM_ERR) == MEM_ERR) {

            oper_code = MEMERROR;
        }

        dev_stack_dump (stack, oper_code, verif_code);
    }
#endif

    memset (stack->data, 0, datasize);

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    *((unsigned long long*) stack->data) = CANARY;
    *((unsigned long long*) (stack->data + datasize - sizeof (CANARY))) = CANARY;
    stack->data += sizeof (CANARY);
#endif

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
    stack_hash (stack);
#endif

    return oper_code;
}

OPER_CODE stack_dtor (stack_t* stack) {

    OPER_CODE oper_code = UNDEFINED;

#ifdef STACK_VERIFICATION_ON
    unsigned short verif_code = stack_verify (stack);

    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }
#endif

    oper_code = SUCCESS;

    memset (stack->data, BROKEN_BYTE, stack->typesize * stack->capacity);

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    free (stack->data - sizeof (CANARY));
#else
    free (stack->data);
#endif

    stack->data = (unsigned char*) OS_RESERVED_ADDRESS;
    stack->size = SIZE_MAX;
    stack->capacity = SIZE_MAX;
    stack->mincapacity = SIZE_MAX;

    return oper_code;
}

OPER_CODE stack_push (stack_t* stack, void* value) {

    OPER_CODE oper_code = UNDEFINED;

#ifdef STACK_VERIFICATION_ON
    unsigned short verif_code = stack_verify (stack);
    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (!value) {

        val_dump (value);
    }
#endif

    oper_code = SUCCESS;

    if (stack->size == stack->capacity) {

        unsigned char* real_buffer = stack_resize_up (stack);

        if (real_buffer) {

            oper_code = RESIZED;

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
            stack->data = real_buffer + sizeof (CANARY);
            memset (stack->data + stack->size * stack->typesize, 0, (stack->capacity - stack->size) * stack->typesize);
            *((unsigned long long*) (stack->data + stack->capacity * stack->typesize)) = CANARY;
#else
            stack->data = real_buffer;
            memset (stack->data + stack->size * stack->typesize, 0, (stack->capacity - stack->size) * stack->typesize);
#endif

        } else {

            oper_code = MEMERROR;
        }
    }

#ifdef STACK_VERIFICATION_ON
    verif_code = stack_verify (stack);
    verif_code &= ~MEMDMG_DATAHASH;
    verif_code &= ~MEMDMG_PARAMHASH;
    if  (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }
#endif

    memcpy (stack->data + stack->size ++ * stack->typesize, (unsigned char*) value, stack->typesize);

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
    stack_hash (stack);
#endif

#ifdef STACK_VERIFICATION_ON
    verif_code = stack_verify (stack);
    if  (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }
#endif

    return oper_code;
}

OPER_CODE stack_pop (stack_t* stack, void* value) {

    OPER_CODE oper_code = UNDEFINED;

#ifdef STACK_VERIFICATION_ON
    unsigned short verif_code = stack_verify (stack);
    if (verif_code != STK_OK) {

        dev_stack_dump (stack, oper_code, verif_code);
    }

    if (!value) {

        val_dump (value);
    }
#endif

    oper_code = SUCCESS;

    if (stack->size == 0) {

        oper_code = POPPED_EMPTY;
        return oper_code;
    }

    memcpy ((unsigned char*) value, stack->data + -- stack->size * stack->typesize, stack->typesize);

    if (stack->size == (stack->capacity - 3) / 4 && stack->size >= stack->mincapacity) {

        unsigned char* real_buffer = stack_resize_down (stack);

        if (real_buffer) {

            oper_code = RESIZED;

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
            stack->data = real_buffer + sizeof (CANARY);
            *((unsigned long long*) (stack->data + stack->capacity * stack->typesize)) = CANARY;
#else
            stack->data = real_buffer;
#endif

        } else {

            oper_code = MEMERROR;
        }
    }

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
    stack_hash (stack);
#endif

#ifdef STACK_VERIFICATION_ON
    verif_code = stack_verify (stack);
    if (verif_code != STK_OK || (oper_code & POPPED_EMPTY) == POPPED_EMPTY) {

        dev_stack_dump (stack, oper_code, verif_code);
    }
#endif

    return oper_code;
}

unsigned char* stack_resize_up (stack_t* stack) {

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    unsigned char* real_buffer = (unsigned char*) realloc (stack->data - sizeof (CANARY),                    \
    (stack->capacity * 2 + 1) * stack->typesize + 2 * sizeof (CANARY));
    if (real_buffer) {
        
        stack->capacity = stack->capacity * 2 + 1;
    }
#else
    unsigned char* real_buffer = (unsigned char*) realloc (stack->data,                                      \
    (stack->capacity * 2 + 1) * stack->typesize);
    if (real_buffer) {

        stack->capacity = stack->capacity * 2 + 1;
    }
#endif

    return real_buffer;
}

unsigned char* stack_resize_down (stack_t* stack) {

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    unsigned char* real_buffer = (unsigned char*) realloc (stack->data - sizeof (CANARY),                    \
    (stack->capacity - 1) / 2 * stack->typesize + 2 * sizeof (CANARY));
    if (real_buffer) {
        
        stack->capacity = (stack->capacity - 1) / 2;
    }
#else
    unsigned char* real_buffer = (unsigned char*) realloc (stack->data,                                      \
    (stack->capacity - 1) / 2 * stack->typesize);
    if (real_buffer) {
        
        stack->capacity = (stack->capacity - 1) / 2;
    }
#endif

    return real_buffer;
}

#ifdef STACK_VERIFICATION_ON
unsigned short stack_verify (stack_t* stack) {

    unsigned short verif_code = STK_OK;

    if (! stack) {
        
        verif_code |= NO_STK;
        return verif_code;
    }

    if (stack->size > stack->capacity) {

        verif_code |= OVERFLOW;
    }

    if (stack->typesize == 0) {

        verif_code |= TYPESIZE_ERR;
    }

    if (stack->capacity < stack->mincapacity) {

        verif_code |= CAP_DRAIN;
    }

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    if (stack->left_canary != CANARY) {

        verif_code |= LEFTDEAD_PARAMCAN;
    }

    if (stack->right_canary != CANARY) {

        verif_code |= RIGHTDEAD_PARAMCAN;
    }
#endif

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
    unsigned int temp_hash = stack->param_hash;
    stack->param_hash = 0;
    stack->param_hash = hash ((unsigned char*) stack, sizeof (stack_t));

    if (stack->param_hash != temp_hash) {

        verif_code |= MEMDMG_PARAMHASH;
    }
#endif

    if (stack->data == NULL) {

        verif_code |= MEM_ERR;
        return verif_code;
    }

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
    if (*((unsigned long long*) (stack->data - sizeof (CANARY))) != CANARY) {

        verif_code |= LEFTDEAD_DATACAN;
    }

    if (*((unsigned long long*) (stack->data + stack->capacity * stack->typesize)) != CANARY) {

        verif_code |= RIGHTDEAD_DATACAN;
    }
#endif

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
#ifdef STACK_CANARY_DEFENSE_ON
    if (hash (stack->data, stack->capacity * stack->typesize + 2 * sizeof (CANARY)) != stack->data_hash) {

        verif_code |= MEMDMG_DATAHASH;
    }
#else
    if (hash (stack->data, stack->capacity * stack->typesize) != stack->data_hash) {

        verif_code |= MEMDMG_DATAHASH;
    }
#endif
#endif

    return verif_code;
}
#endif

void stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,
                  void (*elem_fprint) (FILE*, const void*)) {

    FILE* logfile = fopen ("LOG.txt", "a");

    fprintf (logfile, "dump called from file %s; function %s; line %d", filename, funcname, linenum);

    if (stack) {

        fprintf (logfile, "\n\nstack address [%p]", stack);
        fprintf (logfile, "\n\nsize %llu\ncapacity %llu\nmincapacity %llu\ntypesize %d\n\ndata [%p]\n", stack->size, stack->capacity, \
                 stack->mincapacity, stack->typesize, stack->data);

        size_t sizesize = stack->size * stack->typesize;
        size_t i = 0;
        for ( ; i < sizesize; i += stack->typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack->typesize);
            elem_fprint (logfile, stack->data + i);
        }
        fprintf (logfile, " ____ SIZE EDGE");

        size_t capsize = stack->capacity * stack->typesize;
        for ( ; i < capsize; i += stack->typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack->typesize);
            elem_fprint (logfile, stack->data + i);
        }

    } else {

        fprintf (logfile, "\n-------------------------------------\n\nWARNING: dump function found no stack address");
    }

    fprintf (logfile, "\n\n__________________________________\n\n\n");

    fclose (logfile);
}

#ifdef STACK_VERIFICATION_ON
void dev_stack_dump_ (const stack_t* stack, const int linenum, const char* funcname, const char* filename,
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

    fprintf (logfile, "WARNING: if you see this message, described further part of your progaram works some kind incorrectly\n\n");
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

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
        if ((verif_code & MEMDMG_DATAHASH) == MEMDMG_DATAHASH) {

            fputs ("\nerror -6: external memory damage was detected (hash sum changed): actual stack's data is no longer valid", logfile);
        }

        if ((verif_code & MEMDMG_PARAMHASH) == MEMDMG_PARAMHASH) {

            fputs ("\nerror -7: external memory damage was detected (hash sum changed): actual stack's parameters are no longer valid", logfile);
        }
#endif

#if defined STACK_CANARY_DEFENSE_ON && defined STACK_VERIFICATION_ON
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
#endif
    }

    if (stack) {

        fprintf (logfile, "\n\nstack address [%p]", stack);
        fprintf (logfile, "\n\nsize %llu\ncapacity %llu\nmincapacity %llu\ntypesize %d\n\ndata [%p]\n", stack->size, stack->capacity,   \
                 stack->mincapacity, stack->typesize, stack->data);

        size_t sizesize = stack->size * stack->typesize;
        size_t i = 0;
        for ( ; i < sizesize; i += stack->typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack->typesize);
                
            for (unsigned int k = 0; k < stack->typesize; k++) {

                fprintf (logfile, "|%.3d| ", *(stack->data + i + k));
            }  
        }
        fprintf (logfile, " ____ SIZE EDGE");

        size_t capsize = stack->capacity * stack->typesize;
        for ( ; i < capsize; i += stack->typesize) {

            fprintf (logfile, "\n[%llu]   ", i / stack->typesize);

            for (unsigned int k = 0; k < stack->typesize; k++) {

                fprintf (logfile, "|%.3d| ", *(stack->data + i + k));
            }
        }

    } else {

        fprintf (logfile, "\n-------------------------------------\n\nWARNING: dump function found no stack address");
    }

    fprintf (logfile, "\n\n__________________________________\n\n\n");

    fclose (logfile);
}
#endif

void val_dump_ (void* value, const int linenum, const char* funcname, const char* filename) {

    FILE* logfile = fopen ("LOG.txt", "a");

    fprintf (logfile, "WARNING: if you see this message, described further part of your progaram works some kind incorrectly\n\n");
    fprintf (logfile, "internal dump called from file %s; function %s; line %d", filename, funcname, linenum);

    if (value) {

        fprintf (logfile, "\n\nvalue address [%p]", value);

    } else {

        fprintf (logfile, "\n-------------------------------------\n\nWARNING: dump function found no value address");
    }

    fprintf (logfile, "\n\n__________________________________\n\n\n");

    fclose (logfile);
}

#if defined STACK_HASH_DEFENSE_ON && defined STACK_VERIFICATION_ON
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

#ifdef STACK_CANARY_DEFENSE_ON
    stack->data_hash = hash (stack->data, stack->capacity * stack->typesize + 2 * sizeof (CANARY));
#else
    stack->data_hash = hash (stack->data, stack->capacity * stack->typesize);
#endif
    stack->param_hash = 0;
    stack->param_hash = hash ((unsigned char*) stack, sizeof (stack_t));
}
#endif
