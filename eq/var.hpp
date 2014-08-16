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

	Var(const Var&)= default;
	Var(Var&&)= default;
	Var& operator=(const Var&)= default;
	Var& operator=(Var&&)= default;

	operator const T&() const
	{
		getDomain().solve();
		return value;
	}

	/// @todo These could be private
	T& get() { return value; }
	const T& get() const { return value; }

private:
	T value= 0;
};

} // eq

#endif // EQ_VAR_HPP
