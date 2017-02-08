#pragma once
#include "common.h"

#include "cell.h"
#include "grid.h"
#include "text.h"

namespace treesheets {

	/*
		A structure describing an operation.
	*/
	struct Operation {
		const char *args;

		virtual Cell *run() { return NULL; }
		virtual double runn(double a) { return 0; }
		virtual Cell *runt(Text *t) { return NULL; }
		virtual Cell *runl(Grid *l) { return NULL; }
		virtual Cell *rung(Grid *g) { return NULL; }
		virtual Cell *runc(Cell *c) { return NULL; }
		virtual double runnn(double a, double b) { return 0; }
	};

	WX_DECLARE_STRING_HASH_MAP(Operation *, wxHashMapOperation);
	WX_DECLARE_STRING_HASH_MAP(Cell *, wxHashMapCell);

	/*
		Provides running evaluation of a grid.
	*/
	class Evaluator {
	public:
		wxHashMapOperation ops;
		wxHashMapCell vars;
		bool vert;

		~Evaluator();

		void Init();

		int InferCellType(Text &t);

		Cell *Lookup(wxString &name);

		bool IsValidSymbol(wxString const &symbol) const;
		void SetSymbol(wxString const &symbol, Cell *val);

		void Assign(Cell const *sym, Cell const *val);
		void DestructuringAssign(Grid const *names, Cell *val);

		inline Operation *FindOp(wxString &name) { return ops[name]; }

		Cell *Execute(Operation *op);
		Cell *Execute(Operation *op, Cell *left);
		Cell *Execute(Operation *op, Cell *left, Cell *right);
		Cell *Execute(Operation *op, Cell *left, Cell *a, Cell *b);

		void Eval(Cell *root);
	};

}
