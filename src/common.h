#pragma once

#include <new>
#include <vector>
#include <clocale>
#include <ctype.h>
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/txtstrm.h>
#include <wx/intl.h>
#include <wx/dcbuffer.h>
#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include <wx/tokenzr.h>
#include <wx/numdlg.h>
#include <wx/aboutdlg.h>
#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#ifdef WIN32
#include <wx/msw/regconf.h>
#include <wx/msw/dc.h>
#endif
#include <wx/fontdlg.h>
#include <wx/colordlg.h>
#include <wx/filename.h>
#include <wx/filehistory.h>
#include <wx/xml/xml.h>
#include <wx/docview.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/odcombo.h>
#include <wx/sysopt.h>
#include <wx/taskbar.h>
#include <wx/notebook.h>
#include <wx/snglinst.h>
#include <wx/srchctrl.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include <wx/aui/auibook.h>
#include <wx/display.h>
#include <wx/fswatcher.h>

#include "tools.h"

#ifdef WIN32
    #include "..\treesheets\resource.h"
#endif

extern int g_grid_margin;
extern int g_cell_margin;
extern int g_margin_extra;
extern int g_line_width;
extern int g_selmargin;
extern int g_deftextsize;
extern int g_grid_left_offset;
int g_mintextsize();
int g_maxtextsize();

extern int g_scrollratecursor;
extern int g_scrollratewheel;

#define N_CELLTEXTCOLORS 42
extern uint celltextcolors[N_CELLTEXTCOLORS];
#define CUSTOMCOLORIDX 0

enum { TS_VERSION = 18, TS_TEXT = 0, TS_GRID, TS_BOTH, TS_NEITHER };

enum {
	A_NEW = 500,
	A_OPEN,
	A_CLOSE,
	A_SAVE,
	A_SAVEAS,
	A_CUT,
	A_COPY,
	A_PASTE,
	A_NEWGRID,
	A_UNDO,
	A_ABOUT,
	A_RUN,
	A_CLRVIEW,
	A_MARKDATA,
	A_MARKVIEWH,
	A_MARKVIEWV,
	A_MARKCODE,
	A_IMAGE,
	A_EXPIMAGE,
	A_EXPXML,
	A_EXPHTMLT,
	A_EXPHTMLO,
	A_EXPTEXT,
	A_ZOOMIN,
	A_ZOOMOUT,
	A_TRANSPOSE,
	A_DELETE,
	A_BACKSPACE,
	A_LEFT,
	A_RIGHT,
	A_UP,
	A_DOWN,
	A_MLEFT,
	A_MRIGHT,
	A_MUP,
	A_MDOWN,
	A_SLEFT,
	A_SRIGHT,
	A_SUP,
	A_SDOWN,
	A_ALEFT,
	A_ARIGHT,
	A_AUP,
	A_ADOWN,
	A_SCLEFT,
	A_SCRIGHT,
	A_SCUP,
	A_SCDOWN,
	A_DEFFONT,
	A_IMPXML,
	A_IMPXMLA,
	A_IMPTXTI,
	A_IMPTXTC,
	A_IMPTXTS,
	A_IMPTXTT,
	A_HELP,
	A_MARKVARD,
	A_MARKVARU,
	A_SHOWSBAR,
	A_SHOWTBAR,
	A_LEFTTABS,
	A_TRADSCROLL,
	A_HOME,
	A_END,
	A_CHOME,
	A_CEND,
	A_PRINT,
	A_PREVIEW,
	A_PAGESETUP,
	A_PRINTSCALE,
	A_EXIT,
	A_NEXT,
	A_PREV,
	A_BOLD,
	A_ITALIC,
	A_TT,
	A_UNDERL,
	A_SEARCH,
	A_REPLACE,
	A_REPLACEONCE,
	A_REPLACEONCEJ,
	A_REPLACEALL,
	A_SELALL,
	A_CANCELEDIT,
	A_BROWSE,
	A_ENTERCELL,
	A_CELLCOLOR,
	A_TEXTCOLOR,
	A_BORDCOLOR,
	A_INCSIZE,
	A_DECSIZE,
	A_INCWIDTH,
	A_DECWIDTH,
	A_ENTERGRID,
	A_LINK,
	A_LINKREV,
	A_SEARCHNEXT,
	A_CUSTCOL,
	A_COLCELL,
	A_SORT,
	A_SEARCHF,
	A_MAKEBAKS,
	A_TOTRAY,
	A_AUTOSAVE,
	A_FULLSCREEN,
	A_SCALED,
	A_SCOLS,
	A_SROWS,
	A_SHOME,
	A_SEND,
	A_BORD0,
	A_BORD1,
	A_BORD2,
	A_BORD3,
	A_BORD4,
	A_BORD5,
	A_HSWAP,
	A_TEXTGRID,
	A_TAGADD,
	A_TAGREMOVE,
	A_WRAP,
	A_HIFY,
	A_FLATTEN,
	A_BROWSEF,
	A_ROUND0,
	A_ROUND1,
	A_ROUND2,
	A_ROUND3,
	A_ROUND4,
	A_ROUND5,
	A_ROUND6,
	A_FILTER5,
	A_FILTER10,
	A_FILTER20,
	A_FILTER50,
	A_FILTERM,
	A_FILTERL,
	A_FILTERS,
	A_FILTEROFF,
	A_FASTRENDER,
	A_EXPCSV,
	A_PASTESTYLE,
	A_PREVFILE,
	A_NEXTFILE,
	A_IMAGER,
	A_INCWIDTHNH,
	A_DECWIDTHNH,
	A_ZOOMSCR,
	A_ICONSET,
	A_V_GS,
	A_V_BS,
	A_V_LS,
	A_H_GS,
	A_H_BS,
	A_H_LS,
	A_GS,
	A_BS,
	A_LS,
	A_RESETSIZE,
	A_RESETWIDTH,
	A_RESETSTYLE,
	A_RESETCOLOR,
	A_DDIMAGE,
	A_MINCLOSE,
	A_SINGLETRAY,
	A_CENTERED,
	A_SORTD,
	A_STRIKET,
	A_FOLD,
	A_FOLDALL,
	A_UNFOLDALL,
	A_IMAGESC,
	A_HELPI,
	A_REDO,
	A_FSWATCH,
	A_DEFBGCOL,
	A_THINSELC,
	A_COPYCT,
	A_MINISIZE,
	A_CUSTKEY,
	A_AUTOEXPORT,
	A_NOP,
	A_TAGSET = 1000  // and all values from here on
};

enum {
	STYLE_BOLD = 1,
	STYLE_ITALIC = 2,
	STYLE_FIXED = 4,
	STYLE_UNDERLINE = 8,
	STYLE_STRIKETHRU = 16
};

namespace treesheets {
	class System;
	extern System *g_sys;
}
