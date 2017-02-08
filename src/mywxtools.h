#pragma once
#include "common.h"

namespace treesheets {

	void DrawRectangle(wxDC &dc, uint c, int x, int y, int xs, int ys, bool outline = false);
	void DrawLine(wxDC &dc, uint c, int x, int y, int xd, int yd);
	void MyDrawText(wxDC &dc, const wxString &s, wxCoord x, wxCoord y, wxCoord w, wxCoord h);

	struct DropTarget : wxDropTarget {
		DropTarget(wxDataObject *data) : wxDropTarget(data) {};

		wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);

		bool OnDrop(wxCoord x, wxCoord y);
		wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
	};

	struct BlinkTimer : wxTimer {
		void Notify();
	};

	struct ThreeChoiceDialog : public wxDialog {
		ThreeChoiceDialog(wxWindow *parent, const wxString &title, const wxString &msg,
			const wxString &ch1, const wxString &ch2, const wxString &ch3);

		void OnButton(wxCommandEvent &ce);
		int Run();
		DECLARE_EVENT_TABLE()
	};

	struct ColorPopup : wxVListBoxComboPopup {
		ColorPopup(wxWindow *parent) {}

		void OnComboDoubleClick();
	};

	struct ColorDropdown : wxOwnerDrawnComboBox {
		ColorDropdown(wxWindow *parent, wxWindowID id, int sel = 0);

		inline wxCoord OnMeasureItem(size_t item) const { return 24; }
		inline wxCoord OnMeasureItemWidth(size_t item) const { return 40; }

		void OnDrawBackground(wxDC &dc, const wxRect &rect, int item, int flags) const;
		void OnDrawItem(wxDC &dc, const wxRect &rect, int item, int flags) const;
	};

	struct ImagePopup : wxVListBoxComboPopup {
		void OnComboDoubleClick();
	};

	struct ImageDropdown : wxOwnerDrawnComboBox {
		Vector<wxBitmap *> bitmaps;  // FIXME: delete these somewhere
		wxArrayString as;

		ImageDropdown(wxWindow *parent, wxString &path);

		inline wxCoord OnMeasureItem(size_t item) const { return 22; }
		inline wxCoord OnMeasureItemWidth(size_t item) const { return 22; }

		void OnDrawBackground(wxDC &dc, const wxRect &rect, int item, int flags) const;
		void OnDrawItem(wxDC &dc, const wxRect &rect, int item, int flags) const;
	};

}
