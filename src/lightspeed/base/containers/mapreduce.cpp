/*
 * mapreduce.cpp
 *
 *  Created on: 10. 4. 2016
 *      Author: ondra
 */

#include "mapreduce.tcc"
#include "map.tcc"

namespace LightSpeed {

template<typename Value>
struct ReduceSum {
	Value result;
	ReduceSum():result(0) {}
	ReduceSum(const Value &v):result(v) {}
	template<typename Key>
	ReduceSum(const Key &, const Value &v):result(v) {}
	ReduceSum operator+(const ReduceSum &other) const {
		return ReduceSum(result + other.result);
	}
};


template class MapReduce<int, float, ReduceSum<float> >;

}


