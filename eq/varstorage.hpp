#ifndef EQ_VARSTORAGE_HPP
#define EQ_VARSTORAGE_HPP

#include "util.hpp"

namespace eq {

template <typename T, typename M>
class VarStorage {
public:
	struct VarInfo {
		VarInfo()= default;
		VarInfo(T* actual, M* model)
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
		M* model= nullptr;
	};
	
	using Iter= typename DynArray<VarInfo>::iterator;
	using CIter= typename DynArray<VarInfo>::const_iterator;

	void add(T& ref, M& model);
	VarInfo& getInfo(const T& ref);
	void tryEraseInfo(const T& ref);

	Iter begin() { return vars.begin(); }
	Iter end() { return vars.end(); }

	CIter begin() const { return vars.begin(); }
	CIter end() const { return vars.end(); }

private:
	DynArray<VarInfo> vars;

};

#include "varstorage.tpp"

} // eq

#endif // EQ_VARSTORAGE_HPP
