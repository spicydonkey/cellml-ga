#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <limits>

#define MAX_DOUBLE std::numeric_limits<double>::max()


std::string convert(const std::wstring& wstr);	// convert wstring to a char string with '_' as default char
std::wstring convert(const std::string& str);	// convert string to a wstring

// generate a random double in [min,max]
double rnd_generate(double min, double max);


// Evaluate equality of pair by comparing the first member
template<class T,class S> struct pair_equal_to:std::binary_function<T,std::pair<T,S>,bool> {
	bool operator()(const T& y, const std::pair<T,S>& x) const
	{
		return x.first==y;
	}
};


// Evaluate if value is within eps range from point
inline bool in_range(double point, double value, double eps)
{
    return (point>=value-eps && point<=value+eps);
}


// Get current date/time: YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime();

#endif