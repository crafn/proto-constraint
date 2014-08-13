#include "rel.hpp"
#include "var.hpp"

#include <iostream>

namespace gui {

class Box {
public:

	Box()
	{
		eq::rel(height() >= 0);
	}

	eq::Var<int>& top() { return m_top; }
	eq::Var<int>& bottom() { return m_bottom; }

	auto height() -> decltype(top() - bottom()) { return top() - bottom(); }

private:
	eq::Var<int> m_top;
	eq::Var<int> m_bottom;
};

} // gui

std::ostream& operator<< (std::ostream& stream, gui::Box& box)
{
	stream << "top: " << box.top() << ", bottom: " << box.bottom();
}

int main()
{
	{
		gui::Box box1, box2;
		rel(box1.top() == 30 && box2.bottom() == 0);
		rel(box1.bottom() == box2.top() && box1.height() == box2.height());

		std::cout << "box1 " << box1 << std::endl;
		std::cout << "box2 " << box2 << std::endl;
	}

	{
		eq::Var<int> x, y;
		//eq::rel(x == y/2 && y + 1 == x - 1);
		//eq::rel(x == 1, 100);
		eq::rel(x > 0, 3);
		eq::rel(x < 10, 2);
		eq::rel(x*x == 12*12);
		//eq::rel(x == 2, 5);
		std::cout << x << ", " << y << std::endl; // -2, -4
	}
}
