#include "stecc.hpp"
#include <stdio.h>

struct test_t {

    int int_test;
    double double_test;
};

static void unittest_normal ();
static void unittest_hash_1 ();
static void unittest_hash_2 ();
static void unittest_canary_1 ();
static void unittest_canary_2 ();
static void unittest_fail ();
static void unittest_stack_vanished ();

static void test_fprint (FILE* logfile, const void* test_struct);

int main () {

    unittest_fail ();

    return 0;
}

void test_fprint (FILE* logfile, const void* test_struct) {

    fprintf (logfile, "%6.d%10.3lf", ((test_t*) test_struct)->int_test, ((test_t*)test_struct)->double_test);
}

void unittest_normal () {

    stack_t stack = {};
    stack_t* stack_p = &stack;

    size_t mincapacity = 1;

    stack_ctor (stack_p, sizeof (test_t), mincapacity);

    stack_dump (stack_p, test_fprint);

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_dump (stack_p, test_fprint);

    stack_push (stack_p, test_p);
    stack_dump (stack_p, test_fprint);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);
    stack_dump (stack_p, test_fprint);

    stack_push (stack_p, test_p);
    stack_dump (stack_p, test_fprint);

    test_t result = {};
    test_t* result_p = &result;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);
    stack_dump (stack_p, test_fprint);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);
    stack_dump (stack_p, test_fprint);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);
    stack_dump (stack_p, test_fprint);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);
    stack_dump (stack_p, test_fprint);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);
    stack_dump (stack_p, test_fprint);

    stack_dtor (stack_p);
}

void unittest_hash_1 () {

    stack_t stack = {};
    stack_t* stack_p = &stack;

    size_t mincapacity = 1;

    stack_ctor (stack_p, sizeof (test_t));          //  Небольшая демонстрация параметра по умолчанию

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test_t result = {};
    test_t* result_p = &result;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_p->mincapacity = 12;                      //  Несанкционированно меняем небольшой параметр стека

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_dtor (stack_p);
}

void unittest_hash_2 () {

    stack_t stack = {};
    stack_t* stack_p = &stack;

    size_t mincapacity = 12;

    stack_ctor (stack_p, sizeof (test_t), mincapacity);

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);

    stack_p->data [2] = 80;                       //  Несанкционированно меняем буфер с данными

    stack_push (stack_p, test_p);

    test_t result = {};
    test_t* result_p = &result;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_dtor (stack_p);
}

void unittest_canary_1 () {

    struct stack_crack_t {

        char left_bulldozer;
        stack_t stack;
        char right_bulldozer;
    };

    stack_crack_t crack = {};

    stack_t* stack_p = &(crack.stack);

    size_t mincapacity = 1;

    stack_ctor (stack_p, sizeof (test_t), mincapacity);

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test_t result = {};
    test_t* result_p = &result;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    for (int i = 0; i < 12; i++) {                   //  Сметаем левую канарейку параметров

        *(&(crack.left_bulldozer) + i) = 4;
    }

    stack_dtor (stack_p);
}

void unittest_canary_2 () {

    stack_t stack = {};
    stack_t* stack_p = &stack;

    size_t mincapacity = 1;

    stack_ctor (stack_p, sizeof (test_t), mincapacity);

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test_t result = {};
    test_t* result_p = &result;

    for (int i = 0; i < 12; i++) {                   //  Сметаем правую канарейку буфера с данными (имитируем, начиная с точного удара по канарейке)

        *(stack_p->data + stack_p->capacity * stack_p->typesize + (sizeof (unsigned long long) - 1) - i) = 4;
    }

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_dtor (stack_p);
}

void unittest_fail () {

    stack_t stack = {};
    stack_t* stack_p = &stack;

    size_t mincapacity = 1;

    stack_ctor (stack_p, sizeof (test_t), mincapacity);

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test_t result = {};
    test_t* result_p = &result;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_p->capacity = 2;                          //  Портим парамтетры стека так, чтобы заставить дамп вывести побольше ошибок
    stack_p->mincapacity = 800;
    stack_p->typesize = 0;
    stack_p->data = NULL;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_dtor (stack_p);
}

void unittest_stack_vanished () {

    stack_t stack = {};
    stack_t* stack_p = &stack;

    size_t mincapacity = 1;

    stack_ctor (stack_p, sizeof (test_t), mincapacity);

    test_t test = {};
    test_t* test_p = &test;

    test.int_test = 3;
    test.double_test = -45.662;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test.int_test = -12;
    test.double_test = 862.14;

    stack_push (stack_p, test_p);
    stack_push (stack_p, test_p);

    test_t result = {};
    test_t* result_p = &result;

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_p = NULL;                                 //  Стек принял ислам

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_pop (stack_p, result_p);
    printf ("pop %d %.3lf\n", result.int_test, result.double_test);

    stack_dtor (stack_p);
}