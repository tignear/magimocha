#include <iostream>

using namespace std;
class Foo;
int main() {
	cout << is_constructible<pair<const int, unique_ptr<Foo> >, pair<const int, unique_ptr<Foo> >& >::value << '\n';
	cout << is_constructible<pair<const int, unique_ptr<Foo> >, pair<const int, unique_ptr<Foo> >&& >::value << '\n';
	system("pause");
}