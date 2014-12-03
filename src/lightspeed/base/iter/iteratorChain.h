/*
 * iteratorChain.h
 *
 *  Created on: 20.8.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ITERATORCHAIN_H_
#define LIGHTSPEED_ITERATORCHAIN_H_


namespace LightSpeed {

    ///Base class for iterator chains
    /** Iterator chain is object created by connecting particular iterators
     * into each other. Iterator which accepts another iterator as its
     * input (declaration, not data flow direction) can be part of chain
     *
     * To help write valid iterator chains, you should extend this class
     * for each  iterator chain, instead of IteratorBase (WriteIteratorBase).
     *
     * Class also introduces variable, that holds next chained iterator
     * and source and target item type
     *
     * @tparam Iterator type of next iterator
     * @tparam Impl class that finally implements this iterator
     * @tparam IterClass Name of template from iterator family specifying
     *              iterator's class. It can be IteratorBase, MutableIteratorBase,
     *              WriteIteratorBase
     * @tparam T result item type. Default value is source item type give
     *          from the Iterator
     *
     */
    template<class Iterator, class Impl,
             template<class,class> class IterClass = IteratorBase,
             class T = typename DeRefType<Iterator>::T::ItemT>
    class IteratorChain: public IterClass<T,Impl>
    {
    public:
            ///superclass
            typedef IterClass<T, Impl> Super;
            ///type of iterator next in the chain
            typedef typename DeRefType<Iterator>::T NxChainIter;
            ///type of items of next iterator in the chain
            typedef typename NxChainIter::ItemT SrcItemT;
            ///target item (result of this iterator)
            typedef typename Super::ItemT TrgItemT;

            typedef typename FastParam<Iterator>::T IterCRef;
            ///Constructs iterator chain
            /**
             * @param iter next iterator
             * @return
             */
            IteratorChain(IterCRef iter) :iter(iter) {}

			bool equalTo(const Impl &other) const {
                return nxChain().equalTo(other.nxChain());
			}
			bool lessThan(const Impl &other) const {
                return nxChain().lessThan(other.nxChain());
            }
            NxChainIter &nxChain() {return iter;}
            const NxChainIter &nxChain() const {return iter;}

    protected:
            Iterator iter;

    };



}


#endif /* ITERATORCHAIN_H_ */
