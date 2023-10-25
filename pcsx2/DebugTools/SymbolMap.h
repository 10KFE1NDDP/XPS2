/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014  PCSX2 Dev Team
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

#include <vector>
#include <set>
#include <map>
#include <string>
#include <mutex>

#include "common/Pcsx2Types.h"

enum SymbolType
{
	ST_NONE = 0,
	ST_FUNCTION = 1,
	ST_DATA = 2,
	ST_ALL = 3,
};

struct SymbolInfo
{
	SymbolType type;
	u32 address;
	u32 size;
};

struct SymbolEntry
{
	std::string name;
	u32 address;
	u32 size;
};

struct LoadedModuleInfo
{
	std::string name;
	u32 address;
	u32 size;
	bool active;
};

enum DataType
{
	DATATYPE_NONE,
	DATATYPE_BYTE,
	DATATYPE_HALFWORD,
	DATATYPE_WORD,
	DATATYPE_ASCII
};

class SymbolMap
{
public:
	SymbolMap() {}
	void Clear();
	void SortSymbols();

	bool LoadNocashSym(const std::string& filename);

	SymbolType GetSymbolType(u32 address) const;
	bool GetSymbolInfo(SymbolInfo* info, u32 address, SymbolType symmask = ST_FUNCTION) const;
	u32 GetNextSymbolAddress(u32 address, SymbolType symmask);
	std::string GetDescription(unsigned int address) const;
	std::vector<SymbolEntry> GetAllSymbols(SymbolType symmask);

	void AddFunction(const std::string& name, u32 address, u32 size);
	u32 GetFunctionStart(u32 address) const;
	int GetFunctionNum(u32 address) const;
	u32 GetFunctionSize(u32 startAddress) const;
	bool SetFunctionSize(u32 startAddress, u32 newSize);
	bool RemoveFunction(u32 startAddress);

	void AddLabel(const std::string& name, u32 address);
	std::string GetLabelName(u32 address) const;
	void SetLabelName(const std::string& name, u32 address);
	bool GetLabelValue(const std::string& name, u32& dest);

	void AddData(u32 address, u32 size, DataType type, int moduleIndex = -1);
	u32 GetDataStart(u32 address) const;
	u32 GetDataSize(u32 startAddress) const;
	DataType GetDataType(u32 startAddress) const;

	static const u32 INVALID_ADDRESS = (u32)-1;

	bool IsEmpty() const { return functions.empty() && labels.empty() && data.empty(); };

private:
	void AssignFunctionIndices();

	struct FunctionEntry
	{
		u32 start;
		u32 size;
		int index;
	};

	struct LabelEntry
	{
		u32 addr;
		std::string name;
	};

	struct DataEntry
	{
		DataType type;
		u32 start;
		u32 size;
	};

	std::map<u32, FunctionEntry> functions;
	std::map<u32, LabelEntry> labels;
	std::map<u32, DataEntry> data;

	mutable std::recursive_mutex m_lock;
};

extern SymbolMap R5900SymbolMap;
extern SymbolMap R3000SymbolMap;
