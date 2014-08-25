template <typename T, typename M>
void VarStorage<T, M>::add(T& ref, M& model)
{
	vars.emplace_back(&ref, &model);
}

template <typename T, typename M>
auto VarStorage<T, M>::getInfo(const T& ref) -> VarInfo&
{
	for (auto&& m : vars) {
		if (m.actual == &ref)
			return m;
	}

	throw std::runtime_error{"var not found"};
}

template <typename T, typename M>
void VarStorage<T, M>::tryEraseInfo(const T& ref)
{
	auto it= std::find_if(vars.begin(), vars.end(),
		[&ref] (const VarInfo& info)
		{
			return info.actual == &ref;
		});

	if (it != vars.end())
		vars.erase(it);
}

