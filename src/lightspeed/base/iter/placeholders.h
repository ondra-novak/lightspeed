#pragma once

#include "../containers/autoArray.h"
#include "../containers/constStr.h"
#include "iteratorFilter.h"
#include "../memory/staticAlloc.h"

namespace LightSpeed {

	///implements table to replacing variable by values in the PlaceholderFilter
	template<typename T>
	class IPlaceholders {
	public:
		virtual ConstStringT<T> getValue(ConstStringT<T> varName) const = 0;	
	};

	template<typename T>
	class declPlaceholder: public IPlaceholders<T> {
	public:
		declPlaceholder(ConstStringT<T> var, ConstStringT<T> val, const IPlaceholders<T> *nx = 0)
			:var(var),val(val),nx(nx) {}		
		virtual ConstStringT<T> getValue(ConstStringT<T> varName) const {
			if (varName == var) return val;
			if (nx) return nx->getValue(varName); else return ConstStringT<T>();
		}
		
		declPlaceholder operator()(ConstStringT<T> var, ConstStringT<T> val) {
			return declPlaceholder(var,val,this);
		}
	protected:
		ConstStringT<T> var;
		ConstStringT<T> val;
		const IPlaceholders<T> *nx;
	};

	template<typename T, typename Table = IPlaceholders<T>, typename Allocator = StaticAlloc<32> >
	class PlaceholderFilter: public IteratorFilterBase<T,T,PlaceholderFilter<T,Table,Allocator> > {
		enum State {
			empty,
			charin,
			beginVar,
			readVar,
			blockin
		};
	public:

		typedef IteratorFilterBase<T,T,PlaceholderFilter<T,Table,Allocator> > Super;

		PlaceholderFilter(const Table &table)
			:state(empty),table(table) {}
		PlaceholderFilter(const Table &table,const Allocator &alloc)
			:state(empty),varSpace(alloc),table(table) {}

		bool needItems() const {
			return state == empty || state == beginVar || state == readVar;
		}
		void input(const T &x) {
			switch (state) {
			case empty: if (x == '$') state = beginVar;
						else {chrout = &x;state = charin;}
						break;
			
			case beginVar: if (x == '{') 
							state = readVar;
						   else {
							   chrout = &x;state = charin;
						   }
						   break;
			case readVar: if (x == '}')	{
							outText = table.getValue(varSpace);
							state = outText.empty()?empty:blockin;
							varSpace.clear();
						  } else {
								varSpace.add(x);
						  }
						break;
			default: throwWriteIteratorNoSpace(THISLOCATION,typeid(PlaceholderFilter));
			}

		}
		bool hasItems() const {
			return state == charin || state == blockin;
		}
		T output() {
			switch (state) {
			case charin: state = empty; return *chrout;
			case blockin: {
							const T *out = outText.data();
							outText = outText.offset(1);
							if (outText.empty()) state = empty;
							return *out;
						  }
			default: throwIteratorNoMoreItems(THISLOCATION,typeid(PlaceholderFilter));
			}

			return T();
		}
		void flush() {
			
		}
	protected:
		State state;
		const T *chrout;
		AutoArray<T, SmallAlloc<32> > varSpace;
		const Table &table;
		ConstStringT<T> outText;

	};




}
