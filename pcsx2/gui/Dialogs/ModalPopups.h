/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "App.h"
#include "ConfigurationDialog.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/wizard.h>
#include <wx/treectrl.h>


class FirstTimeWizard : public wxWizard
{
	typedef wxWizard _parent;

protected:
	wxWizardPageSimple& m_page_intro;
	wxWizardPageSimple& m_page_plugins;
	wxWizardPageSimple& m_page_bios;

	wxPanelWithHelpers& m_panel_Intro;
	Panels::PluginSelectorPanel& m_panel_PluginSel;
	Panels::BiosSelectorPanel& m_panel_BiosSel;

public:
	FirstTimeWizard(wxWindow* parent);
	virtual ~FirstTimeWizard() = default;

	wxWizardPage* GetFirstPage() const { return &m_page_intro; }

	void ForceEnumPlugins()
	{
		m_panel_PluginSel.OnShown();
	}

	int ShowModal();

protected:
	virtual void OnPageChanging(wxWizardEvent& evt);
	virtual void OnPageChanged(wxWizardEvent& evt);
	virtual void OnDoubleClicked(wxCommandEvent& evt);

	void OnRestartWizard(wxCommandEvent& evt);
};


namespace Dialogs
{
	class AboutBoxDialog : public wxDialogWithHelpers
	{
	public:
		AboutBoxDialog(wxWindow* parent = NULL);
		virtual ~AboutBoxDialog() = default;

		static wxString GetNameStatic() { return L"AboutBox"; }
		wxString GetDialogName() const { return GetNameStatic(); }
	};

	class GSDumpDialog : public wxDialogWithHelpers
	{
	public:
		GSDumpDialog(wxWindow* parent = NULL);
		virtual ~GSDumpDialog() = default;

		static wxString GetNameStatic()
		{
			return L"AboutBox";
		}
		wxString GetDialogName() const
		{
			return GetNameStatic();
		}

	protected:
		wxListView* m_dump_list;
		wxStaticBitmap* m_preview_image;
		wxString* m_selected_dump;
		wxCheckBox* m_debug_mode;
		wxRadioBox* m_renderer_overrides;
		wxTreeCtrl* m_gif_list;
		wxTreeCtrl* m_gif_packet;
		void GetDumpsList();
		void SelectedDump(wxListEvent& evt);
		void RunDump(wxCommandEvent& event);
		void ToStart(wxCommandEvent& event);
		void StepPacket(wxCommandEvent& event);
		void ToCursor(wxCommandEvent& event);
		void ToVSync(wxCommandEvent& event);
		void ParsePacket(wxTreeEvent& event);
		enum
		{
			ID_DUMP_LIST,
			ID_RUN_DUMP,
			ID_RUN_START,
			ID_RUN_STEP,
			ID_RUN_CURSOR,
			ID_RUN_VSYNC,
			ID_SEL_PACKET
		};
		enum GSType : u8
		{
			Transfer = 0,
			VSync = 1,
			ReadFIFO2 = 2,
			Registers = 3
		};
		enum GSTransferPath : u8
		{
			Path1Old = 0,
			Path2 = 1,
			Path3 = 2,
			Path1New = 3,
			Dummy = 4
		};
		struct GSData
		{
			GSType id;
			char* data;
			int length;
			GSTransferPath path;
		};
		enum ButtonState
		{
			Step,
			RunCursor,
			RunVSync
		};
		enum GifFlag : u8
		{
			GIF_FLG_PACKED = 0,
			GIF_FLG_REGLIST = 1,
			GIF_FLG_IMAGE = 2,
			GIF_FLG_IMAGE2 = 3
		};
		enum GsPrim : u8
		{
			GS_POINTLIST = 0,
			GS_LINELIST = 1,
			GS_LINESTRIP = 2,
			GS_TRIANGLELIST = 3,
			GS_TRIANGLESTRIP = 4,
			GS_TRIANGLEFAN = 5,
			GS_SPRITE = 6,
			GS_INVALID = 7,
		};
		enum GsIIP : u8
		{
			FlatShading = 0,
			Gouraud = 1
		};
		enum GsFST : u8
		{
			STQValue = 0,
			UVValue = 1
		};
		enum GsCTXT : u8
		{
			Context1 = 0,
			Context2 = 1
		};
		enum GsFIX : u8
		{
			Unfixed = 0,
			Fixed = 1
		};
		struct GSEvent
		{
			ButtonState btn;
			int index;
		};
		std::vector<GSEvent> m_button_events;
		std::vector<GSData> m_dump_packets;
		void ProcessDumpEvent(GSData event, char* regs);
		void GenPacketList(std::vector<GSData>& dump);
		void GenPacketInfo(GSData& dump);
	};


	class PickUserModeDialog : public BaseApplicableDialog
	{
	protected:
		Panels::DocsFolderPickerPanel* m_panel_usersel;
		Panels::LanguageSelectionPanel* m_panel_langsel;

	public:
		PickUserModeDialog(wxWindow* parent);
		virtual ~PickUserModeDialog() = default;

	protected:
		void OnOk_Click(wxCommandEvent& evt);
	};


	class ImportSettingsDialog : public wxDialogWithHelpers
	{
	public:
		ImportSettingsDialog(wxWindow* parent);
		virtual ~ImportSettingsDialog() = default;

	protected:
		void OnImport_Click(wxCommandEvent& evt);
		void OnOverwrite_Click(wxCommandEvent& evt);
	};

	class AssertionDialog : public wxDialogWithHelpers
	{
	public:
		AssertionDialog(const wxString& text, const wxString& stacktrace);
		virtual ~AssertionDialog() = default;
	};

	class IPCDialog : public wxDialogWithHelpers
	{
	public:
		IPCDialog(wxWindow* parent = NULL);
		virtual ~IPCDialog() = default;

		void OnConfirm(wxCommandEvent& evt);
		static wxString GetNameStatic() { return L"IPCSettings"; }
		wxString GetDialogName() const { return GetNameStatic(); }
	};
} // namespace Dialogs

wxWindowID pxIssueConfirmation(wxDialogWithHelpers& confirmDlg, const MsgButtons& buttons);
wxWindowID pxIssueConfirmation(wxDialogWithHelpers& confirmDlg, const MsgButtons& buttons, const wxString& disablerKey);
