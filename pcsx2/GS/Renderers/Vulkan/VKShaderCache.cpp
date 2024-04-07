// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "GS/GS.h"
#include "GS/Renderers/Vulkan/GSDeviceVK.h"
#include "GS/Renderers/Vulkan/VKBuilders.h"
#include "GS/Renderers/Vulkan/VKShaderCache.h"

#include "Config.h"
#include "ShaderCacheVersion.h"

#include "common/Assertions.h"
#include "common/Console.h"
#include "common/FileSystem.h"
#include "common/MD5Digest.h"
#include "common/Path.h"

#include "fmt/format.h"
#include "shaderc/shaderc.hpp"

#include <cstring>
#include <fstream>
#include <memory>

// TODO: store the driver version and stuff in the shader header

std::unique_ptr<VKShaderCache> g_vulkan_shader_cache;

static std::unique_ptr<shaderc::Compiler> s_shaderc_compiler;
static u32 s_next_bad_shader_id = 0;

namespace
{
#pragma pack(push, 4)
	struct VK_PIPELINE_CACHE_HEADER
	{
		u32 header_length;
		u32 header_version;
		u32 vendor_id;
		u32 device_id;
		u8 uuid[VK_UUID_SIZE];
	};

	struct CacheIndexEntry
	{
		u64 source_hash_low;
		u64 source_hash_high;
		u32 source_length;
		u32 shader_type;
		u32 file_offset;
		u32 blob_size;
	};
#pragma pack(pop)
} // namespace

static bool ValidatePipelineCacheHeader(const VK_PIPELINE_CACHE_HEADER& header)
{
	if (header.header_length < sizeof(VK_PIPELINE_CACHE_HEADER))
	{
		Console.Error("Pipeline cache failed validation: Invalid header length");
		return false;
	}

	if (header.header_version != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
	{
		Console.Error("Pipeline cache failed validation: Invalid header version");
		return false;
	}

	if (header.vendor_id != GSDeviceVK::GetInstance()->GetDeviceProperties().vendorID)
	{
		Console.Error("Pipeline cache failed validation: Incorrect vendor ID (file: 0x%X, device: 0x%X)",
			header.vendor_id, GSDeviceVK::GetInstance()->GetDeviceProperties().vendorID);
		return false;
	}

	if (header.device_id != GSDeviceVK::GetInstance()->GetDeviceProperties().deviceID)
	{
		Console.Error("Pipeline cache failed validation: Incorrect device ID (file: 0x%X, device: 0x%X)",
			header.device_id, GSDeviceVK::GetInstance()->GetDeviceProperties().deviceID);
		return false;
	}

	if (std::memcmp(header.uuid, GSDeviceVK::GetInstance()->GetDeviceProperties().pipelineCacheUUID, VK_UUID_SIZE) != 0)
	{
		Console.Error("Pipeline cache failed validation: Incorrect UUID");
		return false;
	}

	return true;
}

static void FillPipelineCacheHeader(VK_PIPELINE_CACHE_HEADER* header)
{
	header->header_length = sizeof(VK_PIPELINE_CACHE_HEADER);
	header->header_version = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
	header->vendor_id = GSDeviceVK::GetInstance()->GetDeviceProperties().vendorID;
	header->device_id = GSDeviceVK::GetInstance()->GetDeviceProperties().deviceID;
	std::memcpy(header->uuid, GSDeviceVK::GetInstance()->GetDeviceProperties().pipelineCacheUUID, VK_UUID_SIZE);
}

std::optional<VKShaderCache::SPIRVCodeVector> VKShaderCache::CompileShaderToSPV(u32 stage, std::string_view source, bool debug)
{
	// TODO: NOT thread safe, yet.
	if (!s_shaderc_compiler)
		s_shaderc_compiler = std::make_unique<shaderc::Compiler>();

	shaderc::CompileOptions options;
	options.SetSourceLanguage(shaderc_source_language_glsl);
	options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);

	if (debug)
	{
		options.SetOptimizationLevel(shaderc_optimization_level_zero);
		options.SetGenerateDebugInfo();
	}
	else
	{
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
	}

	const shaderc::SpvCompilationResult result = s_shaderc_compiler->CompileGlslToSpv(
		source.data(), source.length(), static_cast<shaderc_shader_kind>(stage), "source", "main", options);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		const std::string msg = result.GetErrorMessage();
		const std::string filename =
			Path::Combine(EmuFolders::Logs, fmt::format("pcsx2_bad_shader_{}.txt", ++s_next_bad_shader_id));
		Console.ErrorFmt("CompileShaderToSPV(): {}, writing to {}", msg, filename.c_str());

		std::ofstream ofs(filename, std::ofstream::out | std::ofstream::binary);
		if (ofs.is_open())
		{
			ofs << source;
			ofs << "\n";

			ofs << msg << std::endl;
			ofs.close();
		}

		return std::nullopt;
	}
	else if (result.GetNumWarnings() > 0)
	{
		Console.WarningFmt("CompileShaderToSPV(): Shader compiled with warnings:\n{}", result.GetErrorMessage());
	}

	return SPIRVCodeVector(result.cbegin(), result.cend());
}

VKShaderCache::VKShaderCache() = default;

VKShaderCache::~VKShaderCache()
{
	CloseShaderCache();
	FlushPipelineCache();
	ClosePipelineCache();
}

bool VKShaderCache::CacheIndexKey::operator==(const CacheIndexKey& key) const
{
	return (source_hash_low == key.source_hash_low && source_hash_high == key.source_hash_high &&
			source_length == key.source_length && shader_type == key.shader_type);
}

bool VKShaderCache::CacheIndexKey::operator!=(const CacheIndexKey& key) const
{
	return (source_hash_low != key.source_hash_low || source_hash_high != key.source_hash_high ||
			source_length != key.source_length || shader_type != key.shader_type);
}

void VKShaderCache::Create()
{
	pxAssert(!g_vulkan_shader_cache);
	g_vulkan_shader_cache.reset(new VKShaderCache());
	g_vulkan_shader_cache->Open();
}

void VKShaderCache::Destroy()
{
	g_vulkan_shader_cache.reset();
}

void VKShaderCache::Open()
{
	if (!GSConfig.DisableShaderCache)
	{
		m_pipeline_cache_filename = GetPipelineCacheBaseFileName(GSConfig.UseDebugDevice);

		const std::string base_filename = GetShaderCacheBaseFileName(GSConfig.UseDebugDevice);
		const std::string index_filename = base_filename + ".idx";
		const std::string blob_filename = base_filename + ".bin";

		if (!ReadExistingShaderCache(index_filename, blob_filename))
			CreateNewShaderCache(index_filename, blob_filename);

		if (!ReadExistingPipelineCache())
			CreateNewPipelineCache();
	}
	else
	{
		CreateNewPipelineCache();
	}
}

VkPipelineCache VKShaderCache::GetPipelineCache(bool set_dirty /*= true*/)
{
	if (m_pipeline_cache == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	m_pipeline_cache_dirty |= set_dirty;
	return m_pipeline_cache;
}

bool VKShaderCache::CreateNewShaderCache(const std::string& index_filename, const std::string& blob_filename)
{
	if (FileSystem::FileExists(index_filename.c_str()))
	{
		Console.Warning("Removing existing index file '%s'", index_filename.c_str());
		FileSystem::DeleteFilePath(index_filename.c_str());
	}
	if (FileSystem::FileExists(blob_filename.c_str()))
	{
		Console.Warning("Removing existing blob file '%s'", blob_filename.c_str());
		FileSystem::DeleteFilePath(blob_filename.c_str());
	}

	m_index_file = FileSystem::OpenCFile(index_filename.c_str(), "wb");
	if (!m_index_file)
	{
		Console.Error("Failed to open index file '%s' for writing", index_filename.c_str());
		return false;
	}

	const u32 file_version = SHADER_CACHE_VERSION;
	VK_PIPELINE_CACHE_HEADER header;
	FillPipelineCacheHeader(&header);

	if (std::fwrite(&file_version, sizeof(file_version), 1, m_index_file) != 1 ||
		std::fwrite(&header, sizeof(header), 1, m_index_file) != 1)
	{
		Console.Error("Failed to write header to index file '%s'", index_filename.c_str());
		std::fclose(m_index_file);
		m_index_file = nullptr;
		FileSystem::DeleteFilePath(index_filename.c_str());
		return false;
	}

	m_blob_file = FileSystem::OpenCFile(blob_filename.c_str(), "w+b");
	if (!m_blob_file)
	{
		Console.Error("Failed to open blob file '%s' for writing", blob_filename.c_str());
		std::fclose(m_index_file);
		m_index_file = nullptr;
		FileSystem::DeleteFilePath(index_filename.c_str());
		return false;
	}

	return true;
}

bool VKShaderCache::ReadExistingShaderCache(const std::string& index_filename, const std::string& blob_filename)
{
	m_index_file = FileSystem::OpenCFile(index_filename.c_str(), "r+b");
	if (!m_index_file)
	{
		// special case here: when there's a sharing violation (i.e. two instances running),
		// we don't want to blow away the cache. so just continue without a cache.
		if (errno == EACCES)
		{
			Console.WriteLn("Failed to open shader cache index with EACCES, are you running two instances?");
			return true;
		}

		return false;
	}

	u32 file_version = 0;
	if (std::fread(&file_version, sizeof(file_version), 1, m_index_file) != 1 || file_version != SHADER_CACHE_VERSION)
	{
		Console.Error("Bad file/data version in '%s'", index_filename.c_str());
		std::fclose(m_index_file);
		m_index_file = nullptr;
		return false;
	}

	VK_PIPELINE_CACHE_HEADER header;
	if (std::fread(&header, sizeof(header), 1, m_index_file) != 1 || !ValidatePipelineCacheHeader(header))
	{
		Console.Error("Mismatched pipeline cache header in '%s' (GPU/driver changed?)", index_filename.c_str());
		std::fclose(m_index_file);
		m_index_file = nullptr;
		return false;
	}

	m_blob_file = FileSystem::OpenCFile(blob_filename.c_str(), "a+b");
	if (!m_blob_file)
	{
		Console.Error("Blob file '%s' is missing", blob_filename.c_str());
		std::fclose(m_index_file);
		m_index_file = nullptr;
		return false;
	}

	std::fseek(m_blob_file, 0, SEEK_END);
	const u32 blob_file_size = static_cast<u32>(std::ftell(m_blob_file));

	for (;;)
	{
		CacheIndexEntry entry;
		if (std::fread(&entry, sizeof(entry), 1, m_index_file) != 1 ||
			(entry.file_offset + entry.blob_size) > blob_file_size)
		{
			if (std::feof(m_index_file))
				break;

			Console.Error("Failed to read entry from '%s', corrupt file?", index_filename.c_str());
			m_index.clear();
			std::fclose(m_blob_file);
			m_blob_file = nullptr;
			std::fclose(m_index_file);
			m_index_file = nullptr;
			return false;
		}

		const CacheIndexKey key{entry.source_hash_low, entry.source_hash_high, entry.source_length, entry.shader_type};
		const CacheIndexData data{entry.file_offset, entry.blob_size};
		m_index.emplace(key, data);
	}

	// ensure we don't write before seeking
	std::fseek(m_index_file, 0, SEEK_END);

	Console.WriteLn("Read %zu entries from '%s'", m_index.size(), index_filename.c_str());
	return true;
}

void VKShaderCache::CloseShaderCache()
{
	if (m_index_file)
	{
		std::fclose(m_index_file);
		m_index_file = nullptr;
	}
	if (m_blob_file)
	{
		std::fclose(m_blob_file);
		m_blob_file = nullptr;
	}
}

bool VKShaderCache::CreateNewPipelineCache()
{
	if (!m_pipeline_cache_filename.empty() && FileSystem::FileExists(m_pipeline_cache_filename.c_str()))
	{
		Console.Warning("Removing existing pipeline cache '%s'", m_pipeline_cache_filename.c_str());
		FileSystem::DeleteFilePath(m_pipeline_cache_filename.c_str());
	}

	const VkPipelineCacheCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, 0, nullptr};
	VkResult res = vkCreatePipelineCache(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &m_pipeline_cache);
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkCreatePipelineCache() failed: ");
		return false;
	}

	m_pipeline_cache_dirty = true;
	return true;
}

bool VKShaderCache::ReadExistingPipelineCache()
{
	std::optional<std::vector<u8>> data = FileSystem::ReadBinaryFile(m_pipeline_cache_filename.c_str());
	if (!data.has_value())
		return false;

	if (data->size() < sizeof(VK_PIPELINE_CACHE_HEADER))
	{
		Console.Error("Pipeline cache at '%s' is too small", m_pipeline_cache_filename.c_str());
		return false;
	}

	VK_PIPELINE_CACHE_HEADER header;
	std::memcpy(&header, data->data(), sizeof(header));
	if (!ValidatePipelineCacheHeader(header))
		return false;

	const VkPipelineCacheCreateInfo ci{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, 0, data->size(), data->data()};
	VkResult res = vkCreatePipelineCache(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &m_pipeline_cache);
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkCreatePipelineCache() failed: ");
		return false;
	}

	return true;
}

bool VKShaderCache::FlushPipelineCache()
{
	if (m_pipeline_cache == VK_NULL_HANDLE || !m_pipeline_cache_dirty || m_pipeline_cache_filename.empty())
		return false;

	size_t data_size;
	VkResult res =
		vkGetPipelineCacheData(GSDeviceVK::GetInstance()->GetDevice(), m_pipeline_cache, &data_size, nullptr);
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkGetPipelineCacheData() failed: ");
		return false;
	}

	std::vector<u8> data(data_size);
	res = vkGetPipelineCacheData(GSDeviceVK::GetInstance()->GetDevice(), m_pipeline_cache, &data_size, data.data());
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkGetPipelineCacheData() (2) failed: ");
		return false;
	}

	data.resize(data_size);

	// Save disk writes if it hasn't changed, think of the poor SSDs.
	FILESYSTEM_STAT_DATA sd;
	if (!FileSystem::StatFile(m_pipeline_cache_filename.c_str(), &sd) || sd.Size != static_cast<s64>(data_size))
	{
		Console.WriteLn("Writing %zu bytes to '%s'", data_size, m_pipeline_cache_filename.c_str());
		if (!FileSystem::WriteBinaryFile(m_pipeline_cache_filename.c_str(), data.data(), data.size()))
		{
			Console.Error("Failed to write pipeline cache to '%s'", m_pipeline_cache_filename.c_str());
			return false;
		}
	}
	else
	{
		Console.WriteLn("Skipping updating pipeline cache '%s' due to no changes.", m_pipeline_cache_filename.c_str());
	}

	m_pipeline_cache_dirty = false;
	return true;
}

void VKShaderCache::ClosePipelineCache()
{
	if (m_pipeline_cache == VK_NULL_HANDLE)
		return;

	vkDestroyPipelineCache(GSDeviceVK::GetInstance()->GetDevice(), m_pipeline_cache, nullptr);
	m_pipeline_cache = VK_NULL_HANDLE;
}

std::string VKShaderCache::GetShaderCacheBaseFileName(bool debug)
{
	std::string base_filename = "vulkan_shaders";

	if (debug)
		base_filename += "_debug";

	return Path::Combine(EmuFolders::Cache, base_filename);
}

std::string VKShaderCache::GetPipelineCacheBaseFileName(bool debug)
{
	std::string base_filename = "vulkan_pipelines";

	if (debug)
		base_filename += "_debug";

	base_filename += ".bin";

	return Path::Combine(EmuFolders::Cache, base_filename);
}

VKShaderCache::CacheIndexKey VKShaderCache::GetCacheKey(u32 type, const std::string_view& shader_code)
{
	union HashParts
	{
		struct
		{
			u64 hash_low;
			u64 hash_high;
		};
		u8 hash[16];
	};
	HashParts h;

	MD5Digest digest;
	digest.Update(shader_code.data(), static_cast<u32>(shader_code.length()));
	digest.Final(h.hash);

	return CacheIndexKey{h.hash_low, h.hash_high, static_cast<u32>(shader_code.length()), type};
}

std::optional<VKShaderCache::SPIRVCodeVector> VKShaderCache::GetShaderSPV(u32 type, std::string_view shader_code)
{
	const auto key = GetCacheKey(type, shader_code);
	auto iter = m_index.find(key);
	if (iter == m_index.end())
		return CompileAndAddShaderSPV(key, shader_code);

	std::optional<SPIRVCodeVector> spv = SPIRVCodeVector(iter->second.blob_size);

	if (std::fseek(m_blob_file, iter->second.file_offset, SEEK_SET) != 0 ||
		std::fread(spv->data(), sizeof(SPIRVCodeType), iter->second.blob_size, m_blob_file) != iter->second.blob_size)
	{
		Console.Error("Read blob from file failed, recompiling");
		spv = CompileShaderToSPV(type, shader_code, GSConfig.UseDebugDevice);
	}

	return spv;
}

VkShaderModule VKShaderCache::GetShaderModule(u32 type, std::string_view shader_code)
{
	std::optional<SPIRVCodeVector> spv = GetShaderSPV(type, shader_code);
	if (!spv.has_value())
		return VK_NULL_HANDLE;

	const VkShaderModuleCreateInfo ci{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, spv->size() * sizeof(SPIRVCodeType), spv->data()};

	VkShaderModule mod;
	VkResult res = vkCreateShaderModule(GSDeviceVK::GetInstance()->GetDevice(), &ci, nullptr, &mod);
	if (res != VK_SUCCESS)
	{
		LOG_VULKAN_ERROR(res, "vkCreateShaderModule() failed: ");
		return VK_NULL_HANDLE;
	}

	return mod;
}

VkShaderModule VKShaderCache::GetVertexShader(std::string_view shader_code)
{
	return GetShaderModule(shaderc_glsl_vertex_shader, std::move(shader_code));
}

VkShaderModule VKShaderCache::GetFragmentShader(std::string_view shader_code)
{
	return GetShaderModule(shaderc_glsl_fragment_shader, std::move(shader_code));
}

VkShaderModule VKShaderCache::GetComputeShader(std::string_view shader_code)
{
	return GetShaderModule(shaderc_glsl_compute_shader, std::move(shader_code));
}

std::optional<VKShaderCache::SPIRVCodeVector> VKShaderCache::CompileAndAddShaderSPV(
	const CacheIndexKey& key, std::string_view shader_code)
{
	std::optional<SPIRVCodeVector> spv = CompileShaderToSPV(key.shader_type, shader_code, GSConfig.UseDebugDevice);
	if (!spv.has_value())
		return {};

	if (!m_blob_file || std::fseek(m_blob_file, 0, SEEK_END) != 0)
		return spv;

	CacheIndexData data;
	data.file_offset = static_cast<u32>(std::ftell(m_blob_file));
	data.blob_size = static_cast<u32>(spv->size());

	CacheIndexEntry entry = {};
	entry.source_hash_low = key.source_hash_low;
	entry.source_hash_high = key.source_hash_high;
	entry.source_length = key.source_length;
	entry.shader_type = static_cast<u32>(key.shader_type);
	entry.blob_size = data.blob_size;
	entry.file_offset = data.file_offset;

	if (std::fwrite(spv->data(), sizeof(SPIRVCodeType), entry.blob_size, m_blob_file) != entry.blob_size ||
		std::fflush(m_blob_file) != 0 || std::fwrite(&entry, sizeof(entry), 1, m_index_file) != 1 ||
		std::fflush(m_index_file) != 0)
	{
		Console.Error("Failed to write shader blob to file");
		return spv;
	}

	m_index.emplace(key, data);
	return spv;
}
