#include <app/Application.hpp>

#include <exception>
#include <iostream>

int main()
{
	try {
		Application app;
		return app.run();
	} catch (const std::exception& exception) {
		std::cerr << "Fatal error: " << exception.what() << '\n';
		return 1;
	} catch (...) {
		std::cerr << "Fatal error: unknown exception\n";
		return 1;
	}
}
