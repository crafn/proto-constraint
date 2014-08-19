#include "solver.hpp"

namespace eq {
namespace detail {

/// Custom optimizer for or-tools solver
/// Optimizes for solution which has biggest value for given IntVar
/// Needs only 32 steps for int32 range, while or-tools MakeMaximize needs 4 billion
/// @note May miss the absolute maximum. This can be prevented by using 64 steps
class MaximizeVar : public op::SearchMonitor {
public:
	MaximizeVar(op::Solver& solver, op::IntVar* var)
		: SearchMonitor(&solver), var_(var) { }
	virtual ~MaximizeVar() { }
	
	int64 best() const { return best_; }

	op::IntVar* Var() const { return var_; }

	virtual void EnterSearch()
	{
		has_init_solution_= false;
		best_= kint64min;
	}

	virtual void RestartSearch() { ApplyBound(); }
	virtual void RefuteDecision(op::Decision* d) { ApplyBound(); }
	
	virtual bool AtSolution()
	{
		int64 val= var_->Value();
		if (!has_init_solution_ || val > best_)
			best_= val;
		has_init_solution_= true;
	}

	virtual bool AcceptSolution()
	{
		int64 val= var_->Value();
		if (!has_init_solution_)
			return true;
		else
			return val > best_;
	}

	virtual std::string Print() const { return ""; }
	virtual std::string DebugString() const { return ""; }

	virtual void Accept(op::ModelVisitor* const visitor) const
	{
		visitor->BeginVisitExtension(op::ModelVisitor::kObjectiveExtension);
		visitor->VisitIntegerExpressionArgument(
				op::ModelVisitor::kExpressionArgument,
				var_);
		visitor->EndVisitExtension(op::ModelVisitor::kObjectiveExtension);
	}
	 
	void ApplyBound()
	{
		if (has_init_solution_) {
			var_->SetMin(best_ + 1);
		}
	}
	 
private:
	op::IntVar* const var_= nullptr;
	int64 best_= 0;
	bool has_init_solution_= false;
	
	DISALLOW_COPY_AND_ASSIGN(MaximizeVar);
};

	

} // detail

void Solver::addVar(int& ref)
{
	intVars.emplace_back(&ref, solver.MakeIntVar(minInt, maxInt));
}

void Solver::apply()
{
	/// @todo Should undo this after solving
	auto success_amount= solver.MakeSum(successAmounts)->Var();
	auto optimizer= solver.RevAlloc(new detail::MaximizeVar(solver, success_amount));

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
