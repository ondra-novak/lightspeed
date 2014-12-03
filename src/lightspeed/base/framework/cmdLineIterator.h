#pragma once
#include "../containers/constStr.h"
#include "app.h"


namespace LightSpeed {

	struct CmdLineItem {
		
		enum ItemType {
			shortSwitch,
			longSwitch,
			text
		};

		ItemType itemType;
		ConstStrW value;

		CmdLineItem () {}
		CmdLineItem (ItemType type, ConstStrW v):itemType(type),value(v) {}
	};

class CmdLineIterator :public IteratorBase<CmdLineItem, CmdLineIterator>
{
public:
	CmdLineIterator(const IApp::Args &args, natural offset = 1);

	const CmdLineItem &getNext();
	const CmdLineItem &peek() const;
	bool hasItems() const;
	bool equalTo(const CmdLineIterator &other) const;
	bool lessThan(const CmdLineIterator &other) const;
	ConstStrW peekText() const;
	ConstStrW getNextText() ;
	integer getNextNumber(natural base = 10);
	IApp::Args getRemainArgs();

protected:
	const IApp::Args &args;
	natural argPos,charPos;
	mutable CmdLineItem tmp;
	bool readRaw;
};



}
