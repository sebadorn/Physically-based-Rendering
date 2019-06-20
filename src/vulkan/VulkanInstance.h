#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <set>
#include <vector>

#include "../Cfg.h"
#include "../Logger.h"
#include "../VulkanHandler.h"

using std::vector;


const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};


class VulkanInstance {


	public:
		static const bool checkValidationLayerSupport();
		static VkInstance createInstance();
		static vector<const char*> getRequiredExtensions();
		static uint32_t getVersionPBR();
		static void setupDebugCallback( VkInstance* instance, VkDebugReportCallbackEXT* callback );


	protected:
		static VkApplicationInfo buildApplicationInfo();
		static VkInstanceCreateInfo buildInstanceCreateInfo(
			VkApplicationInfo* appInfo,
			vector<const char*>* extensions
		);


};

#endif
