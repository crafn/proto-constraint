#include "solver.hpp"

namespace eq {

void Solver::addVar(int& ref)
{
	intVars.emplace_back(&ref, solver.MakeIntVar(minInt, maxInt));
}

void Solver::apply()
{
	/// @todo Should undo this after solving
	auto success_amount= solver.MakeSum(successAmounts)->Var();
	auto optimizer= solver.MakeMaximize(success_amount, 100);

	std::vector<op::IntVar*> vars;
	for (auto&& v : intVars) {
		vars.push_back(v.model);
	}
	auto db= solver.MakePhase(vars, op::Solver::CHOOSE_FIRST_UNBOUND, op::Solver::ASSIGN_CENTER_VALUE);

	solver.NewSearch(db, optimizer);
	if (solver.NextSolution()) {
		// Apparently last solution is the one which has the best success amount
		do {
			// Apply solution to actual variables
			for (auto&& v : intVars) {
				ensure(v.actual && v.model);
				*v.actual= v.model->Value();
				//std::cout << "Solution: " << v.model->Value() << std::endl;
			}
		} while (solver.NextSolution());
	} else {
		/// @todo Throw
		std::cout << "Solving error, failure count: " << solver.failures() << std::endl;
	}
	
	solver.EndSearch();
}

Solver::VarInfo<int>& Solver::getVarInfo(const int& ref)
{
	for (auto&& m : intVars) {
		if (m.actual == &ref)
			return m;
	}

	throw std::runtime_error{"var not found"};
}

void Solver::tryEraseVarInfo(const int& ref)
{
	auto it= std::find_if(intVars.begin(), intVars.end(),
		[&ref] (const VarInfo<int>& info)
		{
			return info.actual == &ref;
		});

	if (it != intVars.end())
		intVars.erase(it);
}

void Solver::addSuccessVar(op::IntVar* success, detail::Priority p)
{
	ensure(!p.hard());
	ensure(success);
	auto prod= solver.MakeProd(success, p.value())->Var();
	successAmounts.push_back(prod);
}

} // eq
