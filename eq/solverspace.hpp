#ifndef EQ_SOLVERSPACE_HPP
#define EQ_SOLVERSPACE_HPP

#include "expr.hpp"
#include "util.hpp"

#include <gecode/int.hh>
#include <gecode/gist.hh>
#include <gecode/minimodel.hh>

namespace eq {

/// Gecode constraint solver
class SolverSpace : public Gecode::Space {
public:

    SolverSpace()= default;
	
	SolverSpace(bool share, SolverSpace& s)
		: Space(share, s)
		, intVars(s.intVars)
	{
		for (size_t i= 0; i < intVars.size(); ++i) 
			intVars[i].model.update(*this, share, s.intVars[i].model);
	}

	virtual Space* copy(bool share)
	{
		return new SolverSpace(share, *this);
	}

	void print() const
	{
		for (auto&& v : intVars)
			std::cout << v.model << std::endl;
	}

	void addVar(int& ref)
	{
		intVars.emplace_back(&ref, Gecode::IntVar{*this, minInt, maxInt});
	}

	void removeVar(const int& ref)
	{
		eraseIf(intVars, [&ref] (const VarInfo<int>& v) { return v.actual == &ref; });
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		Gecode::rel(*this, gecodeRel(rel));
	}

	/// Apply solver variables to original
	void apply()
	{
		for (auto&& v : intVars) {
			assert(v.actual);
			*v.actual= v.model.med();
		}
	}

protected:
	template <typename T>
	struct VarInfo {
		VarInfo()= default;
		VarInfo(T* actual, Gecode::IntVar model)
			: actual(actual)
			, model(model)
		{ }
		VarInfo(const VarInfo&)= default;
		VarInfo(VarInfo&& other) : VarInfo(other) {}

		VarInfo& operator=(const VarInfo&)= default;
		VarInfo& operator=(VarInfo&& other){
			// std::vector insists using move assign even when its deleted... (gcc 4.8.1)
			return operator=(other); 
		}

		T* actual= nullptr;
		Gecode::IntVar model;
	};

	int getGecodeVar(const int& ref)
	{
		return ref;
	}

	template <typename T>
	Gecode::IntVar getGecodeVar(const Var<T>& value)
	{
		auto it= std::find_if(intVars.begin(), intVars.end(),
				[&value] (const VarInfo<int>& v) { return v.actual == &value.get(); });
		assert(it != intVars.end());
		return it->model;
	}

	
	template <typename T>
	struct GecodeRel;

	/// Expr to Gecode relation which can be feed to Gecode::rel
	template <typename E>
	static auto gecodeRel(E e)
	-> decltype(GecodeRel<E>::eval(e))
	{
		return GecodeRel<E>::eval(e);
	}

	template <typename T>
	struct GecodeRel<Expr<Var<T>>> {
		static auto eval(Expr<Var<T>> e)
		-> decltype(e.get().getDomain().getSolver().getGecodeVar(e.get()))
		{
			return e.get().getDomain().getSolver().getGecodeVar(e.get());
		}
	};

	template <typename T>
	struct GecodeRel<Expr<Constant<T>>> {
		static T eval(Expr<Constant<T>> e)
		{ return e.get().get(); }
	};

	template <typename E1, typename E2, typename Op>
	struct GecodeRel<Expr<BiOp<E1, E2, Op>>> {
		static auto eval(Expr<BiOp<E1, E2, Op>> e)
		-> decltype(Op::eval(gecodeRel(e.get().lhs), gecodeRel(e.get().rhs)))
		{ return	Op::eval(gecodeRel(e.get().lhs), gecodeRel(e.get().rhs)); }
	};

	DynArray<VarInfo<int>> intVars;

	// Completely arbitrary
	static constexpr int minInt= -9999;
	static constexpr int maxInt= 9999;
};


} // eq

#endif // EQ_SOLVERSPACE_HPP
