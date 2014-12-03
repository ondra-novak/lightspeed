/*
 * set.h
 *
 *  Created on: 13.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_SET_H_
#define LIGHTSPEED_CONTAINERS_SET_H_

#include <functional>
#include "avltree.h"
#include "../containers/linkedList.h"

namespace LightSpeed {


	template<typename T, typename Cmp = std::less<T> >
	class Set: public AVLTree<T,Cmp>, public Comparable<Set<T,Cmp> > {
		typedef AVLTree<T,Cmp> Super;
	public:
		typedef typename Super::Iterator Iterator;

		Set() {}
		Set(const Cmp &cmp, SharedPtr<IRuntimeAlloc> factory):Super(cmp,factory) {}

		Set operator+(const Set &other) const {
			return Set(*this,other,include);
		}
		Set operator+(const T &item) const {
			return Set(*this,item,include);
		}
		Set operator-(const Set &other) const {
			return Set(*this,other,exclude);
		}
		Set operator-(const T &item) const{
			return Set(*this,item,exclude);
		}
		Set &operator+=(const Set &other) {
			for (typename Set::Iterator iter = other.getFwIter();iter.hasItems();)
				insert(iter.getNext());
			return *this;
		}
		Set &operator+=(const T &item) {
				insert(item);			
				return *this;
		}
		Set &operator-=(const Set &other) {
			for (typename Set::Iterator iter = other.getFwIter();iter.hasItems();)
				remove(iter.getNext());
			return *this;
		}
		Set &operator-=(const T &item) {
				remove(item);
				return *this;
		}

		CompareResult compare(const Set &other) const {
			typename Super::Iterator iter1 = this->getFwIter();
			typename Super::Iterator iter2 = other.getFwIter();
			
			while(iter1.hasItems() && iter2.hasItems()) {
				const T &a = iter1.getNext();
				const T &b = iter2.getNext();
				if (this->cmpOper(a,b)) return cmpResultLess;
				if (this->cmpOper(b,a)) return cmpResultGreater;
			}
			if (iter1.hasItems()) return cmpResultGreater;
			else if (iter2.hasItems()) return cmpResultLess;
			else return cmpResultEqual;
		}

	protected:
		enum Exclude{exclude};
		enum Include{include};

		Set(const Set &other, const Set &other2, Exclude ) {
			(*this)-=other;
			(*this)-=other2;
		}
		Set(const Set &other, const Set &other2, Include ) {
			(*this)+=other;
			(*this)+=other2;
		}
		Set(const Set &other, const T &other2, Exclude ) {
			(*this)-=other;
			(*this)-=other2;
		}
		Set(const Set &other, const T &other2, Include ) {
			(*this)+=other;
			(*this)+=other2;
		}
	};



	namespace _intr {

		template<typename T, typename Cmp>
		struct CompareLists {
			bool operator()(const LinkedList<T> &a, const LinkedList<T> &b) const {
				return cmp(a.getFwIter().peek(), b.getFwIter().peek());
			}

			CompareLists(const Cmp &cmp):cmp(cmp) {}

			Cmp cmp;
		};

	}

	template<typename T, typename Cmp = std::less<T> >
	class MultiSet: public Set<LinkedList<T>,_intr::CompareLists<T,Cmp> > {
		typedef Set<LinkedList<T>,_intr::CompareLists<T,Cmp> > Super;
		typedef LinkedList<T> MList;
		typedef _intr::CompareLists<T,Cmp> SetCmp;
		typedef typename Super::Iterator SetIter;
		typedef typename MList::Iterator ListIter;

	public:

		MultiSet():Super(SetCmp(Cmp())) {}

		MultiSet(const Cmp &cmp, NodeAlloc factory )
			:Super(SetCmp(cmp),factory),mlistsrc(factory) {}


		class Iterator: public IteratorBase<T,Iterator> {
		public:

			Iterator(const SetIter &spos):spos(spos)
					,lpos(spos.hasItems()?spos.peek().getFwIter():0) {}

			Iterator(const SetIter &spos,const ListIter &lpos)
					:spos(spos),lpos(lpos) {}

			bool hasItems() const {return spos.hasItems();}

			const T &peek() const {return lpos.peek();}

			const T &getNext() {
				const T &x = peek();
				lpos.skip();
				fixAfterStep();
				return x;
			}

			bool equalTo(const Iterator &other) {
				return spos.equalTo(other.spos) && lpos.equalTo(other.lpos);
			}
			bool lessThan(const Iterator &other) {
				return (spos.equalTo(other.spos) && lpos.lessThan(other.lpos))
					|| (spos.lessThan(other.spos));
			}
			bool setDir(Direction::Type e) {
				return spos.setDir(e);
			}
			Direction::Type getDir() const {
				return spos.getDir();
			}


		protected:
			SetIter spos;
			ListIter lpos;

			void fixAfterStep() {
				if (!lpos.hasItems()) {
					spos.skip();
					if (spos.hasItems()) lpos = spos.peek().getFwIter();
				}
			}
			friend class MultiSet;
		};

		friend class Iterator;


		Iterator insert(const T &val) {
			MList tmp(mlistsrc);
			tmp.add(val);
			bool found;
			SetIter spos = Super::insert(tmp,&found);
			if (found) {
				MList &exist = const_cast<MList &>(spos.peek());
				ListIter lpos = exist.add(tmp.removeFirst());
				return Iterator(spos,lpos);
			} else {
				return Iterator(spos);
			}
		}

		using Super::insert;

		void erase(Iterator &iter) {
			MList &k = const_cast<MList &>(iter.spos.peek());
			ListIter tst = k.getFwIter();
			tst.skip();
			if (tst.hasItems()) {
				iter.lpos = k.erase(iter.lpos);
				iter.fixAfterStep();
			} else {
				Super::erase(iter.spos);
				iter = Iterator(iter.spos);
			}
		}

		ListIter find(const T &value) {
			MList tmp;
			tmp.add(value);
			const MList *k = Super::find(tmp);
			if (k == 0) return ListIter(0);
			else return k->getFwIter();
		}

		Iterator seek(const T &value, bool *found = 0) {
			MList tmp;
			tmp.add(value);
			SetIter spos = Super::seek(tmp,found);
			return Iterator(spos);
		}


		///returns count of keys
		/**
		 * @return count of keys in O(1) time. This number
		 * will be lower or equal than size() because there can be more
		 * values than keys.
		 */
		natural keyCount() const {return Super::size();}
		///returns count of items
		/**
		 * @return count of items in O(n) time.
		 *
		 * @note watch out to complexity! To retrieve count of items, function must
		 * count all items under all keys
		 */
		natural size() const {
			natural x = 0;
			for (SetIter i = Super::getFwIter(); i.hasItems();)
				x += i.getNext().size();
			return x;
		}

		///Retrieves forward iterator
		Iterator getFwIter() const {return Super::getFwIter();}
		///Retrieves backward iterator
		Iterator getBkIter() const {return Super::getBkIter();}


		template<typename K>
		void insert(IIterator<T,K> &in) {
			while (in.hasItems()) {
				const T &e = in.getNext();
				Super::insert(e);
			}
		}

	protected:

		MList mlistsrc;

	};


}  // namespace LightSpeed


#endif /* LIGHTSPEED_CONTAINERS_SET_H_ */
