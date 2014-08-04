#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>

#include <iostream>
#include <string>


namespace mj = maps::json;

int main()
{
	std::cout << "hello" << std::endl;
	mj::Builder builder;
	builder << [](mj::ObjectBuilder b) {
		b["aba"] = 1;
		b["array"] = [](mj::ArrayBuilder b) {
			b << "A" << "B" << "C";
		};
	};
	std::cout << builder.str() << std::endl;

	std::string json_str = builder.str();

	auto val = mj::Value::fromString(json_str);
	for (mj::Value v : val["array"]) {
		std::cout << v.as<std::string>() << std::endl;
	}
	return 0;
}