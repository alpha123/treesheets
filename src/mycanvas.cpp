#include "mycanvas.h"

#include "document.h"
#include "system.h"

namespace treesheets {

	TSCanvas::TSCanvas(MyFrame *fr, wxWindow *parent, const wxSize &size)
		: frame(fr),
		wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, size,
			wxScrolledWindowStyle | wxWANTS_CHARS),
		mousewheelaccum(0),
		doc(NULL),
		lastrmbwaswithctrl(false)
	{
		SetBackgroundStyle(wxBG_STYLE_CUSTOM);
		SetBackgroundColour(*wxWHITE);
		DisableKeyboardScrolling();
		// Without this, ScrolledWindow does its own scrolling upon mousewheel events, which
		// interferes with our own.
		EnableScrolling(false, false);
	}

	TSCanvas::~TSCanvas() {
		DELETEP(doc);
		frame = NULL;
	}

	void TSCanvas::OnPaint(wxPaintEvent &event) {
		auto sz = GetClientSize();
		wxBitmap buffer(sz.GetX(), sz.GetY(), 24);
		wxBufferedPaintDC dc(this, buffer);
		// DoPrepareDC(dc);
		doc->Draw(dc);
	};

	void TSCanvas::UpdateHover(int mx, int my, wxDC &dc) {
		int x, y;
		CalcUnscrolledPosition(mx, my, &x, &y);
		DoPrepareDC(dc);
		doc->Hover(x / doc->currentviewscale, y / doc->currentviewscale, dc);
	}

	void TSCanvas::OnMotion(wxMouseEvent &me) {
		wxClientDC dc(this);
		UpdateHover(me.GetX(), me.GetY(), dc);
		if (me.LeftIsDown() || me.RightIsDown())
			Status(doc->Drag(dc));
		else if (me.MiddleIsDown()) {
			wxPoint p = me.GetPosition() - lastmousepos;
			CursorScroll(-p.x, -p.y);
		}

		lastmousepos = me.GetPosition();
	}

	void TSCanvas::SelectClick(int mx, int my, bool right, int isctrlshift) {
		if (mx < 0 || my < 0)
			return;  // for some reason, using just the "menu" key sends a right-click at (-1, -1)
		wxClientDC dc(this);
		UpdateHover(mx, my, dc);
		Status(doc->Select(dc, right, isctrlshift));
	}

	void TSCanvas::OnLeftDown(wxMouseEvent &me) {
#ifndef __WXMSW__
		// seems to not want to give the sw focus otherwise (thinks its already in focus
		// when its not?)
		if (frame->filter) frame->filter->SetFocus();
#endif
		SetFocus();
		if (me.ShiftDown())
			OnMotion(me);
		else
			SelectClick(me.GetX(), me.GetY(), false, me.CmdDown() + me.AltDown() * 2);
	}

	void TSCanvas::OnLeftUp(wxMouseEvent &me) {
		if (me.CmdDown() || me.AltDown()) doc->SelectUp();
	}

	void TSCanvas::OnRightDown(wxMouseEvent &me) {
		SetFocus();
		SelectClick(me.GetX(), me.GetY(), true, 0);
		lastrmbwaswithctrl = me.CmdDown();
#ifndef __WXMSW__
		me.Skip();  // otherwise EVT_CONTEXT_MENU won't be triggered?
#endif
	}

	void TSCanvas::OnLeftDoubleClick(wxMouseEvent &me) {
		wxClientDC dc(this);
		UpdateHover(me.GetX(), me.GetY(), dc);
		Status(doc->DoubleClick(dc));
	}

	void TSCanvas::OnKeyDown(wxKeyEvent &ce) { ce.Skip(); }
	void TSCanvas::OnChar(wxKeyEvent &ce) {
		/*
		if (g_sys->insidefiledialog)
		{
		ce.Skip();
		return;
		}
		*/

		// Without this check, Alt+F (keyboard menu nav) Alt+1..6 (style changes), Alt+cursor
		// (scrolling) don't work.
		// The 128 makes sure unicode entry on e.g. Polish keyboards still works.
		// (on Linux in particular).
		if (ce.AltDown() && ce.GetUnicodeKey() < 128) {
			ce.Skip();
			return;
		}

		wxClientDC dc(this);
		DoPrepareDC(dc);
		bool unprocessed = false;
		Status(doc->Key(dc, ce.GetUnicodeKey(), ce.GetKeyCode(), ce.AltDown(), ce.CmdDown(),
			ce.ShiftDown(), unprocessed));
		if (unprocessed) ce.Skip();
	}

	void TSCanvas::OnMouseWheel(wxMouseEvent &me) {
		bool ctrl = me.CmdDown();
		if (g_sys->zoomscroll) ctrl = !ctrl;
		wxClientDC dc(this);
		if (me.AltDown() || ctrl || me.ShiftDown()) {
			mousewheelaccum += me.GetWheelRotation();
			int steps = mousewheelaccum / me.GetWheelDelta();
			if (!steps) return;
			mousewheelaccum -= steps * me.GetWheelDelta();

			UpdateHover(me.GetX(), me.GetY(), dc);
			Status(doc->Wheel(dc, steps, me.AltDown(), ctrl, me.ShiftDown()));
		}
		else if (me.GetWheelAxis()) {
			CursorScroll(me.GetWheelRotation() * g_scrollratewheel, 0);
			UpdateHover(me.GetX(), me.GetY(), dc);
		}
		else {
			CursorScroll(0, -me.GetWheelRotation() * g_scrollratewheel);
			UpdateHover(me.GetX(), me.GetY(), dc);
		}
	}

	void TSCanvas::OnSize(wxSizeEvent &se) { doc->Refresh(); }
	void TSCanvas::OnContextMenuClick(wxContextMenuEvent &cme) {
		if (lastrmbwaswithctrl) {
			wxMenu *tagmenu = new wxMenu();
			doc->RecreateTagMenu(*tagmenu);
			PopupMenu(tagmenu);
			delete tagmenu;
		}
		else {
			PopupMenu(frame->editmenupopup);
		}
	}

	void TSCanvas::CursorScroll(int dx, int dy) {
		int x, y;
		GetViewStart(&x, &y);
		x += dx;
		y += dy;
		// EnableScrolling(true, true);
		Scroll(x, y);
		// EnableScrolling(false, false);
	}

	void TSCanvas::Status(const char *msg) {
		if (frame->GetStatusBar() && (!msg || *msg))
			frame->SetStatusText(msg ? wxString::Format(L"%s", msg) : L"", 0);
		// using Format instead of FromAscii since the latter doesn't deal with >128 international
		// ascii chars
	}

}