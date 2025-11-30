#include "A.h"

static int a = 5;

extern "C" int getA() {
    return a;
}
