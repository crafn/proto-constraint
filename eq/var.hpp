#ifndef EQ_VAR_HPP
#define EQ_VAR_HPP

#include "basevar.hpp"
#include "domain.hpp"

namespace eq {

/// Variable that has value determined by constraints
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

} // eq

#endif // EQ_VAR_HPP
