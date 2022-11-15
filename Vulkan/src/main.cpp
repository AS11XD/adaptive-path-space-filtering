#include <iostream>
#include <cstdlib>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include "initialization.h"
#include "utils.h"
#include "renderdoc.h"
#include "window.h"

int main()
{
	try
	{
		renderdoc::initialize();
		Window window("Master_Thesis_Project", 800, 600);
		window.run();
		window.cleanup();
	}
	catch (vk::SystemError& err)
	{
		std::cout << "vk::SystemError: " << err.what() << std::endl;
		exit(-1);
	}
	catch (std::exception& err)
	{
		std::cout << "std::exception: " << err.what() << std::endl;
		exit(-1);
	}
	catch (...)
	{
		std::cout << "unknown error\n";
		exit(-1);
	}
	return EXIT_SUCCESS;
}