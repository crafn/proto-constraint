#ifndef EQ_REL_HPP
#define EQ_REL_HPP

#include "domain.hpp"
#include "expr.hpp"

namespace eq {
namespace detail {

template <typename E>
DomainOf<E>& mergeDomains(E&& e)
{
	auto&& base_ds= domains(e.getVars());
	ensure(!base_ds.empty() && "Domain not found");

	using Domain= DomainOf<E>;
	DynArray<Domain*> ds;
	for (auto&& d : base_ds)
		ds.push_back(&static_cast<Domain&>(*d));
	
	auto&& preserved= *ds.begin();
	// Merge all domains which take part in the relation
	for (auto&& d : ds) {
		if (d == preserved)
			continue;

		preserved->merge(std::move(*d));
	}

	return *preserved;
}

} // detail

/// Register expression as relation
template <typename E>
void rel(E e)
{
	static_assert(isRelation<E>(), "Expression is not a relation");
	detail::mergeDomains(e).addRelation(e);
}

/// Register expression as a soft relation
template <typename E>
void rel(E e, PriorityVar& priority)
{
	static_assert(isRelation<E>(), "Expression is not a relation");
	detail::mergeDomains(e).addRelation(e, priority);
}

} // eq

#endif // EQ_REL_HPP
