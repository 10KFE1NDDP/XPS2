// SPDX-FileCopyrightText: 2002-2023 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include "common/Pcsx2Defs.h"

#ifdef _WIN32
#include "common/RedtapeWindows.h"
#elif defined(__linux__)
#include <libaio.h>
#elif defined(__POSIX__)
#include <aio.h>
#endif
#include <memory>
#include <string>

class Error;

class AsyncFileReader
{
protected:
	AsyncFileReader()
		: m_dataoffset(0)
		, m_blocksize(0)
	{
	}

	std::string m_filename;

	u32 m_dataoffset;
	u32 m_blocksize;

public:
	virtual ~AsyncFileReader(){};

	virtual bool Open(std::string filename, Error* error) = 0;

	virtual int ReadSync(void* pBuffer, u32 sector, u32 count) = 0;

	virtual void BeginRead(void* pBuffer, u32 sector, u32 count) = 0;
	virtual int FinishRead() = 0;
	virtual void CancelRead() = 0;

	virtual void Close() = 0;

	virtual u32 GetBlockCount() const = 0;

	virtual void SetBlockSize(u32 bytes) {}
	virtual void SetDataOffset(u32 bytes) {}

	const std::string& GetFilename() const { return m_filename; }
	u32 GetBlockSize() const { return m_blocksize; }
};

class FlatFileReader final : public AsyncFileReader
{
	DeclareNoncopyableObject(FlatFileReader);

#ifdef _WIN32
	HANDLE hOverlappedFile;

	OVERLAPPED asyncOperationContext;

	HANDLE hEvent;

	bool asyncInProgress;
#elif defined(__linux__)
	int m_fd; // FIXME don't know if overlap as an equivalent on linux
	io_context_t m_aio_context;
#elif defined(__POSIX__)
	int m_fd; // TODO OSX don't know if overlap as an equivalent on OSX
	struct aiocb m_aiocb;
	bool m_async_read_in_progress;
#endif

	bool shareWrite;

public:
	FlatFileReader(bool shareWrite = false);
	~FlatFileReader() override;

	bool Open(std::string filenae, Error* error) override;

	int ReadSync(void* pBuffer, u32 sector, u32 count) override;

	void BeginRead(void* pBuffer, u32 sector, u32 count) override;
	int FinishRead() override;
	void CancelRead() override;

	void Close() override;

	u32 GetBlockCount() const override;

	void SetBlockSize(u32 bytes) override { m_blocksize = bytes; }
	void SetDataOffset(u32 bytes) override { m_dataoffset = bytes; }
};
