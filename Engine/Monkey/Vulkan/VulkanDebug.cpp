#include "VulkanRHI.h"
#include "VulkanPlatform.h"
#include <iostream>

#if MONKEY_DEBUG

#define VK_DESTORY_DEBUG_REPORT_CALLBACK_EXT_NAME "vkDestroyDebugReportCallbackEXT"
#define VK_CREATE_DEBUG_REPORT_CALLBACK_EXT_NAME  "vkCreateDebugReportCallbackEXT"
#define VK_CREATE_DEBUG_UTILS_MESSNGER_EXT_NAME   "vkCreateDebugUtilsMessengerEXT"

//全局函数
VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, VK_CREATE_DEBUG_UTILS_MESSNGER_EXT_NAME);
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallBack(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
{
    std::string prefix("");
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        prefix += "ERROR:";
    }

    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        prefix += "WARNING:";
    }

    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        prefix += "PERFORMANCE:";
    }

    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        prefix += "INFO:";
    }
    
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        prefix += "DEBUG:";
    }

    MLOG("%s [%s] Code %d : %s", prefix.c_str(), layerPrefix, code, msg);
    return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{

 //   MLOG("%s [%s]", "validation layer", pCallbackData->pMessage);
	std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VulkanRHI::SetupDebugLayerCallback()
{
    VkDebugReportCallbackCreateInfoEXT debugInfo;
    ZeroVulkanStruct(debugInfo, VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT);
    debugInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugInfo.pfnCallback = VulkanDebugCallBack;
    debugInfo.pUserData   = this;
    
    auto func    = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, VK_CREATE_DEBUG_REPORT_CALLBACK_EXT_NAME);
    bool success = true;
    if (func != nullptr) {
        success = func(m_Instance, &debugInfo, nullptr, &m_MsgCallback) == VK_SUCCESS;
    }
    else {
        success = false;
    }
    
    if (success) {
        MLOG("Setup debug callback success.");
    }
    else {
        MLOG("Setup debug callback failed.")
    }
}

void VulkanRHI::RemoveDebugLayerCallback()
{
    if (m_MsgCallback != VK_NULL_HANDLE)
    {
        PFN_vkDestroyDebugReportCallbackEXT destroyMsgCallback = (PFN_vkDestroyDebugReportCallbackEXT)(void*)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT");
        destroyMsgCallback(m_Instance, m_MsgCallback, nullptr);
    }
}

void VulkanRHI::setupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo;

	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
	createInfo.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = this;

    VkResult resultcode;
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, VK_CREATE_DEBUG_UTILS_MESSNGER_EXT_NAME);
	if (func != nullptr) {
        resultcode = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
	}
	else {
        return;//VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

#endif