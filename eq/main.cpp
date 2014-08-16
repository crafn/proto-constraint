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

	/// @todo Const?
	eq::Var<int>& right() { return right_; }
	eq::Var<int>& top() { return top_; }
	eq::Var<int>& left() { return left_; }
	eq::Var<int>& bottom() { return bottom_; }

	auto height() -> decltype(top() - bottom()) { return top() - bottom(); }
	auto width() -> decltype(right() - left()) { return right() - left(); }

	template <typename T>
	auto contains(T& other) /// @todo Should have const (?)
	-> decltype(other.right() <= right() &&
				other.top() <= top() &&
				other.left() >= left() &&
				other.bottom() >= bottom())
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

	void draw(Screen& s) /// @todo Should have const
	{
		for (int y= bottom(); y < top(); ++y) {
			for (int x= left(); x < right(); ++x) {
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
	char ch= nextCh;
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
		a.add(c);
		rel(	b.bottom() == a.bottom() &&
				b.bottom() == c.bottom() && 
				b.width() + c.width() == a.width() && 
				b.right() == c.left() && 
				b.height() == a.height()/2 &&
				c.height() == a.height()/3);

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
		eq::PriorityVar init;
		eq::Var<int> a, b;

		rel(b == 1337, init);
		rel(a == 1 && b == 2);
		a.clear();

		std::cout << b << std::endl; // 1337
	}
}
