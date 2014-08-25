#ifndef EQ_BASEVAR_HPP
#define EQ_BASEVAR_HPP

#include "util.hpp"

namespace eq {

class BaseDomain;
using BaseDomainPtr= SharedPtr<BaseDomain>;
class VarHandle;

enum class VarType {
	normal,
	priority
};

template <typename T, VarType type>
class Var;
using PriorityVar= Var<int, VarType::priority>;

class BaseVar {
public:
	BaseVar()= default;
	~BaseVar();
	BaseVar(const BaseVar&)= delete;
	BaseVar(BaseVar&& other);

	/// @todo Figure out how copy semantics should work with relations
	BaseVar& operator=(const BaseVar& other)= delete;
	BaseVar& operator=(BaseVar&& other);

	/// @todo These could be protected
	BaseDomain& getDomain() const { return *domain; }
	void setDomainPtr(BaseDomainPtr ptr) { domain= ptr; }
	BaseDomainPtr getDomainPtr() { return domain; }

private:
	friend class VarHandle;
	BaseDomainPtr domain;
	/// Handles to this
	DynArray<VarHandle*> handles;
};

} // eq

#endif // EQ_BASEVAR_HPP
