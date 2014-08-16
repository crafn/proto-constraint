#ifndef EQ_VARHANDLE_HPP
#define EQ_VARHANDLE_HPP

namespace eq {

class BaseVar;

/// @todo Generalize and add to util
class VarHandle {
public:
	using Var= eq::BaseVar;
	using OnClear= std::function<void ()>;

	VarHandle(Var& v)
	{
		redirect(v);
	}

	~VarHandle()
	{
		clear();
	}

	VarHandle(const VarHandle& other)
	{ operator=(other); }

	VarHandle(VarHandle&& other)
	{ operator=(std::move(other)); }

	VarHandle& operator=(const VarHandle& other)
	{
		if (this != &other) {
			if (other)
				redirect(other.get());
			else
				clear();
		}
		return *this;	
	}

	VarHandle& operator=(VarHandle&& other)
	{
		if (this != &other) {
			if (other)
				redirect(other.get());
			else 
				clear();

			other.clear();
		}
		return *this;
	}

	Var* operator->() const
	{
		ensure(var);
		return var;
	}

	Var& get() const
	{
		ensure(var);
		return *var;
	}

	explicit operator bool() const { return !isNull(); }
	bool isNull() const { return var == nullptr; }

	void redirect(Var& v)
	{
		clear();

		var= &v;
		var->handles.push_back(this);
	}

	void clear()
	{
		if (var)
			eraseFrom(var->handles, this);
		var= nullptr;
	}

private:
	Var* var= nullptr;
};

} // eq

#endif // EQ_VARHANDLE_HPP
