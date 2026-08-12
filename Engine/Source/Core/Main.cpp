#include <Lithen.hpp>

#include <iostream>

int main(int argc, char** argv)
{
	try
	{
		Lithen::Engine engine;
		engine.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
