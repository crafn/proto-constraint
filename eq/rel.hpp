#ifndef EQ_REL_HPP
#define EQ_REL_HPP

#include "domain.hpp"
#include "expr.hpp"

namespace eq {

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

#endif // EQ_REL_HPP
