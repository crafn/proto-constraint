#ifndef EQ_DOMAIN_HPP
#define EQ_DOMAIN_HPP

#include "util.hpp"
#include "basevar.hpp"
#include "solver.hpp"

namespace eq {

using PriorityVar= Var<int>;

class Domain : public std::enable_shared_from_this<Domain> {
public:

	template <typename T>
	void addVar(Var<T>& v)
	{
		Var<T>* ptr= &v;
		vars.push_back(ptr);
		addVars[ptr]= [ptr] (Domain& d, Solver& solver)
		{ solver.addVar(ptr->get()); };
		dirty= true;
	}

	template <typename T>
	void removeVar(const Var<T>& v)
	{
		eraseFrom(vars, &v);
		eraseFrom(addVars, &v);
		dirty= true;
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		addRelations.push_back([rel] (Domain& d, Solver& solver)
		{ solver.addRelation(rel); });
		dirty= true;
	}

	template <typename T1, typename T2>
	void addRelation(Expr<T1> rel, Var<T2>& priority)
	{
		static_assert(isSame<Var<T2>, PriorityVar>(),
				"Second parameter must be of type eq::PriorityVar");

		addRelations.push_back([rel, &priority] (Domain& d, Solver& solver)
		{
			// This will solve priority domain
			solver.addRelation(rel, priority);
		});
		dirty= true;
	}

	void solve()
	{
		assert(dirty);

		Solver solver;

		for (auto&& pair : addVars) {
			auto&& post= pair.second;
			post(*this, solver);
		}

		for (auto&& post : addRelations)
			post(*this, solver);
		
		solver.apply();

		dirty= false;
	}

	void merge(Domain&& other)
	{
		assert(this != &other);

		vars= vars + other.vars;
		for (auto&& var : other.vars)
			var->setDomainPtr(shared_from_this());
		
		addVars= addVars + other.addVars;
		addRelations= addRelations + other.addRelations;
		dirty= true;
		other.clear();
	}

	bool isDirty() const { return dirty; }

	void clear()
	{
		vars.clear();
		addRelations.clear();
		addVars.clear();
		dirty= false;
	}

private:

	using AddRelation= std::function<void (Domain& d, Solver& solver)>;
	using AddVar= std::function<void (Domain& d, Solver& solver)>;

	DynArray<BaseVar*> vars;
	/// Quick and easy way to save posted relations for future reposting
	DynArray<AddRelation> addRelations;
	Map<const BaseVar*, AddVar> addVars;

	/// Is solution up-to-date
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
