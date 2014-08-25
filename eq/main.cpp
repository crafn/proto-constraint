#include "rel.hpp"
#include "var.hpp"

#include <iostream>

namespace gui {

class Screen {
public:

	Screen(size_t size_x, size_t size_y)
		: sizeX(size_x)
		, sizeY(size_y)
		, chars(sizeX*sizeY, ' ')
	{ }

	void set(int x, int y, char c)
	{
		ensure(x >= 0 && y >= 0 && x < sizeX && y < sizeY);
		// Upside down
		size_t index= x + (sizeY - y - 1)*sizeX;
		ensure(index < chars.size());
		chars[index]= c;
	}

	void draw()
	{
		size_t x= 0;
		for (auto&& c : chars)
		{
			if (x == sizeX) {
				std::cout << "\n";
				x= 0;
			}

			std::cout << c;
			++x;
		}
		std::cout << "\n";
	}

private:
	size_t sizeX, sizeY;
	std::vector<char> chars;
};

class Box {
public:

	Box()
	{
		rel(height() >= 0 && width() >= 0);
		++nextCh;
	}

	const eq::Var<int>& right() const { return right_; }
	const eq::Var<int>& top() const { return top_; }
	const eq::Var<int>& left() const { return left_; }
	const eq::Var<int>& bottom() const { return bottom_; }

	auto height() const -> decltype(top() - bottom()) { return top() - bottom(); }
	auto width() const -> decltype(right() - left()) { return right() - left(); }

	auto contains(const Box& other) const
	-> decltype(right() <= right() && // Waiting for C++14...
				top() <= top() &&
				left() >= left() &&
				bottom() >= bottom())
	{
		return	other.right() <= right() &&
				other.top() <= top() &&
				other.left() >= left() &&
				other.bottom() >= bottom();
	}

	void add(Box& box)
	{
		rel(contains(box));
		subBoxes.push_back(&box);
	}

	void set(int x, int y, int w, int h)
	{
		rel(left() == x && bottom() == y && width() == w && height() == h);
	}

	void draw(Screen& s) const
	{
		for (int y= bottom(); y < top(); ++y) {
			for (int x= left(); x < right(); ++x) {

				int v= 0, h= 0;

				if (x == left())
					v= -1;
				else if (x == right() - 1)
					v= 1;

				if (y == bottom())
					h= -1;
				else if (y == top() - 1)
					h= 1;

				char ch= ' ';
				if (!v && !h) {
					ch= bgChar;
				}
				else if (v && !h) {
					ch= '|';
				}
				else if (!v && h) {
					ch= '~';
				}
				else {
					if (v*h > 0)
						ch= '\\';
					else
						ch= '/';
				}
				
				s.set(x, y, ch);
			}
		}

		for (Box* b : subBoxes)
			b->draw(s);
	}

private:
	static char nextCh;

	eq::Var<int> right_, top_, left_, bottom_;
	std::vector<Box*> subBoxes;
	char bgChar= nextCh;
};

char Box::nextCh= '+';

} // gui

int main()
{
	{
		gui::Screen screen{30, 30};
		gui::Box a, b, c;

		a.set(0, 0, 30, 30);

		a.add(b);
		rel(	b.bottom() == a.bottom() &&
				b.left() == a.left() &&
				b.height() == a.height()*2/3);

		a.add(c);
		rel(	c.top() == a.top() &&
				c.right() == a.right()  &&
				c.height() == a.height()/2);

		rel(b.width() == c.width());

		a.draw(screen);
		screen.draw();
	}

	{
		eq::PriorityVar low, high;
		rel(high > low);

		eq::Var<int> x, y;
		
		rel(x > 0, low);
		rel(x < 10, high);
		rel(x*x == 12*12);
		rel(y == 2*x, low);

		std::cout << x << ", " << y << std::endl;
	}

	{
		eq::PriorityVar low, med, high;
		rel(low < med && med < high);

		eq::Var<int> x;
		rel(x == 1, high);
		rel(x == 2, med);
		rel(x == 3, low);

		eq::Var<int> y= std::move(x);
		std::cout << y << std::endl;
	}

	{
		// Uses linear solver
		eq::Var<double> x, y;
		rel(y == 1.1);
		rel(x + y == 1.5);
		std::cout << "Linear: " << x << std::endl;
	}
}
