#include <app/Application.hpp>

#include <exception>
#include <iostream>
#include <string_view>

int main(const int argc, char* argv[])
{
    const bool smokeTest = argc == 2 && std::string_view(argv[1]) == "--smoke-test";
    if (argc > 1 && !smokeTest) {
        std::cerr << "Unknown command-line argument. Supported: --smoke-test\n";
        return 2;
    }

    try {
        Application app;
        if (smokeTest) {
            return 0;
        }
        return app.run();
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: unknown exception\n";
        return 1;
    }
}
