#pragma once
#include "common.h"

#include "myframe.h"

namespace treesheets {

	struct MyApp : wxApp {
		MyFrame *frame;
		wxString filename;
		bool initateventloop;
		// wxLocale locale;

		MyApp() : frame(NULL), initateventloop(false) {}
		bool OnInit();

		void OnEventLoopEnter(wxEventLoopBase* WXUNUSED(loop));

		int OnExit();

		void MacOpenFile(const wxString &fn);

		DECLARE_EVENT_TABLE()
	};

}
