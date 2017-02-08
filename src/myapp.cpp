#include "myapp.h"

#include "system.h"

namespace treesheets {

	bool MyApp::OnInit() {
#if wxUSE_UNICODE == 0
#error "must use unicode version of wx libs to ensure data integrity of .cts files"
#endif
		ASSERT(wxUSE_UNICODE);

		//#ifdef __WXMAC__
		// Now needed on WIN32 as well, because of all the locale related asserts. sigh.
		wxDisableAsserts();
		//#endif

		// locale.Init();
		//// wxWidgets forces the use of LC_ALL, and doesn't allow use of setlocale without a
		///wxLocale
		//// so to get what we want, we reset back to C locale first.
		// std::setlocale(LC_ALL, "C");
		std::setlocale(LC_CTYPE, "");  // correct handling of non-latin symbols

		bool portable = false;
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '-') {
				switch ((int)argv[i][1]) {
				case 'p': portable = true; break;
				}
			}
			else {
				filename = argv[i];
			}
		}

		g_sys = new System(portable);
		frame = new MyFrame(argv[0], this);
		SetTopWindow(frame);

		return true;
	}

	void MyApp::OnEventLoopEnter(wxEventLoopBase* WXUNUSED(loop))
	{
		if (!initateventloop)
		{
			initateventloop = true;
			frame->AppOnEventLoopEnter();
			g_sys->Init(filename);
		}
	}

	int MyApp::OnExit() {
		DELETEP(g_sys);
		return 0;
	}

	void MyApp::MacOpenFile(const wxString &fn) {
		if (g_sys) g_sys->Open(fn);
	}
}