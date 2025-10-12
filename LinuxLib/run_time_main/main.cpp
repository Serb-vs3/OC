#include <iostream>
#include <dlfcn.h>

typedef int (*GetBFunc)();

int main() {
    void* handle = dlopen("libdynamic_lib.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot load shared library: " << dlerror() << std::endl;
        return 1;
    }
    dlerror(); // clear
    auto func = reinterpret_cast<GetBFunc>(dlsym(handle, "getB"));
    const char* err = dlerror();
    if (err) {
        std::cerr << "dlsym error: " << err << std::endl;
        dlclose(handle);
        return 1;
    }
    std::cout << "B (runtime) = " << func() << std::endl;
    dlclose(handle);
    return 0;
}
