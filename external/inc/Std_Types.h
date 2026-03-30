#ifndef STD_TYPES_HEADER_GUARD
#define STD_TYPES_HEADER_GUARD

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define NULL_PTR    (0u)

#define E_OK        (0u)
#define E_NOT_OK    (1u)

#ifndef UNIT_TESTS
    #define STATIC static
#else
    #define STATIC
#endif /* UNIT_TESTS */

typedef uint8_t Std_ReturnType_t;

#endif /* STD_TYPES_HEADER_GUARD */