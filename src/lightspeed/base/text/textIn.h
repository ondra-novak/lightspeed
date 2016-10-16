/** @file
  * $Id: textIn.h 311 2012-07-16 14:59:40Z ondrej.novak $
 *
 * 
 *
 */


#ifndef LIGHTSPEED_TEXT_TEXTIN_H_
#define LIGHTSPEED_TEXT_TEXTIN_H_

#pragma once
#include "../containers/autoArray.h"
#include "../containers/constStr.h"
#include "textInBuffer.h"
#include "textParser.h"


namespace LightSpeed {

	///Extractes input stream using patterm
	/** This class is works as text extractor based on TextParser and supported
	 * by TextInBuffer.
	 *
	 * If connected with character stream, it is able to parse stream using pattern,
	 * validate stream and extract the informations stored in it. It should
	 * fetch as many characters need to extract all required information and
	 * make successfully validation
	 *
	 * @note You don't need to use this class when input iterator can be used
	 * for random access. TextParser also supports iterator as an input, but
	 * it requires that iterator supports copying when copy is not depending on
	 * the original. Use TextIn class, when iterator cannot handle copying.
	 */
	template<typename Iter, typename Alloc = StdAlloc>
	class TextIn: public TextParser<typename ConstObject<typename OriginT<Iter>::T::ItemT>::Remove, Alloc> {
	public:
		typedef TextParser<typename ConstObject<typename OriginT<Iter>::T::ItemT>::Remove, Alloc > Super;
		typedef typename ConstObject<typename OriginT<Iter>::T::ItemT>::Remove ItemT;
		typedef TextInBuffer<Iter, Alloc> Buffer;
		typedef typename Super::Str Str;
		typedef Iter IterT;


		///Connects to iterator
		/**
		 *
		 * @param iter iterator through input text stream
		 */
		TextIn(Iter iter):buffer(iter) {}
		///Connects to iterator and specifies allocator to use
		/**
		 *
		 * @param iter iterator through input text stream
		 * @param alloc allocator used to allocate internal buffers
		 */
		TextIn(Iter iter, const Alloc &alloc):Super(alloc),buffer(iter,alloc) {}

		///Extracts informations from the stream using the pattern
		/**
		 *
		 * @param format specifies pattern used to parse and extraction
		 * @retval true success, all informations has been extracted. When
		 *  	this result is returned, all characters parsed by the pattern
		 *  	are discarded from the input stream and extraced informations
		 *  	are stored inside internal buffer
		 *  @retval false failure, no data extracted, no characters discarded
		 *
		 *  @note Function can fetch some extra characters to the internal buffer.
		 *  Even if this characters are not finally discarded, they are no longer
		 *  available by stream object and can be retrieved only by this object.
		 *  See getBuffer() how to retrieve fetched characters
		 *
		 */
		bool operator()(const Str &format);
		///Extracts informations from the stream using the pattern, but doesn't discard parsed characters
		/**
		 * @param format specifies pattern used to parse and extraction
		 * @retval true success, all informations has been extracted.
		 *  @retval false failure, no data extracted, no characters discarded
		 *
		 *  @note Function can fetch some extra characters to the internal buffer.
		 *  Even if this characters are not finally discarded, they are no longer
		 *  available by stream object and can be retrieved only by this object.
		 *  See getBuffer() how to retrieve fetched characters
		 */

		bool peek(const Str &format);

		///Commits parsing and discards characters no longer required from the buffer
		/**
		 * Use this function after peek() if you want to discard characters parsed by
		 * this last peek() function
		 */
		void commit() {getBuffer().commit();}

		///Retrieves internal buffer
		/**
		 * See TextInBuffer class documentation how to work with internal buffer to
		 * iterator through fetched characters.
		 *
		 * @return reference to internal buffer
		 */
		Buffer &getBuffer() {return buffer;}

		TextIn &setNL(const ConstStringT<ItemT> &nl) {
			Super::setNL(nl);return *this;
		}
		TextIn &setWS(const ConstStringT<ItemT> &ws) {
			Super::setWS(ws);return *this;
		}

		bool hasItems() const  {
			return buffer.hasItems();
		}

		ItemT getNext() {
			typename Buffer::Iterator iter = buffer.getFwIter(true);
			ItemT x = iter.getNext();
			buffer.commit();
			return x;
		}

		void skip() {
			getNext();
		}

		ItemT peek() {
			ItemT x = buffer.getFwIter(false).getNext();
			return x;
		}

		///peeks whole line
		/**
		 * @retval true line has been fetched. You need to read first fragment to receive this line
		 * @retval false eof reached
		 *
		 * Function doesn't discard the line. You have to use readLine() to read and discard the line
		 */
		 
		bool peekLine() {
			this->data.clear();
			this->fragments.clear();
			natural nlpos = 0;
			bool hasdata = false;
			typename Buffer::Iterator iter = buffer.getFwIter(true);
			while (iter.hasItems()) {
				hasdata = true;
				const ItemT &item = iter.getNext();
				if (this->nl[nlpos] == item) {
					nlpos++;
					if (nlpos==this->nl.length()) break;
				} else if (nlpos > 0) {
					for (natural k = 0; k < nlpos; k++) 
						this->data.add(this->nl[k]);
					nlpos = 0;
					this->data.add(item);
				} else {
					this->data.add(item);
				}
			}
			this->fragments.add(typename Super::Fragment(this->data,0,this->data.length()));
			return hasdata;
		}

		///reads whole line
		/**
		 * @retval true line has been fetched. You need to read first fragment to receive this line
		 * @retval false eof reached
		 */
		bool readLine() {
			bool k = peekLine();
			commit();
			return k;			
		}

		typename Buffer::IterRaw &nxChain() {
			return buffer.getSourceIterator();
		}

		const typename Buffer::IterRaw &nxChain() const {
			return buffer.getSourceIterator();
		}
		typename Buffer::IterRaw &getSource() {
			return buffer.getSourceIterator();
		}

		const typename Buffer::IterRaw &getSource() const {
			return buffer.getSourceIterator();
		}

		///Returns count of characters processed from begin of stream
		natural getCharCount() const {return buffer.getCharCount();}

	protected:

		Buffer buffer;

	};


template<typename Iter, typename Alloc>
bool TextIn<Iter,Alloc>::operator ()(const Str &format)
{
	typename Buffer::Iterator iter = buffer.getFwIter(true);
	bool res = Super::operator()(format, iter);
	if (res) buffer.commit();
	return res;
}



template<typename Iter, typename Alloc>
bool TextIn<Iter,Alloc>::peek(const Str &format)
{
	typename Buffer::Iterator iter = buffer.getFwIter(true);
	return Super::operator()(format, iter);
}

}


#endif /* LIGHTSPEED_TEXT_TEXTIN_H_ */
