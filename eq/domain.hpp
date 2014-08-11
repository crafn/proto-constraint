#ifndef EQ_DOMAIN_HPP
#define EQ_DOMAIN_HPP

#include "util.hpp"
#include "basevar.hpp"
#include "solverspace.hpp"

namespace eq {

class Domain : public std::enable_shared_from_this<Domain> {
public:

	template <typename T>
	void addVar(Var<T>& v)
	{
		Var<T>* ptr= &v;
		addVars[ptr]= [ptr] (Domain& d, SolverSpace& solver)
		{
			solver.addVar(ptr->get());
			d.vars.push_back(ptr);
		};
		addVars[ptr](*this, getSolver());
	}

	template <typename T>
	void removeVar(const Var<T>& v)
	{
		solver->removeVar(v.get());
		eraseFrom(vars, &v);
		eraseFrom(addVars, &v);
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		static_assert(isRelation<Expr<T>>(), "Expression is not a relation");

		addRelations.push_back([rel] (Domain& d, SolverSpace& solver)
		{
			solver.addRelation(rel);
			d.dirty= true;
		});
		addRelations.back()(*this, getSolver());
	}

	void solve() {
		assert(solver);
		
		// Search solutions
		/// @todo Do something smart in cases of many/none solutions
		Gecode::DFS<SolverSpace> e(solver.get());
		while (UniquePtr<SolverSpace> s{e.next()}) {
			//s->print();
		}  

		solver->apply();
		dirty= false;
	}

	void merge(Domain&& other)
	{
		assert(this != &other);

		for (auto&& pair : other.addVars) {
			auto&& post= pair.second;
			post(*this, getSolver());
		}
		addVars= addVars + other.addVars;
	
		for (auto&& var : other.vars)
			var->setDomainPtr(shared_from_this());

		for (auto&& post : other.addRelations)
			post(*this, getSolver());
		addRelations= addRelations + other.addRelations;
	
		other.clear();
	}

	SolverSpace& getSolver() { return *solver.get(); }

	bool isDirty() const { return dirty; }

	void clear()
	{
		solver.reset(new SolverSpace{});
		vars.clear();
		addRelations.clear();
		addVars.clear();
		dirty= false;
	}

private:
	using AddRelation= std::function<void (Domain& d, SolverSpace& solver)>;
	using AddVar= std::function<void (Domain& d, SolverSpace& solver)>;

	UniquePtr<SolverSpace> solver{new SolverSpace{}};
	DynArray<BaseVar*> vars;
	/// Quick and easy way to save posted relations for future reposting
	DynArray<AddRelation> addRelations;
	Map<const BaseVar*, AddVar> addVars;

	/// Solution is not up-to-date
	bool dirty= false;
};

Set<DomainPtr> domains(const Set<BaseVar*>& vars)
{
	Set<DomainPtr> ds;
	for (auto&& v : vars) {
		assert(v);
		ds.insert(v->getDomainPtr());
	}
	return ds;
}


} // eq

#endif // EQ_DOMAIN_HPP
