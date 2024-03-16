// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "SymbolTreeDelegates.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include "Debugger/SymbolTree/SymbolTreeModel.h"
#include "Debugger/SymbolTree/TypeString.h"

SymbolTreeValueDelegate::SymbolTreeValueDelegate(
	DebugInterface& cpu,
	QObject* parent)
	: QStyledItemDelegate(parent)
	, m_cpu(cpu)
{
}

QWidget* SymbolTreeValueDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid())
		return nullptr;

	const SymbolTreeModel* tree_model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!tree_model)
		return nullptr;

	SymbolTreeNode* node = tree_model->nodeFromIndex(index);
	if (!node || !node->type.valid())
		return nullptr;

	QWidget* result = nullptr;
	m_cpu.GetSymbolGuardian().TryRead([&](const ccc::SymbolDatabase& database) {
		QVariant value = node->readValueAsVariant(m_cpu, database);

		const ccc::ast::Node* logical_type = node->type.lookup_node(database);
		if (!logical_type)
			return;

		const ccc::ast::Node& type = *resolvePhysicalType(logical_type, database).first;
		switch (type.descriptor)
		{
			case ccc::ast::BUILTIN:
			{
				const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();

				switch (builtIn.bclass)
				{
					case ccc::ast::BuiltInClass::UNSIGNED_8:
					case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					case ccc::ast::BuiltInClass::UNSIGNED_16:
					case ccc::ast::BuiltInClass::UNSIGNED_32:
					case ccc::ast::BuiltInClass::UNSIGNED_64:
					{
						QLineEdit* editor = new QLineEdit(parent);
						editor->setText(QString::number(value.toULongLong()));
						result = editor;

						break;
					}
					case ccc::ast::BuiltInClass::SIGNED_8:
					case ccc::ast::BuiltInClass::SIGNED_16:
					case ccc::ast::BuiltInClass::SIGNED_32:
					case ccc::ast::BuiltInClass::SIGNED_64:
					{
						QLineEdit* editor = new QLineEdit(parent);
						editor->setText(QString::number(value.toLongLong()));
						result = editor;

						break;
					}
					case ccc::ast::BuiltInClass::BOOL_8:
					{
						QCheckBox* editor = new QCheckBox(parent);
						editor->setChecked(value.toBool());
						result = editor;

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_32:
					{
						QLineEdit* editor = new QLineEdit(parent);
						editor->setText(QString::number(value.toFloat()));
						result = editor;

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_64:
					{
						QLineEdit* editor = new QLineEdit(parent);
						editor->setText(QString::number(value.toDouble()));
						result = editor;

						break;
					}
					default:
					{
					}
				}
				break;
			}
			case ccc::ast::ENUM:
			{
				const ccc::ast::Enum& enumeration = type.as<ccc::ast::Enum>();

				QComboBox* combo_box = new QComboBox(parent);
				for (s32 i = 0; i < (s32)enumeration.constants.size(); i++)
				{
					combo_box->addItem(QString::fromStdString(enumeration.constants[i].second));
					if (enumeration.constants[i].first == value.toInt())
						combo_box->setCurrentIndex(i);
				}
				connect(combo_box, &QComboBox::currentIndexChanged, this, &SymbolTreeValueDelegate::onComboBoxIndexChanged);
				result = combo_box;

				break;
			}
			case ccc::ast::POINTER_OR_REFERENCE:
			case ccc::ast::POINTER_TO_DATA_MEMBER:
			{
				QLineEdit* editor = new QLineEdit(parent);
				editor->setText(QString::number(value.toULongLong(), 16));
				result = editor;

				break;
			}
			default:
			{
			}
		}
	});

	return result;
}

void SymbolTreeValueDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	// This function is intentionally left blank to prevent the values of
	// editors from constantly being reset every time the model is updated.
}

void SymbolTreeValueDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	if (!index.isValid())
		return;

	const SymbolTreeModel* tree_model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!model)
		return;

	SymbolTreeNode* node = tree_model->nodeFromIndex(index);
	if (!node || !node->type.valid())
		return;

	m_cpu.GetSymbolGuardian().TryRead([&](const ccc::SymbolDatabase& database) {
		const ccc::ast::Node* logical_type = node->type.lookup_node(database);
		if (!logical_type)
			return;

		const ccc::ast::Node& type = *resolvePhysicalType(logical_type, database).first;
		switch (type.descriptor)
		{
			case ccc::ast::BUILTIN:
			{
				const ccc::ast::BuiltIn& builtIn = type.as<ccc::ast::BuiltIn>();

				switch (builtIn.bclass)
				{
					case ccc::ast::BuiltInClass::UNSIGNED_8:
					case ccc::ast::BuiltInClass::UNQUALIFIED_8:
					case ccc::ast::BuiltInClass::UNSIGNED_16:
					case ccc::ast::BuiltInClass::UNSIGNED_32:
					case ccc::ast::BuiltInClass::UNSIGNED_64:
					{
						QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(line_edit);

						bool ok;
						qulonglong value = line_edit->text().toULongLong(&ok);
						if (ok)
							model->setData(index, value, Qt::EditRole);

						break;
					}
					case ccc::ast::BuiltInClass::SIGNED_8:
					case ccc::ast::BuiltInClass::SIGNED_16:
					case ccc::ast::BuiltInClass::SIGNED_32:
					case ccc::ast::BuiltInClass::SIGNED_64:
					{
						QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(line_edit);

						bool ok;
						qlonglong value = line_edit->text().toLongLong(&ok);
						if (ok)
							model->setData(index, value, Qt::EditRole);

						break;
					}
					case ccc::ast::BuiltInClass::BOOL_8:
					{
						QCheckBox* check_box = qobject_cast<QCheckBox*>(editor);
						model->setData(index, check_box->isChecked(), Qt::EditRole);

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_32:
					{
						QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(line_edit);

						bool ok;
						float value = line_edit->text().toFloat(&ok);
						if (ok)
							model->setData(index, value, Qt::EditRole);

						break;
					}
					case ccc::ast::BuiltInClass::FLOAT_64:
					{
						QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
						Q_ASSERT(line_edit);

						bool ok;
						double value = line_edit->text().toDouble(&ok);
						if (ok)
							model->setData(index, value, Qt::EditRole);

						break;
					}
					default:
					{
					}
				}
				break;
			}
			case ccc::ast::ENUM:
			{
				const ccc::ast::Enum& enumeration = type.as<ccc::ast::Enum>();
				QComboBox* combo_box = static_cast<QComboBox*>(editor);
				s32 comboIndex = combo_box->currentIndex();
				if (comboIndex < (s32)enumeration.constants.size())
				{
					s32 value = enumeration.constants[comboIndex].first;
					model->setData(index, QVariant(value), Qt::EditRole);
				}
				break;
			}
			case ccc::ast::POINTER_OR_REFERENCE:
			case ccc::ast::POINTER_TO_DATA_MEMBER:
			{
				QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
				Q_ASSERT(line_edit);

				bool ok;
				qulonglong address = line_edit->text().toUInt(&ok, 16);
				if (ok)
					model->setData(index, address, Qt::EditRole);

				break;
			}
			default:
			{
			}
		}
	});
}

void SymbolTreeValueDelegate::onComboBoxIndexChanged(int index)
{
	QComboBox* combo_box = qobject_cast<QComboBox*>(sender());
	if (combo_box)
		commitData(combo_box);
}

// *****************************************************************************

SymbolTreeLocationDelegate::SymbolTreeLocationDelegate(
	DebugInterface& cpu,
	u32 alignment,
	QObject* parent)
	: QStyledItemDelegate(parent)
	, m_cpu(cpu)
	, m_alignment(alignment)
{
}

QWidget* SymbolTreeLocationDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid())
		return nullptr;

	const SymbolTreeModel* model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!model)
		return nullptr;

	SymbolTreeNode* node = model->nodeFromIndex(index);
	if (!node || !node->symbol.valid() || !node->symbol.is_flag_set(ccc::WITH_ADDRESS_MAP))
		return nullptr;

	if (!node->is_location_editable)
		return nullptr;

	if (m_cpu.GetSymbolGuardian().IsBusy())
		return nullptr;

	return new QLineEdit(parent);
}

void SymbolTreeLocationDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	if (!index.isValid())
		return;

	const SymbolTreeModel* model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!model)
		return;

	SymbolTreeNode* node = model->nodeFromIndex(index);
	if (!node || !node->symbol.valid())
		return;

	QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
	Q_ASSERT(line_edit);

	m_cpu.GetSymbolGuardian().TryRead([&](const ccc::SymbolDatabase& database) {
		const ccc::Symbol* symbol = node->symbol.lookup_symbol(database);
		if (!symbol || !symbol->address().valid())
			return;

		line_edit->setText(QString::number(symbol->address().value, 16));
	});
}

void SymbolTreeLocationDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	if (!index.isValid())
		return;

	const SymbolTreeModel* tree_model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!tree_model)
		return;

	SymbolTreeNode* node = tree_model->nodeFromIndex(index);
	if (!node || !node->symbol.valid() || !node->symbol.is_flag_set(ccc::WITH_ADDRESS_MAP))
		return;

	QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
	Q_ASSERT(line_edit);

	SymbolTreeModel* symbol_tree_model = qobject_cast<SymbolTreeModel*>(model);
	Q_ASSERT(symbol_tree_model);

	bool ok;
	u32 address = line_edit->text().toUInt(&ok, 16);
	if (!ok)
		return;

	address -= address % m_alignment;

	bool success = false;
	m_cpu.GetSymbolGuardian().BlockingReadWrite([&](ccc::SymbolDatabase& database) {
		if (node->symbol.move_symbol(address, database))
			success = true;
	});

	if (success)
	{
		node->location = SymbolTreeLocation(SymbolTreeLocation::MEMORY, address);
		symbol_tree_model->resetChildren(index);
	}
}

// *****************************************************************************

SymbolTreeTypeDelegate::SymbolTreeTypeDelegate(
	DebugInterface& cpu,
	QObject* parent)
	: QStyledItemDelegate(parent)
	, m_cpu(cpu)
{
}

QWidget* SymbolTreeTypeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid())
		return nullptr;

	const SymbolTreeModel* tree_model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!tree_model)
		return nullptr;

	SymbolTreeNode* node = tree_model->nodeFromIndex(index);
	if (!node || !node->symbol.valid())
		return nullptr;

	if (m_cpu.GetSymbolGuardian().IsBusy())
		return nullptr;

	return new QLineEdit(parent);
}

void SymbolTreeTypeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	if (!index.isValid())
		return;

	const SymbolTreeModel* tree_model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!tree_model)
		return;

	SymbolTreeNode* node = tree_model->nodeFromIndex(index);
	if (!node || !node->symbol.valid())
		return;

	QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
	Q_ASSERT(line_edit);

	m_cpu.GetSymbolGuardian().TryRead([&](const ccc::SymbolDatabase& database) {
		const ccc::Symbol* symbol = node->symbol.lookup_symbol(database);
		if (!symbol || !symbol->type())
			return;

		line_edit->setText(typeToString(symbol->type(), database));
	});
}

void SymbolTreeTypeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	if (!index.isValid())
		return;

	const SymbolTreeModel* tree_model = qobject_cast<const SymbolTreeModel*>(index.model());
	if (!tree_model)
		return;

	SymbolTreeNode* node = tree_model->nodeFromIndex(index);
	if (!node || !node->symbol.valid())
		return;

	QLineEdit* line_edit = qobject_cast<QLineEdit*>(editor);
	Q_ASSERT(line_edit);

	SymbolTreeModel* symbol_tree_model = qobject_cast<SymbolTreeModel*>(model);
	Q_ASSERT(symbol_tree_model);

	QString error_message;
	m_cpu.GetSymbolGuardian().BlockingReadWrite([&](ccc::SymbolDatabase& database) {
		ccc::Symbol* symbol = node->symbol.lookup_symbol(database);
		if (!symbol)
		{
			error_message = tr("Symbol no longer exists.");
			return;
		}

		std::unique_ptr<ccc::ast::Node> type = stringToType(line_edit->text().toStdString(), database, error_message);
		if (!error_message.isEmpty())
			return;

		symbol->set_type(std::move(type));
		node->type = ccc::NodeHandle(node->symbol.descriptor(), *symbol, symbol->type());
	});

	if (error_message.isEmpty())
		symbol_tree_model->resetChildren(index);
	else
		QMessageBox::warning(editor, tr("Cannot Change Type"), error_message);
}
