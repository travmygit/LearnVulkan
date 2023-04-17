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

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	}

	void initVulkan()
	{
		createInstance();
		setupDebugCallback();
		pickPhysicalDevice();
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
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
		}

		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("Require validation layers, but not available!");
		}

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

		auto extensions = getRequiredExtensions();
		
		if (!checkExtensionSupport(extensions))
		{
			throw std::runtime_error("Require extensions, but not available!");
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance!");
		}
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = nullptr;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkExtensionSupport(const std::vector<const char*>& glfwExtensions)
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
		for (auto glfwExtension : glfwExtensions)
		{
			std::cout << "\t" << glfwExtension << std::endl;
		}
#endif

		for (auto glfwExtension : glfwExtensions)
		{
			bool found = false;
			for (const auto& extension : extensions)
			{
				if (strcmp(glfwExtension, extension.extensionName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				return false;
			}
		}

		return glfwExtensions.size() > 0;
	}

	bool checkValidationLayerSupport()
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

#ifndef NDEBUG
		std::cout << "Available layers: " << std::endl;
		for (const auto& layer : availableLayers)
		{
			std::cout << "\t" << layer.layerName << std::endl;
		}
#endif

		for (auto layerName : validationLayers)
		{
			bool found = false;
			for (const auto& layer : availableLayers)
			{
				if (strcmp(layerName, layer.layerName) == 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				return false;
			}
		}

		return validationLayers.size() > 0;
	}

	void setupDebugCallback()
	{
		if (!enableValidationLayers)
		{
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;

		if (CreateDebugUtilsMessengerEXT(instance,
			&createInfo, nullptr, &callback) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to set up debug callback!");
		}
	}

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance                                  instance,
		const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
		const VkAllocationCallbacks*                pAllocator,
		VkDebugUtilsMessengerEXT*                   pMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pMessenger);
		}
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(
		VkInstance                                  instance,
		VkDebugUtilsMessengerEXT                    messenger,
		const VkAllocationCallbacks*                pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			func(instance, messenger, pAllocator);
		}
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

#ifndef NDEBUG
		std::cout << "Available devices: " << std::endl;
		for (const auto& device : devices)
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			std::cout << "\t" << deviceProperties.deviceName << std::endl;
		}
#endif

		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

#ifndef NDEBUG
		std::cout << "Selected device: " << std::endl;
		{
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			std::cout << "\t" << deviceProperties.deviceName << std::endl;
		}
#endif
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = findQueueFamilies(device);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& deviceFeatures.geometryShader
			&& indices.isComplete();
	}

	struct QueueFamilyIndices
	{
		int32_t graphicsFamily = -1;
		bool isComplete() { return graphicsFamily >= 0; }
	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamlityCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamlityCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamlityCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamlityCount, queueFamilies.data());

		int32_t i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}

			++i;
		}

		return indices;
	}

	GLFWwindow* window = nullptr;

	VkInstance instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT callback = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	const int WIDTH  = 800;
	const int HEIGHT = 600;

	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void*                                       pUserData)
	{
		std::cerr << "Validate layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}
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