#include "mywxtools.h"

#include "system.h"
#include "mycanvas.h"

namespace treesheets {

	void DrawRectangle(wxDC &dc, uint c, int x, int y, int xs, int ys, bool outline) {
		if (outline)
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
		else
			dc.SetBrush(wxBrush(wxColour(c)));
		dc.SetPen(wxPen(wxColour(c)));
		dc.DrawRectangle(x, y, xs, ys);
	}

	void DrawLine(wxDC &dc, uint c, int x, int y, int xd, int yd) {
		dc.SetPen(wxPen(wxColour(c)));
		dc.DrawLine(x, y, x + xd, y + yd);
	}

	void MyDrawText(wxDC &dc, const wxString &s, wxCoord x, wxCoord y, wxCoord w, wxCoord h) {
#ifdef __WXMSW__  // this special purpose implementation is because the MSW implementation calls
		// TextExtent, which costs
		// 25% of all cpu time
		dc.CalcBoundingBox(x, y);
		dc.CalcBoundingBox(x + w, y + h);
		HDC hdc = (HDC)dc.GetHDC();
		::SetTextColor(hdc, dc.GetTextForeground().GetPixel());
		::SetBkColor(hdc, dc.GetTextBackground().GetPixel());
		::ExtTextOut(hdc, x, y, 0, NULL, s.c_str(), s.length(), NULL);
#else
		dc.DrawText(s, x, y);
#endif
	}

	wxDragResult DropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def) {
		TSCanvas *sw = g_sys->frame->GetCurTab();
		wxClientDC dc(sw);
		sw->UpdateHover(x, y, dc);
		return sw->doc->hover.g ? wxDragCopy : wxDragNone;
	}

	bool DropTarget::OnDrop(wxCoord x, wxCoord y) { return g_sys->frame->GetCurTab()->doc->hover.g != NULL; }
	wxDragResult DropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def) {
		GetData();
		TSCanvas *sw = g_sys->frame->GetCurTab();
		sw->SelectClick(x, y, false, 0);
		sw->doc->PasteOrDrop();
		return wxDragCopy;
	}

	void BlinkTimer::Notify() {
		TSCanvas *tsc = g_sys->frame->GetCurTab();
		if (tsc) tsc->doc->Blink();
	}

	ThreeChoiceDialog::ThreeChoiceDialog(wxWindow *parent, const wxString &title, const wxString &msg,
		const wxString &ch1, const wxString &ch2, const wxString &ch3)
		: wxDialog(parent, wxID_ANY, title)
	{
		wxBoxSizer *bsv = new wxBoxSizer(wxVERTICAL);
		bsv->Add(new wxStaticText(this, -1, msg), 0, wxALL, 5);
		wxBoxSizer *bsb = new wxBoxSizer(wxHORIZONTAL);
		bsb->Prepend(new wxButton(this, 2, ch3), 0, wxALL, 5);
		bsb->PrependStretchSpacer(1);
		bsb->Prepend(new wxButton(this, 1, ch2), 0, wxALL, 5);
		bsb->PrependStretchSpacer(1);
		bsb->Prepend(new wxButton(this, 0, ch1), 0, wxALL, 5);
		bsv->Add(bsb, 1, wxEXPAND);
		SetSizer(bsv);
		bsv->SetSizeHints(this);
	}

	void ThreeChoiceDialog::OnButton(wxCommandEvent &ce) { EndModal(ce.GetId()); }
	int ThreeChoiceDialog::Run() { return ShowModal(); }

	void ColorPopup::OnComboDoubleClick() {
		g_sys->frame->GetCurTab()->doc->ColorChange(m_combo->GetId(), GetSelection());
	}

	ColorDropdown::ColorDropdown(wxWindow *parent, wxWindowID id, int sel) {
		wxArrayString as;
		as.Add(L"", sizeof(celltextcolors) / sizeof(uint));
		Create(parent, id, L"", wxDefaultPosition, wxSize(44, 24), as,
			wxCB_READONLY | wxCC_SPECIAL_DCLICK);
		SetPopupControl(new ColorPopup(this));
		SetSelection(sel);
		SetPopupMaxHeight(2000);
	}

	void ColorDropdown::OnDrawBackground(wxDC &dc, const wxRect &rect, int item, int flags) const {
		DrawRectangle(dc, 0xFFFFFF, rect.x, rect.y, rect.width, rect.height);
	}

	void ColorDropdown::OnDrawItem(wxDC &dc, const wxRect &rect, int item, int flags) const {
		DrawRectangle(dc, item == CUSTOMCOLORIDX ? g_sys->customcolor : celltextcolors[item],
			rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2);
		if (item == CUSTOMCOLORIDX) {
			dc.SetTextForeground(*wxBLACK);
			dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
				false, L""));
			dc.DrawText(L"Custom", rect.x + 1, rect.y + 1);
		}
	}

	void ImagePopup::OnComboDoubleClick() {
		wxString s = GetString(GetSelection());
		g_sys->frame->GetCurTab()->doc->ImageChange(s);
	}

	ImageDropdown::ImageDropdown(wxWindow *parent, wxString &path) {
		wxString f = wxFindFirstFile(path + L"*.*");
		while (!f.empty()) {
			wxBitmap *bm = new wxBitmap();
			if (bm->LoadFile(f, wxBITMAP_TYPE_PNG)) {
				bitmaps.push() = bm;
				as.Add(f);
			}
			f = wxFindNextFile();
		}
		Create(parent, A_DDIMAGE, L"", wxDefaultPosition, wxSize(44, 24), as,
			wxCB_READONLY | wxCC_SPECIAL_DCLICK);
		SetPopupControl(new ImagePopup());
		SetSelection(0);
		SetPopupMaxHeight(2000);
	}

	void ImageDropdown::OnDrawBackground(wxDC &dc, const wxRect &rect, int item, int flags) const {
		DrawRectangle(dc, 0xFFFFFF, rect.x, rect.y, rect.width, rect.height);
	}

	void ImageDropdown::OnDrawItem(wxDC &dc, const wxRect &rect, int item, int flags) const {
		dc.DrawBitmap(*bitmaps[item], rect.x + 3, rect.y + 3);
	}

}
