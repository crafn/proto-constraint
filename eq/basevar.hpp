#ifndef EQ_BASEVAR_HPP
#define EQ_BASEVAR_HPP

#include "util.hpp"

namespace eq {

class Domain;
using DomainPtr= SharedPtr<Domain>;

template <typename T>
class Var;

class BaseVar {
public:

	/// @todo These could be protected
	Domain& getDomain() const { return *domain; }
	void setDomainPtr(DomainPtr ptr) { domain= ptr; }
	DomainPtr getDomainPtr() { return domain; }

protected:
	DomainPtr domain;
};

} // eq

#endif // EQ_BASEVAR_HPP
