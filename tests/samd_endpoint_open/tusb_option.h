#pragma once
#include <stdint.h>
#include <stddef.h>
#define CFG_TUD_ENABLED 1
#define OPT_MCU_SAMD21 21
#define OPT_MCU_SAMD51 51
#define OPT_MCU_SAME5X 54
#define TU_CHECK_MCU(first, ...) ((first) != OPT_MCU_SAMD51 && (first) != OPT_MCU_SAME5X)
#define TU_ATTR_ALIGNED(n) __attribute__((aligned(n)))
