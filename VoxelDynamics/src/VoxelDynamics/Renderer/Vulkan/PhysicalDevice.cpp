#include "VoxelDynamics/Renderer/Vulkan/PhysicalDevice.hpp"

namespace VoxelDynamics::Vulkan
{
PhysicalDevice::PhysicalDevice(
    vk::raii::PhysicalDevice handle_,
    uint32_t graphicsQueueFamilyIndex_,
    uint32_t computeQueueFamilyIndex_,
    Surface surface_,
    const PhysicalDevice::FeaturesChain& features_)
    : handle(std::move(handle_))
    , graphicsQueueFamilyIndex(graphicsQueueFamilyIndex_)
    , computeQueueFamilyIndex(computeQueueFamilyIndex_)
    , surface(std::move(surface_))
    , features(features_)
{
}

PhysicalDevice PhysicalDevice::Create(
    const vk::raii::Instance& instance,
    vk::raii::SurfaceKHR vk_surface,
    const PhysicalDeviceType preferredType,
    const std::vector<const char*>& extraExtensions)
{
    // Get all physical devices
    vk::raii::PhysicalDevices allPhysicalDevices(instance);

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
    switch (preferredType)
    {
    case PhysicalDeviceType::Discrete:
        binOrder = {0, 1, 2};
        break;
    case PhysicalDeviceType::Integrated:
        binOrder = {1, 0, 2};
        break;
    case PhysicalDeviceType::Software:
        binOrder = {2, 1, 0};
        break;
    case PhysicalDeviceType::Virtual:
        Log::Core::Assert(false, "Refusing to pick a virtual device");
        break;
    }

    const auto extensions = Extensions(extraExtensions);

    // Pick the first suitable candidate, starting with the preferred type
    for (const size_t bin : binOrder)
        for (auto& [i, physicalDevice] : physicalDevices[bin]) // NOLINT
        {
            if (!CheckExtensions(physicalDevice, extraExtensions))
            {
                Log::Core::Trace("  reject: missing device extensions");
                continue;
            }

            const auto queueFamilyIndices = findQueueFamilyIndices(physicalDevice, vk_surface);
            if (!queueFamilyIndices)
                continue;

            Log::Core::Trace("Checking device {}", i + 1);

            auto formats = physicalDevice.getSurfaceFormatsKHR(vk_surface);
            if (formats.empty())
            {
                Log::Core::Trace("  reject: no compatible surface formats");
                continue;
            }

            const auto features = Features(physicalDevice);
            if (!features)
            {
                Log::Core::Trace("  reject: missing required device features");
                continue;
            }

            auto presentModes = physicalDevice.getSurfacePresentModesKHR(vk_surface);
            const auto props  = physicalDevice.getProperties();

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
            Log::Core::Info("  formats: {}", Surface::FormatNames(formats));
            Log::Core::Info("  present modes: {}", Surface::PresentModeNames(presentModes));

            return PhysicalDevice(
                std::move(physicalDevice),
                queueFamilyIndices->first,
                queueFamilyIndices->second,
                Surface(std::move(vk_surface), std::move(formats), std::move(presentModes)),
                *features);
        }

    throw std::runtime_error("No suitable physical device found");
}

std::vector<const char*> PhysicalDevice::Extensions(const std::vector<const char*>& extraExtensions)
{
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    extensions.append_range(extraExtensions);
    return extensions;
}

bool PhysicalDevice::CheckExtensions(
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

std::optional<std::pair<uint32_t, uint32_t>> PhysicalDevice::findQueueFamilyIndices(
    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface)
{
    const auto families = physicalDevice.getQueueFamilyProperties();

    std::optional<uint32_t> graphics, compute, maybeCompute;
    for (const auto&& [index, family] : std::ranges::views::enumerate(families))
    {
        if (!graphics && family.queueFlags & vk::QueueFlagBits::eGraphics &&
            physicalDevice.getSurfaceSupportKHR(index, surface))
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

std::optional<PhysicalDevice::FeaturesChain> PhysicalDevice::Features(
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

} // namespace VoxelDynamics::Vulkan
