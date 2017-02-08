#pragma once
#include "common.h"
#include "declarations.h"

namespace treesheets {

	struct Text {
		Cell *cell;

		wxString t;
		int relsize, stylebits, extent;

		Image *image;

		wxDateTime lastedit;
		bool filtered;

		Text();

		wxBitmap *DisplayImage();

		size_t EstimatedMemoryUse();

		double GetNum();

		Cell *SetNum(double d);

		wxString ToText(int indent, const Selection &s, int format);

		void WasEdited();

		int MinRelsize(int rs);
		void RelSize(int dir, int zoomdepth);
		void SetRelSize();
		void TextSize(wxDC &dc, int &sx, int &sy, bool tiny, int &leftoffset, int maxcolwidth);

		bool IsWord(wxChar c);
		wxString GetLinePart(int &i, int p, int l);
		wxString GetLine(int &i, int maxcolwidth);

		bool IsInSearch();

		int Render(Document *doc, int bx, int by, int depth, wxDC &dc, int &leftoffset, int maxcolwidth);

		void FindCursor(Document *doc, int bx, int by, wxDC &dc, Selection &s, int maxcolwidth);
		void DrawCursor(Document *doc, wxDC &dc, Selection &s, bool full, uint color, bool cursoronly, int maxcolwidth);

		void SelectWord(Selection &s);

		bool RangeSelRemove(Selection &s);

		void Insert(Document *doc, const wxString &ins, Selection &s);
		void Delete(Selection &s);
		void Backspace(Selection &s);

		void Key(int k, Selection &s);

		void ReplaceStr(const wxString &str);

		void Clear(Document *doc, Selection &s);

		void HomeEnd(Selection &s, bool home);

		void Save(wxDataOutputStream &dos) const;
		void Load(wxDataInputStream &dis);

		Cell *Eval(Evaluator &ev);

		Cell *Graph();
	};

}
