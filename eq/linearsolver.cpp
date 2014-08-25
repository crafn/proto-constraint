#include "linearsolver.hpp"

namespace eq {

void LinearSolver::addVar(double& ref)
{
	auto infinity= solver.infinity();
	vars.add(ref, *solver.MakeNumVar(-infinity, infinity, ""));
}

void LinearSolver::apply()
{
	op::MPSolver::ResultStatus status= solver.Solve();

	/// @todo Throw error
	if (status != op::MPSolver::OPTIMAL)
		std::cout << "Solving error\n";

	for (auto&& v : vars) {
		ensure(v.actual && v.model);
		*v.actual= v.model->solution_value();
	}
}

} // eq
