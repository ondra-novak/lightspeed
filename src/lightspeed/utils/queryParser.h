/*
 * queryParser.h
 *
 *  Created on: 24.9.2013
 *      Author: ondra
 */

#ifndef JSONSERVER_QUERYPARSER_H_
#define JSONSERVER_QUERYPARSER_H_
#include "../base/containers/autoArray.h"
#include "../base/containers/constStr.h"
#include "../base/memory/smallAlloc.h"

namespace LightSpeed {

	struct QueryField {

		ConstStrA name;
		ConstStrA value;

	};

	///Parses URL query and splits it to pairs field=value. Performs url-decoding
	class QueryParser: public IteratorBase<QueryField,QueryParser> {
	public:

		QueryParser(ConstStrA vpath);

		ConstStrA getPath() const;
		ConstStrA getQuery() const;
		const QueryField &getNext();
		const QueryField &peek() const;
		bool hasItems() const;

	protected:

		ConstStrA _path;
		ConstStrA _query;
		AutoArrayStream<char, SmallAlloc<100> > _tempVal[2];
		QueryField _tmpResult[2];
		ConstStrA::SplitIterator _iter;
		bool _hasNext;
		int _swp;

		void preload();

	};


}


#endif /* JSONSERVER_QUERYPARSER_H_ */

