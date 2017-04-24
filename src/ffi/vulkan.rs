/**
 * adi_screen - Aldaron's Device Interface - Screen - "ffi/vulkan.rs"
 * Copyright 2017 (c) Jeron Lau - Licensed under the MIT LICENSE
**/

use std::fmt;
use std::{u64,usize};
// use std::ptr::null_mut;

// use screen::vw::Vw;

type VkDeviceMemory = u64;
// type VkDescriptorSet = u64;
// type VkDescriptorSetLayout = u64;
// type VkDescriptorPool = u64;

type VkDevice = usize;
type VkCommandBuffer = usize;

// type VkVoid = u8; // Arbitrary Type
// type VkStructureType = u32; // Size of enum is 4 bytes
// type VkFlags = u32;

// const MAX_ELEMENTS : usize = usize::MAX;
const VK_WHOLE_SIZE : u64 = !0; // Bitwise complement of 0

#[repr(C)]
#[allow(dead_code)] // Never used because value set by vulkan.
enum VkResult {
	Success = 0,
	NotReady = 1,
	Timeout = 2,
	EventSet = 3,
	EventReset = 4,
	Incomplete = 5,
	OutOfHostMemory = -1,
	OutOfDeviceMemory = -2,
	InitFailed = -3,
	DeviceLost = -4,
	MemoryMapFailed = -5,
	LayerNotPresent = -6,
	ExtNotPresent = -7,
	FeatureNotPresent = -8,
	IncompatDriver = -9,
	TooManyObjects = -10,
	BadFormat = -11,
	FragmentedPool = -12,
	Other = -1024,
}

impl fmt::Display for VkResult {
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
		match *self {

		VkResult::Success => write!(f, "Success"),
		VkResult::NotReady => write!(f, "Not Ready"),
		VkResult::Timeout => write!(f, "Timeout"),
		VkResult::EventSet => write!(f, "Event Set"),
		VkResult::EventReset => write!(f, "Event Reset"),
		VkResult::Incomplete => write!(f, "Incomplete"),
		VkResult::OutOfHostMemory => write!(f, "Out Of Host Memory"),
		VkResult::OutOfDeviceMemory => write!(f, "Out Of GPU Memory"),
		VkResult::InitFailed => write!(f, "Init Failed"),
		VkResult::DeviceLost => write!(f, "Device Lost"),
		VkResult::MemoryMapFailed => write!(f, "Memory Map Failed"),
		VkResult::LayerNotPresent => write!(f, "Layer Not Present"),
		VkResult::ExtNotPresent => write!(f, "Extension Not Present"),
		VkResult::FeatureNotPresent => write!(f, "Feature Not Present"),
		VkResult::IncompatDriver => write!(f, "Incompatible Driver"),
		VkResult::TooManyObjects => write!(f, "Too Many Objects"),
		VkResult::BadFormat => write!(f, "Format Not Supported"),
		VkResult::FragmentedPool => write!(f, "Fragmented Pool"),
		_ => write!(f, "Unknown Error"),

		}
	}
}

/*#[repr(C)]
struct VkDescriptorSetAllocateInfo {
	s_type: VkStructureType,
	p_next: *const VkVoid,
	descriptor_pool: VkDescriptorPool,
	descriptor_set_count: u32,
	p_set_layouts: *const VkDescriptorSetLayout,
}*/

extern {
	fn test_map(vulkan: VkDevice, vertex_buffer_memory: VkDeviceMemory, c: u64) -> *mut f32;

//	fn vkAllocateDescriptorSets(device: VkDevice,
//		pAllocateInfo: *const VkDescriptorSetAllocateInfo,
//		pDescriptorSets: *mut VkDescriptorSet) -> VkResult;
	fn vw_cmd_draw(commandBuffer: VkCommandBuffer, vertexCount: u32,
		instanceCount: u32, firstVertex: u32, firstInstance: u32) -> ();
//	fn vkMapMemory(device: VkDevice, memory: VkDeviceMemory,
//		offset: u64, size: u64, flags: VkFlags,
//		ppData: *mut usize) -> VkResult;
	fn vkUnmapMemory(device: VkDevice, memory: VkDeviceMemory) -> ();
}

/* fn check_error(name: &str, error: VkResult) {
	match error {
		VkResult::Success => {},
		_ => panic!("{} Failed {}", name, error),
	}
} */

pub fn copy_memory(vk_device: VkDevice, vk_memory: VkDeviceMemory, data: &[f32]) {
//	let len : usize = data.length();
//	let mut mapped : usize = 0;// = null_mut();
//	println!("device {0} memory {1} data {2}", vk_device, vk_memory, data[0]);
//	println!("Mapped {}", mapped);
	
//	panic!("ok");
	// TODO: Figure out why test_map works and not vkMapMemory ffi?
	let mapped = unsafe { test_map(vk_device, vk_memory, VK_WHOLE_SIZE) };
/*	unsafe {
		check_error("Failed to map buffer memory.", vkMapMemory(
			vk_device, vk_memory, 0, VK_WHOLE_SIZE, 0, &mut mapped));
*///		println!("Mapped {}", mapped);
/*	}*/
//	panic!("its ok");
//	let mapped = mapped as *mut _ as *mut f32;
	if mapped.is_null() {
		panic!("Couldn't Map Buffer Memory?  Unknown cause.");
	}
//	println!("Mapped {}", mapped as *mut _ as usize);
	for i in 0..data.len() {
		unsafe { *(mapped.offset(i as isize)) = data[i]; }
	}
//	println!("done.");
	unsafe {
		vkUnmapMemory(vk_device, vk_memory);
	}
}

pub fn cmd_draw(cmd_buffer: VkCommandBuffer, vertex_count: u32) {
	unsafe {
		vw_cmd_draw(cmd_buffer, vertex_count, 1, 0, 0);
	}
}

/*pub fn allocate_descriptor_sets(device: VkDevice, shape: &Shape, vw: &Vw) -> VkDescriptorSet {
	let mut desc_set : VkDescriptorSet = 0;
	let allocate_info = VkDescriptorSetAllocateInfo {
		s_type: 34,
		p_next: null(),
		descriptor_pool: vw.desc_pool,
		descriptor_set_count: 1,
		p_set_layouts: unsafe { &shape.pipeline.descsetlayout }
	};
	println!("hah");
	check_error("Failed to allocate descriptor sets.", unsafe {
		vkAllocateDescriptorSets(device, &allocate_info, &mut desc_set)
	} );
	println!("ha");
	desc_set
}*/
