#include <iostream>
#include <windows.h>

typedef int (__cdecl *GetBFunc)();

int main() {
    HMODULE h = LoadLibraryA("DynamicLib.dll");
    if (!h) {
        std::cerr << "Cannot load DLL" << std::endl;
        return 1;
    }
    auto func = reinterpret_cast<GetBFunc>(GetProcAddress(h, "getB"));
    if (!func) {
        std::cerr << "Cannot find symbol" << std::endl;
        return 1;
    }
    std::cout << "B (runtime) = " << func() << std::endl;
    FreeLibrary(h);
    return 0;
}
