#include "B.h"
static int b = 42;

extern "C" int getB() {
    return b;
}
