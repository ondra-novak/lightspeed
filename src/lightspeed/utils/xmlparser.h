#pragma once
#include "../base/containers/constStr.h"
#include "../base/iter/iterator.h"
#include "../base/streams/fileio.h"
#include "../base/text/textstream.h"

namespace LightSpeed {




	struct XMLEntity {

		enum Type {
			///Entity contains single text character (outside XML tag - chr is valid)
			textchar,
			///Entity contains open of a tag (tag member is valid)
			opentag,
			///Entity contains close of the tag (tag member is valid)
			closetag,
			///Entity contains attribute in the tag (tag member, attr member and value member are valid)
			attribute,
			///reads characters from comment
			commentText,
		};

		Type type;
		StringWI tag;
		StringWI attr;
		String value;
		wchar_t chr;
		bool endTag;

		bool enteringTag(ConstStrW name, bool endTag = false) const { return type == opentag && tag == name && endTag == this->endTag; }
		bool enteringTag() const { return type == opentag; }
		bool exitingTag(ConstStrW name, bool endTag = false) const { return type == closetag && tag == name && endTag == this->endTag; }
		bool exitingTag() const { return type == closetag; }
		bool isAttribute(ConstStrW name) const { return type == attribute && attr == name; }
		bool isAttribute() const { return type == attribute; }
		bool isText() const { return type == textchar; }
		bool isComment() const { return type == commentText; }
	};


	template<typename Scanner>
	class XMLIteratorT: public IteratorBase<XMLEntity, XMLIteratorT<Scanner> > {
	public:

		XMLIteratorT(SeqFileInput &sfi, bool skipComments = false);
		bool hasItems() const;
		const XMLEntity &getNext();
		const XMLEntity &peek() const;
		XMLIteratorT &skipBlock();
		XMLIteratorT &skipTag();

		///reads text until tag is found;
		String readText() ;

	protected:

		XMLEntity p1,p2;
		XMLEntity *cur, *next;		
		Scanner txtin;

		void loadNext();

		void eatSpace();
		enum State {
			textBegin,
			textNext,
			textCData,			
			intag,
			incomment,
			inclosetag,
			inendtag,
			incloseendtag,
		};

		State curState;
		String curTag;
		String curAttr;
		String curValue;
		bool skipComments;
			 
		wchar_t loadEntity(ConstStrW ent);
		void readAttr();
		void enterTag();
		String fixEntities(ConstStrW text);




	};

	typedef XMLIteratorT<ScanTextW> XMLIterator;
	typedef XMLIteratorT<ScanTextWW> XMLIteratorW;

#ifndef LS_MSVC_SKIPEXTERN_XMLIterator
	extern template class XMLIteratorT < ScanTextW > ;
	extern template class XMLIteratorT < ScanTextWW > ;
#endif

}
