#ifndef LIGHTSPEED_CONTAINERS_ARRAYITER_H_
#define LIGHTSPEED_CONTAINERS_ARRAYITER_H_

#include "../iter/iterator.h"
#include "../exceptions/throws.h"



namespace LightSpeed {
    
    ///Iterator, implements iterating through the standard arrays    
	template<class T, class Container>
	class ArrayIterator: public RandomAccessIterator<MutableIteratorBase<T,ArrayIterator<T,Container> > >
	{
	public:
	    typedef RandomAccessIterator<MutableIteratorBase<T,ArrayIterator<T,Container> > > Super;

		ArrayIterator(Container container, natural start, natural end, integer step):
		  container(container),begin(start),end(end),step(step),cur(start) {}


        

		const T &peek() const {
			if (!hasItems()) 
			    throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			return container[cur];
		}

			
		const T &getNext()  {
			if (!hasItems()) 
			    throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			cur += step;
			return container[cur-step];
		}

		T &getNextMutable() {
            if (!hasItems())
                throwIteratorNoMoreItems(THISLOCATION,typeid(T));
            cur += step;
            return container(cur-step);
		}

        T &peekMutable() const {
            if (!hasItems())
                throwIteratorNoMoreItems(THISLOCATION,typeid(T));
            return container(cur-step);
        }


		bool hasItems() const {
			if (step > 0)
                return cur <= end && cur >= 0 && cur < (integer)container.length();
            else
                return cur >= end && cur >= 0 && cur < (integer)container.length();
        }

		natural getRemain() const {
			integer remain = (integer)(end - cur) / step;
            if (remain < 0) return 0;
            else return remain;
		}

        natural seek(natural offset, Direction::Type direction) {
            integer newPos;
            switch (direction) {
                case Direction::absolute: newPos = offset;break;
                case Direction::forward: newPos = cur + offset;break;
                case Direction::backward: newPos = cur - offset;break;
                case Direction::nowhere: newPos = cur; break;
                default: return Super::seek(offset,direction);                
            }
            if (newPos > end) newPos = end + 1;
            if (newPos < begin) newPos = begin;
            natural res = newPos < cur? cur - newPos: newPos - cur;
            cur = newPos;
            return res;
        }

        natural tell() const {
            return cur;
        }

        bool canSeek(Direction::Type direction) const {
            switch (direction) {
                case Direction::absolute: return begin <= end;
                case Direction::forward: return cur <= end;
                case Direction::backward: return cur > begin;
                case Direction::nowhere: return true;
                default: return false;
            }
        }

        natural getRemain(const Direction::Type dir) const {
            switch (dir) {
                case Direction::forward: return end - cur + 1;
                case Direction::backward: return cur - begin;
                default: return naturalNull;
            }
        }


		ArrayIterator(Container cont):container(cont),begin(0),end(integer(cont.length())-1)
			,cur(0),step(1) {}


		void setParams(natural start, natural stop, natural step) {
		    begin = start;
		    end = stop;
		    cur = start;
		    this->step = step;

		}
		
		static ArrayIterator forward(Container cont) {
		    return ArrayIterator(cont);
		}
        static ArrayIterator backward(Container cont) {
            return ArrayIterator(cont,cont.size()-1,0,-1);
        }
		

	protected:

		Container container;
		integer begin;
		integer end;
		integer cur;
        integer step;

	};

 	

}

#endif
