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
					ensure(handle && "Invalid eq::Var handle");
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

	/// Removes `var` from domain and all relations where `var` is present
	/// Doesn't need to be called on move because handles are smart
	void removeVar(BaseVar& var)
	{
		eraseIf(varInfos,
			[&var] (const VarInfo& info)
			{ return &info.handle.get() == &var; }); 

		eraseIf(relInfos,
			[&var] (const RelInfo& info)
			{
				for (auto&& h : info.vars) {
					if (&h.get() == &var)
						return true;
				}
				return false;
			});

		/// @todo Not always necessary
		dirty= true;
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		relInfos.emplace_back(
			RelInfo{
				asHandles(rel.getVars()),
				[rel] (Domain& d, Solver& solver)
				{
					solver.addRelation(rel);
				}
			}
		);
		dirty= true;
	}

	template <typename T1, typename T2>
	void addRelation(Expr<T1> rel, Var<T2, VarType::priority>& priority)
	{
		VarHandle priority_h{priority};
		relInfos.emplace_back(
			RelInfo {
				asHandles(rel.getVars()),
				[rel, priority_h] (Domain& d, Solver& solver)
				{
					ensure(priority_h && "eq::PriorityVar has been destroyed");
					Var<T2, VarType::priority>& p=
						static_cast<Var<T2, VarType::priority>&>(priority_h.get());
					// This will solve priority domain
					solver.addRelation(rel, p);
				}
			}
		);
		dirty= true;
	}

	void solve()
	{
		if (!dirty)
			return;

		Solver solver;
		for (auto&& info : varInfos)
			info.post(*this, solver);
		for (auto&& info : relInfos)
			info.post(*this, solver);
		solver.apply();

		dirty= false;
	}

	void merge(Domain&& other)
	{
		ensure(this != &other);

		varInfos= varInfos + other.varInfos;
		for (auto&& info : other.varInfos)
			info.handle->setDomainPtr(shared_from_this());
	
		relInfos= relInfos + other.relInfos;
	
		dirty= true;
		other.clear();
	}

	void clear()
	{
		varInfos.clear();
		relInfos.clear();
		dirty= false;
	}

private:
	using AddRel= std::function<void (Domain& d, Solver& solver)>;
	using AddVar= std::function<void (Domain& d, Solver& solver)>;

	struct VarInfo {
		VarHandle handle;
		AddVar post;
	};

	struct RelInfo {
		DynArray<VarHandle> vars;
		/// Quick and easy way to save posted relations for future reposting
		AddRel post;
	};

	DynArray<VarHandle> asHandles(const Set<BaseVar*>& container)
	{
		DynArray<VarHandle> var_handles;
		for (auto&& ptr : container) {
			ensure(ptr);
			var_handles.emplace_back(*ptr);
		}
		return var_handles;
	}

	DynArray<VarInfo> varInfos;
	DynArray<RelInfo> relInfos;

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
