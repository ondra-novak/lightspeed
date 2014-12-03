/*
 * merge.h
 *
 *  Created on: 21.8.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_ITER_MERGE_H_
#define _LIGHTSPEED_ITER_MERGE_H_

#include "../exceptions/iterator.h"

namespace LightSpeed {


    ///Contains result of MergeFunctor or SplitFunctor
    template<typename T>
    class MergeSplitResult {
    public:
        ///Creates merge result
        /**
         * @param result result of merge or split. Item must be allocated in context
         *   of object, which generates this result, or it can be one
         *   of the input items (in this case, allocation is not needed)
         *
         * @param commitLeft set true, if left iterator should commit its
         *      current item. You should set true, if you set result of
         *      left iterator. For SplitIterator, specify true, if you
         *      want to write item into left iterator
         * @param commitRight set true, if left iterator should commit its
         *      current item. You should set true, if you set result of
         *      right iterator.For SplitIterator, specify true, if you
         *      want to write item into right iterator
         *
         * @note if you set result to 0 and one or both commit parameters
         *      to true, MergeIterator only commits inputs without the output,
         *      loads next items and calls functor again. If you set result to
         *      0 and both commits to false, MergeIterator reports end of
         *      stream. If you set result to a valid item and both commits
         *      to false, MergeIterator returns items and calls functor
         *      again with the same inputs
         */
        MergeSplitResult(const T *result,bool commitLeft,bool commitRight)
            :result(result),commitLeft(commitLeft), commitRight(commitRight) {}

        ///Returns commit left state
        /**
         * @return merge iterator should commit left iterator
         */
        bool getCommitLeft() const {
            return commitLeft;
        }

        ///Returns commit right state
        /**
         * @return merge iterator should commit right iterator
         */
        bool getCommitRight() const {
            return commitRight;
        }

        ///Returns result
        /**
         * @return Result of mergin
         */
        const T *getResult() const {
            return result;
        }

        static MergeSplitResult<T> stop() {return MergeSplitResult<T>(0,false,false);}
        static MergeSplitResult<T> skipLeft() {return MergeSplitResult<T>(0,true,false);}
        static MergeSplitResult<T> skipRight() {return MergeSplitResult<T>(0,false,true);}
        static MergeSplitResult<T> skipBoth() {return MergeSplitResult<T>(0,true,true);}
        static MergeSplitResult<T> commitL(const T &x) {return MergeSplitResult<T>(x,true,false);}
        static MergeSplitResult<T> commitR(const T &x) {return MergeSplitResult<T>(x,false,true);}
        static MergeSplitResult<T> commitB(const T &x) {return MergeSplitResult<T>(x,true,true);}

    protected:
        const T *result;
        bool commitLeft;
        bool commitRight;
    };
 
    ///Template base class for merge combination
    /**
     * @param T type of item
     *
     * Use this class only as hint, how to declare combine functor. You
     * can use it as base class, but you still need to implement the
     * operator(). Main benefit of this usage is that doxygen will
     * reuse documentation of the operator function
     */
    template<typename T>
    class MergeFunctor {
    public:
        ///Calculates merge operation on two items
        /**
         * @param left contains item from the left iterator. Pointer can
         *  be also 0, in case when left iterator has no more items.
         * @param right contains item from the right iterator.Pointer can
         *  be also 0, in case when right iterator has no more items.
         * @return you have to construct MergeSplitResult which
         * contains result item and information for the iterator which
         * source should be commit.
         *
         * @note function is never called with both (left and right) is equal
         * to 0. In this case, merge iterator itself reports hasItems() == false
         *
         */
        MergeSplitResult<T> operator()(const T *left, const T *right);

        ///Calculates count of items returned by function getRemain()
        /**
         * @param leftCount count reported by left iterator. It can
         *      be also naturalNull, if iterator doesn't support this operation
         * @param rightCount count reported by right iterator. It can
         *      be also naturalNull, if iterator doesn't support this operation
         * @return guessed count of items result of merge. By default
         *      function takes sum of both numbers. But functor can
         *      overwrite this and supply own calculation
         *
         * @note Knowledge of final count is useful for the container. Containers
         *      can preallocate enough space for the iteration. Result
         *      should not be too much far from the reality. Too low
         *      count causes extra reallocations. Too high values
         *      causes allocation of extra memory.
         *
         */
        natural operator()(natural leftCount, natural rightCount) const {
            natural r1 = leftCount;
            if (r1 == naturalNull) return r1;
            natural r2 = rightCount;
            if (naturalNull - r2 < r1) return naturalNull;
            return r1 + r2;

        }

        typedef T ItemT;
    };



    template<typename IterLeft, typename IterRight, typename MergeFunctor>
    class MergeIterator: public IteratorBase<typename MergeFunctor::ItemT,
                            MergeIterator<IterLeft,IterRight,MergeFunctor> >
    {
    public:

        typedef typename MergeFunctor::ItemT ItemT;

        MergeIterator(IterLeft iterLeft, IterRight iterRight)
            :iterLeft(iterLeft),iterRight(iterRight)
            ,prepared(false),cachedResult(0,false,false) {}
        MergeIterator(IterLeft iterLeft, IterRight iterRight, MergeFunctor functor)
            :iterLeft(iterLeft),iterRight(iterRight),functor(functor)
            ,prepared(false),cachedResult(0,false,false) {}

        bool hasItems() const {
            if (!prepared) prepareNextItem();
            return cachedResult.getResult() != 0;
        }

        natural getRemain() const {
            return functor(iterLeft.getRemain(),iterRight.getRemain());
        }

        const ItemT &peek() const {
            if (!prepared) prepareNextItem();
            if (cachedResult.getResult() == 0)
               throw IteratorNoMoreItems(THISLOCATION,typeid(ItemT));
            return *cachedResult.getResult();
        }

        const ItemT &getNext() {
            if (!prepared) prepareNextItem();
            if (cachedResult.getResult() == 0)
               throw IteratorNoMoreItems(THISLOCATION,typeid(ItemT));
            prepared = false;
            return *cachedResult.getResult();
        }



    protected:
        mutable IterLeft iterLeft;
        mutable IterRight iterRight;
        mutable MergeFunctor functor;
        mutable bool prepared;
        mutable MergeSplitResult<ItemT> cachedResult;

        void prepareNextItem() const {
            do {
                if (cachedResult.getCommitLeft()) iterLeft.skip();
                if (cachedResult.getCommitRight()) iterRight.skip();
                const ItemT *l = iterLeft.hasItems()?&iterLeft.peek():0;
                const ItemT *r = iterRight.hasItems()?&iterRight.peek():0;
                if (l == 0 && r == 0) {
                    cachedResult = MergeSplitResult<ItemT>(0,false,false);
                    prepared = true;
                } else {
                    cachedResult = functor(l,r);
                    prepared = true;
                }
            } while (cachedResult.getResult() == 0 &&
                    (cachedResult.getCommitLeft() || cachedResult.getCommitRight()));
        }

    };

    ///Functor defines split operation for SplitIterator
    template<typename T>
    class SplitFunctor {
    public:
        ///Calculates split operation on single item
        /**
         * @param item subject
         * @param commit specifies, whether item will be committed after
         *      return. If true, writing is performed and next call of
         *      the functor will receive new item. If false, this
         *      call has been result of canAccept function, iterator
         *      only testing item without writing it into the stream.
         * @return object MergeSplitResult, which contains item to write
         * (it can refer the same item ... using the same pointer) and
         * information about into which stream will be item written. You
         * can specify any iterator or both. In that case, object will
         * be duplicated and put into both streams
         */
        MergeSplitResult<T> operator()(const T *item, bool commit);

        natural operator()(natural leftCount, natural rightCount) const {
            natural r1 = leftCount;
            natural r2 = rightCount;
            return r1 > r2?r1:r2;

        }

        typedef T ItemT;
    };



    ///Split iterator - splits one stream into two
    /** Split iterator is write iterator, which uses functor to
     * select into which iterator will be item written. Functor can also
     * modify item or write complete different item.
     *
     */
    template<typename IterLeft, typename IterRight, typename SplitFunctor>
    class SplitIterator: public WriteIteratorBase<typename SplitFunctor::ItemT,
                            SplitIterator<IterLeft,IterRight,SplitFunctor> >
    {
    public:
        typedef typename SplitFunctor::ItemT ItemT;


        bool hasItems() const {
            return iterLeft.hasItems() && iterRight.hasItems();
        }

        natural getRemain() const {
            return functor(iterLeft.getRemain(),iterRight.getRemain());
        }

        bool canAccept(const ItemT &item) {
            MergeSplitResult<ItemT> res = functor(item);
            const ItemT *it = res.getResult();
            if (it == 0) return false;
            if (res.getCommitLeft() && !iterLeft.canAccept(item))
                return false;
            if (res.getCommitLeft() && !iterRight.canAccept(item))
                return false;
            return true;
        }

        void write(const ItemT &item) {
            MergeSplitResult<ItemT> res = functor(item);
            const ItemT *it = res.getResult();
            if (it) {
                if (res.getCommitLeft())
                    iterLeft.write(*it);
                if (res.getCommitRight())
                    iterRight.write(*it);
            }
        }

        void flush() {
            iterLeft.flush();
            iterRight.flush();
        }

    protected:
        mutable IterLeft iterLeft;
        mutable IterRight iterRight;
        mutable SplitFunctor functor;


    };


    namespace MergeSplitFunctors {

        ///Split functor performs duplicate items into two streams
        template<typename T>
        class SplitCloneFn: public SplitFunctor<T> {
        public:
            MergeSplitResult<T> operator()(const T *item, bool ) {
                //its simple, return duplicate state for each item
                return MergeSplitResult<T>(item,true,true);
            }
            using SplitFunctor<T>::operator();
        };

        ///Puts each odd item into other stream.
        /** It can be used to separate interleaved stream into two streams */
        template<typename T>
        class SplitInterleaveFn: public SplitFunctor<T> {
        public:
            SplitInterleaveFn():swapState(false) {}
            MergeSplitResult<T> operator()(const T *item, bool commit) {
                bool curSwapState = swapState;
                if (commit) swapState = !swapState;
                return MergeSplitResult<T>(item,!curSwapState,curSwapState);
            }
            using SplitFunctor<T>::operator();
        protected:
            bool swapState;
        };

        ///Implements merge sorting using MergeIterator
        /**
         * Using MergeSortFn you can merge two sorted streams and receive
         * also sorted stream.
         *
         *
         *
         */
        template<typename T, bool allowDups = false, typename Cmp = std::less<T> >
        class MergeSortFn: public MergeFunctor<T> {
        public:
            MergeSortFn(Cmp isLess):isLess(isLess) {}

            MergeSplitResult<T> operator()(const T *left, const T *right) {
                if (right == 0 || isLess(left,right))
                    return MergeSplitResult<T>(left,true,false);
                else if (left == 0 || isLess(right,left))
                    return MergeSplitResult<T>(right,false,true);
                else
                    return MergeSplitResult<T>(left,true,!allowDups);
            }
            using MergeFunctor<T>::operator();
        protected:
            Cmp isLess;
        };

        ///Implements concatenate two streams into one
        /**
         * Functor concatenates two streams into one. Left streams is first
         * and follows by right stream. Left stream have to be limited, right
         * stream don't, because reading of right stream starts, when
         * left stream is empty
         */
        template<typename T>
        class MergeConcatFn: public MergeFunctor<T> {
        public:
            MergeSplitResult<T> operator()(const T *left, const T *right) {
                if (left != 0)
                    return MergeSplitResult<T>(left,true,false);
                else
                    return MergeSplitResult<T>(right,false,true);
            }
            using MergeFunctor<T>::operator();
        };

    }
}



#endif /* _LIGHTSPEED_MERGE_H_ */
