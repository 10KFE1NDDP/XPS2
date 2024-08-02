// SPDX-FileCopyrightText: 2002-2024 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "InputRecordingLogger.h"

#include "DebugTools/Debug.h"
#include "common/Console.h"
#include "IconsFontAwesome5.h"
#include "GS.h"
#include "Host.h"

#include <fmt/core.h>

namespace InputRec
{
	void log(const std::string& log)
	{
		if (!log.empty())
		{
			recordingConLog(fmt::format("[REC]: {}\n", log));
			Host::AddIconOSDMessage("input_rec_status", ICON_FA_KEYBOARD, log, Host::OSD_INFO_DURATION);
		}
	}

	void consoleLog(const std::string& log)
	{
		if (!log.empty())
		{
			recordingConLog(fmt::format("[REC]: {}\n", log));
		}
	}

	void consoleMultiLog(const std::vector<std::string>& logs)
	{
		if (!logs.empty())
		{
			std::string log;
			for (std::string l : logs)
			{
				log.append(fmt::format("[REC]: {}\n", l));
			}
			recordingConLog(log);
		}
	}
} // namespace InputRecording
