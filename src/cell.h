#pragma once
#include "common.h"

#include "declarations.h"
#include "text.h"

namespace treesheets {

	/* The evaluation types for a cell.
	CT_DATA: "Data"
	CT_CODE: "Operation"
	CT_VARD: "Variable Assign"
	CT_VARU: "Variable Read"
	CT_VIEWH: "Horizontal View"
	CT_VIEWV: "Vertical View"
	*/
	enum { CT_DATA = 0, CT_CODE, CT_VARD, CT_VIEWH, CT_VARU, CT_VIEWV };

	/* The drawstyles for a cell:

	*/
	enum { DS_GRID, DS_BLOBSHIER, DS_BLOBLINE };

	/**
		The Cell structure represents the editable cells in the sheet.

		They are mutable structures containing a text and grid object. Along with
		formatting information.
	*/
	class Cell {
	public:
		Cell *parent;
		int sx, sy, ox, oy, minx, miny, ycenteroff, txs, tys;
		int celltype;
		Text text;
		Grid *grid;
		uint cellcolor, textcolor, actualcellcolor;
		bool tiny;
		bool verticaltextandgrid;
		wxUint8 drawstyle;

		Cell(Cell *_p = NULL, Cell const *_clonefrom = NULL, int _ct = CT_DATA, Grid *_g = NULL);
		~Cell();

		void Clear();

		inline bool HasText() const { return !text.t.empty(); }
		inline bool HasTextSize() const { return HasText() || text.relsize; }
		inline bool HasTextState() const { return HasTextSize() || text.image; }
		inline bool HasHeader() const { return HasText() || text.image; }
		inline bool HasContent() const { return HasHeader() || grid; }
		bool GridShown(Document *doc) const;
		int MinRelsize();

		size_t EstimatedMemoryUse();

		void Layout(Document *doc, wxDC &dc, int depth, int maxcolwidth, bool forcetiny);
		void Render(Document *doc, int bx, int by, wxDC &dc, int depth, int ml, int mr, int mt, int mb,
			int maxcolwidth, int cell_margin);

		void CloneStyleFrom(Cell const *o);

		/* Clones _p making a new copy of it. This does not mutate the called on cell */
		Cell *Clone(Cell *_p) const;

		bool IsInside(int x, int y) const;
		int GetX(Document *doc) const;
		int GetY(Document *doc) const;
		int Depth() const;
		Cell *Parent(int i);
		Cell *SetParent(Cell *g);

		uint SwapColor(uint c);
		wxString ToText(int indent, const Selection &s, int format, Document *doc);

		void RelSize(int dir, int zoomdepth);

		void Reset();
		void ResetChildren();
		void ResetLayout();

		void LazyLayout(Document *doc, wxDC &dc, int depth, int maxcolwidth, bool forcetiny);

		void AddUndo(Document *doc);

		void Save(wxDataOutputStream &dos) const;

		Grid *AddGrid(int x = 1, int y = 1);
		Cell *LoadGrid(wxDataInputStream &dis, int &numcells, int &textbytes);

		static Cell *LoadWhich(wxDataInputStream &dis, Cell *_p, int &numcells, int &textbytes);

		Cell *Eval(Evaluator &ev);

		void Paste(Document *doc, Cell *c, Selection &s);

		Cell *FindNextSearchMatch(wxString &search, Cell *best, Cell *selected, bool &lastwasselected);
		Cell *FindLink(Selection &s, Cell *link, Cell *best, bool &lastthis, bool &stylematch, bool forward);
		void FindReplaceAll(const wxString &str);
		Cell *FindExact(wxString &s);

		void ImageRefCount();

		void SetBorder(int width);

		void ColorChange(int which, uint color);

		void SetGridTextLayout(int ds, bool vert, bool noset);

		bool IsTag(Document *doc);
		void MaxDepthLeaves(int curdepth, int &maxdepth, int &leaves);

		int ColWidth();

		void CollectCells(Vector<Cell *> &itercells, bool recurse = true);
	};

}
