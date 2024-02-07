// SPDX-FileCopyrightText: 2002-2023 PCSX2 Dev Team
// SPDX-License-Identifier: LGPL-3.0+

#include "NewSymbolDialogs.h"

#include <QtWidgets/QMessageBox>

#include "TypeString.h"

NewSymbolDialog::NewSymbolDialog(u32 flags, DebugInterface& cpu, QWidget* parent)
	: QDialog(parent)
	, m_cpu(cpu)
{
	m_ui.setupUi(this);

	connect(m_ui.buttonBox, &QDialogButtonBox::accepted, this, &NewSymbolDialog::createSymbol);
	connect(m_ui.storageTabBar, &QTabBar::currentChanged, this, &NewSymbolDialog::onStorageTabChanged);

	if (flags & GLOBAL_STORAGE)
		m_ui.storageTabBar->addTab("Global");

	if (flags & REGISTER_STORAGE)
	{
		setupRegisterField();
		m_ui.storageTabBar->addTab("Register");
	}

	if (flags & STACK_STORAGE)
		m_ui.storageTabBar->addTab("Stack");

	if (m_ui.storageTabBar->count() == 1)
		m_ui.storageTabBar->hide();

	if (!(flags & SIZE_FIELD))
	{
		m_ui.sizeLabel->hide();
		m_ui.expandToFillSpaceRadioButton->hide();
		m_ui.customSizeRadioButton->hide();
		m_ui.customSizeSpinBox->hide();
	}

	if (!(flags & EXISTING_FUNCTIONS_FIELD))
	{
		m_ui.existingFunctionsLabel->hide();
		m_ui.shrinkExistingRadioButton->hide();
		m_ui.doNotModifyExistingRadioButton->hide();
	}

	if (!(flags & TYPE_FIELD))
	{
		m_ui.typeLabel->hide();
		m_ui.typeLineEdit->hide();
	}

	if (flags & FUNCTION_FIELD)
	{
		setupFunctionField();
	}
	else
	{
		m_ui.functionLabel->hide();
		m_ui.functionComboBox->hide();
	}

	adjustSize();
}

void NewSymbolDialog::setupRegisterField()
{
	m_ui.registerComboBox->clear();
	for (int i = 0; i < m_cpu.getRegisterCount(0); i++)
	{
		m_ui.registerComboBox->addItem(m_cpu.getRegisterName(0, i));
	}
}

void NewSymbolDialog::setupFunctionField()
{
	m_cpu.GetSymbolGuardian().BlockingRead([&](const ccc::SymbolDatabase& database) {
		const ccc::Function* default_function = database.functions.symbol_overlapping_address(m_cpu.getPC());

		for (const ccc::Function& function : database.functions)
		{
			QString name = QString::fromStdString(function.name());
			name.truncate(64);
			m_ui.functionComboBox->addItem(name);
			m_functions.emplace_back(function.handle());

			if (default_function && function.handle() == default_function->handle())
				m_ui.functionComboBox->setCurrentIndex(m_ui.functionComboBox->count() - 1);
		}
	});
}

void NewSymbolDialog::onStorageTabChanged(int index)
{
	QString name = m_ui.storageTabBar->tabText(index);

	bool global_storage = name == "Global";
	bool register_storage = name == "Register";
	bool stack_storage = name == "Stack";

	m_ui.addressLabel->setVisible(global_storage);
	m_ui.addressLineEdit->setVisible(global_storage);

	m_ui.registerLabel->setVisible(register_storage);
	m_ui.registerComboBox->setVisible(register_storage);

	m_ui.stackPointerOffsetLabel->setVisible(stack_storage);
	m_ui.stackPointerOffsetSpinBox->setVisible(stack_storage);
}

u32 NewSymbolDialog::storageType() const
{
	QString name = m_ui.storageTabBar->tabText(m_ui.storageTabBar->currentIndex());

	if (name == "Global")
		return GLOBAL_STORAGE;
	if (name == "Register")
		return REGISTER_STORAGE;
	if (name == "Stack")
		return STACK_STORAGE;

	return 0;
}

NewFunctionDialog::NewFunctionDialog(DebugInterface& cpu, QWidget* parent)
	: NewSymbolDialog(GLOBAL_STORAGE | SIZE_FIELD | EXISTING_FUNCTIONS_FIELD, cpu, parent)
{
	setWindowTitle("New Function");
}

void NewFunctionDialog::createSymbol()
{
	QString error_message;
	m_cpu.GetSymbolGuardian().ShortReadWrite([&](ccc::SymbolDatabase& database) {
		// Parse the user input.
		bool ok;

		std::string name = m_ui.nameLineEdit->text().toStdString();
		if (name.empty())
		{
			error_message = tr("No name provided.");
			return;
		}

		u32 address = m_ui.addressLineEdit->text().toUInt(&ok, 16);
		if (!ok)
		{
			error_message = tr("Invalid address.");
			return;
		}

		// Create the symbol.
		ccc::Result<ccc::SymbolSourceHandle> source = database.get_symbol_source("User-defined");
		if (!source.success())
		{
			error_message = tr("Cannot create symbol source.");
			return;
		}

		ccc::Result<ccc::Function*> function = database.functions.create_symbol(std::move(name), address, *source, nullptr);
		if (!function.success())
		{
			error_message = tr("Cannot create symbol.");
			return;
		}
	});

	if (!error_message.isEmpty())
		QMessageBox::warning(this, tr("Cannot Create Function"), error_message);
}

NewGlobalVariableDialog::NewGlobalVariableDialog(DebugInterface& cpu, QWidget* parent)
	: NewSymbolDialog(GLOBAL_STORAGE | TYPE_FIELD, cpu, parent)
{
	setWindowTitle("New Global Variable");
}

void NewGlobalVariableDialog::createSymbol()
{
	QString error_message;
	m_cpu.GetSymbolGuardian().ShortReadWrite([&](ccc::SymbolDatabase& database) {
		// Parse the user input.
		bool ok;

		std::string name = m_ui.nameLineEdit->text().toStdString();
		if (name.empty())
		{
			error_message = tr("No name provided.");
			return;
		}

		u32 address = m_ui.addressLineEdit->text().toUInt(&ok, 16);
		if (!ok)
		{
			error_message = tr("Invalid address.");
			return;
		}

		std::unique_ptr<ccc::ast::Node> type = stringToType(m_ui.typeLineEdit->text().toStdString(), database, error_message);
		if (!type)
			return;

		// Create the symbol.
		ccc::Result<ccc::SymbolSourceHandle> source = database.get_symbol_source("User-defined");
		if (!source.success())
		{
			error_message = tr("Cannot create symbol source.");
			return;
		}

		ccc::Result<ccc::GlobalVariable*> global_variable = database.global_variables.create_symbol(std::move(name), address, *source, nullptr);
		if (!global_variable.success())
		{
			error_message = tr("Cannot create symbol.");
			return;
		}

		(*global_variable)->set_type(std::move(type));
	});

	if (!error_message.isEmpty())
		QMessageBox::warning(this, tr("Cannot Create Global Variable"), error_message);
}

NewLocalVariableDialog::NewLocalVariableDialog(DebugInterface& cpu, QWidget* parent)
	: NewSymbolDialog(GLOBAL_STORAGE | REGISTER_STORAGE | STACK_STORAGE | TYPE_FIELD | FUNCTION_FIELD, cpu, parent)
{
	setWindowTitle("New Local Variable");
}

void NewLocalVariableDialog::createSymbol()
{
	QString error_message;
	m_cpu.GetSymbolGuardian().ShortReadWrite([&](ccc::SymbolDatabase& database) {
		// Parse the user input.
		ccc::FunctionHandle function_handle = m_functions.at(m_ui.functionComboBox->currentIndex());
		ccc::Function* function = database.functions.symbol_from_handle(function_handle);
		if (!function)
		{
			error_message = tr("Invalid function.");
			return;
		}

		std::string name = m_ui.nameLineEdit->text().toStdString();
		if (name.empty())
		{
			error_message = tr("No name provided.");
			return;
		}

		std::variant<ccc::GlobalStorage, ccc::RegisterStorage, ccc::StackStorage> storage;
		ccc::Address address;
		switch (storageType())
		{
			case GLOBAL_STORAGE:
			{
				storage.emplace<ccc::GlobalStorage>();

				bool ok;
				address = m_ui.addressLineEdit->text().toUInt(&ok, 16);
				if (!ok)
				{
					error_message = tr("Invalid address.");
					return;
				}

				break;
			}
			case REGISTER_STORAGE:
			{
				ccc::RegisterStorage& register_storage = storage.emplace<ccc::RegisterStorage>();
				register_storage.dbx_register_number = m_ui.registerComboBox->currentIndex();
				break;
			}
			case STACK_STORAGE:
			{
				ccc::StackStorage& stack_storage = storage.emplace<ccc::StackStorage>();
				stack_storage.stack_pointer_offset = m_ui.stackPointerOffsetSpinBox->value();
				break;
			}
		}

		std::string typeString = m_ui.typeLineEdit->text().toStdString();
		std::unique_ptr<ccc::ast::Node> type = stringToType(typeString, database, error_message);
		if (!type)
			return;

		// Create the symbol.
		ccc::Result<ccc::SymbolSourceHandle> source = database.get_symbol_source("User-defined");
		if (!source.success())
		{
			error_message = tr("Cannot create symbol source.");
			return;
		}

		ccc::Result<ccc::LocalVariable*> local_variable =
			database.local_variables.create_symbol(std::move(name), address, *source, nullptr);
		if (!local_variable.success())
		{
			error_message = tr("Cannot create symbol.");
			return;
		}

		(*local_variable)->set_type(std::move(type));
		(*local_variable)->storage = storage;

		// Add the local variable to the chosen function.
		std::vector<ccc::LocalVariableHandle> local_variables;
		if (function->local_variables().has_value())
			local_variables = *function->local_variables();
		local_variables.emplace_back((*local_variable)->handle());
		function->set_local_variables(local_variables, database);
	});

	if (!error_message.isEmpty())
		QMessageBox::warning(this, tr("Cannot Create Local Variable"), error_message);
}

NewParameterVariableDialog::NewParameterVariableDialog(DebugInterface& cpu, QWidget* parent)
	: NewSymbolDialog(REGISTER_STORAGE | STACK_STORAGE | TYPE_FIELD | FUNCTION_FIELD, cpu, parent)
{
	setWindowTitle("New Parameter Variable");
}

void NewParameterVariableDialog::createSymbol()
{
	QString error_message;
	m_cpu.GetSymbolGuardian().ShortReadWrite([&](ccc::SymbolDatabase& database) {
		// Parse the user input.
		ccc::FunctionHandle function_handle = m_functions.at(m_ui.functionComboBox->currentIndex());
		ccc::Function* function = database.functions.symbol_from_handle(function_handle);
		if (!function)
		{
			error_message = tr("Invalid function.");
			return;
		}

		std::string name = m_ui.nameLineEdit->text().toStdString();
		if (name.empty())
		{
			error_message = tr("No name provided.");
			return;
		}

		std::variant<ccc::RegisterStorage, ccc::StackStorage> storage;
		switch (storageType())
		{
			case GLOBAL_STORAGE:
			{
				return;
			}
			case REGISTER_STORAGE:
			{
				ccc::RegisterStorage& register_storage = storage.emplace<ccc::RegisterStorage>();
				register_storage.dbx_register_number = m_ui.registerComboBox->currentIndex();
				break;
			}
			case STACK_STORAGE:
			{
				ccc::StackStorage& stack_storage = storage.emplace<ccc::StackStorage>();
				stack_storage.stack_pointer_offset = m_ui.stackPointerOffsetSpinBox->value();
				break;
			}
		}

		std::string typeString = m_ui.typeLineEdit->text().toStdString();
		std::unique_ptr<ccc::ast::Node> type = stringToType(typeString, database, error_message);
		if (!type)
			return;

		// Create the symbol.
		ccc::Result<ccc::SymbolSourceHandle> source = database.get_symbol_source("User-defined");
		if (!source.success())
		{
			error_message = tr("Cannot create symbol source.");
			return;
		}

		ccc::Result<ccc::ParameterVariable*> parameter_variable =
			database.parameter_variables.create_symbol(std::move(name), *source, nullptr);
		if (!parameter_variable.success())
		{
			error_message = tr("Cannot create symbol.");
			return;
		}

		(*parameter_variable)->set_type(std::move(type));
		(*parameter_variable)->storage = storage;

		// Add the parameter variable to the chosen function.
		std::vector<ccc::ParameterVariableHandle> parameter_variables;
		if (function->parameter_variables().has_value())
			parameter_variables = *function->parameter_variables();
		parameter_variables.emplace_back((*parameter_variable)->handle());
		function->set_parameter_variables(parameter_variables, database);
	});

	if (!error_message.isEmpty())
		QMessageBox::warning(this, tr("Cannot Create Parameter Variable"), error_message);
}
