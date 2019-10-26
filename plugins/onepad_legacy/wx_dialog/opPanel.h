/*  opPanel.h
 *  PCSX2 Dev Team
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#ifndef __OPPANEL_H__
#define __OPPANEL_H__

#include <wx/wx.h>

#include "EmbeddedImage.h"
#include <onepad.h>

enum gui_img {
    img_background = MAX_KEYS, // background pic
    img_left_cursor,
    img_right_cursor,
    img_l_arrow_up,
    img_l_arrow_right,
    img_l_arrow_bottom,
    img_l_arrow_left,
    img_r_arrow_up,
    img_r_arrow_right,
    img_r_arrow_bottom,
    img_r_arrow_left,

    NB_IMG
};

class opPanel : public wxPanel
{
    wxBitmap m_picture[NB_IMG];
    bool m_show_image[NB_IMG];
    int m_left_cursor_x, m_left_cursor_y, m_right_cursor_x, m_right_cursor_y;
    wxDECLARE_EVENT_TABLE();
    void OnPaint(wxPaintEvent &event);

public:
    opPanel(wxWindow *, wxWindowID, const wxPoint &, const wxSize &);
    void HideImg(int);
    void ShowImg(int);
    void MoveJoystick(int, int);
};

#endif // __OPPANEL_H__
