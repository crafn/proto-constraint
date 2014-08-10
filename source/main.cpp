#include <iostream>
#include <gecode/int.hh>
#include <gecode/gist.hh>
#include <gecode/minimodel.hh>
#include <memory>
#include <vector>

namespace constraint {
namespace detail {

template <bool B>
struct EnableIf {};

template <>
struct EnableIf<true> { using Type= void; };

} // detail

template <bool B>
using EnableIf= typename detail::EnableIf<B>::Type;

template <typename T>
using RemoveRef= typename std::remove_reference<T>::type;

template <typename T>
using RemoveConst= typename std::remove_const<T>::type;

template <typename T>
class Value;

class Domain;

template <typename T>
struct Expr {
	T value;

public:
	Expr(T t)
		: value(t) { }

	T get() { return value; }

	Domain* getDomain() const { return value.getDomain(); }

	explicit operator bool() const { return value.eval(); }

	auto eval() const
	-> decltype(value.eval())
	{ return value.eval(); }

};

template <typename T>
struct Expr<Value<T>> {
	Expr(Value<T>& value)
		: value(&value)
	{ }

	Value<T>& get() { return *value; }

	Domain* getDomain() const { return &value->getDomain(); }

	T eval() const { return value->get(); }

private:
	Value<T>* value;
};

template <typename T>
struct Constant {
	Constant(T value)
		: value(value)
	{ }

	T get() { return value; }

	Domain* getDomain() const { return nullptr; }

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

	Domain* getDomain() const /// @todo Check for different domains
	{ return lhs.getDomain() ? lhs.getDomain() : rhs.getDomain(); }


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
struct IsValue { static constexpr bool value= false; };

template <typename T>
struct IsValue<Value<T>> { static constexpr bool value= true; };

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
constexpr bool isValue() { return detail::IsValue<T>::value; }

template <typename T1_, typename T2_>
constexpr bool isExprOpQuality()
{
	using T1= RemoveRef<RemoveConst<T1_>>;
	using T2= RemoveRef<RemoveConst<T2_>>;
	return isExpr<T1>() || isValue<T1>() || isExpr<T2>() || isValue<T2>();
}

template <typename T>
constexpr auto expr(T&& t)
-> decltype(detail::ToExpr<T>::eval(std::forward<T>(t)))
{ return detail::ToExpr<T>::eval(std::forward<T>(t)); }


struct Add {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs + rhs)
	{
		return lhs + rhs;
	}
};

struct Sub {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs - rhs)
	{
		return lhs - rhs;
	}
};

struct Eq {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs == rhs)
	{
		return lhs == rhs;
	}

};

struct Greater {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs > rhs)
	{
		return lhs > rhs;
	}
};

struct And {
	template <typename T1, typename T2>
	static auto eval(T1&& lhs, T2&& rhs)
	-> decltype(lhs && rhs)
	{
		return lhs && rhs;
	}
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

/// Register expression as constraint
template <typename E>
void rel(E e)
{
	static_assert(isRelation<E>(), "Expression is not a relation");
	auto domain= e.getDomain();
	assert(domain != nullptr && "Domain not found");
	domain->addRelation(e);
}

template <typename T>
struct PrintType {
	static_assert(sizeof(T) == 0, "");
};

/// Gecode constraint solver
class SolverSpace : public Gecode::Space {
public:

    SolverSpace()= default;
	
	SolverSpace(bool share, SolverSpace& s)
		: Space(share, s), intVars(s.intVars)
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
		std::remove_if(intVars.begin(), intVars.end(),
				[&ref] (const Var<int>& v) { return v.actual == &ref; });
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
	struct Var {
		Var()= default;
		Var(T* actual, Gecode::IntVar model)
			: actual(actual)
			, model(model)
		{ }
		Var(const Var&)= default;
		Var(Var&& other) : Var(other) {}

		Var& operator=(const Var&)= default;
		Var& operator=(Var&& other){
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
	Gecode::IntVar getGecodeVar(const Value<T>& value)
	{
		auto it= std::find_if(intVars.begin(), intVars.end(),
				[&value] (const Var<int>& v) { return v.actual == &value.get(); });
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
	struct GecodeRel<Expr<Value<T>>> {
		static auto eval(Expr<Value<T>> e)
		-> decltype(e.get().getDomain().getSpace().getGecodeVar(e.get()))
		{
			return e.get().getDomain().getSpace().getGecodeVar(e.get());
		}
	};

	template <typename T>
	struct GecodeRel<Expr<Constant<T>>> {
		static T eval(Expr<Constant<T>> e)
		{
			return e.get().get();
		}
	};

	template <typename E1, typename E2, typename Op>
	struct GecodeRel<Expr<BiOp<E1, E2, Op>>> {
		static auto eval(Expr<BiOp<E1, E2, Op>> e)
		-> decltype(Op::eval(gecodeRel(e.get().lhs), gecodeRel(e.get().rhs)))
		{
			return Op::eval(gecodeRel(e.get().lhs), gecodeRel(e.get().rhs));
		}
	};

	std::vector<Var<int>> intVars;

	// Completely arbitrary
	static constexpr int minInt= -9999;
	static constexpr int maxInt= 9999;
};

class Domain {
public:
	void solve() {
		Gecode::DFS<SolverSpace> e(space.get());
		// search and print all solutions
		while (std::unique_ptr<SolverSpace> s{e.next()}) {
			//s->print();
		}  

		space->apply();
		dirty= false;
	}

	//
	// Private interface
	//

	template <typename T>
	void addValue(Value<T>& v)
	{
		space->addVar(v.get());
	}

	template <typename T>
	void removeValue(const Value<T>& v)
	{
		space->removeVar(v.get());
	}

	template <typename T>
	void addRelation(Expr<T> rel)
	{
		static_assert(isRelation<Expr<T>>(), "Expression is not a relation");
		space->addRelation(rel);
		dirty= true;
	}

	SolverSpace& getSpace() { return *space.get(); }

	bool isDirty() const { return dirty; }

private:
	std::unique_ptr<SolverSpace> space{new SolverSpace};
	bool dirty= false;
};

/// A value that is solved by constraints
template <typename T>
class Value {
public:

	Value(Domain& domain, T&& t= 0)
		: domain(&domain)
		, value(std::move(t))
	{
		domain.addValue(*this);
	}

	~Value()
	{
		domain->removeValue(*this);
	}

	Value(const Value&)= delete; /// @todo
	Value(Value&&)= delete; /// @todo

	T& get() { return value; }
	const T& get() const { return value; }

	operator const T&() const
	{
		if (getDomain().isDirty())
			getDomain().solve();
		return value;
	}

	Domain& getDomain() const { return *domain; }

private:
	Domain* domain;
	T value;
};

} // constraint
namespace gui {

class Box {
public:

	Box(constraint::Domain& solver)
		: m_top(solver, 0)
		, m_bottom(solver, 0)
	{
		constraint::rel(height() > 0); /// @todo Change to >=
	}

	constraint::Value<int>& top() { return m_top; }
	constraint::Value<int>& bottom() { return m_bottom; }

	auto height() -> decltype(top() - bottom()) { return top() - bottom(); }

private:
	constraint::Value<int> m_top;
	constraint::Value<int> m_bottom;
};

} // gui

std::ostream& operator<< (std::ostream& stream, gui::Box& box)
{
	stream << "top: " << box.top().get() << ", bottom: " << box.bottom().get();
}

int main()
{
	constraint::Domain domain;

	gui::Box box1{domain}, box2{domain};
	rel(box1.top() == 30 && box2.bottom() == 0);
	rel(box1.bottom() == box2.top() && box1.height() == box2.height());

	std::cout << box1.bottom() << std::endl;

	std::cout << "box1 " << box1 << std::endl;
	std::cout << "box2 " << box2 << std::endl;


	constraint::Domain d;
	constraint::Value<int> x{d}, y{d};
	rel(x + x == y && y + 1 == x - 1);
	std::cout << x << ", " << y << std::endl; // -2, -4
}
