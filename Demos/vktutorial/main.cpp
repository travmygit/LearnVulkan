#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>

#if defined(_MSC_VER) && !defined(NDEBUG) && defined(_WIN32)
#include <ostream>
#include <sstream>
#include <Windows.h>

/// \brief This class is derives from basic_stringbuf which will output
/// all the written data using the OutputDebugString function
template<typename TChar, typename TTraits = std::char_traits<TChar>>
class OutputDebugStringBuf : public std::basic_stringbuf<TChar, TTraits>
{
public:
	OutputDebugStringBuf() : _buffer(256)
	{
		setg(nullptr, nullptr, nullptr);
		setp(_buffer.data(), _buffer.data(), _buffer.data() + _buffer.size());
	}

	~OutputDebugStringBuf()
	{
	}

	static_assert(std::is_same<TChar, char>::value ||
		std::is_same<TChar, wchar_t>::value,
		"OutputDebugStringBuf only supports char and wchar_t types");

protected:
	int sync()
	{
		try
		{
			MessageOutputer<TChar, TTraits>()(pbase(), pptr());
			setp(_buffer.data(), _buffer.data(), _buffer.data() + _buffer.size());
			return 0;
		}
		catch (...)
		{
			return -1;
		}
	}

	virtual int_type overflow(int_type c = TTraits::eof()) override
	{
		int syncRet = sync();
		if (c != TTraits::eof())
		{
			_buffer[0] = (TChar)c;
			setp(_buffer.data(), _buffer.data() + 1, _buffer.data() + _buffer.size());
		}
		return syncRet == -1 ? TTraits::eof() : 0;
	}

private:
	std::vector<TChar> _buffer;

	template<typename TChar, typename TTraits>
	struct MessageOutputer;

	template<>
	struct MessageOutputer<char, std::char_traits<char>>
	{
		template<typename TIterator>
		void operator()(TIterator begin, TIterator end) const
		{
			std::string s(begin, end);
			OutputDebugStringA(s.c_str());
		}
	};

	template<>
	struct MessageOutputer<wchar_t, std::char_traits<wchar_t>>
	{
		template<typename TIterator>
		void operator()(TIterator begin, TIterator end) const
		{
			std::wstring s(begin, end);
			OutputDebugStringW(s.c_str());
		}
	};
};
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, nullptr, nullptr);

	}

	void initVulkan()
	{
		createInstance();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = nullptr;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		checkExtensions(glfwExtensionCount, glfwExtensions);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	bool checkExtensions(uint32_t glfwExtensionCount, const char** glfwExtensions)
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

#ifndef NDEBUG
		std::cout << "Available extensions: " << std::endl;
		for (const auto& extension : extensions)
		{
			std::cout << "\t" << extension.extensionName << std::endl;
		}

		std::cout << "GLFW extensions: " << std::endl;
		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			std::cout << "\t" << glfwExtensions[i] << std::endl;
		}
#endif

		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			for (const auto& extension : extensions)
			{
				if (strcmp(glfwExtensions[i], extension.extensionName) != 0)
				{
					return false;
				}
			}
		}

		return glfwExtensionCount > 0;
	}

	GLFWwindow* window = nullptr;

	VkInstance instance = nullptr;

	const int WIDTH  = 800;
	const int HEIGHT = 600;
	const char* TITLE = "Vulkan";
};

int main()
{
#if defined(_MSC_VER) && !defined(NDEBUG) && defined(_WIN32)
	static OutputDebugStringBuf<char> charDebugOutput;
	std::cout.rdbuf(&charDebugOutput);
	std::cerr.rdbuf(&charDebugOutput);
	std::clog.rdbuf(&charDebugOutput);

	static OutputDebugStringBuf<wchar_t> wcharDebugOutput;
	std::wcout.rdbuf(&wcharDebugOutput);
	std::wcerr.rdbuf(&wcharDebugOutput);
	std::wclog.rdbuf(&wcharDebugOutput);
#endif

	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}