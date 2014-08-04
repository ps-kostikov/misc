#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

int main()
{
	std::vector<int> v;
	v.push_back(1);
	v.push_back(2);
	v.push_back(3);

	std::map<int, int> m;
	m[1] = 10;
	m[2] = 20;
	m[3] = 30;

	std::for_each(v.begin(), v.end(), [](int i) {
		std::cout << i << std::endl;
	});
	std::cout << std::endl;

	for (int i : v) {
		std::cout << i << std::endl;
	}
	std::cout << std::endl;

	std::for_each(m.begin(), m.end(), [](std::pair<int, int> p) {
		std::cout << p.first << ": " << p.second << std::endl;
	});
	std::cout << std::endl;

	for (auto& p : m) {
		std::cout << p.first << ": " << p.second << std::endl;
	}
	std::cout << std::endl;

    return 0;
}
