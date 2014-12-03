/*
 * list.h
 *
 *  Created on: 21.8.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_CONTAINERS_LIST_H_
#define _LIGHTSPEED_CONTAINERS_LIST_H_

#include "../memory/clusterFactory.h"

namespace LightSpeed {

    ///List container
    /**Implements container which is organized as both-linked list.
     *
     * You can store any count of items, limited to available memory
     * You don't need to define moveConstructor for the objects
     * You can add, or remove items anywher inside to list without need
     * to move items.
     *
     * Random access to items is very slow. This is reason, why List
     * doesn't extend GenericArray. List can be used as stream or items
     * can be accessed via iterator.
     *
     *
     */
    template<typename T, typename Factory = ClusterFactory<> >
    class List  {

		enum AnchorInit {anchorInit};

		class Item;

        class ItemBase: public RefCntObj {
        public:
            Item *prev;
            Item *next;

            ItemBase():prev(asItem(this)),next(asItem(this)) {}
            ItemBase(const ItemBase &other):prev(asItem(this)),next(asItem(this)) {}
            ItemBase &operator=(const ItemBase &other) {return *this;}

			ItemBase(AnchorInit init):prev(asItem(this)),next(asItem(this)) {
				RefCntPtr<ItemBase> x(this);
				x.manualAddRef();
			}
			
			static Item *asItem(ItemBase *x) {return static_cast<Item *>(x);}

        };


        class Item: public ItemBase {
        public:
            T data;

            Item(const T &data):data(data) {}
        };

        typedef typename Factory::template Factory<Item> Alloc;
        typedef RefCntPtr<Item,Alloc > PItem;

        class ListPosition {
        public:
        	friend class List;

            ListPosition(List &owner, Item *pos):owner(&owner),pos(pos,owner.alloc) {}

        protected:
            List *owner;
            PItem pos;
        };

    public:


        class Iterator: public ListPosition,
						public RandomAccessIterator<IteratorBase<T, Iterator> > {
        public:
            Iterator(List &owner,Item *pos, Direction::Type dir)
				:ListPosition(owner,pos),dir(dir),lastDir(dir) {}
            Iterator(const ListPosition &pos, Direction::Type dir)
				:ListPosition(pos),dir(dir),lastDir(dir) {}

            bool hasItems() const {
            	return this->pos != &this->owner->anchor;
            }

            const T &getNext() {
            	const T &ret = peek();
            	switch (dir) {
					case Direction::forward:
						this->pos = (this->pos->next);
						break;
					case Direction::backward:
						this->pos = (this->pos->prev);
						break;
					default:break;
            	}
            	lastDir = dir;
            	return ret;
            }

            const T &peek() const {
            	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
            	const Item *x = static_cast<const Item *>(this->pos.get());
            	return x->data;
            }

            bool equalTo(const Iterator &other) const {
            	return this->pos == other.pos;
            }

            bool lessThan(const Iterator &other) const {
            	Iterator tmp(*this);
            	while (tmp.hasItems()) {
					tmp.skip();
					if (tmp.equalTo(other)) return true;
            	}
            	return false;
            }

            natural getRemain() const {
            	Iterator tmp(*this);
            	natural cnt;
            	while (tmp.hasItems()) {
            		cnt++;
            		tmp.skip();
            	}
            	return cnt;
            }

            Direction::Type getDirection() const {return dir;}
            void setDirection(Direction::Type dir) {this->dir = dir;}

            natural seek(natural offset, Direction::Type direction) {
            	natural save = offset;
            	while ((hasItems() || offset == save) && offset > 0) {
            		switch(direction) {
						case Direction::forward:
							this->pos = (this->pos->next);
							break;
						case Direction::backward:
							this->pos = (this->pos->prev);
							break;
						default: return 0;
            		}
            		offset--;
            		lastDir = direction;
            	}
            	return save - offset;
            }

            natural tell() const {
                return naturalNull;
            }

            bool canSeek(Direction::Type direction) const {
            	if (hasItems()) return true;
            	return direction != lastDir;
            }

            natural distance(const Iterator &other) const {
            	if (equalTo(other)) return 0;
            	{
					Iterator tmp(*this);
					natural cnt = 0;
					while (tmp.hasItems()) {
						tmp.skip();
						cnt++;
						if (tmp.equalTo(other)) return cnt;
					}
            	}
            	{
					Iterator tmp(other);
					natural cnt = 0;
					while (tmp.hasItems()) {
						tmp.skip();
						cnt++;
						if (tmp.equalTo(*this)) return cnt;
					}
            	}
            	return naturalNull;

            }

            natural getRemain(const Direction::Type dir) const {
            	Iterator tmp(*this);
            	natural x = tmp.seek(naturalNull,dir);
            	return naturalNull - x;

           }

            Direction::Type direction(const Iterator &other) const {
            	if (this->equalTo(other)) return Direction::nowhere;
            	if (this->lessThan(other)) return dir;
            	else return Direction::reverse(dir);
            }

            const List &getOwner() const {return this->owner;}


        protected:
			Direction::Type dir;
			Direction::Type lastDir;


        };

        class MutableIterator: public RandomAccessIterator<MutableIteratorBase<T, MutableIterator> > {
        public:
            MutableIterator(List &owner,Item *pos, Direction::Type dir)
                :iter(owner,pos,dir) {}
            MutableIterator(const MutableIterator &pos, Direction::Type dir)
                :iter(pos.iter,dir) {}


            operator const Iterator &() const {return iter;}

            T &getNextMutable() {
                return const_cast<T &>(iter.getNext());
            }

            T &peekMutable() const {
            	return const_cast<T &>(iter.peek());
            }

            bool hasItems() const {return iter.hasItems();}
            const T &getNext() {return iter.getNext();}
            const T &peek() const {return iter.peek();}
            bool equalTo(const Iterator &other) const {return iter.equalTo(other);}
            bool lessThan(const Iterator &other) const {return iter.lessThan(other);}
            natural getRemain() const {return iter.getRemain();}
            Direction::Type getDirection() const {return iter.getDirection();}
            void setDirection(Direction::Type dir) {iter.setDirection(dir);}
            natural seek(natural offset, Direction::Type direction) {return iter.seek(offset,direction);}
            natural tell() const {return iter.tell();}
            bool canSeek(Direction::Type direction) const {return iter.canSeek(direction);}
            natural distance(const Iterator &other) const {return iter.distance(other);}
            natural getRemain(const Direction::Type dir) const {return iter.getRemain(dir);}
            Direction::Type direction(const Iterator &other) const {return direction(other);}
            const List &getOwner() const {return iter.getOwner();}
        protected:
            Iterator iter;

        };

        class WriteIterator: public WriteIteratorBase<T,WriteIterator> {
        public:
            WriteIterator(const Iterator &iter):iter(iter) {}

            void write(const T &x) {
                switch(iter.getDirection()) {
                    case Direction::forward:
						const_cast<List &>(iter.getOwner()).insertBefore(iter,x);
						break;
                    case Direction::backward:
                    	const_cast<List &>(iter.getOwner()).insertAfter(iter,x);
                    	break;
                    default:
                    	break;
                }
            }

            bool hasItems() const {
                return true;
            }

            operator const Iterator &() const {return iter;}
        protected:
            Iterator iter;
        }; 

        List():alloc(Factory()),
        	  anchor(anchorInit),count(0) {}

        List(const Factory &factory): alloc(factory),
				anchor(anchorInit),count(0) {}

		~List() {
			clear();
		}

		List(const List &other):alloc(other.alloc),anchor(anchorInit),count(0) {
			WriteIterator witer(getFwIter());
			Iterator riter = other.getFwIter();
			witer.copy(riter);
		}
        ///Adds item to the front of list
        /**
         * @param itm item to add
         */
        void addFront(const T &itm) {
            return addAfterItm(ItemBase::asItem(&anchor),createItem(itm));
        }

        ///Adds item to the back of list
        /**
         * @param itm item to add
         */
        void addBack(const T &itm) {
            return addBeforeItm(ItemBase::asItem(&anchor),createItem(itm));
        }

        ///Adds item after location specified by iterator
        /**
         * @param loc location, type is base for all iterators
         * @param itm item to add
         */
        void insertAfter(const ListPosition &loc,const T &itm) {
            return addAfterItm(loc.pos,createItem(itm));
        }

        ///Adds item before location specified by iterator
        /**
         * @param loc location, type is base for all iterators
         * @param itm item to add
         */
        void insertBefore(const ListPosition &loc,const T &itm) {
            return addBeforeItm(loc.pos,createItem(itm));
        }

        Iterator getFwIter() const {
        	return Iterator(const_cast<List &>(*this),
        			const_cast<Item *>(anchor.next),Direction::forward);
        }
        Iterator getBkIter() const {
        	return Iterator(const_cast<List &>(*this),
        			const_cast<Item *>(anchor.prev),Direction::backward);
        }
        MutableIterator getFwIterM()  {
        	return MutableIterator(*this,anchor.next,Direction::forward);
        }
        MutableIterator getBkIterM() {
        	return MutableIterator(*this,anchor.prev,Direction::backward);
        }

        Iterator erase(const Iterator &iter) {
			Iterator res = iter;

            const ListPosition &pos = iter;
            const ItemBase *x = pos.pos;
			if (x != &anchor) {
				res.skip();
            	removeItem(pos.pos);
			}
			return res;
        }

		private:
			friend class Remover;
			class Remover {
			public:
				List &owner;
				Remover(List &owner):owner(owner) {}
				void remove() {
					while (!owner.empty()) {
						owner.removeItem((owner.anchor.next));
					}
				}
				~Remover() {remove();}

			};
		public:

		void clear() {
			Remover rm(*this);
			rm.remove();
        }

        bool empty() const {
            return anchor.next == &anchor && anchor.prev == &anchor;
        }

        natural size() const {
            return count;
        }

		/// Converts pointer into iterator, when pointer points to data inside in list
		/**
		 * This function can be very helpfull when pointers are used to
		   create search indexes, and you need later convert this pointer
		   to iterator into the list. Pointer must always point at item
		   stored in the list, check twice this condition before you will use
		   this function. Because function cannot make any check that pointer
		   is really valid. If you use invalid pointer, returned iterator
		   will already invalid

		 * @param itemPointer pointer to item, which iterator we need
		 * @param dir default direction is set to the iterator
		 * @return iterator to that item
		 */
		Iterator iteratorFromPointer(const T *itemPointer, 
					Direction::Type dir = Direction::forward) {
			List &owner = const_cast<List &>(*this);

			const Item *x = reinterpret_cast<const Item *>(itemPointer);

			natural shift = reinterpret_cast<const byte *>(&x->data) -
					reinterpret_cast<const byte *>(x);

			Item *itm = const_cast<Item *>(
				reinterpret_cast<const Item *>(
				reinterpret_cast<const byte *>(itemPointer) - shift));
			return Iterator(owner,itm,dir);
		}

		bool oneItem() const {return anchor.next == anchor.prev;}

    protected:


        Alloc alloc;
        ItemBase anchor;
        natural count;

        PItem createItem(const T &itm){
            PItem ptr(alloc);
            ptr.create(itm);
            return ptr;
        }


        void addAfterItm(Item *loc, PItem itm) {
            if (itm->next != itm || itm->prev != itm)
                removeItem(itm);
            itm->next = loc->next;
            itm->prev = loc;
            loc->next = itm;
            itm->next->prev = itm;
			PItem dummy(itm,alloc);
			dummy.manualAddRef();
            count++;
        }

        void addBeforeItm(Item *loc, PItem itm) {
            if (itm->next != itm || itm->prev != itm)
                removeItem(itm);
            itm->next = loc;
            itm->prev = loc->prev;
            loc->prev = itm;
            itm->prev->next = itm;
			PItem dummy(itm,alloc);
			dummy.manualAddRef();
            count++;
        }

        void removeItem(Item *itm) {
            itm->next->prev = itm->prev;
            itm->prev->next = itm->next;
            itm->next = itm;
            itm->prev = itm;
			PItem dummy(itm,alloc);
			dummy.manualRelease();
            count--;
        }
    };




} // namespace LightSpeed


#endif /* _LIGHTSPEED_CONTAINERS_LIST_H_ */
