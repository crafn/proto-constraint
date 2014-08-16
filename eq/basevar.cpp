#include "basevar.hpp"
#include "varhandle.hpp"

namespace eq {

BaseVar::~BaseVar()
{
	while (!handles.empty()) {
		auto&& h= handles.back();
		ensure(h);
		ensure(*h);
		ensure(this == &h->get());
		h->clear();
	}
}

BaseVar::BaseVar(BaseVar&& other)
{
	operator=(std::move(other));
}

BaseVar& BaseVar::operator=(BaseVar&& other)
{
	if (this != &other) {
		domain= std::move(other.domain);

		// Clear current handles
		for (auto&& h : handles) {
			ensure(h);
			h->clear();
		}

		handles.clear();

		// Add handles of other
		while (!other.handles.empty()) {
			ensure(other.handles.back());
			other.handles.back()->redirect(*this);
		}

		ensure(other.handles.empty());
	}
	return *this;
}

}
