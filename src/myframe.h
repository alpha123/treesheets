#pragma once
#include "common.h"
#include "declarations.h"

#include "mywxtools.h"

namespace treesheets {

	class TSCanvas;

	struct MyFrame : wxFrame {
		typedef std::vector<std::pair<wxString, wxString>> MenuString;
		typedef MenuString::iterator MenuStringIterator;
		wxMenu *editmenupopup;
		wxString exepath_;
		wxFileHistory filehistory;
		wxTextCtrl *filter, *replaces;
		wxToolBar *tb;
		int refreshhack, refreshhackinstances;
		BlinkTimer bt;
		wxTaskBarIcon tbi;
		wxIcon icon;
		ImageDropdown *idd;
		wxAuiNotebook *nb;
		wxAuiManager *aui;
		wxBitmap line_nw, line_sw;
		wxBitmap foldicon;
		bool fromclosebox;
		wxApp *app;
		wxFileSystemWatcher *watcher;
		bool watcherwaitingforuser;

		wxString GetPath(const wxString &relpath);

		MenuString menustrings;

		void MyAppend(wxMenu *menu, int tag, const wxString &contents, const wchar_t *help = L"");

		MyFrame(wxString exename, wxApp *_app);

		void AppOnEventLoopEnter();

		~MyFrame();

		TSCanvas *NewTab(Document *doc, bool append = false);
		TSCanvas *GetCurTab();
		TSCanvas *GetTabByFileName(const wxString &fn);

		void OnTabChange(wxAuiNotebookEvent &nbe);
		void TabsReset();
		void OnTabClose(wxAuiNotebookEvent &nbe);
		void CycleTabs(int offset = 1);

		void SetPageTitle(const wxString &fn, wxString mods, int page = -1);

		void AddTBIcon(wxToolBar *tb, const wxChar *name, int action, wxString file);
		void TBMenu(wxToolBar *tb, wxMenu *menu, const wxChar *name, int id = 0);

		void OnMenu(wxCommandEvent &ce);
		void OnSearch(wxCommandEvent &ce);

		void ReFocus();

		void OnCellColor(wxCommandEvent &ce);
		void OnTextColor(wxCommandEvent &ce);
		void OnBordColor(wxCommandEvent &ce);
		void OnDDImage(wxCommandEvent &ce);
		void OnSizing(wxSizeEvent &se);
		void OnMaximize(wxMaximizeEvent &me);
		void OnActivate(wxActivateEvent &ae);
		void OnIconize(wxIconizeEvent &me);
		void DeIconize();
		void OnTBIDBLClick(wxTaskBarIconEvent &e);
		void OnClosing(wxCloseEvent &ce);
		void OnFileSystemEvent(wxFileSystemWatcherEvent &event);

		DECLARE_EVENT_TABLE()
	};

}
