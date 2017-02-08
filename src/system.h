#pragma once
#include "common.h"

#include "cell.h"
#include "document.h"
#include "evaluator.h"
#include "grid.h"
#include "selection.h"
#include "text.h"

#include "mycanvas.h"
#include "myframe.h"

namespace treesheets {

	struct Image {
		wxBitmap bm;

		int trefc;
		int savedindex;
		int checksum;

		Image(wxBitmap _bm, int _cs) : bm(_bm), checksum(_cs) {}
		void Scale(float sc);
	};

	class System {
	public:
		MyFrame *frame;

		wxString defaultfont, searchstring;

		wxConfigBase *cfg;

		Evaluator ev;

		wxString clipboardcopy;
		Cell *cellclipboard;

		Vector<Image *> imagelist;
		Vector<int> loadimageids;

		uchar versionlastloaded;
		wxLongLong fakelasteditonload;

		wxPen pen_tinytext, pen_gridborder, pen_tinygridlines, pen_gridlines, pen_thinselect;

		uint customcolor;

		int roundness;
		int defaultmaxcolwidth;

		bool makebaks;
		bool totray;
		bool autosave;
		bool zoomscroll;
		bool thinselc;
		bool minclose;
		bool singletray;
		bool centered;
		bool fswatch;
		bool autohtmlexport;

		int sortcolumn, sortxs, sortdescending;

		bool fastrender;
		wxHashMapBool watchedpaths;

		bool insidefiledialog;

		struct SaveChecker : wxTimer {
			void Notify() {
				g_sys->SaveCheck();
				g_sys->cfg->Flush();
			}
		} savechecker;

		System(bool portable);
		~System();

		Document *NewTabDoc(bool append = false);

		void TabChange(Document *newdoc);

		void Init(const wxString &filename);
		Cell *&InitDB(int sizex, int sizey = 0);
		void LoadTut();
		const char *LoadDB(const wxString &filename, bool frominit = false, bool fromreload = false);

		wxString BakName(const wxString &filename);
		wxString TmpName(const wxString &filename);
		wxString ExtName(const wxString &filename, wxString ext);

		void FileUsed(const wxString &filename, Document *doc);

		const char *Open(const wxString &fn);
		void RememberOpenFiles();

		void UpdateStatus(Selection &s);

		void SaveCheck();
		const char *Import(int k);

		int GetXMLNodes(wxXmlNode *n, Vector<wxXmlNode *> &ns, Vector<wxXmlAttribute *> *ps = NULL,
			bool attributestoo = false);
		void FillXML(Cell *c, wxXmlNode *n, bool attributestoo);

		int CountCol(const wxString &s);

		int FillRows(Grid *g, const wxArrayString &as, int column, int startrow, int starty);

		int AddImageToList(const wxImage &im);
		void ImageSize(wxBitmap *bm, int &xs, int &ys);
		void ImageDraw(wxBitmap *bm, wxDC &dc, int x, int y, int xs, int ys);
	};

}
