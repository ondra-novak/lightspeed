/*
 * queryParser.cpp
 *
 *  Created on: 24.9.2013
 *      Author: ondra
 */


#include "queryParser.h"

#include "../base/containers/constStr.h"
#include "../base/iter/iteratorFilter.tcc"
#include "../base/containers/autoArray.tcc"
#include "urlencode.h"
namespace LightSpeed {



static ConstStrA findBeginQuery(ConstStrA vpath) {

	natural q = vpath.find('?');
	if (q == naturalNull) return vpath;
	else return vpath.head(q);
}

QueryParser::QueryParser(ConstStrA vpath)
	:_path(findBeginQuery(vpath)),_query(vpath.offset(_path.length()+1))
	,_iter(_query.split('&')),_swp(0)
{
	preload();
}

ConstStrA QueryParser::getPath() const {
	return _path;
}

const QueryField& QueryParser::getNext() {
	const QueryField *out = _tmpResult+_swp;
	preload();
	return *out;

}

const QueryField& QueryParser::peek() const {
	return _tmpResult[_swp];
}

LightSpeed::ConstStrA QueryParser::getQuery() const {
	return _query;
}

bool QueryParser::hasItems() const {
	return _hasNext;
}

void QueryParser::preload() {
	int ld = 1-_swp;
	if (!_iter.hasItems()) {
		_hasNext = false;
	} else {
		ConstStrA itm = _iter.getNext();
		natural q = itm.find('=');
		ConstStrA field;
		ConstStrA val;
		if (q == naturalNull) {
			if (itm.empty()) preload();
			else {
				field = itm;
				val = ConstStrA("1");
			}
		} else {
			field = itm.head(q);
			val = itm.offset(q+1);
		}

		Filter<UrlDecoder>::Read<ConstStrA::Iterator> rd(val.getFwIter());
		_tempVal[ld].clear();
		_tempVal[ld].copy(rd);
		_tmpResult[ld].name = field;
		_tmpResult[ld].value = _tempVal[ld].getArray();
		_swp = ld;
		_hasNext = true;
	}
}

}
