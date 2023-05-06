/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2023  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/Pcsx2Defs.h"
#include "common/RedtapeWindows.h"
#include "GS/Renderers/DX12/D3D12DescriptorHeapManager.h"
#include "GS/Renderers/DX12/D3D12StreamBuffer.h"

#include <array>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <memory>
#include <vector>
#include <wil/com.h>

struct IDXGIAdapter;
struct IDXGIFactory;
namespace D3D12MA
{
	class Allocator;
	class Allocation;
} // namespace D3D12MA

class D3D12Context
{
public:
	template <typename T>
	using ComPtr = wil::com_ptr_nothrow<T>;

	enum : u32
	{
		/// Number of command lists. One is being built while the other(s) are executed.
		NUM_COMMAND_LISTS = 3,

		/// Textures that don't fit into this buffer will be uploaded with a staging buffer.
		TEXTURE_UPLOAD_BUFFER_SIZE = 64 * 1024 * 1024,

		/// Maximum number of samples in a single allocation group.
		SAMPLER_GROUP_SIZE = 2,

		/// Start/End timestamp queries.
		NUM_TIMESTAMP_QUERIES_PER_CMDLIST = 2,
	};

	~D3D12Context();

	/// Creates new device and context.
	static bool Create(IDXGIFactory5* dxgi_factory, IDXGIAdapter1* adapter, bool enable_debug_layer);

	/// Destroys active context.
	static void Destroy();

	__fi IDXGIAdapter* GetAdapter() const { return m_adapter.get(); }
	__fi ID3D12Device* GetDevice() const { return m_device.get(); }
	__fi ID3D12CommandQueue* GetCommandQueue() const { return m_command_queue.get(); }
	__fi D3D12MA::Allocator* GetAllocator() const { return m_allocator.get(); }

	/// Returns the PCI vendor ID of the device, if known.
	u32 GetAdapterVendorID() const;

	/// Returns the current command list, commands can be recorded directly.
	ID3D12GraphicsCommandList4* GetCommandList() const
	{
		return m_command_lists[m_current_command_list].command_lists[1].get();
	}

	/// Returns the init command list for uploading.
	ID3D12GraphicsCommandList4* GetInitCommandList();

	/// Returns the per-frame SRV/CBV/UAV allocator.
	D3D12DescriptorAllocator& GetDescriptorAllocator()
	{
		return m_command_lists[m_current_command_list].descriptor_allocator;
	}

	/// Returns the per-frame sampler allocator.
	D3D12GroupedSamplerAllocator<SAMPLER_GROUP_SIZE>& GetSamplerAllocator()
	{
		return m_command_lists[m_current_command_list].sampler_allocator;
	}

	/// Invalidates GPU-side sampler caches for all command lists. Call after you've freed samplers,
	/// and are going to re-use the handles from GetSamplerHeapManager().
	void InvalidateSamplerGroups();

	// Descriptor manager access.
	D3D12DescriptorHeapManager& GetDescriptorHeapManager() { return m_descriptor_heap_manager; }
	D3D12DescriptorHeapManager& GetRTVHeapManager() { return m_rtv_heap_manager; }
	D3D12DescriptorHeapManager& GetDSVHeapManager() { return m_dsv_heap_manager; }
	D3D12DescriptorHeapManager& GetSamplerHeapManager() { return m_sampler_heap_manager; }
	const D3D12DescriptorHandle& GetNullSRVDescriptor() const { return m_null_srv_descriptor; }
	D3D12StreamBuffer& GetTextureStreamBuffer() { return m_texture_stream_buffer; }

	// Root signature access.
	ComPtr<ID3DBlob> SerializeRootSignature(const D3D12_ROOT_SIGNATURE_DESC* desc);
	ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC* desc);

	/// Fence value for current command list.
	u64 GetCurrentFenceValue() const { return m_current_fence_value; }

	/// Last "completed" fence.
	u64 GetCompletedFenceValue() const { return m_completed_fence_value; }

	/// Feature level to use when compiling shaders.
	D3D_FEATURE_LEVEL GetFeatureLevel() const { return m_feature_level; }

	/// Test for support for the specified texture format.
	bool SupportsTextureFormat(DXGI_FORMAT format);

	enum class WaitType
	{
		None, ///< Don't wait (async)
		Sleep, ///< Wait normally
		Spin, ///< Wait by spinning
	};

	/// Executes the current command list.
	bool ExecuteCommandList(WaitType wait_for_completion);

	/// Waits for a specific fence.
	void WaitForFence(u64 fence, bool spin);

	/// Waits for any in-flight command buffers to complete.
	void WaitForGPUIdle();

	/// Defers destruction of a D3D resource (associates it with the current list).
	void DeferObjectDestruction(ID3D12DeviceChild* resource);

	/// Defers destruction of a D3D resource (associates it with the current list).
	void DeferResourceDestruction(D3D12MA::Allocation* allocation, ID3D12Resource* resource);

	/// Defers destruction of a descriptor handle (associates it with the current list).
	void DeferDescriptorDestruction(D3D12DescriptorHeapManager& manager, u32 index);
	void DeferDescriptorDestruction(D3D12DescriptorHeapManager& manager, D3D12DescriptorHandle* handle);

	float GetAndResetAccumulatedGPUTime();
	void SetEnableGPUTiming(bool enabled);

	// Allocates a temporary CPU staging buffer, fires the callback with it to populate, then copies to a GPU buffer.
	bool AllocatePreinitializedGPUBuffer(u32 size, ID3D12Resource** gpu_buffer, D3D12MA::Allocation** gpu_allocation,
		const std::function<void(void*)>& fill_callback);

private:
	struct CommandListResources
	{
		std::array<ComPtr<ID3D12CommandAllocator>, 2> command_allocators;
		std::array<ComPtr<ID3D12GraphicsCommandList4>, 2> command_lists;
		D3D12DescriptorAllocator descriptor_allocator;
		D3D12GroupedSamplerAllocator<SAMPLER_GROUP_SIZE> sampler_allocator;
		std::vector<std::pair<D3D12MA::Allocation*, ID3D12DeviceChild*>> pending_resources;
		std::vector<std::pair<D3D12DescriptorHeapManager&, u32>> pending_descriptors;
		u64 ready_fence_value = 0;
		bool init_command_list_used = false;
		bool has_timestamp_query = false;
	};

	D3D12Context();

	bool CreateDevice(IDXGIFactory5* dxgi_factory, IDXGIAdapter1* adapter, bool enable_debug_layer);
	bool CreateCommandQueue();
	bool CreateAllocator();
	bool CreateFence();
	bool CreateDescriptorHeaps();
	bool CreateCommandLists();
	bool CreateTextureStreamBuffer();
	bool CreateTimestampQuery();
	void MoveToNextCommandList();
	void DestroyPendingResources(CommandListResources& cmdlist);
	void DestroyResources();

	ComPtr<IDXGIAdapter> m_adapter;
	ComPtr<ID3D12Debug> m_debug_interface;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_command_queue;
	ComPtr<D3D12MA::Allocator> m_allocator;

	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fence_event = {};
	u32 m_current_fence_value = 0;
	u64 m_completed_fence_value = 0;

	std::array<CommandListResources, NUM_COMMAND_LISTS> m_command_lists;
	u32 m_current_command_list = NUM_COMMAND_LISTS - 1;

	ComPtr<ID3D12QueryHeap> m_timestamp_query_heap;
	ComPtr<ID3D12Resource> m_timestamp_query_buffer;
	ComPtr<D3D12MA::Allocation> m_timestamp_query_allocation;
	double m_timestamp_frequency = 0.0;
	float m_accumulated_gpu_time = 0.0f;
	bool m_gpu_timing_enabled = false;

	D3D12DescriptorHeapManager m_descriptor_heap_manager;
	D3D12DescriptorHeapManager m_rtv_heap_manager;
	D3D12DescriptorHeapManager m_dsv_heap_manager;
	D3D12DescriptorHeapManager m_sampler_heap_manager;
	D3D12DescriptorHandle m_null_srv_descriptor;
	D3D12StreamBuffer m_texture_stream_buffer;

	D3D_FEATURE_LEVEL m_feature_level = D3D_FEATURE_LEVEL_11_0;
};

extern std::unique_ptr<D3D12Context> g_d3d12_context;

namespace D3D12
{
#ifdef _DEBUG

	void SetObjectName(ID3D12Object* object, const char* name);
	void SetObjectNameFormatted(ID3D12Object* object, const char* format, ...);

#else

	static inline void SetObjectName(ID3D12Object* object, const char* name)
	{
	}
	static inline void SetObjectNameFormatted(ID3D12Object* object, const char* format, ...)
	{
	}

#endif
} // namespace D3D12
