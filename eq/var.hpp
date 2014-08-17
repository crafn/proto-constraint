#ifndef EQ_VAR_HPP
#define EQ_VAR_HPP

#include "basevar.hpp"
#include "domain.hpp"

namespace eq {

/// Variable that has value determined by constraints
template <typename T, VarType type= VarType::normal>
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
		if (domain)
			domain->removeVar(*this);
	}

	Var(const Var&)= default;
	Var(Var&&)= default;
	Var& operator=(const Var&)= default;
	Var& operator=(Var&&)= default;

	operator const T&() const
	{
		getDomain().solve();
		return value;
	}

	void clear()
	{
		getDomain().removeVar(*this);
		getDomain().addVar(*this);
	}

	/// @todo Could be private
	T& get() const { return value; }

private:
	/// Consider:
	///	  auto area() const { return width*height; } // Expr of const Vars
	/// It should be possible to use this in the following contexts:
	///	  if (obj.area() > 5) { ... } // Works
	///   rel(x == obj.area()); // Fails because of const members in Vars
	/// -> const machinery of C++ is too limited for this use case so we need
	/// to bypass it with mutable.
	/// This is probably not so bad because we aren't trying to represent
	/// conventional state, but a handle to mathematical entity
	mutable T value= 0;
};

} // eq

#endif // EQ_VAR_HPP
