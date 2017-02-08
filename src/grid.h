#pragma once
#include "common.h"
#include "declarations.h"

namespace treesheets {

class Grid {
public:
    // owning cell.
    Cell *cell;
    // subcells
    Cell **cells;
    // widths for each column
    int *colwidths;
    // xsize, ysize
    int xs, ys;
    int view_margin, view_grid_outer_spacing, user_grid_outer_spacing, cell_margin;
    int bordercolor;
    bool horiz;
    bool tinyborder;
    bool folded;

    inline Cell *&C(int x, int y) const {
        ASSERT(x >= 0 && y >= 0 && x < xs && y < ys);
        return cells[x + y * xs];
    }

	Grid(int _xs, int _ys, Cell *_c = NULL);
	~Grid();

	void InitCells(Cell *clonestylefrom = NULL);
	void InitColWidths();
	void CloneStyleFrom(Grid *o);
    /* Clones g into this grid. This mutates the grid this function is called on. */
	void Clone(Grid *g);
	Cell *CloneSel(const Selection &s);

	size_t EstimatedMemoryUse();

	void SetOrient();

	bool Layout(Document *doc, wxDC &dc, int depth, int &sx, int &sy, int startx, int starty, bool forcetiny);
	void Render(Document *doc, int bx, int by, wxDC &dc, int depth, int sx, int sy, int xoff, int yoff);

	void FindXY(Document *doc, int px, int py, wxDC &dc);
	Cell *FindLink(Selection &s, Cell *link, Cell *best, bool &lastthis, bool &stylematch, bool forward);
	Cell *FindNextSearchMatch(wxString &search, Cell *best, Cell *selected, bool &lastwasselected);
	void FindReplaceAll(const wxString &str);
	void ReplaceCell(Cell *o, Cell *n);
	Selection FindCell(Cell *o);

	Selection SelectAll();

	void ImageRefCount();

	void DrawHover(Document *doc, wxDC &dc, Selection &s);
	void DrawCursor(Document *doc, wxDC &dc, Selection &s, bool full, uint color, bool cursoronly);
	void DrawInsert(Document *doc, wxDC &dc, Selection &s, uint colour);
	void DrawSelect(Document *doc, wxDC &dc, Selection &s, bool cursoronly);

	wxRect GetRect(Document *doc, Selection &s, bool minimal = false);

	void DeleteCells(int dx, int dy, int nxs, int nys);
	void MultiCellDelete(Document *doc, Selection &s);
	void MultiCellDeleteSub(Document *doc, Selection &s);
	void DelSelf(Document *doc, Selection &s);

	void IdealRelSize(int &rs, bool wantsize = false);

	void InsertCells(int dx, int dy, int nxs, int nys, Cell *nc = NULL);

	void Save(wxDataOutputStream &dos) const;
	bool LoadContents(wxDataInputStream &dis, int &numcells, int &textbytes);

	void Formatter(wxString &r, int format, int indent, const wxChar *xml, const wxChar *html);

	wxString ToText(int indent, const Selection &s, int format, Document *doc);
	wxString ConvertToText(const Selection &s, int indent, int format, Document *doc);

	void RelSize(int dir, int zoomdepth);
	void RelSize(int dir, Selection &s, int zoomdepth);
	int MinRelsize(int rs);

	void SetBorder(int width, Selection &s);

	void ResetChildren();

	void Move(int dx, int dy, Selection &s);

	void Add(Cell *c);

	void MergeWithParent(Grid *p, Selection &s);

	char *SetStyle(Document *doc, Selection &s, int sb);

	void ColorChange(Document *doc, int which, uint color, Selection &s);

	void ReplaceStr(Document *doc, const wxString &str, Selection &s);

	void CSVImport(const wxArrayString &as, wxChar sep);

	Cell *EvalGridCell(Evaluator &ev, Cell *&c, Cell *acc, int &x, int &y, bool &alldata, bool vert);
	Cell *Eval(Evaluator &ev);

	void Split(Vector<Grid *> &gs, bool vert);

	Cell *Sum();
	Cell *Transpose();

	void Sort(Selection &s, bool descending);

	Cell *FindExact(wxString &s);

	Selection HierarchySwap(wxString tag);

	void ReParent(Cell *p);

	Cell *DeleteTagParent(Cell *tag, Cell *basecell, Cell *found);

	void MergeTagCell(Cell *f, Cell *&selcell);
	void MergeTagAll(Cell *into);

	void SetGridTextLayout(int ds, bool vert, bool noset, const Selection &s);

	bool IsTable();

	void Hierarchify(Document *doc);

	void MergeRow(Grid *tm);

	void MaxDepthLeaves(int curdepth, int &maxdepth, int &leaves);

	int Flatten(int curdepth, int cury, Grid *g);

	void ResizeColWidths(int dir, const Selection &s, bool hierarchical);

	void CollectCells(Vector<Cell *> &itercells);
	void CollectCellsSel(Vector<Cell *> &itercells, Selection &s, bool recurse);

	void SetStyles(Selection &s, Cell *o);

	void ClearImages(Selection &s);
};

}
