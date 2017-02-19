#pragma once
#include "common.h"

#include "myframe.h"

namespace treesheets {

	struct MyApp : wxApp {
		MyFrame *frame;
		wxString filename;
		wxLocale locale;
		bool initateventloop;

		MyApp() : frame(NULL), locale(wxLANGUAGE_DEFAULT), initateventloop(false) {}
		bool OnInit();

		void OnEventLoopEnter(wxEventLoopBase* WXUNUSED(loop));

		int OnExit();

		void MacOpenFile(const wxString &fn);

		DECLARE_EVENT_TABLE()
	};

}
