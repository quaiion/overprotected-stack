#ifndef STACK_TYPE
#define STACK_TYPE

struct stack_t {

    unsigned long long left_canary;
    unsigned char* data;
    unsigned int typesize;
    size_t capacity;
    size_t mincapacity;
    size_t size;
    unsigned int data_hash;
    unsigned int param_hash;
    unsigned long long right_canary;
};

#endif
