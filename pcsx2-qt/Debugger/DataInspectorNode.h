// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#pragma once

#include <QString>

#include "common/Pcsx2Defs.h"
#include "DebugTools/ccc/ast.h"

class DebugInterface;

struct DataInspectorLocation
{
	enum
	{
		NONE,
		EE_REGISTER,
		IOP_REGISTER,
		EE_MEMORY,
		IOP_MEMORY,
	} type = NONE;
	u32 address = 0;
	
	QString name() const;
	
	DataInspectorLocation addOffset(u32 offset) const;
	DataInspectorLocation createAddress(u32 address) const;
	
	DebugInterface& cpu() const;
	
	u8 read8();
	u16 read16();
	u32 read32();
	u64 read64();
	u128 read128();
	
	void write8(u8 value);
	void write16(u16 value);
	void write32(u32 value);
	void write64(u64 value);
	void write128(u128 value);
	
	friend auto operator<=>(const DataInspectorLocation& lhs, const DataInspectorLocation& rhs) = default;
};

struct DataInspectorNode
{
public:
	QString name;
	ccc::NodeHandle type;
	DataInspectorLocation location;
	
	const DataInspectorNode* parent() const;
	
	const std::vector<std::unique_ptr<DataInspectorNode>>& children() const;
	bool children_fetched() const;
	void set_children(std::vector<std::unique_ptr<DataInspectorNode>> new_children);
	void insert_children(std::vector<std::unique_ptr<DataInspectorNode>> new_children);
	void emplace_child(std::unique_ptr<DataInspectorNode> new_child);
	
	void sortChildrenRecursively();

protected:
	DataInspectorNode* m_parent = nullptr;
	std::vector<std::unique_ptr<DataInspectorNode>> m_children;
	bool m_children_fetched = false;
};
