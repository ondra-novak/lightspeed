/*
 * indexMap.h
 *
 *  Created on: Dec 29, 2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_CONTAINERS_INDEXMAP_H_
#define LIGHTSPEED_BASE_CONTAINERS_INDEXMAP_H_

namespace LightSpeed {


///Index Map is map where key is automatic number
/** This class helps to work with indexed object where index is automatic number
 *
 *  Map tracks used indexes and updates nextIndex. Function getNextId return next unused ID to achieve unique
 *  IDs.
 *
 */
template<typename T, typename ID = natural, typename F = std::less<ID> >
class IndexMap: public LightSpeed::Map<ID, T, F  > {
public:


	IndexMap():nextId(1) {}

	ID getNextId() const {return nextId;}

	void insert(ID id, const T &t) {
		LightSpeed::Map<ID, T, F>::insert(id,t);
		if (id >= nextId) nextId = id+1;
	}

protected:
	ID nextId;

};


///Cointains object indexed by its ID with lazy erase
/** When erase command is issued, like other commands, it is first executed
 * and then notified to the observers. Observers receive ID of already erased object
 * To give chance to the observers access to erased object, this container
 * saves last erased object. Object is available only through command find(). It
 * is not visible through enumeration, and also seek will not find it. You can
 * insert object with the same ID without the conflict
 */
template<typename T, typename ID = natural, typename F = std::less<ID> >
class IndexMapLazyErase: public IndexMap<T,ID, F> {
public:

	IndexMapLazyErase() {}

	using IndexMap<T,ID,F>::find;
	const T *find(const ID &id) const {
		const T *x = IndexMap<T,ID,F>::find(id);
		if (x == 0) {
			if (erased.isSet() && id == erasedID) return erased.data();
		}
		return x;
	}

	using IndexMap<T,ID, F>::erase;
	void erase(const ID &id) {
		bool found;
		typename IndexMap<T,ID,F>::Iterator iter = IndexMap<T,ID,F>::seek(id,&found);
		if (found) {
			erasedID = id;
			erased.set(iter.peek().value);
			IndexMap<T,ID,F>::erase(iter);
		}
	}

protected:
	Optional<T> erased;
	ID erasedID;
};



}



#endif /* LIGHTSPEED_BASE_CONTAINERS_INDEXMAP_H_ */
