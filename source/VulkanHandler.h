#ifndef VULKANHANDLER_H
#define VULKANHANDLER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>
#include <vector>

#include "Cfg.h"
#include "Logger.h"

using std::vector;


const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_LUNARG_standard_validation"
};


class VulkanHandler {

	public:
		const bool checkValidationLayerSupport();
		void setup();

	protected:
		VkInstance createInstance();

	private:
		bool mUseValidationLayer;
		VkInstance mInstance;

};

#endif
