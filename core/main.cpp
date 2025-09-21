#include "Application.h"
#include <filesystem>
#include <iostream>

int main() {
    Application app;
    // std::cout << std::filesystem::current_path() << std::endl;
    app.Run();
    return 0;
}
