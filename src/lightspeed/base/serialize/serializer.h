#ifndef LIGHTSPEED_SERIALIZER_H_
#define LIGHTSPEED_SERIALIZER_H_

#include "basicTypes.h"
#include "../exceptions/serializerException.h"
#include "../framework/services.h"
#include "struct.h"
#include "requiredField.h"
#include "../memory/staticAlloc.h"
#include "sectionId.h"


namespace LightSpeed
{


	template<typename Serializer>
	class SectionBase;
	template<typename Formatter, class Factory>
	class Serializer;



	///Declaration of serializer

	template<class Formatter, class Fact = ClusterFactory<> >
	class Serializer {


		///Contains formatter. 
		Formatter fmt;

		///Contains attached services
		Services services;

		Fact factory;
	public:

	
		typedef Serializer<Formatter,Fact> ThisSerializer;

		typedef Fact Factory;

		typedef typename OriginT<Formatter>::T FormatterT;

		template<typename T>
		class TypeTable: public FormatterT::template TypeTable<T> {};




		///Constructs serializer and specifies formatter or parser
		Serializer(Formatter fmt):fmt(fmt) {}

		///Constructs serializer and specifies formatter or parser
		Serializer(Formatter fmt, const Fact &factory):fmt(fmt),factory(factory) {}

		///Retrieves current allocator
		/** 
		*	@return current allocator 
		*/
		const Fact &getFactory() const { return factory; }


		///Serialization of simple variable
		/**
		* @param x variable to serialize
		* @retval true success
		* @retval false nonfatal fail. This can happen when you read
		*  out of section. In this case, you can assume default value
		* 
		*/
		template<class T>
		Serializer &operator()(T &x) {

			TypeTable<T>::serialize(x, *this);
			return *this;
		}

		///Serialization of simple variable with the name
		/**
		* @param x variable to serialize
		* @param name name of variable
		* @retval true successfully serialized
		* @retval false cannot open section with this name (probably, not found)
		*/
		template<class T, class Impl>
		bool operator()(T &x, const SectionId<Impl> &name) {
			if (!fmt.openSection(name._invoke()))
				return false;
			try {
				(*this)(x);
				fmt.closeSection(name._invoke());
			} catch ( Exception & e) {
				e.appendReason(SubsectionException(THISLOCATION,name.toString()));
				throw;
			} catch (...) {
				fmt.closeSection(name._invoke());
				Exception::rethrow(THISLOCATION);
			}
			return true;
		}

		template<class T>
		bool operator()(T &x, const char *name) {
			return operator()(x,StdSectionName(name));
		}
				///Serialization of simple variable with the name and default value
		/**
		* @param x variable to serialize
		* @param name name of variable
		* @param defValue default value
		* @retval true successfully serialized
		* @retval false not serialized, used default value
		*/
		template<class T, class Impl>
		bool operator()(T &x, const SectionId<Impl> & name, const T &defValue) {
			bool skip = false;
			if (storing())               
				skip = (x == defValue);
			if (!fmt.openSectionDefValue(name._invoke(),!skip)) return false;
			try {
				(*this)(x);
				fmt.closeSection(name._invoke());
				return true;
			} catch ( Exception & e){
			     e.appendReason(SubsectionException(THISLOCATION,name.toString()));
				throw;
			} catch (...) {
				fmt.closeSection(name._invoke());
				Exception::rethrow(THISLOCATION);
				throw;
			}
		}

		template<class T>
		bool operator()(T &x, const char *name, const T &defValue) {
			return operator()(x,StdSectionName(name),defValue);
		}

		template<class T, class Impl>
		void operator()(T &x, const SectionId<Impl> &name, _TRequiredField m) {
			bool res = (*this)(x,name);
			if (!res)
				throw RequiredSectionException(THISLOCATION,name.toString());	
		}

		template<class T>
		void operator()(T &x, const char *name, _TRequiredField m) {
			return operator()(x,StdSectionName(name),m);
		}



		///Serialization in one direction
		/** This serializes only when serializer is in the storing state. 
		Otherwise, function does nothing

		@param x object to store
		@return self reference allows to make chains
		*/
		template<typename T>
		Serializer &operator << (const T &x) {
			if (storing()) (*this)(const_cast<T &>(x));
			return *this;
		}

		///Serialization in one direction
		/** This serializes only when serializer is in the loading state. 
		Otherwise, function does nothing

		@param x object to load
		@return self reference allows to make chains
		*/
		template<typename T>
		Serializer &operator >> (T &x) {
			if (loading()) (*this)(x);
			return *this;
		}


		///Serializes expression 
		/** This is special both-way serialization command. If
		serializer is in the loading state, it loads object from the
		stream, and ignores parameter. If serializer is in the  storing
		state, it stores the parameter and returns it as result. Function
		is designed to serialize expressions, where result of expression
		is stored and used as result of function. or stored
		result is taken in the case of loading.
		*/
		template<typename T>
		T expr(const T &src) {
			if (loading()) {
				T tmp;
				(*this) >> tmp;
				return tmp;
			} else {
				(*this) << src;
				return src;
			}
		}


		///returns true, whether serializer is writting into stream               
		bool storing() const {return fmt.isStoring();}
		///returns true, whether serializer is reading from stream
		bool loading() const {return !fmt.isStoring();}
		///returns true, whether serializer is writting into stream               
		bool operator+() const {return fmt.isStoring();}
		///returns true, whether serializer is reading from stream
		bool operator-() const {return !fmt.isStoring();}

		///retrieves current formatter
		FormatterT *getFormatterPtr() {return &fmt;}
		///retrieves current fotmatter
		FormatterT &getFormatter() {return fmt;}

		///Returns false, when there is end of section
		/**
		 * @retval false end of section reached
		 * @retval true section can continue
		 *
		 * @note During reading, function returns false at the end of section,
		 * during writting, function return false, if reserved space for
		 * the section is exhausted
		 */
		bool hasItems() const {return fmt->hasSectionItems();}
	protected:

		///Retrieves const reference to service container		
		const IServices &getServices() const {return services;}

		///Retrieves const reference to service container		
		IServices &getServices() {return services;}

	public:

		///Makes section
		/** Section is object, that should be instantiated in the stack
		inside of serialization script. It open section when it is
		created and closes section when it is destroyed. Variables
		between creation and destruction are serialized into this section
		*/
		template<typename SectId = StdSectionName>
		class Section {
		public:

			Section(Serializer &arch, const SectId &sectionName)
				:arch(arch),sectionName(sectionName),opened(true) {
					if (!arch.getFormatter().openSection(sectionName)) {
						throw RequiredSectionException(THISLOCATION,sectionName.toString());
					}
			}

			Section(Serializer &arch, bool cond, const SectId &sectionName)
				:arch(arch),sectionName(sectionName),opened(false) {
					this->opened = arch.getFormatter().openSectionDefValue(sectionName._invoke(),cond);			
			}

			bool operator! () const {return !opened;}
			operator bool() const {return opened;}

			~Section() try {			
				if (opened) arch.getFormatter().closeSection(sectionName);
			} catch (...) {
				if (std::uncaught_exception()) return;
			}

		protected:

			bool opened;
			Serializer &arch;
			SectId sectionName;
		};
		

		///Opens array section
		/** Array section is section, where objects are serialized in
		sequence without names. Object is useful to serialize containers.
		Depend on formatter, there are more techniques, how to store count
		of items in the container. Formatter may want to store count
		first and then all items. But formatters that supports true sections
		may not want to store count as first value, because they are able to
		find end of array reaching end of section. This object respect
		both types of formatters and will use the best method to store 
		array.
		*/
		class Array {
		public:


			///Opens array section		
			/**
			* @param arch current serializer
			* @param count count of items in the array. Ignored, when loading
			*/
			Array(Serializer &arch, natural count):arch(arch) {
				init(count);
			}

			/// retrieves remain count of items
			/** 
			@return count of items, or naturalNull, if unknown
			*/
			natural length() const {return remain;}


			/// retrieves, whether array knows its length
			/**
			 * @retval true array knows its length
			 * @retval false array doesn't known its length
			 */
			bool hasLength() const {return remain != naturalNull;}

			/// retrieves index of currently serialized item
			/**
			 * @return Index of currently serialized item. Function
			 * returns correct value after next() is called. Otherwise
			 * it returns naturalNull before first next() is called.
			 *
			 * If size of array is unknown, function returns naturalNull in
			 * all cases.
			 */
			natural index() const {return (count - remain) - 1;}
			///call this to move to the next item. 
			/**
			*/
			bool next() {
				if (remain == 0) return false;
				if (hasLength()) remain--;
				return this->arch.getFormatter().nextArrayItem();
			}

			~Array() try { 				
				this->arch.getFormatter().closeArray();
			} catch (...) {
				if (std::uncaught_exception()) 
					return;
			}
		protected:
			Serializer &arch;
			natural remain;
			natural count;

			void init(natural count) {
				if (this->arch.loading()) count = this->arch.getFormatter().openArray(naturalNull);
				else this->arch.getFormatter().openArray(count);			
				remain = count;
				this->count = count;
			}

		};

		template<typename SectId = StdSectionName>
		class Struct: public SerializeStruct<Serializer,SectId> {
		public:
			Struct(Serializer &arch)
				:SerializeStruct<Serializer,SectId>(arch) {}
		};

		

		
		template<typename Ifc>
		Ifc &getService()  {
			return services.template getIfc<Ifc>();
		}

		void addService(IService &svc) {
			services.addService(svc);
		}
		void removeService(IService &svc) {
			services.removeService(svc);
		}

	};

	template<typename Interface>
	class SrAttr: public Pointer<Interface> {
	public:
		SrAttr(Interface *ptr):Pointer<Interface>(ptr) {}

		template<typename X, typename Y>
		SrAttr(Serializer<X,Y> &arch)
			:Pointer<Interface>(&arch.template getService<Interface>()) {}

	};



} // namespace LightSpeed

#endif /*SERIALIZER_H_*/

