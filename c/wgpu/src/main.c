// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

// this example is similar in scope to the wgpu 'triangle' example

#include <root.h>

#include <aquabsd/alps/win.h>
#include <aquabsd/black/wgpu.h>
#include <core/fs.h>

#include <core/log.h>
#define LOG_COMPONENT "aqua.examples.wgpu"

#define X_RES 500
#define Y_RES 400

#define SHADER_NAME "main"
#define CLEAR_COLOUR { 0, 0, 0, 0 }

typedef struct {
	win_t win;

	WGPUInstance instance;
	WGPUSurface surface;
	WGPUAdapter adapter;
	WGPUDevice device;
	WGPURenderPipeline render_pipeline;
	WGPUTextureFormat surface_preferred_format;
	WGPUSwapChain swapchain;
	WGPUQueue queue;
} state_t;

static void texture_view_err_cb(WGPUErrorType type, char const* msg, void* data) {
	(void) data;

	if (type == WGPUErrorType_NoError) {
		return;
	}

	LOG_WARN("%s: type = %#.8x, message = \"%s\"", __func__, type, msg);
}

static int draw(uint64_t _ __attribute__((unused)), uint64_t data) {
	state_t* const state = (void*) data;

	// get texture view for next frame

	wgpuDevicePushErrorScope(state->device, WGPUErrorFilter_Validation);
	WGPUTextureView const texture_view = wgpuSwapChainGetCurrentTextureView(state->swapchain);
	wgpuDevicePopErrorScope(state->device, texture_view_err_cb, state);

	if (!texture_view) {
		LOG_ERROR("Failed to get current texture view");
		goto err_texture_view;
	}

	// create command encoder

	WGPUCommandEncoderDescriptor const command_encoder_descr = { .label = "command_encoder" };
	WGPUCommandEncoder const command_encoder = wgpuDeviceCreateCommandEncoder(state->device, &command_encoder_descr);

	if (!command_encoder) {
		LOG_ERROR("Failed to create command encoder");
		goto err_command_encoder;
	}

	// create render pass encoder

	WGPURenderPassColorAttachment const colour_attachments[] = {
		{
			.view = texture_view,
			.loadOp = WGPULoadOp_Clear,
			.storeOp = WGPUStoreOp_Store,
			.clearValue = (WGPUColor const) CLEAR_COLOUR,
		},
	};

	WGPURenderPassDescriptor const render_pass_descr = {
		.label = "render_pass_encoder",
		.colorAttachmentCount = sizeof colour_attachments / sizeof *colour_attachments,
		.colorAttachments = colour_attachments,
	};

	WGPURenderPassEncoder const render_pass_encoder = wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_descr);

	if (!render_pass_encoder) {
		LOG_ERROR("Failed to create render pass encoder");
		goto err_render_pass_encoder;
	}

	wgpuRenderPassEncoderSetPipeline(render_pass_encoder, state->render_pipeline);
	wgpuRenderPassEncoderDraw(render_pass_encoder, 3, 1, 0, 0);
	wgpuRenderPassEncoderEnd(render_pass_encoder);

	// create final command buffer

	WGPUCommandBufferDescriptor const command_buffer_descr = { .label = "command_buffer" };
	WGPUCommandBuffer const command_buffer = wgpuCommandEncoderFinish(command_encoder, &command_buffer_descr);

	if (!command_buffer) {
		LOG_ERROR("Failed to create command buffer");
		goto err_command_buffer;
	}

	// submit command buffer to queue

	WGPUCommandBuffer const command_buffers[] = { command_buffer };
	wgpuQueueSubmit(state->queue, sizeof command_buffers / sizeof *command_buffers, command_buffers);

	// present swapchain

	wgpuSwapChainPresent(state->swapchain);

	// cleanup

	wgpuCommandBufferRelease(command_buffer);

err_command_buffer:

	wgpuRenderPassEncoderRelease(render_pass_encoder);

err_render_pass_encoder:

	wgpuCommandEncoderRelease(command_encoder);

err_command_encoder:

	wgpuTextureViewRelease(texture_view);

err_texture_view:

	return 0;
}

static int resize(uint64_t _ __attribute__((unused)), uint64_t data) {
	state_t* const state = (void*) data;

	uint64_t const x_res = win_get_x_res(&state->win);
	uint64_t const y_res = win_get_y_res(&state->win);

	LOG_INFO("Window resized to resolution %dx%d", x_res, y_res);

	// free previous swapchain if there is one

	if (state->swapchain) {
		wgpuSwapChainRelease(state->swapchain);
	}

	// XXX this is very glitchy-looking	on Xorg
	//     the solution apparently is to reuse the old swapchain by setting VkSwapchainCreateInfoKHR.oldSwapchain
	//     unfortunately, WebGPU doesn't expose this (even through WGPUSwapChainDescriptorExtras)...
	//     ...so we'll have to hope things are smoother on Wayland 😉

	LOG_VERBOSE("Recreating swapchain with new resolution");

	WGPUSwapChainDescriptor const swapchain_descr = {
		.usage = WGPUTextureUsage_RenderAttachment,
		.format = state->surface_preferred_format,
		.presentMode = WGPUPresentMode_Immediate,
		.width = x_res,
		.height = y_res,
	};

	state->swapchain = wgpuDeviceCreateSwapChain(state->device, state->surface, &swapchain_descr);

	if (!state->swapchain) {
		LOG_ERROR("Failed to recreate swapchain");
	}

	return 0;
}

static void request_adapter_cb(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* msg, void* data) {
	state_t* const state = data;

	if (status == WGPURequestAdapterStatus_Success) {
		state->adapter = adapter;
	}

	else {
		LOG_ERROR("%s: status = %#.8x, message = \"%s\"", __func__, status, msg);
	}
}

static void request_device_cb(WGPURequestDeviceStatus status, WGPUDevice device, char const* msg, void* data) {
	state_t* const state = data;

	if (status == WGPURequestAdapterStatus_Success) {
		state->device = device;
	}

	else {
		LOG_ERROR("%s: status = %#.8x, message = \"%s\"", __func__, status, msg);
	}
}

int main(void) {
	int rv = EXIT_FAILURE;
	state_t state = {};

	LOG_VERBOSE("Initializing aquabsd.black.wgpu");

	// we must do this before we issue and WebGPU calls

	if (wgpu_init() < 0) {
		LOG_FATAL("Failed to initialize aquabsd.black.wgpu");
		goto err_init;
	}

	LOG_VERBOSE("Creating WebGPU instance");

	WGPUInstanceDescriptor const create_instance_descr = {};
	state.instance = wgpuCreateInstance(&create_instance_descr);

	if (!state.instance) {
		LOG_FATAL("Failed to create WebGPU instance");
		goto err_instance;
	}

	LOG_VERBOSE("Creating window");

	err_t const err = win_create(&state.win, X_RES, Y_RES);

	if (err < 0) {
		LOG_FATAL("Failed to create window: %s", err_str(err));
		goto err_win_create;
	}

	win_set_caption(&state.win, "WebGPU Example");
	win_register_cb(&state.win, WIN_CB_DRAW, draw, &state);
	win_register_cb(&state.win, WIN_CB_RESIZE, resize, &state);

	LOG_INFO("Window created of resolution %dx%d", X_RES, Y_RES);

	LOG_VERBOSE("Creating surface");
	state.surface = wgpu_surface_from_win(state.instance, &state.win);

	if (!state.surface) {
		LOG_FATAL("Failed to create WebGPU surface from window");
		goto err_surface_from_win;
	}

	LOG_VERBOSE("Requesting WebGPU adapter");

	WGPURequestAdapterOptions const request_adapter_options = { .compatibleSurface = state.surface };
	wgpuInstanceRequestAdapter(state.instance, &request_adapter_options, request_adapter_cb, &state);

	if (!state.adapter) {
		LOG_FATAL("Failed to get WebGPU adapter");
		goto err_request_adapter;
	}

	WGPUAdapterProperties adapter_props;
	wgpuAdapterGetProperties(state.adapter, &adapter_props);

	LOG_INFO("Got adapter: %s (%s)", adapter_props.name, adapter_props.driverDescription);
	LOG_VERBOSE("Requesting WebGPU device");

	wgpuAdapterRequestDevice(state.adapter, NULL, request_device_cb, &state);

	if (!state.device) {
		LOG_FATAL("Failed to get WebGPU device");
		goto err_request_device;
	}

	LOG_VERBOSE("Loading shader module source ('" SHADER_NAME "')");
	fs_descr_t const shader_descr = fs_open("res", SHADER_NAME ".wgsl", FS_FLAGS_READ);

	if (shader_descr == FS_OPEN_ERR) {
		LOG_FATAL("Failed to open shader source");
		goto err_fs_open;
	}

	size_t const shader_size = fs_size(shader_descr);
	char* const shader_src = fs_mmap(shader_descr);

	if (shader_src == NULL) {
		LOG_FATAL("Failed to read shader source");
		goto err_fs_mmap;
	}

	LOG_INFO("Shader is %zu bytes in length", shader_size);
	LOG_VERBOSE("Creating shader module");

	WGPUShaderModuleWGSLDescriptor const wgsl_descr = {
		.chain = (WGPUChainedStruct const) {
			.sType = WGPUSType_ShaderModuleWGSLDescriptor,
		},
		.code = shader_src,
	};

	WGPUShaderModuleDescriptor const create_shader_descr = {
		.label = SHADER_NAME,
		.nextInChain = (WGPUChainedStruct const*) &wgsl_descr,
	};

	WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(state.device, &create_shader_descr);

	if (!shader_module) {
		LOG_FATAL("Failed to create shader module");
		goto err_shader_module;
	}

	LOG_VERBOSE("Creating pipeline layout");

	WGPUPipelineLayoutDescriptor const pipeline_layout_descr = { .label = "pipeline_layout" };
	WGPUPipelineLayout const pipeline_layout = wgpuDeviceCreatePipelineLayout(state.device, &pipeline_layout_descr);

	if (!pipeline_layout) {
		LOG_FATAL("Failed to create pipeline layout");
		goto err_pipeline_layout;
	}

	LOG_VERBOSE("Getting preferred surface format");

	state.surface_preferred_format = wgpuSurfaceGetPreferredFormat(state.surface, state.adapter);

	if (state.surface_preferred_format == WGPUTextureFormat_Undefined) {
		LOG_FATAL("Found no preferred surface format");
		goto err_preferred_format;
	}

	LOG_INFO("Preferred surface format is 0x%x", state.surface_preferred_format);
	LOG_VERBOSE("Creating render pipeline");

	WGPUVertexState const vert_state = {
		.module = shader_module,
		.entryPoint = "vert_main",
	};

	WGPUColorTargetState const target_states[] = {
		{
			.format = state.surface_preferred_format,
			.writeMask = WGPUColorWriteMask_All,
		}
	};

	WGPUFragmentState const frag_state = {
		.module = shader_module,
		.entryPoint = "frag_main",
		.targetCount = sizeof target_states / sizeof *target_states,
		.targets = target_states,
	};

	WGPUPrimitiveState const primitive_state = {
		.topology = WGPUPrimitiveTopology_TriangleList,
	};

	WGPUMultisampleState const ms_state = {
		.count = 1,
		.mask = 0xFFFFFFFF,
	};

	WGPURenderPipelineDescriptor const render_pipeline_descr = {
		.label = "render_pipeline",
		.layout = pipeline_layout,
		.vertex = vert_state,
		.fragment = &frag_state,
		.primitive = primitive_state,
		.multisample = ms_state,
	};

	state.render_pipeline = wgpuDeviceCreateRenderPipeline(state.device, &render_pipeline_descr);

	if (!state.render_pipeline) {
		LOG_FATAL("Failed to create render pipeline");
		goto err_render_pipeline;
	}

	LOG_VERBOSE("Get device queue");
	state.queue = wgpuDeviceGetQueue(state.device);

	if (!state.queue) {
		LOG_FATAL("Failed to get device queue");
		goto err_queue;
	}

	LOG_VERBOSE("Create swapchain");

	WGPUSwapChainDescriptor const swapchain_descr = {
		.usage = WGPUTextureUsage_RenderAttachment,
		.format = state.surface_preferred_format,
		.presentMode = WGPUPresentMode_Immediate,
		.width = X_RES,
		.height = Y_RES,
	};

	state.swapchain = wgpuDeviceCreateSwapChain(state.device, state.surface, &swapchain_descr);

	if (!state.swapchain) {
		LOG_FATAL("Failed to create swapchain");
		goto err_swapchain;
	}

	LOG_VERBOSE("Starting main draw loop");

	if (win_loop(&state.win) == ERR_INTERNAL) {
		goto err_loop;
	}

	LOG_SUCCESS("Everything ran successfully! 🎉");
	rv = EXIT_SUCCESS;

	// cleanup

err_loop:

	wgpuQueueRelease(state.queue);

err_queue:

	wgpuSwapChainRelease(state.swapchain);

err_swapchain:

	wgpuRenderPipelineRelease(state.render_pipeline);

err_render_pipeline:
err_preferred_format:

	wgpuPipelineLayoutRelease(pipeline_layout);

err_pipeline_layout:

	wgpuShaderModuleRelease(shader_module);

err_shader_module:
err_fs_mmap:

	fs_close(shader_descr);

err_fs_open:

	wgpuDeviceRelease(state.device);

err_request_device:

	wgpuAdapterRelease(state.adapter);

err_request_adapter:

	wgpuSurfaceRelease(state.surface);

err_surface_from_win:

	win_destroy(&state.win);

err_win_create:

	wgpuInstanceRelease(state.instance);

err_instance:
err_init:

	return rv;
}
