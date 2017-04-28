/**
 * Aldaron's Device Interface - "vw.c"
 * Copyright 2017 (c) Jeron Lau - Licensed under the GNU GENERAL PUBLIC LICENSE
 *
 * This file is based off of LunarG's SDK example:
 * https://www.lunarg.com/vulkan-sdk/, and this tutorial by José Henriques:
 * http://jhenriques.net/vulkan_shaders.html,
**/

#include "vw.h"

/*#if defined(VK_USE_PLATFORM_ANDROID_KHR)

void android_main(struct android_app *app) {
	wrapper_main();
}

#endif*/

// Vulkan

static inline void vw_vulkan_error(const char *msg, VkResult result) {
	if(result != VK_SUCCESS) {
		printf("abort on error %d!", result);
		puts(msg);
		abort();
	}
}

static inline void vw_vulkan_swapchain(vw_t* vulkan) {
	// Find preferred format:
	uint32_t formatCount = 1;
	VkSurfaceFormatKHR surface_format;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan->gpu, vulkan->surface,
		&formatCount, &surface_format);
	vulkan->color_format = surface_format.format == VK_FORMAT_UNDEFINED ?
		VK_FORMAT_B8G8R8_UNORM : surface_format.format;
	VkSurfaceCapabilitiesKHR surface_capables;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->gpu, vulkan->surface,
		&surface_capables);
	uint32_t min = surface_capables.minImageCount;
	uint32_t max = surface_capables.maxImageCount;
	uint32_t desiredImageCount = 0;

	if(min >= max) {
		// Gotta use at least the minimum.
		desiredImageCount = min;
	}else{
		// If double-buffering isn't supported, use single-buffering.
		if(max < 2) desiredImageCount = 1;
	}
	// TODO DEBUG
	printf("min: %d, max: %d, chosen: %d\n", min, max, desiredImageCount);

	// Surface Resolution
	VkExtent2D surfaceResolution = surface_capables.currentExtent;
	if(surfaceResolution.width == -1) {
		surfaceResolution.width = vulkan->width;
		surfaceResolution.height = vulkan->height;
	} else {
		vulkan->width = surfaceResolution.width;
		vulkan->height = surfaceResolution.height;
	}
	// Choose a present mode.
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->gpu, vulkan->surface,
		&presentModeCount, NULL);
	VkPresentModeKHR presentModes[presentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan->gpu, vulkan->surface,
		&presentModeCount, presentModes);
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR; // fallback
	for(uint32_t i = 0; i < presentModeCount; i++) {
		if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR; // optimal
			break;
		}
	}
	// Create the swapchain.
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = vulkan->surface,
		.minImageCount = desiredImageCount,
		.imageFormat = vulkan->color_format,
		.imageColorSpace = 0,
		.imageExtent = surfaceResolution,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = (surface_capables.supportedTransforms & 
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
			: surface_capables.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = 1,
		.oldSwapchain = 0, // NULL TODO: ?
	};
	vw_vulkan_error("Failed to create swapchain.", vkCreateSwapchainKHR(
		vulkan->device, &swapChainCreateInfo, NULL, &vulkan->swapchain));
	vw_vulkan_error("Failed to get swapchain #", vkGetSwapchainImagesKHR(
		vulkan->device, vulkan->swapchain, &vulkan->image_count, NULL));
	vw_vulkan_error("Failed to get swapchain !", vkGetSwapchainImagesKHR(
		vulkan->device, vulkan->swapchain, &vulkan->image_count,
		vulkan->present_images));
}

static inline void vw_vulkan_image_view(vw_t* vulkan) {
	VkComponentMapping components = {
		.r = VK_COMPONENT_SWIZZLE_R, .g = VK_COMPONENT_SWIZZLE_G,
		.b = VK_COMPONENT_SWIZZLE_B, .a = VK_COMPONENT_SWIZZLE_A,
	};
	VkImageViewCreateInfo presentImagesViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = vulkan->color_format,
		.components = components,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	};
	vkCreateFence(vulkan->device, &fenceCreateInfo, NULL,
		&vulkan->submit_fence);

	for(uint32_t i = 0; i < vulkan->image_count; i++) {
		vkBeginCommandBuffer(vulkan->command_buffer, &beginInfo); // TODO: Mem Error
		presentImagesViewCreateInfo.image = vulkan->present_images[i];

		VkImageMemoryBarrier layoutTransitionBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkan->present_images[i],
		};
		VkImageSubresourceRange resourceRange = {
			VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
		};
		layoutTransitionBarrier.subresourceRange = resourceRange;

		vkCmdPipelineBarrier(vulkan->command_buffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 
			1, &layoutTransitionBarrier);

		vkEndCommandBuffer(vulkan->command_buffer);

		VkPipelineStageFlags waitStageMash =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = NULL,
			.pWaitDstStageMask = &waitStageMash,
			.commandBufferCount = 1,
			.pCommandBuffers = &vulkan->command_buffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = NULL,
		};
		vw_vulkan_error("couldn't submit queue", vkQueueSubmit(
			vulkan->present_queue, 1, &submitInfo,
			vulkan->submit_fence));

		vkWaitForFences(vulkan->device, 1, &vulkan->submit_fence,
			VK_TRUE, UINT64_MAX);
		vkResetFences(vulkan->device, 1, &vulkan->submit_fence);

		vkResetCommandBuffer(vulkan->command_buffer, 0);

		puts("vkCreateImageView #1");
		vw_vulkan_error("Could not create ImageView.", vkCreateImageView(
			vulkan->device, &presentImagesViewCreateInfo, NULL,
			&vulkan->present_image_views[i]));
	}
}

static uint32_t memory_type_from_properties(const vw_t* vulkan, uint32_t typeBits,
	VkFlags reqs_mask)
{
	VkPhysicalDeviceMemoryProperties props;
	vkGetPhysicalDeviceMemoryProperties(vulkan->gpu, &props);
	for (uint32_t i = 0; i < props.memoryTypeCount; i++) {
		// Memory type req's matches vkGetImageMemoryRequirements()?
		if ((typeBits & 1) == 1) {
			// Is requirements_mask's requirements fullfilled?
			if ((props.memoryTypes[i].propertyFlags & reqs_mask) ==
				reqs_mask)
			{
				return i;
			}
		}
		// Check next bit from vkGetImageMemoryRequirements().
		typeBits >>= 1;
	}
	// Nothing works ... fallback to 0 and hope nothing bad happens.
	puts("ALDARON WARNING: Couldn't find suitable memory type.");
	return 0;
}

static inline void vw_vulkan_depth_buffer(vw_t* vulkan) {
	VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_D16_UNORM,
		.extent = {
			.width = vulkan->width,
			.height = vulkan->height,
			.depth = 1,
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	vw_vulkan_error("Failed to create depth image.", vkCreateImage(
		vulkan->device, &imageCreateInfo, NULL, &vulkan->depth_image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(vulkan->device, vulkan->depth_image,
		&memoryRequirements);

	VkMemoryAllocateInfo image_ai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memory_type_from_properties(vulkan,
			memoryRequirements.memoryTypeBits,
			0 ),
	};
	vw_vulkan_error("Failed to allocate device memory.", vkAllocateMemory(
		vulkan->device, &image_ai, NULL, &vulkan->depth_image_memory));
	vw_vulkan_error("Failed to bind image memory.", vkBindImageMemory(
		vulkan->device, vulkan->depth_image, vulkan->depth_image_memory,
		0));

	// before using this depth buffer we must change it's layout:
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(vulkan->command_buffer, &beginInfo);

	VkImageSubresourceRange resourceRange = {
		VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1
	};
	VkImageMemoryBarrier layoutTransitionBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = vulkan->depth_image,
		.subresourceRange = resourceRange,
	};

	vkCmdPipelineBarrier(vulkan->command_buffer, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1,
		&layoutTransitionBarrier);

	vkEndCommandBuffer(vulkan->command_buffer);

	VkPipelineStageFlags waitStageMash = 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = &waitStageMash,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan->command_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL,
	};
	vw_vulkan_error("couldn't submit queue", vkQueueSubmit(
		vulkan->present_queue, 1, &submitInfo, vulkan->submit_fence));

	vkWaitForFences(vulkan->device, 1, &vulkan->submit_fence, VK_TRUE,
		UINT64_MAX);
	vkResetFences(vulkan->device, 1, &vulkan->submit_fence);
	vkResetCommandBuffer(vulkan->command_buffer, 0);

	// create the depth image view:
	VkComponentMapping depthComponents = { VK_COMPONENT_SWIZZLE_IDENTITY };
	VkImageViewCreateInfo imageViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = vulkan->depth_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = imageCreateInfo.format,
		.components = depthComponents,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};
	puts("vkCreateImageView #2");
	vw_vulkan_error("Failed to create image view.", vkCreateImageView(
		vulkan->device, &imageViewCreateInfo, NULL,
		&vulkan->depth_image_view));
}

static inline void vw_vulkan_render_pass(vw_t* vulkan) {
	VkAttachmentDescription passAttachments[2] = {
		// Color Buffer
		[0].format = vulkan->color_format,
		[0].samples = VK_SAMPLE_COUNT_1_BIT,
		[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		[0].initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		// Depth Buffer
		[1].format = VK_FORMAT_D16_UNORM,
		[1].samples = VK_SAMPLE_COUNT_1_BIT,
		[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		[1].initialLayout =VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference colorAttachmentReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkAttachmentReference depthAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentReference,
		.pDepthStencilAttachment = &depthAttachmentReference,
	};
	VkRenderPassCreateInfo render_pass_ci = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = passAttachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
	};
	vw_vulkan_error("Failed to create renderpass!", vkCreateRenderPass(
		vulkan->device, &render_pass_ci, NULL, &vulkan->render_pass));
}

static inline void vw_vulkan_framebuffers(vw_t* vulkan) {
	VkImageView frameBufferAttachments[2];
	frameBufferAttachments[1] = vulkan->depth_image_view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = vulkan->render_pass,
		.attachmentCount = 2,
		.pAttachments = frameBufferAttachments,
		.width = vulkan->width, .height = vulkan->height, .layers = 1,
	};

	// create a framebuffer per swap chain imageView:
	for(uint32_t i = 0; i < vulkan->image_count; i++) {
		frameBufferAttachments[0] = vulkan->present_image_views[i];
		vw_vulkan_error("Failed to create framebuffer.",
			vkCreateFramebuffer(vulkan->device,
				&frameBufferCreateInfo, NULL,
				&vulkan->frame_buffers[i]));
	}
}

// Called From Rust FFI
void vw_vulkan_txuniform(const vw_t* vulkan, vw_instance_t* instance,
	const vw_texture_t* tx, uint8_t tex_count)
{
	const int NUM_WRITES = !!tex_count;
	VkDescriptorImageInfo tex_desc;
	if(tex_count) {
		tex_desc = (VkDescriptorImageInfo) {
			.sampler = tx->sampler,
			.imageView = tx->view,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};
	}

	VkWriteDescriptorSet writes[1 + NUM_WRITES]; // 2
	memset(&writes, 0, sizeof(writes));

	VkDescriptorBufferInfo buffer_info = {
		.buffer = instance->matrix_buffer,
		.offset = 0,
		.range = sizeof(float) * 16,
	};
	writes[0] = (VkWriteDescriptorSet) {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = instance->desc_set,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &buffer_info,
	};

	if(NUM_WRITES) {
		writes[1] = (VkWriteDescriptorSet) {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = instance->desc_set,
			.dstBinding = 1,
			.descriptorCount = tex_count,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &tex_desc,
		};
	}

	vkUpdateDescriptorSets(vulkan->device, 1 + NUM_WRITES, writes, 0, NULL);
}

// Called From Rust FFI
vw_instance_t vw_vulkan_uniforms(const vw_t* vulkan, vw_shape_t* shape,
	const vw_texture_t* tx, uint8_t tex_count)
{
	vw_instance_t instance;

	// Buffers
	VkBufferCreateInfo uniform_buffer_ci = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = sizeof(float) * 16, // mat4
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
	};
	puts("E: MATRIX BUFFER");
	vw_vulkan_error("Failed to create matrix buffer.", vkCreateBuffer(
		vulkan->device, &uniform_buffer_ci, NULL,
		&instance.matrix_buffer));
	puts("E: MATRIX BUFFER");

	// Descriptor Pool
	const VkDescriptorPoolSize type_counts[2] = {
		[0] = {
			 .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			 .descriptorCount = 1,
		},
		[1] = {
			 .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			 .descriptorCount = tex_count, // Texture count
		},
	};
	const VkDescriptorPoolCreateInfo descriptor_pool = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.maxSets = 1,
		.poolSizeCount = 1 + tex_count,
		.pPoolSizes = &type_counts[0],
	};
	printf("VvkCreateDescriptorPool %d\n", tex_count);
	vw_vulkan_error("Failed to create descriptor pool.",
		vkCreateDescriptorPool(vulkan->device, &descriptor_pool, NULL,
			&instance.desc_pool));
	puts("VvkCreateDescriptorPool");

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = instance.desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &shape->pipeline->descsetlayout
	};
	puts("VvkAllocateDescriptorSets");
	vw_vulkan_error("Failed to allocate descriptor sets.",
		vkAllocateDescriptorSets(vulkan->device, &alloc_info,
			&instance.desc_set));
	puts("VvkAllocateDescriptorSets");

// {
	instance.uniform_memory = 0;

	// Allocate memory for uniform buffer.
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(vulkan->device, instance.matrix_buffer,
		&mem_reqs);
	VkMemoryAllocateInfo buffer_ai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = memory_type_from_properties(vulkan,
			mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	};

	vw_vulkan_error("Failed to allocate uniform memory.", vkAllocateMemory(
		vulkan->device, &buffer_ai, NULL, &instance.uniform_memory));
	printf("kkkkkVVBindBufferMemory %lu\n", (long unsigned int) vulkan->device);
	vkBindBufferMemory(vulkan->device, instance.matrix_buffer,
		instance.uniform_memory, 0);
	printf("kkkkkkVVBindBufferMemory %lu\n", (long unsigned int) instance.uniform_memory);
// }
	vw_vulkan_txuniform(vulkan, &instance, tx, tex_count);
	return instance;
}

void vw_vulkan_shape(vw_shape_t* shape, vw_t vulkan, const float* v,
	uint32_t size)
{
	// Create our vertex buffer:
	VkBufferCreateInfo vertex_buffer_ci = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = sizeof(float) * size, // size in Bytes
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
	};
	vw_vulkan_error("Failed to create vertex input buffer.", vkCreateBuffer(
		vulkan.device, &vertex_buffer_ci, NULL,
		&shape->vertex_input_buffer));

	// Allocate memory for vertex buffer.
	VkMemoryRequirements vertexBufferMemoryRequirements;
	vkGetBufferMemoryRequirements(vulkan.device, shape->vertex_input_buffer,
		&vertexBufferMemoryRequirements);
	VkMemoryAllocateInfo bufferAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = vertexBufferMemoryRequirements.size,
		.memoryTypeIndex = memory_type_from_properties(&vulkan,
			vertexBufferMemoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ),
	};
	vw_vulkan_error("Failed to allocate buffer memory.", vkAllocateMemory(
		vulkan.device, &bufferAllocateInfo, NULL,
		&shape->vertex_buffer_memory));
	// Copy buffer data.
	void *mapped;
	puts("vVKMapEMOY");
//	printf("%d %d %d %d %d\n", sizeof(VkDevice), sizeof(VkDeviceMemory), sizeof(VkDeviceSize), sizeof(VkMemoryMapFlags), sizeof(void**));
	vw_vulkan_error("Failed to map buffer memory.", vkMapMemory(
		vulkan.device, shape->vertex_buffer_memory, 0, VK_WHOLE_SIZE, 0,
		&mapped));
	puts("vVKMapEMOY");
	memcpy(mapped, v, sizeof(float) * size);
	vkUnmapMemory(vulkan.device, shape->vertex_buffer_memory);
	puts("vVkBindBufferMemory");
	vw_vulkan_error("Failed to bind buffer memory.", vkBindBufferMemory(
		vulkan.device, shape->vertex_input_buffer,
		shape->vertex_buffer_memory, 0));
	puts("vVkBindBufferMemory");
}

float* test_map(VkDevice device, VkDeviceMemory vertex_buffer_memory, uint64_t wholesize) {
	void* mapped = NULL;
	vw_vulkan_error("Failed to test map buffer memory.", vkMapMemory(
		device, vertex_buffer_memory, 0, wholesize, 0,
		&mapped));
	return mapped;
}

void execute_end_command_buffer(vw_t* vulkan) {
	puts("END COMMAND BUFFER");
	vw_vulkan_error("Failed to end command buffer.", vkEndCommandBuffer(
		vulkan->command_buffer));
	puts("END COMMAND BUFFER");
}

void execute_queue_command_buffer(vw_t* vulkan) {
	VkFence drawFence;
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
	};
	const VkPipelineStageFlags pipe_stage_flags =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {
		.pNext = NULL,
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = &pipe_stage_flags,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan->command_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL,
	};

	vw_vulkan_error("Failed to create fence", vkCreateFence(vulkan->device,
		&fenceInfo, NULL, &drawFence));
	vw_vulkan_error("Failed to submit queue", vkQueueSubmit(vulkan->present_queue, 1,
		&submit_info, drawFence));
	printf("Begin wait for fences....\n");
	/*vw_vulkan_error("Failed to wait for fences", */while(vkWaitForFences(vulkan->device, 1,
		&drawFence, VK_TRUE, 1000) == 2);
	printf("End wait for fences....\n");
	vkDestroyFence(vulkan->device, drawFence, NULL);
}

void set_image_layout(vw_t* vulkan, VkImage image,
	VkImageAspectFlags aspectMask, VkImageLayout old_image_layout,
	VkImageLayout new_image_layout,	VkPipelineStageFlags src_stages,
	VkPipelineStageFlags dest_stages)
{
/*	VkImageMemoryBarrier image_memory_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = old_image_layout,
		.newLayout = new_image_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = aspectMask,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};
*/
/*	switch(old_image_layout) {
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			image_memory_barrier.srcAccessMask =
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			image_memory_barrier.srcAccessMask =
				VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			image_memory_barrier.srcAccessMask =
				VK_ACCESS_HOST_WRITE_BIT;
			break;
		default:
			break;
	}

	switch(new_image_layout) {
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			image_memory_barrier.dstAccessMask =
				VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			image_memory_barrier.dstAccessMask =
				VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			image_memory_barrier.dstAccessMask =
				VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			image_memory_barrier.dstAccessMask =
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			image_memory_barrier.dstAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		default:
			break;
	}
*/
//	printf("CMDPIPELINEBARRIER\n");
//	vkCmdPipelineBarrier(vulkan->command_buffer, src_stages, dest_stages, 0,
//		0, NULL, 0, NULL, 1, &image_memory_barrier);
//	printf("CMDPIPELINEBARRIER\n");
}

void vw_vulkan_animate(vw_t* vulkan, vw_texture_t* tx, uint32_t w, uint32_t h,
	const uint8_t* p, uint8_t ka, uint8_t kr, uint8_t kg, uint8_t kb)
{
	void *data;
	vw_vulkan_error("map memory", vkMapMemory(vulkan->device,
		tx->mappable_memory, 0, tx->size, 0, &data));

	for (int y = 0; y < h; y++) {
		uint8_t *rowPtr = data;
		for (int x = 0; x < w; x++) {
			memcpy(rowPtr, &p[((y * w) + x) * 3], 3);
			if(ka == 0) {
				rowPtr[3] = 255; // Alpha of 1
			}else if(ka == 1) {
				if(rowPtr[0]==kr&&rowPtr[1]==kg&&rowPtr[2]==kb)
					rowPtr[3] = 0;
				else
					rowPtr[3] = 255;
			}else if(ka == 2) {
				rowPtr[3] = kr;
			}
			rowPtr += 4;
		}
		data += tx->pitch;
	}
	puts("ok then.");

	vkUnmapMemory(vulkan->device, tx->mappable_memory);

	if (!tx->staged) {
		// Use a linear tiled image for the texture, is supported
		tx->image = tx->mappable_image;
		tx->memory = tx->mappable_memory;
		set_image_layout(vulkan, tx->image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	} else {
		// Use optimal tiled image - create from linear tiled image
		VkMemoryRequirements mem_reqs;
		vkGetImageMemoryRequirements(vulkan->device, 0,
			&mem_reqs);

		// Prepare mappable image for blitting onto optimal image
		set_image_layout(vulkan, tx->mappable_image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);
		// Prepare optimal image for being blitted onto.
		set_image_layout(vulkan, tx->image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		VkImageCopy copy_region = {
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.srcOffset = { .x = 0, .y = 0, .z = 0 },
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.dstOffset = { .x = 0, .y = 0, .z = 0 },
			.extent = { .width = w, .height = h, .depth = 1 },
		};

		// Copy data from linear image to optimal image.
		vkCmdCopyImage(vulkan->command_buffer, tx->mappable_image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tx->image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

		// Change layout from DESTINATION_OPTIMAL to SHADER_READ_ONLY
		set_image_layout(vulkan, tx->image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}
	printf("well then\n");
//	execute_end_command_buffer(vulkan);
	printf("oh well then\n");
//	execute_queue_command_buffer(vulkan);
	printf("pe well then\n");
}

vw_texture_t vw_vulkan_texture(vw_t* vulkan, uint32_t w, uint32_t h,
	const uint8_t* p, uint8_t ka, uint8_t kr, uint8_t kg, uint8_t kb)
{
	vw_texture_t texture;
	VkFormatProperties formatProps;
	VkMemoryRequirements mem_reqs;

	// Use staging image if linear tiled image isn't supported 
	vkGetPhysicalDeviceFormatProperties(vulkan->gpu,
		VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	texture.staged = (!(formatProps.linearTilingFeatures &
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) ? 1 : 0;

	VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent = { .width = w, .height = h, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_LINEAR,
		.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
		.usage = texture.staged ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT
			: VK_IMAGE_USAGE_SAMPLED_BIT,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.flags = 0,
	};

	// Create linear tiled image
	vw_vulkan_error("create image", vkCreateImage(vulkan->device,
		&image_create_info, NULL, &texture.mappable_image));
	vkGetImageMemoryRequirements(vulkan->device, texture.mappable_image, &mem_reqs);

	VkMemoryAllocateInfo mem_alloc = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = memory_type_from_properties(vulkan,
			mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	};

	vw_vulkan_error("allocate memory", vkAllocateMemory(vulkan->device,
		&mem_alloc, NULL, &(texture.mappable_memory)));
	vw_vulkan_error("bind memory", vkBindImageMemory(vulkan->device,
		texture.mappable_image, texture.mappable_memory, 0));

	const VkImageSubresource subres = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0,
		.arrayLayer = 0,
	};

	VkSubresourceLayout layout;
	vkGetImageSubresourceLayout(vulkan->device, texture.mappable_image,
		&subres, &layout);

	texture.size = mem_reqs.size;
	texture.pitch = layout.rowPitch;

	const VkImageCreateInfo image_create_2info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UNORM,
		.extent = { .width = w, .height = h, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.flags = 0,
	};

	vw_vulkan_error("bind memory", vkCreateImage(vulkan->device,
		&image_create_2info, NULL, &texture.image));

	if (texture.staged) {
		const VkMemoryAllocateInfo mem_alloc = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = NULL,
			.allocationSize = mem_reqs.size,
			.memoryTypeIndex = memory_type_from_properties(vulkan,
				mem_reqs.memoryTypeBits, 0),
		};

		vw_vulkan_error("allocate memory", vkAllocateMemory(
			vulkan->device, &mem_alloc, NULL, &texture.memory));
		vw_vulkan_error("bind image memory", vkBindImageMemory(
			vulkan->device, texture.image, texture.memory, 0));
	}

	printf("oh ha\n");
	
	vw_vulkan_animate(vulkan, &texture, w, h, p, ka, kr, kg, kb);

	printf("oh ho\n");
	
	VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias = 0.0,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 0,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_NEVER,
		.minLod = 0.0,
		.maxLod = 0.0,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
	};

	vw_vulkan_error("create sampler", vkCreateSampler(vulkan->device,
		&samplerCreateInfo, NULL, &texture.sampler));

	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.image = VK_NULL_HANDLE,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.components.r = VK_COMPONENT_SWIZZLE_R,
		.components.g = VK_COMPONENT_SWIZZLE_G,
		.components.b = VK_COMPONENT_SWIZZLE_B,
		.components.a = VK_COMPONENT_SWIZZLE_A,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.image = texture.image,
	};
	puts("vkCreateImageView #3");
	vw_vulkan_error("create image view", vkCreateImageView(vulkan->device,
		&view_info, NULL, &texture.view));
	puts("vkCreateImageView #3");

	return texture;
}

void vw_vulkan_shader(vw_shader_t* shader, vw_t vulkan,
	void* vdata, uint32_t vsize, void* fdata, uint32_t fsize)
{
	// Vertex Shader
	VkShaderModuleCreateInfo vertexShaderCreationInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vsize,
		.pCode = (void *)vdata,
	};
	vw_vulkan_error("Failed to create vertex shader.", vkCreateShaderModule(
		vulkan.device,&vertexShaderCreationInfo,NULL,&shader->vertex));
	// Fragment Shader
	VkShaderModuleCreateInfo fragmentShaderCreationInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = fsize,
		.pCode = (void *)fdata,
	};
	vw_vulkan_error("Failed to create vertex shader.", vkCreateShaderModule(
		vulkan.device,&fragmentShaderCreationInfo,NULL,&shader->fragment));
}

void vw_vulkan_pipeline(vw_pipeline_t* pipeline, vw_t* vulkan, vw_shader_t* shaders,
	uint32_t ns/*, uint32_t ni, void* pixels, uint32_t* w, uint32_t* h*/)
{
	const VkDescriptorSetLayoutBinding layout_bindings[2] = {
		[0] = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		},
		[1] = {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1, // Texture Count
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		},
	};

	// vertex input configuration:
	VkVertexInputBindingDescription vertexBindingDescription = {
		.binding = 0,
		.stride = sizeof(float) * 4 * 2,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	VkVertexInputAttributeDescription vertexAttributeDescriptions[2] = {{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.offset = 0,
	}, {
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		.offset = 4 * sizeof(float),
	}};
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexBindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = vertexAttributeDescriptions,
	};
	// vertex topology config:
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};
	// viewport config:
	VkViewport viewport = {
		.x = 0, .y = 0,
		.width = vulkan->width, .height = vulkan->height,
		.minDepth = 0.f, .maxDepth = 1.f,
	};
	VkRect2D scissors = {
		.extent = { .width = vulkan->width, .height = vulkan->height },
	};
	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissors,
	};
	// rasterization config:
	VkPipelineRasterizationStateCreateInfo rasterizationState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
		.lineWidth = 1,
	};
	// sampling config:
	VkPipelineMultisampleStateCreateInfo multisampleState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};
	// depth/stencil config:
	VkStencilOpState noOPStencilState = {
		.failOp = VK_STENCIL_OP_KEEP,
		.passOp = VK_STENCIL_OP_KEEP,
		.depthFailOp = VK_STENCIL_OP_KEEP,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.compareMask = 0,
		.writeMask = 0,
		.reference = 0,
	};
	VkPipelineDepthStencilStateCreateInfo depthState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = noOPStencilState,
		.back = noOPStencilState,
		.minDepthBounds = 0,
		.maxDepthBounds = 0,
	};
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = 0xf, // RGBA
	};
	VkPipelineColorBlendStateCreateInfo colorBlendState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_CLEAR,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants[0] = 0.0,
		.blendConstants[1] = 0.0,
		.blendConstants[2] = 0.0,
		.blendConstants[3] = 0.0,
	};
	VkDynamicState dynamicState[2] = {
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicState,
	};

	for(int i = 0; i < ns; i++) {
		//
		const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = NULL,
			.bindingCount = 1 + shaders[i].textures,
			.pBindings = layout_bindings,
		};
		vw_vulkan_error("Failed to create descriptor set layout.",
			vkCreateDescriptorSetLayout(vulkan->device,
				&descriptor_layout, NULL,
				&pipeline[i].descsetlayout));

		// pipeline layout:
		VkPipelineLayoutCreateInfo layoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &pipeline[i].descsetlayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = NULL,
		};
		vw_vulkan_error("Failed to create pipeline layout.",
			vkCreatePipelineLayout(vulkan->device,
				&layoutCreateInfo, NULL,
				&pipeline[i].pipeline_layout));

		// setup shader stages:
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shaders[i].vertex,
			.pName = "main", // shader main function name
			.pSpecializationInfo = NULL,
		}, {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shaders[i].fragment,
			.pName = "main", // shader main function name
			.pSpecializationInfo = NULL,
		}};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = shaderStageCreateInfo,
			.pVertexInputState = &vertexInputStateCreateInfo,
			.pInputAssemblyState = &inputAssemblyStateCreateInfo,
			.pTessellationState = NULL,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &depthState,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicStateCreateInfo,
			.layout = pipeline[i].pipeline_layout,
			.renderPass = vulkan->render_pass,
			.subpass = 0,
			.basePipelineHandle = 0, // NULL TODO: ?
			.basePipelineIndex = 0,
		};
		vw_vulkan_error("Failed to create graphics pipeline.",
			vkCreateGraphicsPipelines(vulkan->device,
				VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL,
				&pipeline[i].pipeline));
	}

	vkDestroyShaderModule(vulkan->device, shaders[0].vertex, NULL);
	vkDestroyShaderModule(vulkan->device, shaders[0].fragment, NULL);
}

void vw_vulkan_resize(vw_t* vulkan) {
	vw_vulkan_swapchain(vulkan); // Link Swapchain to Vulkan Instance
	vw_vulkan_image_view(vulkan); // Link Image Views for each framebuffer
	vw_vulkan_depth_buffer(vulkan); // Link Depth Buffer to swapchain
	vw_vulkan_render_pass(vulkan); // Link Render Pass to swapchain
	vw_vulkan_framebuffers(vulkan); // Link Framebuffers to swapchain
}

void vw_vulkan_swapchain_delete(vw_t* vulkan) {
	// Free framebuffers & image view #1
	for (int i = 0; i < vulkan->image_count; i++) {
		vkDestroyFramebuffer(vulkan->device, vulkan->frame_buffers[i],
			NULL);
		vkDestroyImageView(vulkan->device,
			vulkan->present_image_views[i], NULL);
//		vkDestroyImage(vulkan->device, vulkan->present_images[i], NULL);
	}
	// Free render pass
	vkDestroyRenderPass(vulkan->device, vulkan->render_pass, NULL);
	// Free depth buffer
	vkDestroyImageView(vulkan->device, vulkan->depth_image_view, NULL);
	vkDestroyImage(vulkan->device, vulkan->depth_image, NULL);
	vkFreeMemory(vulkan->device, vulkan->depth_image_memory, NULL);
	// Free image view #2
//	vkDestroyFence(vulkan->device, vulkan->submit_fence, NULL);  // TODO: Mem Error
	// Free swapchain
	vkDestroySwapchainKHR(vulkan->device, vulkan->swapchain, NULL);
}

void vw_vulkan_draw_begin(vw_t* vulkan, float r, float g, float b) {
	VkSemaphoreCreateInfo semaphore_ci = {
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0
	};
	vkCreateSemaphore(vulkan->device, &semaphore_ci, NULL,
		&vulkan->presenting_complete_sem);
	vkCreateSemaphore(vulkan->device, &semaphore_ci, NULL,
		&vulkan->rendering_complete_sem);

	if (vkAcquireNextImageKHR(
		vulkan->device, vulkan->swapchain, UINT64_MAX,
		vulkan->presenting_complete_sem, VK_NULL_HANDLE,
		&vulkan->next_image_index) != VK_SUCCESS)
	{
		vulkan->do_draw = 0;
		return;
	};
	vulkan->do_draw = 1;

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(vulkan->command_buffer, &beginInfo);

	VkImageMemoryBarrier layoutTransitionBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = vulkan->present_images[ vulkan->next_image_index ],
	};

	VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(vulkan->command_buffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		0, 0, NULL, 0, NULL, 1, &layoutTransitionBarrier);

	// activate render pass:
	VkClearValue clearValue[2] = {
		[0] = { .color.float32 = { r, g, b, 1.0f } },
		[1] = { .depthStencil = (VkClearDepthStencilValue) { 1.0, 0 } },
	};
	VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = vulkan->render_pass,
		.framebuffer = vulkan->frame_buffers[vulkan->next_image_index],
		.renderArea = {
			.offset = { .x = 0, .y = 0 },
			.extent = {
				.width = vulkan->width,
				.height = vulkan->height
			},
		},
		.clearValueCount = 2,
		.pClearValues = clearValue,
	};
	vkCmdBeginRenderPass(vulkan->command_buffer, &renderPassBeginInfo,
		VK_SUBPASS_CONTENTS_INLINE);

	// take care of dynamic state:
	VkViewport viewport = { 0, 0, vulkan->width, vulkan->height, 0, 1 };
	vkCmdSetViewport(vulkan->command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {
		.offset = { 0, 0 }, .extent = { vulkan->width, vulkan->height },
	};
	vkCmdSetScissor(vulkan->command_buffer, 0, 1, &scissor);
}

void vw_cmd_draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
	vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vw_vulkan_draw_shape(vw_t* vulkan, vw_shape_t* shape, const float* v,
	vw_instance_t instance)
{
	vulkan->offset = 0;
// TODO: vkBeginCommandBuffer() is not called before this on occasion during resize.
	vkCmdBindVertexBuffers(vulkan->command_buffer, 0, 1,
		&shape->vertex_input_buffer, &vulkan->offset);
	// Bind pipeline.
	vkCmdBindPipeline(vulkan->command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, shape->pipeline->pipeline);
	vkCmdBindDescriptorSets(vulkan->command_buffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		shape->pipeline->pipeline_layout, 0, 1, &instance.desc_set, 0,
		NULL);
}

void vw_vulkan_draw_update(vw_t* vulkan) {
	if(vulkan->do_draw == 0) {
		vkDestroySemaphore(vulkan->device,
			vulkan->presenting_complete_sem, NULL);
		vkDestroySemaphore(vulkan->device,
			vulkan->rendering_complete_sem, NULL);
		return;
	}
	vkCmdEndRenderPass(vulkan->command_buffer);
	// change layout back to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	VkImageMemoryBarrier prePresentBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.image = vulkan->present_images[vulkan->next_image_index],
	};
	vkCmdPipelineBarrier(vulkan->command_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1,
		&prePresentBarrier);

	vkEndCommandBuffer(vulkan->command_buffer);
	// present:
	VkFence render_fence;
	VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	};
	vkCreateFence(vulkan->device, &fenceCreateInfo, NULL, &render_fence);
	VkPipelineStageFlags waitStageMash=VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vulkan->presenting_complete_sem,
		.pWaitDstStageMask = &waitStageMash,
		.commandBufferCount = 1,
		.pCommandBuffers = &vulkan->command_buffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vulkan->rendering_complete_sem,
	};
	vkQueueSubmit(vulkan->present_queue, 1, &submitInfo, render_fence);
	vkWaitForFences(vulkan->device, 1, &render_fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(vulkan->device, render_fence, NULL);
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vulkan->rendering_complete_sem,
		.swapchainCount = 1,
		.pSwapchains = &vulkan->swapchain,
		.pImageIndices = &vulkan->next_image_index,
		.pResults = NULL,
	};
	vkQueuePresentKHR(vulkan->present_queue, &present_info);
	vkDestroySemaphore(vulkan->device, vulkan->presenting_complete_sem, NULL);
	vkDestroySemaphore(vulkan->device, vulkan->rendering_complete_sem, NULL);
	vkDeviceWaitIdle(vulkan->device);
}
