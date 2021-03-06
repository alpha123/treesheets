#include "myframe.h"

#include "system.h"

namespace treesheets {

#ifdef WIN32
	static void SetRegKey(wxChar *key, wxString val) {
		wxRegKey rk(key);
		rk.Create();
		rk.SetValue(L"", val);
	}
#endif

	static void SetFileAssoc(wxString &exename) {
#ifdef WIN32
		SetRegKey(L"HKEY_CLASSES_ROOT\\.cts", L"TreeSheets");
		SetRegKey(L"HKEY_CLASSES_ROOT\\TreeSheets", L"TreeSheets file");
		SetRegKey(L"HKEY_CLASSES_ROOT\\TreeSheets\\Shell\\Open\\Command",
			wxString(L"\"") + exename + L"\" \"%1\"");
		SetRegKey(L"HKEY_CLASSES_ROOT\\TreeSheets\\DefaultIcon",
			wxString(L"\"") + exename + L"\",0");
#else
		// TODO: do something similar for mac/kde/gnome?
#endif
	}

	wxString MyFrame::GetPath(const wxString &relpath) {
		if (!exepath_.Length()) return relpath;
		return exepath_ + "/" + relpath;
	}

	void MyFrame::MyAppend(wxMenu *menu, int tag, const wxString &contents, const wxString &help) {
		wxString item = contents;
		wxString key = L"";
		int pos = contents.Find("\t");
		if (pos >= 0) {
			item = contents.Mid(0, pos);
			key = contents.Mid(pos + 1);
		}
		key = g_sys->cfg->Read(item, key);
		wxString newcontents = item;
		if (key.Length()) newcontents += "\t" + key;
		menu->Append(tag, newcontents, help);
		menustrings.push_back(std::make_pair(item, key));
	}

	MyFrame::MyFrame(wxString exename, wxApp *_app)
		: wxFrame((wxFrame *)NULL, wxID_ANY, L"TreeSheets", wxDefaultPosition, wxDefaultSize,
			wxDEFAULT_FRAME_STYLE),
		filter(NULL),
		replaces(NULL),
		tb(NULL),
		nb(NULL),
		idd(NULL),
		refreshhack(0),
		refreshhackinstances(0),
		aui(NULL),
		fromclosebox(true),
		app(_app),
		watcherwaitingforuser(false),
		watcher(NULL)
	{
		g_sys->frame = this;

		exepath_ = wxFileName(exename).GetPath();
#ifdef __WXMAC__
		int cut = exepath_.Find("/MacOS");
		if (cut > 0) { exepath_ = exepath_.SubString(0, cut) + "/Resources"; }
#endif

		class MyLog : public wxLog {
			void DoLogString(const wxChar *msg, time_t timestamp) { DoLogText(*msg); }
			void DoLogText(const wxString &msg) {
#ifdef WIN32
				OutputDebugString(msg.c_str());
				OutputDebugString(L"\n");
#else
				fputws(msg.c_str(), stderr);
				fputws(L"\n", stderr);
#endif
			}
		};

		wxLog::SetActiveTarget(new MyLog());

		wxInitAllImageHandlers();

		wxIconBundle icons;
		wxIcon iconbig;
		icon.LoadFile(GetPath(L"images/icon16.png"), wxBITMAP_TYPE_PNG);
		iconbig.LoadFile(GetPath(L"images/icon32.png"), wxBITMAP_TYPE_PNG);
#ifdef WIN32
		int iconsmall = ::GetSystemMetrics(SM_CXSMICON);
		int iconlarge = ::GetSystemMetrics(SM_CXICON);
		icon.SetSize(iconsmall, iconsmall);  // this shouldn't be necessary...
		iconbig.SetSize(iconlarge, iconlarge);
#endif
		icons.AddIcon(icon);
		icons.AddIcon(iconbig);
		SetIcons(icons);

		wxImage foldiconi;
		if (!line_nw.LoadFile(GetPath(L"images/render/line_nw.png"), wxBITMAP_TYPE_PNG) ||
			!line_sw.LoadFile(GetPath(L"images/render/line_sw.png"), wxBITMAP_TYPE_PNG) ||
			!foldiconi.LoadFile(GetPath(L"images/nuvola/fold.png"))) {
			wxMessageBox(_(L"Error loading core data file (TreeSheets not installed correctly?)"),
				_(L"Initialization Error"), wxOK, this);
			// FIXME: what is the correct way to exit?
		}
		foldicon = wxBitmap(foldiconi);

		if (g_sys->singletray)
			tbi.Connect(wxID_ANY, wxEVT_TASKBAR_LEFT_UP,
				wxTaskBarIconEventHandler(MyFrame::OnTBIDBLClick), NULL, this);
		else
			tbi.Connect(wxID_ANY, wxEVT_TASKBAR_LEFT_DCLICK,
				wxTaskBarIconEventHandler(MyFrame::OnTBIDBLClick), NULL, this);

		aui = new wxAuiManager(this);

		bool mergetbar = false;

		bool showtbar, showsbar, lefttabs, iconset;

		g_sys->cfg->Read(L"showtbar", &showtbar, true);
		g_sys->cfg->Read(L"showsbar", &showsbar, true);
		g_sys->cfg->Read(L"lefttabs", &lefttabs, true);
		g_sys->cfg->Read(L"iconset", &iconset, false);

		filehistory.Load(*g_sys->cfg);

		wxMenu *expmenu = new wxMenu();
		MyAppend(expmenu, A_EXPXML, _(L"&XML..."),
			_(L"Export the current view as XML (which can also be reimported without losing "
			  L"structure)"));
		MyAppend(expmenu, A_EXPHTMLT, _(L"&HTML (Tables+Styling)..."),
			_(L"Export the current view as HTML using nested tables, that will look somewhat "
			  L"like the TreeSheet"));
		MyAppend(expmenu, A_EXPHTMLO, _(L"HTML (&Outline)..."),
			_(L"Export the current view as HTML as nested headers, suitable for importing into "
			  L"Word's outline mode"));
		MyAppend(expmenu, A_EXPTEXT, _(L"Indented &Text..."),
			_(L"Export the current view as tree structured text, using spaces for each "
			  L"indentation level. "
			  L"Suitable for importing into mindmanagers and general text programs"));
		MyAppend(expmenu, A_EXPCSV, _(L"&Comma delimited text (CSV)..."),
			_(L"Export the current view as CSV. Good for spreadsheets and databases. Only works "
			  L"on grids with no sub-grids (use the Flatten operation first if need be)"));
		MyAppend(expmenu, A_EXPIMAGE, _(L"&Image..."),
			_(L"Export the current view as an image. Useful for faithfull renderings of the "
			  L"TreeSheet, and programs that don't accept any of the above options"));

		wxMenu *impmenu = new wxMenu();
		MyAppend(impmenu, A_IMPXML, _(L"XML..."));
		MyAppend(impmenu, A_IMPXMLA, _(L"XML (attributes too, for OPML etc)..."));
		MyAppend(impmenu, A_IMPTXTI, _(L"Indented text..."));
		MyAppend(impmenu, A_IMPTXTC, _(L"Comma delimited text (CSV)..."));
		MyAppend(impmenu, A_IMPTXTS, _(L"Semi-Colon delimited text (CSV)..."));
		MyAppend(impmenu, A_IMPTXTT, _(L"Tab delimited text..."));

		wxMenu *recentmenu = new wxMenu();
		filehistory.UseMenu(recentmenu);
		filehistory.AddFilesToMenu();

		wxMenu *filemenu = new wxMenu();
		MyAppend(filemenu, A_NEW, _(L"&New\tCTRL+n"));
		MyAppend(filemenu, A_OPEN, _(L"&Open...\tCTRL+o"));
		MyAppend(filemenu, A_CLOSE, _(L"&Close\tCTRL+w"));
		filemenu->AppendSubMenu(recentmenu, _(L"&Recent files"));
		MyAppend(filemenu, A_SAVE, _(L"&Save\tCTRL+s"));
		MyAppend(filemenu, A_SAVEAS, _(L"Save &As..."));
		filemenu->AppendSeparator();
		MyAppend(filemenu, A_PAGESETUP, _(L"Page Setup..."));
		MyAppend(filemenu, A_PRINTSCALE, _(L"Set Print Scale..."));
		MyAppend(filemenu, A_PREVIEW, _(L"Print preview..."));
		MyAppend(filemenu, A_PRINT, _(L"&Print...\tCTRL+p"));
		filemenu->AppendSeparator();
		filemenu->AppendSubMenu(expmenu, _(L"Export &view as"));
		filemenu->AppendSubMenu(impmenu, _(L"Import file from"));
		filemenu->AppendSeparator();
		MyAppend(filemenu, A_EXIT, _(L"&Exit\tCTRL+q"));

		wxMenu *editmenu;
		loop(twoeditmenus, 2) {
			wxMenu *sizemenu = new wxMenu();
			MyAppend(sizemenu, A_INCSIZE, _(L"&Increase text size (SHIFT+mousewheel)\tSHIFT+PGUP"));
			MyAppend(sizemenu, A_DECSIZE, _(L"&Decrease text size (SHIFT+mousewheel)\tSHIFT+PGDN"));
			MyAppend(sizemenu, A_RESETSIZE, _(L"&Reset text sizes\tSHIFT+CTRL+s"));
			MyAppend(sizemenu, A_MINISIZE, _(L"&Shrink text of all sub-grids\tSHIFT+CTRL+m"));
			sizemenu->AppendSeparator();
			MyAppend(sizemenu, A_INCWIDTH, _(L"Increase column width (ALT+mousewheel)\tALT+PGUP"));
			MyAppend(sizemenu, A_DECWIDTH, _(L"Decrease column width (ALT+mousewheel)\tALT+PGDN"));
			MyAppend(sizemenu, A_INCWIDTHNH,
				L"Increase column width (no sub grids)\tCTRL+ALT+PGUP");
			MyAppend(sizemenu, A_DECWIDTHNH,
				L"Decrease column width (no sub grids)\tCTRL+ALT+PGDN");
			MyAppend(sizemenu, A_RESETWIDTH, _(L"Reset column widths\tSHIFT+CTRL+w"));

			wxMenu *bordmenu = new wxMenu();
			MyAppend(bordmenu, A_BORD0, _(L"&0\tCTRL+SHIFT+0"));
			MyAppend(bordmenu, A_BORD1, _(L"&1\tCTRL+SHIFT+1"));
			MyAppend(bordmenu, A_BORD2, _(L"&2\tCTRL+SHIFT+2"));
			MyAppend(bordmenu, A_BORD3, _(L"&3\tCTRL+SHIFT+3"));
			MyAppend(bordmenu, A_BORD4, _(L"&4\tCTRL+SHIFT+4"));
			MyAppend(bordmenu, A_BORD5, _(L"&5\tCTRL+SHIFT+5"));

			wxMenu *selmenu = new wxMenu();
			MyAppend(selmenu, A_NEXT, _(L"Move to next cell\tTAB"));
			MyAppend(selmenu, A_PREV, _(L"Move to previous cell\tSHIFT+TAB"));
			selmenu->AppendSeparator();
			MyAppend(selmenu, A_SELALL, _(L"Select &all in current grid\tCTRL+a"));
			selmenu->AppendSeparator();
			MyAppend(selmenu, A_LEFT, _(L"Move Selection Left\tLEFT"));
			MyAppend(selmenu, A_RIGHT, _(L"Move Selection Right\tRIGHT"));
			MyAppend(selmenu, A_UP, _(L"Move Selection Up\tUP"));
			MyAppend(selmenu, A_DOWN, _(L"Move Selection Down\tDOWN"));
			selmenu->AppendSeparator();
			MyAppend(selmenu, A_MLEFT, _(L"Move Cells Left\tCTRL+LEFT"));
			MyAppend(selmenu, A_MRIGHT, _(L"Move Cells Right\tCTRL+RIGHT"));
			MyAppend(selmenu, A_MUP, _(L"Move Cells Up\tCTRL+UP"));
			MyAppend(selmenu, A_MDOWN, _(L"Move Cells Down\tCTRL+DOWN"));
			selmenu->AppendSeparator();
			MyAppend(selmenu, A_SLEFT, _(L"Extend Selection Left\tSHIFT+LEFT"));
			MyAppend(selmenu, A_SRIGHT, _(L"Extend Selection Right\tSHIFT+RIGHT"));
			MyAppend(selmenu, A_SUP, _(L"Extend Selection Up\tSHIFT+UP"));
			MyAppend(selmenu, A_SDOWN, _(L"Extend Selection Down\tSHIFT+DOWN"));
			MyAppend(selmenu, A_SCOLS, _(L"Extend Selection Full Columns"));
			MyAppend(selmenu, A_SROWS, _(L"Extend Selection Full Rows"));
			selmenu->AppendSeparator();
			MyAppend(selmenu, A_CANCELEDIT, _(L"Select &Parent\tESC"));
			MyAppend(selmenu, A_ENTERGRID, _(L"Select First &Child\tSHIFT+ENTER"));
			selmenu->AppendSeparator();
			MyAppend(selmenu, A_LINK, _(L"Go To &Matching Cell\tF6"));
			MyAppend(selmenu, A_LINKREV, _(L"Go To Matching Cell (Reverse)\tSHIFT+F6"));

			wxMenu *temenu = new wxMenu();
			MyAppend(temenu, A_LEFT, _(L"Cursor Left\tLEFT"));
			MyAppend(temenu, A_RIGHT, _(L"Cursor Right\tRIGHT"));
			MyAppend(temenu, A_MLEFT, _(L"Word Left\tCTRL+LEFT"));
			MyAppend(temenu, A_MRIGHT, _(L"Word Right\tCTRL+RIGHT"));
			temenu->AppendSeparator();
			MyAppend(temenu, A_SLEFT, _(L"Extend Selection Left\tSHIFT+LEFT"));
			MyAppend(temenu, A_SRIGHT, _(L"Extend Selection Right\tSHIFT+RIGHT"));
			MyAppend(temenu, A_SCLEFT, _(L"Extend Selection Word Left\tSHIFT+CTRL+LEFT"));
			MyAppend(temenu, A_SCRIGHT, _(L"Extend Selection Word Right\tSHIFT+CTRL+RIGHT"));
			MyAppend(temenu, A_SHOME, _(L"Extend Selection to Start\tSHIFT+HOME"));
			MyAppend(temenu, A_SEND, _(L"Extend Selection to End\tSHIFT+END"));
			temenu->AppendSeparator();
			MyAppend(temenu, A_HOME, _(L"Start of line of text\tHOME"));
			MyAppend(temenu, A_END, _(L"End of line of text\tEND"));
			MyAppend(temenu, A_CHOME, _(L"Start of text\tCTRL+HOME"));
			MyAppend(temenu, A_CEND, _(L"End of text\tCTRL+END"));
			temenu->AppendSeparator();
			MyAppend(temenu, A_ENTERCELL, _(L"Enter/exit text edit mode\tENTER"));
			MyAppend(temenu, A_ENTERCELL, _(L"Enter/exit text edit mode\tF2"));
			MyAppend(temenu, A_CANCELEDIT, _(L"Cancel text edits\tESC"));

			wxMenu *stmenu = new wxMenu();
			MyAppend(stmenu, A_BOLD, _(L"Toggle cell &BOLD\tCTRL+b"));
			MyAppend(stmenu, A_ITALIC, _(L"Toggle cell &ITALIC\tCTRL+i"));
			MyAppend(stmenu, A_TT, _(L"Toggle cell &typewriter"));
			MyAppend(stmenu, A_UNDERL, _(L"Toggle cell &underlined\tCTRL+u"));
			MyAppend(stmenu, A_STRIKET, _(L"Toggle cell &strikethrough\tCTRL+t"));
			stmenu->AppendSeparator();
			MyAppend(stmenu, A_RESETSTYLE, _(L"&Reset text styles\tSHIFT+CTRL+r"));
			MyAppend(stmenu, A_RESETCOLOR, _(L"Reset &colors\tSHIFT+CTRL+c"));

			wxMenu *tagmenu = new wxMenu();
			MyAppend(tagmenu, A_TAGADD, _(L"&Add Cell Text as Tag"));
			MyAppend(tagmenu, A_TAGREMOVE, _(L"&Remove Cell Text from Tags"));
			MyAppend(tagmenu, A_NOP, _(L"&Set Cell Text to tag (use CTRL+RMB)"),
				_(L"Hold CTRL while pressing right mouse button to quickly set a tag for the "
				  L"current cell using a popup menu"));

			wxMenu *orgmenu = new wxMenu();
			MyAppend(orgmenu, A_TRANSPOSE, _(L"&Transpose\tSHIFT+CTRL+t"),
				_(L"changes the orientation of a grid"));
			MyAppend(orgmenu, A_SORT, _(L"Sort &Ascending"),
				_(L"Make a 1xN selection to indicate which column to sort on, and which rows to "
				  L"affect"));
			MyAppend(orgmenu, A_SORTD, _(L"Sort &Descending"),
				_(L"Make a 1xN selection to indicate which column to sort on, and which rows to "
				  L"affect"));
			MyAppend(orgmenu, A_HSWAP, _(L"Hierarchy &Swap\tF8"),
				_(L"Swap all cells with this text at this level (or above) with the parent"));
			MyAppend(orgmenu, A_HIFY, _(L"&Hierarchify"),
				_(L"Convert an NxN grid with repeating elements per column into an 1xN grid "
				  L"with hierarchy, useful to convert data from spreadsheets"));
			MyAppend(orgmenu, A_FLATTEN, _(L"&Flatten"),
				_(L"Takes a hierarchy (nested 1xN or Nx1 grids) and converts it into a flat NxN "
				  L"grid, useful for export to spreadsheets"));

			wxMenu *imgmenu = new wxMenu();
			MyAppend(imgmenu, A_IMAGE, _(L"&Add Image"), _(L"Adds an image to the selected cell"));
			MyAppend(imgmenu, A_IMAGESC, _(L"&Scale Image"),
				_(L"Change the image size if it is too big or too small"));
			MyAppend(imgmenu, A_IMAGER, _(L"&Remove Image(s)"),
				_(L"Remove image(s) from the selected cells"));

			wxMenu *navmenu = new wxMenu();
			MyAppend(navmenu, A_BROWSE, _(L"Open link in &browser\tF5"),
				_(L"Opens up the text from the selected cell in browser (should start be a "
				  L"valid URL)"));
			MyAppend(navmenu, A_BROWSEF, _(L"Open &file\tF4"),
				_(L"Opens up the text from the selected cell in default application for the "
				  L"file type"));

			wxMenu *laymenu = new wxMenu();
			MyAppend(laymenu, A_V_GS, _(L"Vertical Layout with Grid Style Rendering\tALT+1"));
			MyAppend(laymenu, A_V_BS, _(L"Vertical Layout with Bubble Style Rendering\tALT+2"));
			MyAppend(laymenu, A_V_LS, _(L"Vertical Layout with Line Style Rendering\tALT+3"));
			laymenu->AppendSeparator();
			MyAppend(laymenu, A_H_GS, _(L"Horizontal Layout with Grid Style Rendering\tALT+4"));
			MyAppend(laymenu, A_H_BS, _(L"Horizontal Layout with Bubble Style Rendering\tALT+5"));
			MyAppend(laymenu, A_H_LS, _(L"Horizontal Layout with Line Style Rendering\tALT+6"));
			laymenu->AppendSeparator();
			MyAppend(laymenu, A_GS, _(L"Grid Style Rendering\tALT+7"));
			MyAppend(laymenu, A_BS, _(L"Bubble Style Rendering\tALT+8"));
			MyAppend(laymenu, A_LS, _(L"Line Style Rendering\tALT+9"));
			laymenu->AppendSeparator();
			MyAppend(laymenu, A_TEXTGRID, _(L"Toggle Vertical Layout\tF7"),
				_(L"Make a hierarchy layout more vertical (default) or more horizontal"));

			editmenu = new wxMenu();
			MyAppend(editmenu, A_CUT, _(L"Cu&t\tCTRL+x"));
			MyAppend(editmenu, A_COPY, _(L"&Copy\tCTRL+c"));
			MyAppend(editmenu, A_COPYCT, _(L"Copy As Continuous Text"));
			MyAppend(editmenu, A_PASTE, _(L"&Paste\tCTRL+v"));
			MyAppend(editmenu, A_PASTESTYLE, _(L"Paste Style Only\tCTRL+SHIFT+v"),
				_(L"only sets the colors and style of the copied cell, and keeps the text"));
			editmenu->AppendSeparator();
			MyAppend(editmenu, A_UNDO, _(L"&Undo\tCTRL+z"), _(L"revert the changes, one step at a time"));
			MyAppend(editmenu, A_REDO, _(L"&Redo\tCTRL+y"),
				_(L"redo any undo steps, if you haven't made changes since"));
			editmenu->AppendSeparator();
			MyAppend(editmenu, A_DELETE, _(L"&Delete After\tDEL"),
				_(L"Deletes the column of cells after the selected grid line, or the row below"));
			MyAppend(
				editmenu, A_BACKSPACE, _(L"Delete Before\tBACK"),
				_(L"Deletes the column of cells before the selected grid line, or the row above"));
			editmenu->AppendSeparator();
			MyAppend(editmenu, A_NEWGRID,
#ifdef __WXMAC__
				_(L"&Insert New Grid\tCTRL+g")
#else
				_(L"&Insert New Grid\tINS")
#endif
				, _(L"Adds a grid to the selected cell"));
			MyAppend(editmenu, A_WRAP, _(L"&Wrap in new parent\tF9"),
				_(L"Creates a new level of hierarchy around the current selection"));
			editmenu->AppendSeparator();
			// F10 is tied to the OS on both Ubuntu and OS X, and SHIFT+F10 is now right
			// click on all platforms?
			MyAppend(editmenu, A_FOLD,
#ifndef WIN32
				_(L"Toggle Fold\tCTRL+F10")
#else
				_(L"Toggle Fold\tF10")
#endif
				, _(L"Toggles showing the grid of the selected cell(s)"));
			MyAppend(editmenu, A_FOLDALL, L"Fold All\tCTRL+SHIFT+F10",
				_(L"Folds the grid of the selected cell(s) recursively"));
			MyAppend(editmenu, A_UNFOLDALL, L"Unfold All\tCTRL+ALT+F10",
				_(L"Unfolds the grid of the selected cell(s) recursively"));
			editmenu->AppendSeparator();
			editmenu->AppendSubMenu(selmenu, _(L"&Selection..."));
			editmenu->AppendSubMenu(orgmenu, _(L"&Grid Reorganization..."));
			editmenu->AppendSubMenu(laymenu, _(L"&Layout && Render Style..."));
			editmenu->AppendSubMenu(imgmenu, _(L"&Images..."));
			editmenu->AppendSubMenu(navmenu, _(L"&Browsing..."));
			editmenu->AppendSubMenu(temenu, _(L"Text &Editing..."));
			editmenu->AppendSubMenu(sizemenu, _(L"Text Sizing..."));
			editmenu->AppendSubMenu(stmenu, _(L"Text Style..."));
			editmenu->AppendSubMenu(bordmenu, _(L"Set Grid Border Width..."));
			editmenu->AppendSubMenu(tagmenu, _(L"Tag..."));

			if (!twoeditmenus) editmenupopup = editmenu;
		}

		wxMenu *semenu = new wxMenu();
		MyAppend(semenu, A_SEARCHF, _(L"&Search\tCTRL+f"));
		MyAppend(semenu, A_SEARCHNEXT, _(L"&Go To Next Search Result\tF3"));
		MyAppend(semenu, A_REPLACEONCE, _(L"&Replace in Current Selection\tCTRL+h"));
		MyAppend(semenu, A_REPLACEONCEJ, _(L"&Replace in Current Selection & Jump Next\tCTRL+j"));
		MyAppend(semenu, A_REPLACEALL, _(L"Replace &All"));

		wxMenu *scrollmenu = new wxMenu();
		MyAppend(scrollmenu, A_AUP, _(L"Scroll Up (mousewheel)\tPGUP"));
		MyAppend(scrollmenu, A_AUP, _(L"Scroll Up (mousewheel)\tALT+UP"));
		MyAppend(scrollmenu, A_ADOWN, _(L"Scroll Down (mousewheel)\tPGDN"));
		MyAppend(scrollmenu, A_ADOWN, _(L"Scroll Down (mousewheel)\tALT+DOWN"));
		MyAppend(scrollmenu, A_ALEFT, _(L"Scroll Left\tALT+LEFT"));
		MyAppend(scrollmenu, A_ARIGHT, _(L"Scroll Right\tALT+RIGHT"));

		wxMenu *filtermenu = new wxMenu();
		MyAppend(filtermenu, A_FILTEROFF, _(L"Turn filter &off"));
		MyAppend(filtermenu, A_FILTERS, _(L"Show only cells in current search"));
		MyAppend(filtermenu, A_FILTER5, _(L"Show 5% of last edits"));
		MyAppend(filtermenu, A_FILTER10, _(L"Show 10% of last edits"));
		MyAppend(filtermenu, A_FILTER20, _(L"Show 20% of last edits"));
		MyAppend(filtermenu, A_FILTER50, _(L"Show 50% of last edits"));
		MyAppend(filtermenu, A_FILTERM, _(L"Show 1% more than the last filter"));
		MyAppend(filtermenu, A_FILTERL, _(L"Show 1% less than the last filter"));

		wxMenu *viewmenu = new wxMenu();
		MyAppend(viewmenu, A_ZOOMIN, _(L"Zoom &In (CTRL+mousewheel)\tCTRL+PGUP"));
		MyAppend(viewmenu, A_ZOOMOUT, _(L"Zoom &Out (CTRL+mousewheel)\tCTRL+PGDN"));
		MyAppend(viewmenu, A_NEXTFILE,
#ifndef __WXGTK__
			// On Linux, this conflicts with CTRL+I, see Document::Key()
			// CTRL+SHIFT+TAB below still works, so that will have to be used to switch tabs.
			_(L"Switch to &next file/tab\tCTRL+TAB")
#else
			_(L"Switch to &next file/tab")
#endif
		);
		MyAppend(viewmenu, A_PREVFILE, _(L"Switch to &previous file/tab\tSHIFT+CTRL+TAB"));
		MyAppend(viewmenu, A_FULLSCREEN,
#ifdef __WXMAC__
			_(L"Toggle &Fullscreen View\tCTRL+F11")
#else
			_(L"Toggle &Fullscreen View\tF11")
#endif
		);
		MyAppend(viewmenu, A_SCALED,
#ifdef __WXMAC__
			_(L"Toggle &Scaled Presentation View\tCTRL+F12")
#else
			_(L"Toggle &Scaled Presentation View\tF12")
#endif
		);
		viewmenu->AppendSubMenu(scrollmenu, _(L"Scroll Sheet..."));
		viewmenu->AppendSubMenu(filtermenu, _(L"Filter..."));

		wxMenu *roundmenu = new wxMenu();
		roundmenu->AppendRadioItem(A_ROUND0, _(L"Radius &0"));
		roundmenu->AppendRadioItem(A_ROUND1, _(L"Radius &1"));
		roundmenu->AppendRadioItem(A_ROUND2, _(L"Radius &2"));
		roundmenu->AppendRadioItem(A_ROUND3, _(L"Radius &3"));
		roundmenu->AppendRadioItem(A_ROUND4, _(L"Radius &4"));
		roundmenu->AppendRadioItem(A_ROUND5, _(L"Radius &5"));
		roundmenu->AppendRadioItem(A_ROUND6, _(L"Radius &6"));
		roundmenu->Check(g_sys->roundness + A_ROUND0, true);

		wxMenu *optmenu = new wxMenu();
		MyAppend(optmenu, A_DEFFONT, _(L"Pick Default Font..."));
		MyAppend(optmenu, A_CUSTKEY, _(L"Change a key binding..."));
		MyAppend(optmenu, A_CUSTCOL, _(L"Pick Custom &Color..."));
		MyAppend(optmenu, A_COLCELL, _(L"&Set Custom Color From Cell BG"));
		MyAppend(optmenu, A_DEFBGCOL, _(L"Pick Document Background..."));
		optmenu->AppendSeparator();
		optmenu->AppendCheckItem(A_SHOWSBAR, _(L"Show Statusbar"));
		optmenu->Check(A_SHOWSBAR, showsbar);
		optmenu->AppendCheckItem(A_SHOWTBAR, _(L"Show Toolbar"));
		optmenu->Check(A_SHOWTBAR, showtbar);
		optmenu->AppendCheckItem(A_LEFTTABS, _(L"File Tabs on the bottom"));
		optmenu->Check(A_LEFTTABS, lefttabs);
		optmenu->AppendCheckItem(A_TOTRAY, _(L"Minimize to tray"));
		optmenu->Check(A_TOTRAY, g_sys->totray);
		optmenu->AppendCheckItem(A_MINCLOSE, _(L"Minimize on close"));
		optmenu->Check(A_MINCLOSE, g_sys->minclose);
		optmenu->AppendCheckItem(A_SINGLETRAY, _(L"Single click maximize from tray"));
		optmenu->Check(A_SINGLETRAY, g_sys->singletray);
		optmenu->AppendSeparator();
		optmenu->AppendCheckItem(A_ZOOMSCR, _(L"Swap mousewheel scrolling and zooming"));
		optmenu->Check(A_ZOOMSCR, g_sys->zoomscroll);
		optmenu->AppendCheckItem(A_THINSELC, _(L"Navigate in between cells with cursor keys"));
		optmenu->Check(A_THINSELC, g_sys->thinselc);
		optmenu->AppendSeparator();
		optmenu->AppendCheckItem(A_MAKEBAKS, _(L"Create .bak files"));
		optmenu->Check(A_MAKEBAKS, g_sys->makebaks);
		optmenu->AppendCheckItem(A_AUTOSAVE, _(L"Autosave to .tmp"));
		optmenu->Check(A_AUTOSAVE, g_sys->autosave);
		optmenu->AppendCheckItem(
			A_FSWATCH, _(L"Auto reload documents"),
			_(L"Reloads when another computer has changed a file (if you have made changes, asks)"));
		optmenu->Check(A_FSWATCH, g_sys->fswatch);
		optmenu->AppendCheckItem(A_AUTOEXPORT, _(L"Automatically export a .html on every save"));
		optmenu->Check(A_AUTOEXPORT, g_sys->autohtmlexport);
		optmenu->AppendSeparator();
		optmenu->AppendCheckItem(A_CENTERED, _(L"Render document centered"));
		optmenu->Check(A_CENTERED, g_sys->centered);
		optmenu->AppendCheckItem(A_FASTRENDER, _(L"Faster line rendering"));
		optmenu->Check(A_FASTRENDER, g_sys->fastrender);
		optmenu->AppendCheckItem(A_ICONSET, _(L"black and white toolbar icons"));
		optmenu->Check(A_ICONSET, iconset);
		optmenu->AppendSubMenu(roundmenu, _(L"&Roundness of grid borders..."));

		wxMenu *markmenu = new wxMenu();
		MyAppend(markmenu, A_MARKDATA, _(L"&Data"));
		MyAppend(markmenu, A_MARKCODE, _(L"&Operation"));
		MyAppend(markmenu, A_MARKVARD, _(L"Variable &Assign"));
		MyAppend(markmenu, A_MARKVARU, _(L"Variable &Read"));
		MyAppend(markmenu, A_MARKVIEWH, _(L"&Horizontal View"));
		MyAppend(markmenu, A_MARKVIEWV, _(L"&Vertical View"));

		wxMenu *langmenu = new wxMenu();
		MyAppend(langmenu, A_RUN, _(L"&Run"));
		langmenu->AppendSubMenu(markmenu, _(L"&Mark as"));
		MyAppend(langmenu, A_CLRVIEW, _(L"&Clear Views"));

		wxMenu *helpmenu = new wxMenu();
		MyAppend(helpmenu, A_ABOUT, _(L"&About..."));
		MyAppend(helpmenu, A_HELPI, _(L"Load interactive &tutorial...\tF1"));
		MyAppend(helpmenu, A_HELP, _(L"View tutorial &web page..."));

		wxAcceleratorEntry entries[3];
		entries[0].Set(wxACCEL_SHIFT, WXK_DELETE, A_CUT);
		entries[1].Set(wxACCEL_SHIFT, WXK_INSERT, A_PASTE);
		entries[2].Set(wxACCEL_CTRL, WXK_INSERT, A_COPY);
		wxAcceleratorTable accel(3, entries);
		SetAcceleratorTable(accel);

		if (!mergetbar) {
			wxMenuBar *menubar = new wxMenuBar();
			menubar->Append(filemenu, _(L"&File"));
			menubar->Append(editmenu, _(L"&Edit"));
			menubar->Append(semenu, _(L"&Search"));
			menubar->Append(viewmenu, _(L"&View"));
			menubar->Append(optmenu, _(L"&Options"));
			menubar->Append(langmenu, _(L"&Program"));
			menubar->Append(helpmenu,
#ifdef __WXMAC__
				wxApp::s_macHelpMenuTitleName  // so merges with osx provided help
#else
				_(L"&Help")
#endif
			);
#ifdef __WXMAC__
			// these don't seem to work anymore in the newer wxWidgets, handled in the menu event
			// handler below instead
			wxApp::s_macAboutMenuItemId = A_ABOUT;
			wxApp::s_macExitMenuItemId = A_EXIT;
			wxApp::s_macPreferencesMenuItemId =
				A_DEFFONT;  // we have no prefs, so for now just select the font
#endif
			SetMenuBar(menubar);
		}

		wxColour toolbgcol(iconset ? 0xF0ECE8 : 0xD8C7BC);

		if (showtbar || mergetbar) {
			tb = CreateToolBar(wxBORDER_NONE | wxTB_HORIZONTAL | wxTB_FLAT | wxTB_NODIVIDER);
			tb->SetOwnBackgroundColour(toolbgcol);

#ifdef __WXMAC__
#define SEPARATOR
#else
#define SEPARATOR tb->AddSeparator()
#endif

			wxString iconpath =
				GetPath(iconset ? L"images/webalys/toolbar/" : L"images/nuvola/toolbar/");
			tb->SetToolBitmapSize(iconset ? wxSize(18, 18) : wxSize(22, 22));

			AddTBIcon(tb, _(L"New (CTRL+n)"), A_NEW, iconpath + L"filenew.png");
			AddTBIcon(tb, _(L"Open (CTRL+o)"), A_OPEN, iconpath + L"fileopen.png");
			AddTBIcon(tb, _(L"Save (CTRL+s)"), A_SAVE, iconpath + L"filesave.png");
			AddTBIcon(tb, _(L"Save As"), A_SAVEAS, iconpath + L"filesaveas.png");
			SEPARATOR;
			AddTBIcon(tb, _(L"Undo (CTRL+z)"), A_UNDO, iconpath + L"undo.png");
			AddTBIcon(tb, _(L"Copy (CTRL+c)"), A_COPY, iconpath + L"editcopy.png");
			AddTBIcon(tb, _(L"Paste (CTRL+v)"), A_PASTE, iconpath + L"editpaste.png");
			SEPARATOR;
			AddTBIcon(tb, _(L"Zoom In (CTRL+mousewheel)"), A_ZOOMIN, iconpath + L"zoomin.png");
			AddTBIcon(tb, _(L"Zoom Out (CTRL+mousewheel)"), A_ZOOMOUT, iconpath + L"zoomout.png");
			SEPARATOR;
			AddTBIcon(tb, _(L"New Grid (INS)"), A_NEWGRID, iconpath + L"newgrid.png");
			AddTBIcon(tb, _(L"Add Image"), A_IMAGE, iconpath + L"image.png");
			SEPARATOR;
			AddTBIcon(tb, _(L"Run"), A_RUN, iconpath + L"run.png");
			tb->AddSeparator();
			tb->AddControl(new wxStaticText(tb, wxID_ANY, _(L"Search ")));
			tb->AddControl(filter =
				new wxTextCtrl(tb, A_SEARCH, "", wxDefaultPosition, wxSize(80, 24)));
			SEPARATOR;
			tb->AddControl(new wxStaticText(tb, wxID_ANY, _(L"Replace ")));
			tb->AddControl(
				replaces = new wxTextCtrl(tb, A_REPLACE, "", wxDefaultPosition, wxSize(60, 24)));
			tb->AddSeparator();
			tb->AddControl(new wxStaticText(tb, wxID_ANY, _(L"Cell ")));
			tb->AddControl(new ColorDropdown(tb, A_CELLCOLOR, 1));
			SEPARATOR;
			tb->AddControl(new wxStaticText(tb, wxID_ANY, _(L"Text ")));
			tb->AddControl(new ColorDropdown(tb, A_TEXTCOLOR, 2));
			SEPARATOR;
			tb->AddControl(new wxStaticText(tb, wxID_ANY, _(L"Border ")));
			tb->AddControl(new ColorDropdown(tb, A_BORDCOLOR, 7));
			tb->AddSeparator();
			tb->AddControl(new wxStaticText(tb, wxID_ANY, _(L"Image ")));
			wxString imagepath = GetPath("images/nuvola/dropdown/");
			idd = new ImageDropdown(tb, imagepath);
			tb->AddControl(idd);
			tb->Realize();
		}

		if (showsbar) {
			wxStatusBar *sb = CreateStatusBar(4);
			sb->SetOwnBackgroundColour(toolbgcol);
			SetStatusBarPane(0);
			int swidths[] = { -1, 200, 120, 100 };
			SetStatusWidths(4, swidths);
		}

		nb = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
			wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS |
			wxAUI_NB_WINDOWLIST_BUTTON | wxAUI_NB_CLOSE_ON_ALL_TABS |
			(lefttabs ? wxAUI_NB_BOTTOM : wxAUI_NB_TOP));
		nb->SetOwnBackgroundColour(toolbgcol);

		int display_id = wxDisplay::GetFromWindow(this);
		wxRect disprect = wxDisplay(display_id == wxNOT_FOUND ? 0 : display_id).GetClientArea();
		const int screenx = disprect.width - disprect.x;
		const int screeny = disprect.height - disprect.y;

		const int boundary = 64;
		const int defx = screenx - 2 * boundary;
		const int defy = screeny - 2 * boundary;
		int resx, resy, posx, posy;
		g_sys->cfg->Read(L"resx", &resx, defx);
		g_sys->cfg->Read(L"resy", &resy, defy);
		g_sys->cfg->Read(L"posx", &posx, boundary + disprect.x);
		g_sys->cfg->Read(L"posy", &posy, boundary + disprect.y);
		if (resx > screenx || resy > screeny || posx < disprect.x || posy < disprect.y ||
			posx + resx > disprect.width + disprect.x ||
			posy + resy > disprect.height + disprect.y) {
			// Screen res has been resized since we last ran, set sizes to default to avoid being
			// off-screen.
			resx = defx;
			resy = defy;
			posx = posy = boundary;
			posx += disprect.x;
			posy += disprect.y;
		}
		SetSize(resx, resy);
		SetPosition(wxPoint(posx, posy));

		bool ismax;
		g_sys->cfg->Read(L"maximized", &ismax, true);

		aui->AddPane(nb, wxCENTER);
		aui->Update();

		Show(TRUE);

		// needs to be after Show() to avoid scrollbars rendered in the wrong place?
		if (ismax) Maximize(true);

		SetFileAssoc(exename);

		wxSafeYield();
	}

	void MyFrame::AppOnEventLoopEnter()
	{
		// Have to do this here, if we do it in the Frame constructor above, it crashes on OS X.
		watcher = new wxFileSystemWatcher();
		watcher->SetOwner(this);
		Connect(wxEVT_FSWATCHER, wxFileSystemWatcherEventHandler(MyFrame::OnFileSystemEvent));
	}

	MyFrame::~MyFrame() {
		filehistory.Save(*g_sys->cfg);
		if (!IsIconized()) {
			g_sys->cfg->Write(L"maximized", IsMaximized());
			if (!IsMaximized()) {
				g_sys->cfg->Write(L"resx", GetSize().x);
				g_sys->cfg->Write(L"resy", GetSize().y);
				g_sys->cfg->Write(L"posx", GetPosition().x);
				g_sys->cfg->Write(L"posy", GetPosition().y);
			}
		}
		aui->ClearEventHashTable();
		aui->UnInit();
		DELETEP(aui);
		DELETEP(editmenupopup);
		DELETEP(watcher);
	}

	TSCanvas *MyFrame::NewTab(Document *doc, bool append) {
		TSCanvas *sw = new TSCanvas(this, nb);
		sw->doc = doc;
		doc->sw = sw;
		sw->SetScrollRate(1, 1);
		if (append)
			nb->AddPage(sw, L"<unnamed>", true, wxNullBitmap);
		else
			nb->InsertPage(0, sw, L"<unnamed>", true, wxNullBitmap);
		sw->SetDropTarget(new DropTarget(doc->dataobjc));
		sw->SetFocus();
		return sw;
	}

	TSCanvas *MyFrame::GetCurTab() {
		return nb && nb->GetSelection() >= 0 ? (TSCanvas *)nb->GetPage(nb->GetSelection()) : NULL;
	}
	TSCanvas *MyFrame::GetTabByFileName(const wxString &fn) {
		if (nb) loop(i, nb->GetPageCount()) {
			TSCanvas *p = (TSCanvas *)nb->GetPage(i);
			if (p->doc->filename == fn) {
				nb->SetSelection(i);
				return p;
			}
		}
		return NULL;
	}

	void MyFrame::OnTabChange(wxAuiNotebookEvent &nbe) {
		TSCanvas *sw = (TSCanvas *)nb->GetPage(nbe.GetSelection());
		sw->Status();
		g_sys->TabChange(sw->doc);
	}

	void MyFrame::TabsReset() {
		if (nb) loop(i, nb->GetPageCount()) {
			TSCanvas *p = (TSCanvas *)nb->GetPage(i);
			p->doc->rootgrid->ResetChildren();
		}
	}

	void MyFrame::OnTabClose(wxAuiNotebookEvent &nbe) {
		TSCanvas *sw = (TSCanvas *)nb->GetPage(nbe.GetSelection());
		g_sys->RememberOpenFiles();
		if (nb->GetPageCount() <= 1) {
			nbe.Veto();
			Close();
		}
		else if (sw->doc->CloseDocument()) {
			nbe.Veto();
		}
	}

	void MyFrame::CycleTabs(int offset) {
		int numtabs = nb->GetPageCount();
		offset = ((offset >= 0) ? 1 : numtabs - 1);  // normalize to non-negative wrt modulo
		nb->SetSelection((nb->GetSelection() + offset) % numtabs);
	}

	void MyFrame::SetPageTitle(const wxString &fn, wxString mods, int page) {
		if (page < 0) page = nb->GetSelection();
		if (page < 0) return;
		if (page == nb->GetSelection()) SetTitle(L"TreeSheets - " + fn + mods);
		nb->SetPageText(page, (fn.empty() ? L"<unnamed>" : wxFileName(fn).GetName()) + mods);
	}

	void MyFrame::AddTBIcon(wxToolBar *tb, const wxChar *name, int action, wxString file) {
		wxBitmap bm;
		if (bm.LoadFile(file, wxBITMAP_TYPE_PNG))
			tb->AddTool(action, name, bm, bm, wxITEM_NORMAL, name);
	}

	void MyFrame::TBMenu(wxToolBar *tb, wxMenu *menu, const wxChar *name, int id) {
		tb->AddTool(id, name, wxNullBitmap, wxEmptyString, wxITEM_DROPDOWN);
		tb->SetDropdownMenu(id, menu);
	}

	void MyFrame::OnMenu(wxCommandEvent &ce) {
		wxTextCtrl *tc;
		if (((tc = filter) && filter == wxWindow::FindFocus()) ||
			((tc = replaces) && replaces == wxWindow::FindFocus())) {
			// FIXME: have to emulate this behavior because menu always captures these events (??)
			long from, to;
			tc->GetSelection(&from, &to);
			switch (ce.GetId()) {
			case A_MLEFT:
			case A_LEFT:
				if (from != to)
					tc->SetInsertionPoint(from);
				else if (from)
					tc->SetInsertionPoint(from - 1);
				return;
			case A_MRIGHT:
			case A_RIGHT:
				if (from != to)
					tc->SetInsertionPoint(to);
				else if (to < tc->GetLineLength(0))
					tc->SetInsertionPoint(to + 1);
				return;

			case A_SHOME: tc->SetSelection(0, to); return;
			case A_SEND: tc->SetSelection(from, 1000); return;

			case A_SCLEFT:
			case A_SLEFT:
				if (from) tc->SetSelection(from - 1, to);
				return;
			case A_SCRIGHT:
			case A_SRIGHT:
				if (to < tc->GetLineLength(0)) tc->SetSelection(from, to + 1);
				return;

			case A_BACKSPACE: tc->Remove(from - (from == to), to); return;
			case A_DELETE: tc->Remove(from, to + (from == to)); return;
			case A_HOME: tc->SetSelection(0, 0); return;
			case A_END: tc->SetSelection(1000, 1000); return;
			case A_SELALL: tc->SetSelection(0, 1000); return;
			}
		}
		TSCanvas *sw = GetCurTab();
		wxClientDC dc(sw);
		sw->DoPrepareDC(dc);
		sw->doc->ShiftToCenter(dc);
		switch (ce.GetId()) {
		case A_NOP: break;

		case A_ALEFT: sw->CursorScroll(-g_scrollratecursor, 0); break;
		case A_ARIGHT: sw->CursorScroll(g_scrollratecursor, 0); break;
		case A_AUP: sw->CursorScroll(0, -g_scrollratecursor); break;
		case A_ADOWN: sw->CursorScroll(0, g_scrollratecursor); break;

		case A_ICONSET:
			g_sys->cfg->Write(L"iconset", ce.IsChecked());
			sw->Status(_("change will take effect next run of TreeSheets"));
			break;
		case A_SHOWSBAR:
			g_sys->cfg->Write(L"showsbar", ce.IsChecked());
			sw->Status(_("change will take effect next run of TreeSheets"));
			break;
		case A_SHOWTBAR:
			g_sys->cfg->Write(L"showtbar", ce.IsChecked());
			sw->Status(_("change will take effect next run of TreeSheets"));
			break;
		case A_LEFTTABS:
			g_sys->cfg->Write(L"lefttabs", ce.IsChecked());
			sw->Status(_("change will take effect next run of TreeSheets"));
			break;
		case A_SINGLETRAY:
			g_sys->cfg->Write(L"singletray", ce.IsChecked());
			sw->Status(_("change will take effect next run of TreeSheets"));
			break;
		case A_MAKEBAKS: g_sys->cfg->Write(L"makebaks", g_sys->makebaks = ce.IsChecked()); break;
		case A_TOTRAY: g_sys->cfg->Write(L"totray", g_sys->totray = ce.IsChecked()); break;
		case A_MINCLOSE: g_sys->cfg->Write(L"minclose", g_sys->minclose = ce.IsChecked()); break;
		case A_ZOOMSCR: g_sys->cfg->Write(L"zoomscroll", g_sys->zoomscroll = ce.IsChecked()); break;
		case A_THINSELC: g_sys->cfg->Write(L"thinselc", g_sys->thinselc = ce.IsChecked()); break;
		case A_AUTOSAVE: g_sys->cfg->Write(L"autosave", g_sys->autosave = ce.IsChecked()); break;
		case A_CENTERED:
			g_sys->cfg->Write(L"centered", g_sys->centered = ce.IsChecked());
			Refresh();
			break;
		case A_FSWATCH:
			g_sys->cfg->Write(L"fswatch", g_sys->fswatch = ce.IsChecked());
			sw->Status(_("change will take effect next run of TreeSheets"));
			break;
		case A_AUTOEXPORT:
			g_sys->cfg->Write(L"autohtmlexport", g_sys->autohtmlexport = ce.IsChecked());
			break;
		case A_FASTRENDER:
			g_sys->cfg->Write(L"fastrender", g_sys->fastrender = ce.IsChecked());
			Refresh();
			break;
		case A_FULLSCREEN:
			ShowFullScreen(!IsFullScreen());
			if (IsFullScreen()) sw->Status(_("press F11 to exit fullscreen mode"));
			break;
		case A_SEARCHF:
			if (filter) {
				filter->SetFocus();
				filter->SetSelection(0, 1000);
			}
			else {
				sw->Status(_("Please enable (Options -> Show Toolbar) to use search"));
			}
			break;
#ifdef __WXMAC__
		case wxID_OSX_HIDE: Iconize(true); break;
		case wxID_OSX_HIDEOTHERS: sw->Status("NOT IMPLEMENTED"); break;
		case wxID_OSX_SHOWALL: Iconize(false); break;
		case wxID_ABOUT: sw->doc->Action(dc, A_ABOUT); break;
		case wxID_PREFERENCES: sw->doc->Action(dc, A_DEFFONT); break;
		case wxID_EXIT:  // FALL THRU:
#endif
		case A_EXIT:
			fromclosebox = false;
			this->Close();
			break;
		case A_CLOSE: sw->doc->Action(dc, ce.GetId()); break;  // sw dangling pointer on return
		default:
			if (ce.GetId() >= wxID_FILE1 && ce.GetId() <= wxID_FILE9) {
				wxString f(filehistory.GetHistoryFile(ce.GetId() - wxID_FILE1));
				sw->Status(g_sys->Open(f));
			}
			else if (ce.GetId() >= A_TAGSET) {
				sw->Status(sw->doc->TagSet(ce.GetId() - A_TAGSET));
			}
			else {
				sw->Status(sw->doc->Action(dc, ce.GetId()));
				break;
			}
		}
	}

	void MyFrame::OnSearch(wxCommandEvent &ce) {
		g_sys->searchstring = ce.GetString().Lower();
		Document *doc = GetCurTab()->doc;
		doc->selected.g = NULL;
		if (doc->searchfilter)
			doc->SetSearchFilter(g_sys->searchstring.Len() != 0);
		else
			doc->Refresh();
		GetCurTab()->Status();
	}

	void MyFrame::ReFocus() {
		if (GetCurTab()) GetCurTab()->SetFocus();
	}

	void MyFrame::OnCellColor(wxCommandEvent &ce) {
		GetCurTab()->doc->ColorChange(A_CELLCOLOR, ce.GetInt());
		ReFocus();
	}
	void MyFrame::OnTextColor(wxCommandEvent &ce) {
		GetCurTab()->doc->ColorChange(A_TEXTCOLOR, ce.GetInt());
		ReFocus();
	}
	void MyFrame::OnBordColor(wxCommandEvent &ce) {
		GetCurTab()->doc->ColorChange(A_BORDCOLOR, ce.GetInt());
		ReFocus();
	}
	void MyFrame::OnDDImage(wxCommandEvent &ce) {
		GetCurTab()->doc->ImageChange(idd->as[ce.GetInt()]);
		ReFocus();
	}

	void MyFrame::OnSizing(wxSizeEvent &se) { se.Skip(); }
	void MyFrame::OnMaximize(wxMaximizeEvent &me) {
		ReFocus();
		me.Skip();
	}
	void MyFrame::OnActivate(wxActivateEvent &ae) {
		// This causes warnings in the debug log, but without it keyboard entry upon window select
		// doesn't work.
		ReFocus();
	}

	void MyFrame::OnIconize(wxIconizeEvent &me) {
		if (me.IsIconized()) {
#ifdef WIN32
			if (g_sys->totray) {
				tbi.SetIcon(icon, L"TreeSheets");
				Show(false);
				Iconize();
			}
#endif
		}
		else {
			if (GetCurTab()) GetCurTab()->SetFocus();
		}
	}

	void MyFrame::DeIconize() {
		if (!IsIconized()) {
			RequestUserAttention();
			return;
		}
		Show(true);
		Iconize(false);
		tbi.RemoveIcon();
	}

	void MyFrame::OnTBIDBLClick(wxTaskBarIconEvent &e) { DeIconize(); }

	void MyFrame::OnClosing(wxCloseEvent &ce) {
		bool fcb = fromclosebox;
		fromclosebox = true;
		if (fcb && g_sys->minclose) {
			ce.Veto();
			Iconize();
			return;
		}
		g_sys->RememberOpenFiles();
		if (ce.CanVeto())
			while (nb->GetPageCount()) {
				if (GetCurTab()->doc->CloseDocument()) {
					ce.Veto();
					g_sys->RememberOpenFiles();  // may have closed some, but not all
					return;
				}
				else {
					nb->DeletePage(nb->GetSelection());
				}
			}
		bt.Stop();
		g_sys->savechecker.Stop();
		Destroy();
	}

	void MyFrame::OnFileSystemEvent(wxFileSystemWatcherEvent &event) {
		// 0xF == create/delete/rename/modify
		if ((event.GetChangeType() & 0xF) == 0 || watcherwaitingforuser || !nb) return;
		const wxString &modfile = event.GetPath().GetFullPath();
		loop(i, nb->GetPageCount()) {
			Document *doc = ((TSCanvas *)nb->GetPage(i))->doc;
			if (modfile == doc->filename) {
				wxDateTime modtime = wxFileName(modfile).GetModificationTime();
				// Compare with last modified to trigger multiple times.
				if (modtime == doc->lastmodificationtime) {
					return;
				}
				if (doc->modified) {
					// TODO: this dialog is problematic since it may be on an unattended
					// computer and more of these events may fire. since the occurrence of this
					// situation is rare, it may be better to just take the most
					// recently changed version (which is the one that has just been modified
					// on disk) this potentially throws away local changes, but this can only
					// happen if the user left changes unsaved, then decided to go edit an older
					// version on another computer.
					// for now, we leave this code active, and guard it with
					// watcherwaitingforuser
					wxString msg = wxString::Format(
						_(L"%s\nhas been modified on disk by another program / computer:\nWould "
						  L"you like to discard your changes and re-load from disk?"),
						doc->filename);
					watcherwaitingforuser = true;
					int res = wxMessageBox(msg, _(L"File modification conflict!"),
						wxYES_NO | wxICON_QUESTION, this);
					watcherwaitingforuser = false;
					if (res != wxYES) return;
				}
				const char *msg = g_sys->LoadDB(doc->filename, false, true);
				assert(msg);
				if (*msg) {
					GetCurTab()->Status(msg);
				}
				else {
					loop(j, nb->GetPageCount()) if (((TSCanvas *)nb->GetPage(j))->doc == doc)
						nb->DeletePage(j);
					::wxRemoveFile(g_sys->TmpName(modfile));
					GetCurTab()->Status(
						_("File has been re-loaded because of modifications of another program / "
						  "computer"));
				}
				return;
			}
		}
	}

}
