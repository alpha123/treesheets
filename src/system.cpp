#include "system.h"

namespace treesheets {

	void Image::Scale(float sc) {
		bm = wxBitmap(bm.ConvertToImage().Scale(bm.GetWidth() * sc, bm.GetHeight() * sc,
			wxIMAGE_QUALITY_HIGH));
	}

	System::System(bool portable)
		: cfg(portable ? (wxConfigBase *)new wxFileConfig(
			L"", wxT(""), wxGetCwd() + wxT("/TreeSheets.ini"), wxT(""), 0)
			: (wxConfigBase *)new wxConfig(L"TreeSheets")),
		cellclipboard(NULL),
		defaultfont(
#ifdef WIN32
			L"Lucida Sans Unicode"
#else
			L"Verdana"
#endif
		),
		pen_tinytext(wxColour(0x808080ul)),
		pen_gridborder(wxColour(0xb5a6a4)),
		pen_tinygridlines(wxColour(0xf2dcd8)),
		pen_gridlines(wxColour(0xe5b7b0)),
		pen_thinselect(*wxLIGHT_GREY),
		versionlastloaded(0),
		customcolor(0xFFFFFF),
		roundness(3),
		defaultmaxcolwidth(80),
		makebaks(true),
		totray(false),
		autosave(true),
#ifdef __WXMAC__
		fastrender(true),
#else
		fastrender(false),
#endif
		zoomscroll(false),
		thinselc(true),
		minclose(false),
		singletray(false),
		centered(true),
		fswatch(true),
		autohtmlexport(false),
		insidefiledialog(false)
	{
		static const wxDash glpattern[] = { 1, 3 };
		pen_gridlines.SetDashes(2, glpattern);
		pen_gridlines.SetStyle(wxPENSTYLE_USER_DASH);
		static const wxDash tspattern[] = { 2, 4 };
		pen_thinselect.SetDashes(2, tspattern);
		pen_thinselect.SetStyle(wxPENSTYLE_USER_DASH);

		roundness = cfg->Read(L"roundness", roundness);
		defaultfont = cfg->Read(L"defaultfont", defaultfont);
		cfg->Read(L"makebaks", &makebaks, makebaks);
		cfg->Read(L"totray", &totray, totray);
		cfg->Read(L"zoomscroll", &zoomscroll, zoomscroll);
		cfg->Read(L"thinselc", &thinselc, thinselc);
		cfg->Read(L"autosave", &autosave, autosave);
		cfg->Read(L"fastrender", &fastrender, fastrender);
		cfg->Read(L"minclose", &minclose, minclose);
		cfg->Read(L"singletray", &singletray, singletray);
		cfg->Read(L"centered", &centered, centered);
		cfg->Read(L"fswatch", &fswatch, fswatch);
		cfg->Read(L"autohtmlexport", &autohtmlexport, autohtmlexport);

		cfg->Read(L"defaultfontsize", &g_deftextsize, g_deftextsize);

		// fsw.Connect(wxID_ANY, wxID_ANY, wxEVT_FSWATCHER,
		// wxFileSystemWatcherEventHandler(System::OnFileChanged));
	}

	System::~System() {
		DELETEP(cfg);
		DELETEP(cellclipboard);
	}

	Document *System::NewTabDoc(bool append) {
		Document *doc = new Document();
		frame->NewTab(doc, append);
		return doc;
	}

	void System::TabChange(Document *newdoc) {
		// hover = selected = begindrag = Selection();
		newdoc->sw->SetFocus();
		newdoc->UpdateFileName();
	}

	void System::Init(const wxString &filename) {
		ev.Init();

		if (filename.Len()) LoadDB(filename);

		if (!frame->nb->GetPageCount()) {
			int numfiles = cfg->Read(L"numopenfiles", (long)0);
			loop(i, numfiles) {
				wxString fn;
				cfg->Read(wxString::Format(L"lastopenfile_%d", i), &fn);
				LoadDB(fn, true);
			}
		}

		if (!frame->nb->GetPageCount()) LoadTut();

		if (!frame->nb->GetPageCount()) InitDB(10);

		// Refresh();

		frame->bt.Start(400);
		savechecker.Start(1000);
	}

	void System::LoadTut() { LoadDB(frame->GetPath(L"examples/tutorial.cts")); }
	Cell *&System::InitDB(int sizex, int sizey) {
		Cell *c = new Cell(NULL, NULL, CT_DATA, new Grid(sizex, sizey ? sizey : sizex));
		c->cellcolor = 0xCCDCE2;
		c->grid->InitCells();
		Document *doc = NewTabDoc();
		doc->InitWith(c, L"");
		return doc->rootgrid;
	}

	wxString System::BakName(const wxString &filename) { return ExtName(filename, L".bak"); }
	wxString System::TmpName(const wxString &filename) { return ExtName(filename, L".tmp"); }
	wxString System::ExtName(const wxString &filename, wxString ext) {
		wxFileName fn(filename);
		return fn.GetPathWithSep() + fn.GetName() + ext;
	}

	const char *System::LoadDB(const wxString &filename, bool frominit, bool fromreload) {
		wxString fn = filename;
		bool loadedfromtmp = false;

		if (!fromreload) {
			if (frame->GetTabByFileName(filename)) return NULL;  //"this file is already loaded";

			if (::wxFileExists(TmpName(filename))) {
				if (::wxMessageBox(
					L"A temporary autosave file exists, would you like to load it instead?",
					L"Autosave load", wxYES_NO, frame) == wxYES) {
					fn = TmpName(filename);
					loadedfromtmp = true;
				}
			}
		}

		Document *doc = NULL;
		bool anyimagesfailed = false;

		{  // limit destructors
			wxBusyCursor wait;
			wxFFileInputStream fis(fn);
			if (!fis.IsOk()) return "cannot open file";

			char buf[4];
			fis.Read(buf, 4);
			if (strncmp(buf, "TSFF", 4)) return "not a TreeSheets file";
			fis.Read(&versionlastloaded, 1);
			if (versionlastloaded > TS_VERSION) return "file of newer version";

			fakelasteditonload = wxDateTime::Now().GetValue();

			loadimageids.setsize(0);

			for (;;) {
				fis.Read(buf, 1);
				switch (*buf) {
				case 'I': {
					wxDataInputStream dis(fis);
					if (versionlastloaded < 9) dis.ReadString();
					wxImage im;
					off_t beforepng = fis.TellI();
					bool ok = im.LoadFile(fis);
					// ok = false;
					if (!ok) {
						// Uhoh.. the decoder failed. Try to save the situation by skipping this
						// PNG.
						anyimagesfailed = true;
						if (beforepng == wxInvalidOffset) return "cannot tell/seek document?";
						fis.SeekI(beforepng);
						// Now try to skip past this PNG
						uchar header[8];
						fis.Read(header, 8);
						uchar expected[] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
						if (memcmp(header, expected, 8)) return "corrupt PNG header";
						dis.BigEndianOrdered(true);
						for (;;)  // Skip all chunks.
						{
							wxInt32 len = dis.Read32();
							char fourcc[4];
							fis.Read(fourcc, 4);
							fis.SeekI(len, wxFromCurrent);  // skip data
							dis.Read32();                   // skip CRC
							if (memcmp(fourcc, "IEND", 4) == 0) break;
						}
						// Set empty image here, since document expect there to be one here.
						int sz = 32;
						im.Create(sz, sz, false);
						im.SetRGB(wxRect(0, 0, sz, sz), 0xFF, 0,
							0);  // Set to red to indicate error.
					}
					loadimageids.push() = AddImageToList(im);
					break;
				}

				case 'D': {
					wxZlibInputStream zis(fis);
					if (!zis.IsOk()) return "cannot decompress file";
					wxDataInputStream dis(zis);
					int numcells = 0, textbytes = 0;
					Cell *root = Cell::LoadWhich(dis, NULL, numcells, textbytes);
					if (!root) return "file corrupted!";

					doc = NewTabDoc(true);
					if (loadedfromtmp) {
						doc->undolistsizeatfullsave =
							-1;  // if not, user will lose tmp without warning when he closes
						doc->modified = true;
					}
					doc->InitWith(root, filename);

					if (versionlastloaded >= 11) {
						for (;;) {
							wxString s = dis.ReadString();
							if (!s.Len()) break;
							doc->tags.insert(s);
						}
					}

					doc->sw->Status(wxString::Format(L"loaded %s (%d cells, %d characters)",
						filename.c_str(), numcells, textbytes)
						.c_str());

					goto done;
				}

				default: return "corrupt block header";
				}
			}
		}

	done:

		FileUsed(filename, doc);

		doc->ClearSelectionRefresh();

		if (anyimagesfailed)
			wxMessageBox(
				L"PNG decode failed on some images in this document\nThey have been replaced by "
				L"red squares.",
				"PNG decoder failure", wxOK, frame);

		return "";
	}

	void System::FileUsed(const wxString &filename, Document *doc) {
		frame->filehistory.AddFileToHistory(filename);
		RememberOpenFiles();
		if (fswatch) {
			doc->lastmodificationtime = wxFileName(filename).GetModificationTime();
			const wxString &d =
				wxFileName(filename).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			if (watchedpaths.count(d) == 0) {
				watchedpaths.insert(d);
				frame->watcher->Add(wxFileName(d), wxFSW_EVENT_ALL);
			}
		}
	}

	const char *System::Open(const wxString &fn) {
		if (!fn.empty()) {
			const char *msg = LoadDB(fn);
			if (msg && *msg) wxMessageBox(wxString::FromAscii(msg), fn.wx_str(), wxOK, frame);
			return msg;
		}
		return "open file cancelled";
	}

	void System::RememberOpenFiles() {
		int n = frame->nb->GetPageCount();
		int namedfiles = 0;

		loop(i, n) {
			TSCanvas *p = (TSCanvas *)frame->nb->GetPage(i);
			if (p->doc->filename.Len()) {
				cfg->Write(wxString::Format(L"lastopenfile_%d", namedfiles), p->doc->filename);
				namedfiles++;
			}
		}

		cfg->Write(L"numopenfiles", namedfiles);
		cfg->Flush();
	}

	void System::UpdateStatus(Selection &s) {
		if (frame->GetStatusBar()) {
			Cell *c = s.GetCell();
			if (c && s.xs) {
				frame->SetStatusText(wxString::Format(L"Size %d", -c->text.relsize), 3);
				frame->SetStatusText(wxString::Format(L"Width %d", s.g->colwidths[s.x]), 2);
				frame->SetStatusText(
					wxString::Format(L"Edited %s", c->text.lastedit.FormatDate().c_str()), 1);
			}
		}
	}

	void System::SaveCheck() {
		loop(i, frame->nb->GetPageCount()) {
			((TSCanvas *)frame->nb->GetPage(i))->doc->AutoSave(!frame->IsActive(), i);
		}
	}

	const char *System::Import(int k) {
		wxString fn = ::wxFileSelector(L"Please select file to import:", L"", L"", L"", L"*.*",
			wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR);
		if (!fn.empty()) {
			wxBusyCursor wait;
			switch (k) {
			case A_IMPXML:
			case A_IMPXMLA: {
				wxXmlDocument doc;
				if (!doc.Load(fn)) goto problem;
				Cell *&r = InitDB(1);
				Cell *c = *r->grid->cells;
				FillXML(c, doc.GetRoot(), k == A_IMPXMLA);
				if (!c->HasText() && c->grid) {
					*r->grid->cells = NULL;
					delete r;
					r = c;
					c->parent = NULL;
				}
				break;
			}
			case A_IMPTXTI:
			case A_IMPTXTC:
			case A_IMPTXTS:
			case A_IMPTXTT: {
				wxFFile f(fn);
				if (!f.IsOpened()) goto problem;
				wxString s;
				if (!f.ReadAll(&s)) goto problem;
				const wxArrayString &as = wxStringTokenize(s, LINE_SEPERATOR);

				if (as.size()) switch (k) {
				case A_IMPTXTI: {
					Cell *r = InitDB(1);
					FillRows(r->grid, as, CountCol(as[0]), 0, 0);
				}; break;
				case A_IMPTXTC: InitDB(1, as.size())->grid->CSVImport(as, L','); break;
				case A_IMPTXTS: InitDB(1, as.size())->grid->CSVImport(as, L';'); break;
				case A_IMPTXTT: InitDB(1, as.size())->grid->CSVImport(as, L'\t'); break;
				}
				break;
			}
			}
			frame->GetCurTab()->doc->ChangeFileName(fn.BeforeLast(L'.').Append(L".cts"));
			frame->GetCurTab()->doc->ClearSelectionRefresh();
		}
		return NULL;
	problem:
		wxMessageBox(L"couldn't import file!", fn, wxOK, frame);
		return "file load error";
	}

	int System::GetXMLNodes(wxXmlNode *n, Vector<wxXmlNode *> &ns, Vector<wxXmlAttribute *> *ps, bool attributestoo) {
		for (wxXmlNode *child = n->GetChildren(); child; child = child->GetNext()) {
			if (child->GetType() == wxXML_ELEMENT_NODE) ns.push() = child;
		}
		if (attributestoo && ps)
			for (wxXmlAttribute *child = n->GetAttributes(); child; child = child->GetNext()) {
				ps->push() = child;
			}
		return ns.size() + (ps ? ps->size() : 0);
	}

	void System::FillXML(Cell *c, wxXmlNode *n, bool attributestoo) {
		const wxArrayString &as = wxStringTokenize(
			n->GetType() == wxXML_ELEMENT_NODE ? n->GetNodeContent() : n->GetContent());
		loop(i, as.GetCount()) {
			if (c->text.t.Len()) c->text.t.Append(L' ');
			c->text.t.Append(as[i]);
		}

		if (n->GetName() == L"cell") {
			c->text.relsize = -wxAtoi(n->GetAttribute(L"relsize", L"0"));
			c->text.stylebits = wxAtoi(n->GetAttribute(L"stylebits", L"0"));
			c->cellcolor = wxAtoi(n->GetAttribute(L"colorbg", L"16777215"));
			c->textcolor = wxAtoi(n->GetAttribute(L"colorfg", L"0"));
		}

		Vector<wxXmlNode *> ns;
		Vector<wxXmlAttribute *> ps;
		int numrows = GetXMLNodes(n, ns, &ps, attributestoo);
		if (!numrows) return;

		if (ns.size() == 1 && (!c->text.t.Len() || ns[0]->IsWhitespaceOnly()) &&
			ns[0]->GetName() != L"row") {
			FillXML(c, ns[0], attributestoo);
		}
		else {
			bool allrow = n->GetName() == L"grid";
			loopv(i, ns) if (ns[i]->GetName() != L"row") allrow = false;
			if (allrow) {
				int desiredxs;
				loopv(i, ns) {
					Vector<wxXmlNode *> ins;
					int xs = GetXMLNodes(ns[i], ins);
					if (!i) {
						desiredxs = xs ? xs : 1;
						c->AddGrid(desiredxs, ns.size());
					}
					loop(j, desiredxs) if (ins.size() > j)
						FillXML(c->grid->C(j, i), ins[j], attributestoo);
					ins.setsize_nd(0);
				}
			}
			else {
				c->AddGrid(1, numrows);
				loopv(i, ps) c->grid->C(0, i)->text.t = ps[i]->GetValue();
				loopv(i, ns) FillXML(c->grid->C(0, i + ps.size()), ns[i], attributestoo);
			}
		}

		ns.setsize_nd(0);
		ps.setsize_nd(0);
	}

	int System::CountCol(const wxString &s) {
		int col = 0;
		while (s[col] == ' ' || s[col] == '\t') col++;
		return col;
	}

	int System::FillRows(Grid *g, const wxArrayString &as, int column, int startrow, int starty) {
		int y = starty;
		for (int i = startrow; i < (int)as.size(); i++) {
			wxString s = as[i];
			int col = CountCol(s);
			if (col < column && startrow != 0) return i;
			if (col > column) {
				Cell *c = g->C(0, y - 1);
				Grid *sg = c->grid;
				i = FillRows(sg ? sg : c->AddGrid(), as, col, i, sg ? sg->ys : 0) - 1;
			}
			else {
				if (g->ys <= y) g->InsertCells(-1, y, 0, 1);
				Text &t = g->C(0, y)->text;
				t.t = s.Trim(false);
				y++;
			}
		}
		return (int)as.size();
	}

	int System::AddImageToList(const wxImage &im) {
		uint *p = (uint *)im.GetData();
		uint checksum = im.GetWidth() | (im.GetHeight() << 16);
		loop(i, im.GetWidth() * im.GetHeight() * 3 / 4) checksum ^= *p++;

		loopv(i, imagelist) {
			if (imagelist[i]->checksum == checksum) return i;
		}

		imagelist.push() = new Image(wxBitmap(im), checksum);
		return imagelist.size() - 1;
	}

	void System::ImageSize(wxBitmap *bm, int &xs, int &ys) {
		if (!bm) return;
		xs = bm->GetWidth();
		ys = bm->GetHeight();
	}

	void System::ImageDraw(wxBitmap *bm, wxDC &dc, int x, int y, int xs, int ys) {
		double xscale = xs / (double)bm->GetWidth();
		double yscale = ys / (double)bm->GetHeight();
		double prevx, prevy;
		dc.GetUserScale(&prevx, &prevy);
		dc.SetUserScale(xscale * prevx, yscale * prevy);
		dc.DrawBitmap(*bm, x / xscale, y / yscale);
		dc.SetUserScale(prevx, prevy);
	}
}