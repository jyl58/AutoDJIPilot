#pragma once

#include <sstream>
namespace Tools{
	
	template <typename T>
	std::string ToString(T value,int pre){
		std::ostringstream out;
		out.precision(pre);
		out<<value;
		return out.str();
	}
};
