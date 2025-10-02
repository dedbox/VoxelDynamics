#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

Context::Context(const Window& window, const CreateInfo& createInfo)
    : _instance(createInstance(createInfo))
    , _surface(createSurface(window))
    , _physicalDevice(pickPhysicalDevice(createInfo))
    , _device(CreateDevice(createInfo.deviceExtensions))
{
}

// CreateInstance //////////////////////////////////////////////////////////////////////////////////

vk::raii::Instance Context::createInstance(const CreateInfo& createInfo)
{
    vk::ApplicationInfo appInfo(
        createInfo.appName.c_str(),
        createInfo.appVersion,
        EngineName.c_str(),
        EngineVersion,
        EngineApiVersion);

    const auto layers     = ValidationLayers();
    const auto extensions = InstanceExtensions();

    CheckValidationLayers(layers);
    CheckInstanceExtensions(extensions);

    const auto instanceCreateInfo = vk::InstanceCreateInfo({}, &appInfo, layers, extensions);
    if constexpr (is_debugging_enabled)
    {
        const auto debugMessengerCreateInfo = DebugUtilsMessengerCreateInfoEXT();
        vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT>
            createInfoChain(instanceCreateInfo, debugMessengerCreateInfo);
        return vk::raii::Instance(_context, createInfoChain.get<vk::InstanceCreateInfo>());
    }
    else
        return vk::raii::Instance(_context, instanceCreateInfo);
}

std::vector<const char*> Context::ValidationLayers()
{
    if constexpr (is_debugging_enabled)
        return {
            // "VK_LAYER_LUNARG_api_dump",
            "VK_LAYER_KHRONOS_validation",
        };
    else
        return {};
}

std::vector<const char*> Context::InstanceExtensions()
{
    uint32_t count; // NOLINT
    const auto raw_extensions = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> extensions(raw_extensions, raw_extensions + count); // NOLINT
    if (is_debugging_enabled)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Context::DebugUtilsMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* /*pUserData*/)
{
    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        Log::Core::Trace("{}", pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        Log::Core::Info("{}", pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        Log::Core::Warn("{}", pCallbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        Log::Core::Error("{}", pCallbackData->pMessage);
        break;
    }
    return vk::False;
}

vk::DebugUtilsMessengerCreateInfoEXT Context::DebugUtilsMessengerCreateInfoEXT()
{
    constexpr vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    constexpr vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    constexpr vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        {}, severityFlags, messageTypeFlags, &DebugUtilsMessengerCallback);

    return createInfo;
}

void Context::CheckValidationLayers(const std::vector<const char*>& layers)
{
    auto required =
        layers | std::ranges::views::transform([](const auto layer) { return std::string(layer); });

    for (const auto& layer : required)
        Log::Core::Trace("Required layer: {}", layer);

    auto available =
        vk::enumerateInstanceLayerProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.layerName); });

    for (const auto& layer : available)
        Log::Core::Trace("Available layer: {}", layer);

    std::set<std::string> missing(required.begin(), required.end());
    for (const auto& layer : available)
        missing.erase(layer);

    if (missing.empty())
        for (const auto& layer : layers)
            Log::Core::Info("Enabling validation layer: {}", layer);
    else
        for (const auto& layer : missing)
            Log::Core::Warn("Unsupported validation layer: {}", layer);
}

void Context::CheckInstanceExtensions(const std::vector<const char*>& extensions)
{
    const auto required =
        extensions | std::ranges::views::transform([](const auto ext) { return std::string(ext); });

    for (const auto& ext : required)
        Log::Core::Trace("Required instance extension: {}", ext);

    const auto available =
        vk::enumerateInstanceExtensionProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.extensionName); });

    for (const auto& ext : available)
        Log::Core::Trace("Available instance extension: {}", ext);

    std::set<std::string> missing(required.begin(), required.end());
    for (const auto& layer : available)
        missing.erase(layer);

    if (missing.empty())
        for (const auto& ext : extensions)
            Log::Core::Info("Enabling instance extension: {}", ext);
    else
        for (const auto& ext : missing)
            Log::Core::Warn("Unsupported instance extension: {}", ext);
}

// CreateSurface ///////////////////////////////////////////////////////////////////////////////////

vk::raii::SurfaceKHR Context::createSurface(const Window& window)
{
    return vk::raii::SurfaceKHR(_instance, window.createSurface(*_instance));
}

// PickPhysicaldevice //////////////////////////////////////////////////////////////////////////////

Context::PhysicalDevice Context::pickPhysicalDevice(const CreateInfo& createInfo)
{
    // Get all physical devices
    vk::raii::PhysicalDevices allPhysicalDevices(_instance);

    Log::Core::Info("Found {} physical devices:", allPhysicalDevices.size());

    // Partition by device type, ignoring unsuitable candidates
    std::array<std::vector<std::pair<int, vk::raii::PhysicalDevice>>, 3> physicalDevices;
    size_t binSize = allPhysicalDevices.size() / 3;
    for (auto& vec : physicalDevices)
        vec.reserve(binSize);

    std::map<vk::PhysicalDeviceType, int> binIndex{{
        {vk::PhysicalDeviceType::eDiscreteGpu, 0},
        {vk::PhysicalDeviceType::eIntegratedGpu, 1},
        {vk::PhysicalDeviceType::eCpu, 2},
    }};

    for (auto&& [i, physicalDevice] : std::ranges::views::enumerate(allPhysicalDevices))
    {
        const auto props = physicalDevice.getProperties();

        Log::Core::Info(
            "  [{}] {} ({}), API version {}.{}.{}",
            i + 1,
            std::string(props.deviceName),
            vk::to_string(props.deviceType),
            vk::apiVersionMajor(props.apiVersion),
            vk::apiVersionMinor(props.apiVersion),
            vk::apiVersionPatch(props.apiVersion));

        if (props.deviceType == vk::PhysicalDeviceType::eVirtualGpu ||
            props.deviceType == vk::PhysicalDeviceType::eOther)
            continue;

        if (props.apiVersion < EngineApiVersion)
            continue;

        // NOLINTNEXTLINE
        physicalDevices[binIndex[props.deviceType]].emplace_back(i, std::move(physicalDevice));
    }

    // Determine the preferred order of device types
    std::array<int, 3> binOrder; // NOLINT
    switch (createInfo.preferredDeviceType)
    {
    case DeviceType::Discrete:
        binOrder = {0, 1, 2};
        break;
    case DeviceType::Integrated:
        binOrder = {1, 0, 2};
        break;
    case DeviceType::Software:
        binOrder = {2, 1, 0};
        break;
    case DeviceType::Virtual:
        Log::Core::Assert(false, "Refusing to pick a virtual device");
        break;
    }

    const auto deviceExtensions = DeviceExtensions(createInfo.deviceExtensions);

    // Pick the first suitable candidate, starting with the preferred type
    for (const size_t bin : binOrder)
        for (auto& [i, physicalDevice] : physicalDevices[bin]) // NOLINT
        {
            const auto queueFamilyIndices = findQueueFamilyIndices(physicalDevice);
            if (!queueFamilyIndices)
                continue;

            Log::Core::Trace("Checking device {}", i + 1);

            if (!CheckDeviceExtensions(physicalDevice, deviceExtensions))
            {
                Log::Core::Trace("  reject: missing device extensions");
                continue;
            }

            const auto formats = physicalDevice.getSurfaceFormatsKHR(_surface);
            if (formats.empty())
            {
                Log::Core::Trace("  reject: no compatible surface formats");
                continue;
            }

            const auto features = DeviceFeatures(physicalDevice);
            if (!features)
            {
                Log::Core::Trace("  reject: missing required device features");
                continue;
            }

            const auto presentModes = physicalDevice.getSurfacePresentModesKHR(_surface);
            const auto props        = physicalDevice.getProperties();

            Log::Core::Info(
                "Picked physical device {}: {} ({}), API version {}.{}.{}",
                i + 1,
                std::string(props.deviceName),
                vk::to_string(props.deviceType),
                vk::apiVersionMajor(props.apiVersion),
                vk::apiVersionMinor(props.apiVersion),
                vk::apiVersionPatch(props.apiVersion));
            Log::Core::Info("  graphics queue family index: {}", queueFamilyIndices->first);
            Log::Core::Info("  compute queue family index: {}", queueFamilyIndices->second);
            Log::Core::Info("  formats: {}", SurfaceFormatNames(formats));
            Log::Core::Info("  present modes: {}", SufacePresentModeNames(presentModes));

            return PhysicalDevice(
                std::move(physicalDevice),
                queueFamilyIndices->first,
                queueFamilyIndices->second,
                formats,
                presentModes,
                *features);
        }

    throw std::runtime_error("No suitable physical device found");
}

std::optional<std::pair<uint32_t, uint32_t>> Context::findQueueFamilyIndices(
    const vk::raii::PhysicalDevice& physicalDevice)
{
    const auto families = physicalDevice.getQueueFamilyProperties();

    std::optional<uint32_t> graphics, compute, maybeCompute;
    for (const auto&& [index, family] : std::ranges::views::enumerate(families))
    {
        if (!graphics && family.queueFlags & vk::QueueFlagBits::eGraphics &&
            physicalDevice.getSurfaceSupportKHR(index, _surface))
            graphics = index;

        // try to find a dedicated compute queue
        if (!compute && family.queueFlags & vk::QueueFlagBits::eCompute)
        {
            if (!(family.queueFlags & vk::QueueFlagBits::eGraphics))
                compute = index;
            else if (!maybeCompute)
                maybeCompute = index;
        }

        //  if no dedicated compute queue, use shared compute queue if one was found
        if (!compute && maybeCompute)
            compute = maybeCompute;

        if (graphics && compute)
            return std::make_pair(*graphics, *compute);
    }

    return std::nullopt;
}

std::vector<const char*> Context::DeviceExtensions(const std::vector<const char*>& deviceExtensions)
{
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    extensions.append_range(deviceExtensions);
    return extensions;
}

bool Context::CheckDeviceExtensions(
    const vk::raii::PhysicalDevice& physicalDevice, const std::vector<const char*>& extensions)
{
    for (const auto& ext : extensions)
        Log::Core::Trace("Required device extension: {}", ext);

    const auto available =
        physicalDevice.enumerateDeviceExtensionProperties() |
        std::ranges::views::transform([](const auto p) { return std::string(p.extensionName); });

    for (const auto& ext : available)
        Log::Core::Trace("Available device extension: {}", ext);

    std::set<std::string> missing(extensions.begin(), extensions.end());
    for (const auto& layer : available)
        missing.erase(layer);

    if (!missing.empty())
    {
        for (const auto& ext : missing)
            Log::Core::Warn("Unsupported device extension: {}", ext);
        return false;
    }

    Log::Core::Trace("All required device extensions are available");

    return true;
}

std::optional<Context::FeaturesChain> Context::DeviceFeatures(
    const vk::raii::PhysicalDevice& physicalDevice)
{
    auto [f10, features11, features12, features13] = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features>();

    auto& features10 = f10.features;

    bool reuired_result = true, optional_result = true;

    auto require = [&reuired_result](const vk::Bool32 value, const std::string& name) {
        if (value)
            Log::Core::Trace("Found required device feature: {}", name);
        else
        {
            Log::Core::Trace("Missing device feature: {}", name);
            reuired_result = true;
        }
    };

    auto optional = [&optional_result](const vk::Bool32 value, const std::string& name) {
        if (value)
            Log::Core::Trace("Found optional device feature: {}", name);
        else
        {
            Log::Core::Trace("Missing optional device feature: {}", name);
            optional_result = false;
        }
    };

    auto disable = [](vk::Bool32& value, const std::string& name) {
        if (value)
        {
            value = false;
            Log::Core::Trace("Disabling device feature: {}", name);
        }
        else
            Log::Core::Trace("Disabled evice feature: {}", name);
    };

    // API versoion 1.0

    disable(features10.robustBufferAccess, "robustBufferAccess (1.0)");
    disable(features10.fullDrawIndexUint32, "fullDrawIndexUint32 (1.0)");
    disable(features10.imageCubeArray, "imageCubeArray (1.0)");
    disable(features10.independentBlend, "independentBlend (1.0)");
    optional(features10.geometryShader, "geometryShader (1.0");
    optional(features10.tessellationShader, "tessellationShader (1.0");
    require(features10.sampleRateShading, "sampleRateShading (1.0)");
    disable(features10.dualSrcBlend, "dualSrcBlend (1.0)");
    disable(features10.logicOp, "logicOp (1.0)");
    require(features10.multiDrawIndirect, "multiDrawIndirect (1.0)");
    require(features10.drawIndirectFirstInstance, "drawIndirectFirstInstance (1.0)");
    disable(features10.depthClamp, "depthClamp (1.0)");
    require(features10.depthBiasClamp, "depthBiasClamp (1.0)");
    optional(features10.fillModeNonSolid, "fillModeNonSolid (1.0");
    disable(features10.depthBounds, "depthBounds (1.0)");
    disable(features10.wideLines, "wideLines (1.0)");
    disable(features10.largePoints, "largePoints (1.0)");
    disable(features10.alphaToOne, "alphaToOne (1.0)");
    disable(features10.multiViewport, "multiViewport (1.0)");
    require(features10.samplerAnisotropy, "samplerAnisotropy (1.0)");
    disable(features10.textureCompressionETC2, "textureCompressionETC2 (1.0)");
    disable(features10.textureCompressionASTC_LDR, "textureCompressionASTC_LDR (1.0)");
    optional(features10.textureCompressionBC, "textureCompressionBC (1.0)");
    disable(features10.occlusionQueryPrecise, "occlusionQueryPrecise (1.0)");
    disable(features10.pipelineStatisticsQuery, "pipelineStatisticsQuery (1.0)");
    optional(features10.vertexPipelineStoresAndAtomics, "vertexPipelineStoresAndAtomics (1.0)");
    require(features10.fragmentStoresAndAtomics, "fragmentStoresAndAtomics (1.0)");
    disable(
        features10.shaderTessellationAndGeometryPointSize,
        "shaderTessellationAndGeometryPointSize (1.0)");
    require(features10.shaderImageGatherExtended, "shaderImageGatherExtended (1.0)");
    disable(
        features10.shaderStorageImageExtendedFormats, "shaderStorageImageExtendedFormats (1.0)");
    disable(features10.shaderStorageImageMultisample, "shaderStorageImageMultisample (1.0)");
    disable(
        features10.shaderStorageImageReadWithoutFormat,
        "shaderStorageImageReadWithoutFormat (1.0)");
    disable(
        features10.shaderStorageImageWriteWithoutFormat,
        "shaderStorageImageWriteWithoutFormat (1.0)");
    disable(
        features10.shaderUniformBufferArrayDynamicIndexing,
        "shaderUniformBufferArrayDynamicIndexing (1.0)");
    disable(
        features10.shaderSampledImageArrayDynamicIndexing,
        "shaderSampledImageArrayDynamicIndexing (1.0)");
    disable(
        features10.shaderStorageBufferArrayDynamicIndexing,
        "shaderStorageBufferArrayDynamicIndexing (1.0)");
    disable(
        features10.shaderStorageImageArrayDynamicIndexing,
        "shaderStorageImageArrayDynamicIndexing (1.0)");
    disable(features10.shaderClipDistance, "shaderClipDistance (1.0)");
    disable(features10.shaderCullDistance, "shaderCullDistance (1.0)");
    disable(features10.shaderFloat64, "shaderFloat64 (1.0)");
    optional(features10.shaderInt64, "shaderInt64 (1.0)");
    optional(features10.shaderInt16, "shaderInt16 (1.0)");
    disable(features10.shaderResourceResidency, "shaderResourceResidency (1.0)");
    disable(features10.shaderResourceMinLod, "shaderResourceMinLod (1.0)");
    disable(features10.sparseBinding, "sparseBinding (1.0)");
    disable(features10.sparseResidencyBuffer, "sparseResidencyBuffer (1.0)");
    disable(features10.sparseResidencyImage2D, "sparseResidencyImage2D (1.0)");
    disable(features10.sparseResidencyImage3D, "sparseResidencyImage3D (1.0)");
    disable(features10.sparseResidency2Samples, "sparseResidency2Samples (1.0)");
    disable(features10.sparseResidency4Samples, "sparseResidency4Samples (1.0)");
    disable(features10.sparseResidency8Samples, "sparseResidency8Samples (1.0)");
    disable(features10.sparseResidency16Samples, "sparseResidency16Samples (1.0)");
    disable(features10.sparseResidencyAliased, "sparseResidencyAliased (1.0)");
    disable(features10.variableMultisampleRate, "variableMultisampleRate (1.0)");
    disable(features10.inheritedQueries, "inheritedQueries (1.0)");

    // API version 1.1

    require(features11.storageBuffer16BitAccess, "storageBuffer16BitAccess (1.1)");
    disable(
        features11.uniformAndStorageBuffer16BitAccess, "uniformAndStorageBuffer16BitAccess (1.1)");
    disable(features11.storagePushConstant16, "storagePushConstant16 (1.1)");
    disable(features11.storageInputOutput16, "storageInputOutput16 (1.1)");
    optional(features11.multiview, "multiview (1.1)");
    disable(features11.multiviewGeometryShader, "multiviewGeometryShader (1.1)");
    disable(features11.multiviewTessellationShader, "multiviewTessellationShader (1.1)");
    disable(features11.variablePointersStorageBuffer, "variablePointersStorageBuffer (1.1)");
    disable(features11.variablePointers, "variablePointers (1.1)");
    disable(features11.protectedMemory, "protectedMemory (1.1)");
    optional(features11.samplerYcbcrConversion, "samplerYcbcrConversion (1.1)");
    require(features11.shaderDrawParameters, "shaderDrawParameters (1.1)");

    // API verson 1.2

    disable(features12.samplerMirrorClampToEdge, "samplerMirrorClampToEdge (1.2)");
    optional(features12.drawIndirectCount, "drawIndirectCount (1.2)");
    optional(features12.storageBuffer8BitAccess, "storageBuffer8BitAccess (1.2)");
    optional(
        features12.uniformAndStorageBuffer8BitAccess, "uniformAndStorageBuffer8BitAccess (1.2)");
    disable(features12.storagePushConstant8, "storagePushConstant8 (1.2)");
    disable(features12.shaderBufferInt64Atomics, "shaderBufferInt64Atomics (1.2)");
    disable(features12.shaderSharedInt64Atomics, "shaderSharedInt64Atomics (1.2)");
    optional(features12.shaderFloat16, "shaderFloat16 (1.2)");
    optional(features12.shaderInt8, "shaderInt8 (1.2)");
    require(features12.descriptorIndexing, "descriptorIndexing (1.2)");
    disable(
        features12.shaderInputAttachmentArrayDynamicIndexing,
        "shaderInputAttachmentArrayDynamicIndexing (1.2)");
    disable(
        features12.shaderUniformTexelBufferArrayDynamicIndexing,
        "shaderUniformTexelBufferArrayDynamicIndexing (1.2)");
    disable(
        features12.shaderStorageTexelBufferArrayDynamicIndexing,
        "shaderStorageTexelBufferArrayDynamicIndexing (1.2)");
    disable(
        features12.shaderUniformBufferArrayNonUniformIndexing,
        "shaderUniformBufferArrayNonUniformIndexing (1.2)");
    require(
        features12.shaderSampledImageArrayNonUniformIndexing,
        "shaderSampledImageArrayNonUniformIndexing (1.2)");
    disable(
        features12.shaderStorageBufferArrayNonUniformIndexing,
        "shaderStorageBufferArrayNonUniformIndexing (1.2)");
    disable(
        features12.shaderStorageImageArrayNonUniformIndexing,
        "shaderStorageImageArrayNonUniformIndexing (1.2)");
    disable(
        features12.shaderInputAttachmentArrayNonUniformIndexing,
        "shaderInputAttachmentArrayNonUniformIndexing (1.2)");
    disable(
        features12.shaderUniformTexelBufferArrayNonUniformIndexing,
        "shaderUniformTexelBufferArrayNonUniformIndexing (1.2)");
    disable(
        features12.shaderStorageTexelBufferArrayNonUniformIndexing,
        "shaderStorageTexelBufferArrayNonUniformIndexing (1.2)");
    disable(
        features12.descriptorBindingUniformBufferUpdateAfterBind,
        "descriptorBindingUniformBufferUpdateAfterBind (1.2)");
    require(
        features12.descriptorBindingSampledImageUpdateAfterBind,
        "descriptorBindingSampledImageUpdateAfterBind (1.2)");
    require(
        features12.descriptorBindingStorageImageUpdateAfterBind,
        "descriptorBindingStorageImageUpdateAfterBind (1.2)");
    disable(
        features12.descriptorBindingStorageBufferUpdateAfterBind,
        "descriptorBindingStorageBufferUpdateAfterBind (1.2)");
    disable(
        features12.descriptorBindingUniformTexelBufferUpdateAfterBind,
        "descriptorBindingUniformTexelBufferUpdateAfterBind (1.2)");
    disable(
        features12.descriptorBindingStorageTexelBufferUpdateAfterBind,
        "descriptorBindingStorageTexelBufferUpdateAfterBind (1.2)");
    require(
        features12.descriptorBindingUpdateUnusedWhilePending,
        "descriptorBindingUpdateUnusedWhilePending (1.2)");
    require(features12.descriptorBindingPartiallyBound, "descriptorBindingPartiallyBound (1.2)");
    require(
        features12.descriptorBindingVariableDescriptorCount,
        "descriptorBindingVariableDescriptorCount (1.2)");
    require(features12.runtimeDescriptorArray, "runtimeDescriptorArray (1.2)");
    disable(features12.samplerFilterMinmax, "samplerFilterMinmax (1.2)");
    require(features12.scalarBlockLayout, "scalarBlockLayout (1.2)");
    disable(features12.imagelessFramebuffer, "imagelessFramebuffer (1.2)");
    require(features12.uniformBufferStandardLayout, "uniformBufferStandardLayout (1.2)");
    disable(features12.shaderSubgroupExtendedTypes, "shaderSubgroupExtendedTypes (1.2)");
    disable(features12.separateDepthStencilLayouts, "separateDepthStencilLayouts (1.2)");
    optional(features12.hostQueryReset, "hostQueryReset (1.2)");
    require(features12.timelineSemaphore, "timelineSemaphore (1.2)");
    require(features12.bufferDeviceAddress, "bufferDeviceAddress (1.2)");
    disable(features12.bufferDeviceAddressCaptureReplay, "bufferDeviceAddressCaptureReplay (1.2)");
    disable(features12.bufferDeviceAddressMultiDevice, "bufferDeviceAddressMultiDevice (1.2)");
    optional(features12.vulkanMemoryModel, "vulkanMemoryModel (1.2)");
    optional(features12.vulkanMemoryModelDeviceScope, "vulkanMemoryModelDeviceScope (1.2)");
    disable(
        features12.vulkanMemoryModelAvailabilityVisibilityChains,
        "vulkanMemoryModelAvailabilityVisibilityChains (1.2)");
    disable(features12.shaderOutputViewportIndex, "shaderOutputViewportIndex (1.2)");
    disable(features12.shaderOutputLayer, "shaderOutputLayer (1.2)");
    disable(features12.subgroupBroadcastDynamicId, "subgroupBroadcastDynamicId (1.2)");

    // API Version 1.3

    disable(features13.robustImageAccess, "robustImageAccess (1.3)");
    disable(features13.inlineUniformBlock, "inlineUniformBlock (1.3)");
    disable(features13.privateData, "privateData (1.3)");
    optional(features13.shaderDemoteToHelperInvocation, "shaderDemoteToHelperInvocation (1.3)");
    disable(features13.shaderTerminateInvocation, "shaderTerminateInvocation (1.3)");
    require(features13.subgroupSizeControl, "subgroupSizeControl (1.3)");
    disable(features13.computeFullSubgroups, "computeFullSubgroups (1.3)");
    require(features13.synchronization2, "synchronization2 (1.3)");
    disable(features13.textureCompressionASTC_HDR, "textureCompressionASTC_HDR (1.3)");
    require(features13.dynamicRendering, "dynamicRendering (1.3)");
    disable(features13.shaderIntegerDotProduct, "shaderIntegerDotProduct (1.3)");
    require(features13.maintenance4, "maintenance4 (1.3)");

    if (reuired_result)
        Log::Core::Info("All required device features are available");

    if (optional_result)
        Log::Core::Info("All optional device features are available");

    if (!reuired_result)
        return std::nullopt;

    vk::PhysicalDeviceFeatures2 features2(features10);

    FeaturesChain featuresChain = {features2, features11, features12, features13};

    return featuresChain;
}

std::string Context::SurfaceFormatNames(const std::vector<vk::SurfaceFormatKHR>& formats)
{
    const auto formatName = [](const vk::SurfaceFormatKHR& format) {
        return vk::to_string(format.format) + "+" + vk::to_string(format.colorSpace);
    };

    return formats | std::ranges::views::transform(formatName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

std::string Context::SufacePresentModeNames(const std::vector<vk::PresentModeKHR>& presentModes)
{
    const auto modeName = [](const vk::PresentModeKHR& mode) { return vk::to_string(mode); };

    return presentModes | std::ranges::views::transform(modeName) |
           std::ranges::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>();
}

// CreateDevice
// ////////////////////////////////////////////////////////////////////////////////////

Context::Device Context::CreateDevice(const std::vector<const char*>& deviceExtensions)
{
    const float queuePriority = 1.0F;

    const std::array<vk::DeviceQueueCreateInfo, 2> queueCreateInfo{
        vk::DeviceQueueCreateInfo({}, _physicalDevice.graphicsQueueFamilyIndex, 1, &queuePriority),
        vk::DeviceQueueCreateInfo({}, _physicalDevice.computeQueueFamilyIndex, 1, &queuePriority),
    };

    const uint32_t numQueues =
        _physicalDevice.graphicsQueueFamilyIndex == _physicalDevice.computeQueueFamilyIndex ? 1 : 2;
    const auto extensions = DeviceExtensions(deviceExtensions);

    vk::DeviceCreateInfo createInfo(
        {},
        numQueues,
        queueCreateInfo.data(),
        0,
        nullptr,
        extensions.size(),
        extensions.data(),
        &_physicalDevice.features.get<vk::PhysicalDeviceFeatures2>().features,
        _physicalDevice.features.get<vk::PhysicalDeviceVulkan13Features>());

    vk::raii::Device device(_physicalDevice.handle, createInfo);

    Log::Core::Info("Logical device created");

    vk::raii::Queue graphicsQueue = device.getQueue(_physicalDevice.graphicsQueueFamilyIndex, 0);
    vk::raii::Queue computeQueue  = device.getQueue(_physicalDevice.computeQueueFamilyIndex, 0);

    Log::Core::Info("Device queues created");

    return Device(std::move(device), std::move(graphicsQueue), std::move(computeQueue));
}

} // namespace VoxelDynamics::Vulkan
