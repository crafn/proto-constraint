#ifndef EQ_DOMAIN_HPP
#define EQ_DOMAIN_HPP

#include "basevar.hpp"
#include "solver.hpp"
#include "util.hpp"
#include "varhandle.hpp"

namespace eq {

class Domain : public std::enable_shared_from_this<Domain> {
public:
	static int maxPriorityCount() { return 1024; }

	Domain()= default;
	Domain(const Domain&)= delete;
	Domain(Domain&&)= delete;
	Domain& operator=(const Domain&)= delete;
	Domain& operator=(Domain&&)= delete;

	template <typename T, VarType type>
	void addVar(Var<T, type>& var)
	{
		VarHandle handle{var};
		varInfos.emplace_back(
			VarInfo{
				handle,
				[handle] (Domain& d, Solver& solver)
				{
					Var<T, type>& var= static_cast<Var<T, type>&>(handle.get());
					solver.addVar(var.get());
				}
			}
		);

		/// @todo Check for doubles
		if (type == VarType::priority)
			addRelation(var > 0 && var < maxPriorityCount());

		dirty= true;
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		/// @todo Remove relation when var is deleted
		addRelations.push_back([rel] (Domain& d, Solver& solver)
		{ solver.addRelation(rel); });
		dirty= true;
	}

	template <typename T1, typename T2>
	void addRelation(Expr<T1> rel, Var<T2, VarType::priority>& priority)
	{
		VarHandle priority_h{priority};
		addRelations.push_back([rel, priority_h] (Domain& d, Solver& solver)
		{
			ensure(priority_h && "eq::PriorityVar is destroyed/moved even though its in use");
			Var<T2, VarType::priority>& p=
				static_cast<Var<T2, VarType::priority>&>(priority_h.get());
			// This will solve priority domain
			solver.addRelation(rel, p);
		});
		dirty= true;
	}

	void solve()
	{
		if (!dirty)
			return;

		removeDecayed();

		Solver solver;
		for (auto&& info : varInfos)
			info.post(*this, solver);
		for (auto&& post : addRelations)
			post(*this, solver);
		solver.apply();

		dirty= false;
	}

	void merge(Domain&& other)
	{
		ensure(this != &other);

		varInfos= varInfos + other.varInfos;
		for (auto&& info : other.varInfos)
			info.handle->setDomainPtr(shared_from_this());
	
		addRelations= addRelations + other.addRelations;
	
		dirty= true;
		other.clear();
	}

	void clear()
	{
		varInfos.clear();
		addRelations.clear();
		dirty= false;
	}

private:
	bool removeDecayed()
	{
		auto size_before= varInfos.size();

		eraseIf(varInfos,
			[] (const VarInfo& info) -> bool
			{
				return info.handle.isNull();
			});

		return size_before - varInfos.size() > 0;
	}

	using AddRelation= std::function<void (Domain& d, Solver& solver)>;
	using AddVar= std::function<void (Domain& d, Solver& solver)>;

	struct VarInfo {
		VarHandle handle;
		AddVar post;
	};

	DynArray<VarInfo> varInfos;
	/// Quick and easy way to save posted relations for future reposting
	DynArray<AddRelation> addRelations;

	/// Is solution up-to-date
	bool dirty= false;
};

Set<DomainPtr> domains(const Set<BaseVar*>& vars)
{
	Set<DomainPtr> ds;
	for (auto&& v : vars) {
		ensure(v);
		ds.insert(v->getDomainPtr());
	}
	return ds;
}


} // eq

#endif // EQ_DOMAIN_HPP
