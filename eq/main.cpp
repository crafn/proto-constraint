#include <iostream>
#include <set>
#include <gecode/int.hh>
#include <gecode/gist.hh>
#include <gecode/minimodel.hh>
#include <map>
#include <memory>
#include <vector>

namespace eq {
namespace detail {

template <bool B>
struct EnableIf {};

template <>
struct EnableIf<true> { using Type= void; };

} // detail

/// Temp typedefs until using clover-util
template <typename... Ts>
using Set= std::set<Ts...>;
template <typename... Ts>
using DynArray= std::vector<Ts...>;
template <typename... Ts>
using Map= std::map<Ts...>;
template <typename T>
using UniquePtr= std::unique_ptr<T>;
template <typename T>
using SharedPtr= std::shared_ptr<T>;

template <typename... Ts>
Set<Ts...> operator+(Set<Ts...> lhs, const Set<Ts...>& rhs)
{
	lhs.insert(rhs.begin(), rhs.end());
	return lhs;
}

template <typename... Ts>
DynArray<Ts...> operator+(DynArray<Ts...> lhs, const DynArray<Ts...>& rhs)
{
	lhs.insert(lhs.end(), rhs.begin(), rhs.end());
	return lhs;
}

template <typename... Ts>
Map<Ts...> operator+(Map<Ts...> lhs, const Map<Ts...>& rhs)
{
	lhs.insert(rhs.begin(), rhs.end());
	return lhs;
}
template <typename C, typename E>
void eraseFrom(C& c, E&& e)
{
	auto it= find(c.begin(), c.end(), e);
	assert(it != c.end());
	c.erase(it);
}

template <typename... Ts, typename E>
void eraseFrom(std::map<Ts...>& c, E&& e)
{
	auto it= c.find(e);
	assert(it != c.end());
	c.erase(it);
}

template <typename C, typename F>
void eraseIf(C& c, F&& f)
{
	c.erase(std::remove_if(c.begin(), c.end(), f), c.end());
}

template <bool B>
using EnableIf= typename detail::EnableIf<B>::Type;

template <typename T>
using RemoveRef= typename std::remove_reference<T>::type;

template <typename T>
using RemoveConst= typename std::remove_const<T>::type;

class BaseVar;
template <typename T>
class Var;

class Domain;
using DomainPtr= SharedPtr<Domain>;

template <typename T>
struct Expr {
	T value;

public:
	Expr(T t)
		: value(t) { }

	T get() { return value; }

	Set<BaseVar*> getVars() const { return value.getVars(); }

	explicit operator bool() const { return value.eval(); }

	auto eval() const
	-> decltype(value.eval())
	{ return value.eval(); }

};

template <typename T>
struct Expr<Var<T>> {
	Expr(Var<T>& value)
		: value(&value)
	{ }

	Var<T>& get() { return *value; }

	Set<BaseVar*> getVars() const { return {value}; }

	T eval() const { return value->get(); }

private:
	Var<T>* value;
};

template <typename T>
struct Constant {
	Constant(T value)
		: value(value)
	{ }

	T get() { return value; }

	Set<BaseVar*> getVars() const { return {}; }

	T eval() const { return value; }

private:
	T value;
};

template <typename E1, typename E2, typename Op>
struct BiOp {
	E1 lhs;
	E2 rhs;

	BiOp(E1 lhs, E2 rhs)
		: lhs(lhs), rhs(rhs) { }

	Set<BaseVar*> getVars() const
	{ return lhs.getVars() + rhs.getVars(); }

	auto eval() const
	-> decltype(Op::eval(lhs.eval(), rhs.eval()))
	{ return Op::eval(lhs.eval(), rhs.eval()); }
};
namespace detail {

template <typename T>
struct IsExpr { static constexpr bool value= false; };

template <typename T>
struct IsExpr<Expr<T>> { static constexpr bool value= true; };

template <typename T>
struct IsVar { static constexpr bool value= false; };

template <typename T>
struct IsVar<Var<T>> { static constexpr bool value= true; };

/// @todo Simplify

template <typename T>
struct ToExpr {
	using PlainT= RemoveRef<RemoveConst<T>>;
	static Expr<PlainT> eval(T t) { return Expr<PlainT>{std::forward<T>(t)}; }
};

template <typename T>
struct ToExpr<T&> {
	using PlainT= RemoveRef<RemoveConst<T>>;
	static Expr<PlainT> eval(T& t) { return Expr<PlainT>{t}; }
};

template <typename T>
struct ToExpr<Expr<T>> {
	static Expr<T> eval(Expr<T> t) { return t; }
};

template <typename T>
struct ToExpr<Expr<T>&> {
	static Expr<T> eval(Expr<T>& t) { return t; }
};

template <>
struct ToExpr<int> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<const int> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

template <>
struct ToExpr<int&> {
	static Expr<Constant<int>> eval(int t)
	{ return Expr<Constant<int>>{t}; }
};

} // detail

template <typename T>
constexpr bool isExpr() { return detail::IsExpr<T>::value; }

template <typename T>
constexpr bool isVar() { return detail::IsVar<T>::value; }

template <typename T1_, typename T2_>
constexpr bool isExprOpQuality()
{
	using T1= RemoveRef<RemoveConst<T1_>>;
	using T2= RemoveRef<RemoveConst<T2_>>;
	return isExpr<T1>() || isVar<T1>() || isExpr<T2>() || isVar<T2>();
}

template <typename T>
constexpr auto expr(T&& t)
-> decltype(detail::ToExpr<T>::eval(std::forward<T>(t)))
{ return detail::ToExpr<T>::eval(std::forward<T>(t)); }


struct Add {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs + rhs)
	{ return	lhs + rhs; }
};

struct Sub {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs - rhs)
	{ return	lhs - rhs; }
};

struct Eq {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs == rhs)
	{ return	lhs == rhs; }
};

struct Greater {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs > rhs)
	{ return	lhs > rhs; }
};

struct And {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs && rhs)
	{ return	lhs && rhs; }
};

/// @todo Not sure if needs perfect forwarding

/// +
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator+(E1&& e1, E2&& e2)
-> Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Add>>
{
	return BiOp<decltype(expr(e1)), decltype(expr(e2)), Add>{expr(e1), expr(e2)};
}

/// -
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator-(E1&& e1, E2&& e2)
-> Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Sub>>
{
	return BiOp<decltype(expr(e1)), decltype(expr(e2)), Sub>{expr(e1), expr(e2)};
}

/// ==
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator==(E1&& e1, E2&& e2)
-> Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Eq>>
{
	return BiOp<decltype(expr(e1)), decltype(expr(e2)), Eq>{expr(e1), expr(e2)};
}


/// >
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator>(E1&& e1, E2&& e2)
-> Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), Greater>>
{
	return BiOp<decltype(expr(e1)), decltype(expr(e2)), Greater>{expr(e1), expr(e2)};
}

/// &&
template <typename E1, typename E2, typename=
	EnableIf<isExprOpQuality<E1, E2>()>>
auto operator&&(E1&& e1, E2&& e2)
-> Expr<BiOp<decltype(expr(e1)), decltype(expr(e2)), And>>
{
	return BiOp<decltype(expr(e1)), decltype(expr(e2)), And>{expr(e1), expr(e2)};
}

// Expression parsing

namespace detail {

template <typename T>
struct IsRelation {
	static constexpr bool value= false;
};

template <typename E1, typename E2>
struct IsRelation<Expr<BiOp<E1, E2, Eq>>> {
	static constexpr bool value= true;
};

template <typename E1, typename E2>
struct IsRelation<Expr<BiOp<E1, E2, Greater>>> {
	static constexpr bool value= true;
};

template <typename E1, typename E2>
struct IsRelation<Expr<BiOp<E1, E2, And>>> {
	static constexpr bool value= IsRelation<E1>::value && IsRelation<E2>::value;
};

} // detail

template <typename T>
constexpr bool isRelation() { return detail::IsRelation<T>::value; }


template <typename T>
struct PrintType {
	static_assert(sizeof(T) == 0, "");
};

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

class BaseVar {
public:

	/// @todo These could be protected
	Domain& getDomain() const { return *domain; }
	void setDomainPtr(DomainPtr ptr) { domain= ptr; }
	DomainPtr getDomainPtr() { return domain; }

protected:
	DomainPtr domain;
};


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

/// Var that is determined by constraints
template <typename T>
class Var : public BaseVar {
public:

	Var()
	{
		/// @todo Create domain as late as possible
		domain= std::make_shared<Domain>();
		domain->addVar(*this);
	}

	~Var()
	{
		domain->removeVar(*this);
	}

	Var(const Var&)= delete; /// @todo
	Var(Var&&)= delete; /// @todo

	operator const T&() const
	{
		if (getDomain().isDirty())
			getDomain().solve();
		return value;
	}

	/// @todo These could be private
	T& get() { return value; }
	const T& get() const { return value; }

private:
	T value;
};

/// Register expression as relation
template <typename E>
void rel(E e)
{
	static_assert(isRelation<E>(), "Expression is not a relation");
	auto&& ds= domains(e.getVars());
	assert(!ds.empty() && "Domain not found");

	auto preserved= *ds.begin();
	// Merge all domains which take part in the relation
	for (auto&& d : ds) {
		if (d == preserved)
			continue;

		preserved->merge(std::move(*d));
	}

	preserved->addRelation(e);
}

} // eq
namespace gui {

class Box {
public:

	Box()
	{
		eq::rel(height() > 0); /// @todo Change to >=
	}

	eq::Var<int>& top() { return m_top; }
	eq::Var<int>& bottom() { return m_bottom; }

	auto height() -> decltype(top() - bottom()) { return top() - bottom(); }

private:
	eq::Var<int> m_top;
	eq::Var<int> m_bottom;
};

} // gui

std::ostream& operator<< (std::ostream& stream, gui::Box& box)
{
	stream << "top: " << box.top() << ", bottom: " << box.bottom();
}

int main()
{
	{
		gui::Box box1, box2;
		rel(box1.top() == 30 && box2.bottom() == 0);
		rel(box1.bottom() == box2.top() && box1.height() == box2.height());

		std::cout << "box1 " << box1 << std::endl;
		std::cout << "box2 " << box2 << std::endl;
	}

	{
		eq::Var<int> x, y;
		rel(x + x == y && y + 1 == x - 1);
		std::cout << x << ", " << y << std::endl; // -2, -4
	}
}
