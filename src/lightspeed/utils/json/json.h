#include "../../base/interface.h"
#include "../../base/iter/vtiterator.h"
#include "../../base/memory/refcntifc.h"
#include "../../base/meta/emptyClass.h"

/**@file
 * All basic declarations to allow JSON work. To compile this header, you will need declare two classes in
 * LightSpeed namespace
 *
 * ConstStrA and RefCntPtr and SeqFileInput for streams. These classes can be taken from LightSpeed library
 *
 * #include "lightspeed/base/containers/constStr.h"
 * #include "lightspeed/base/memory/refCntPtr.h"
 * #include "lightspeed/base/streams/fileio.h"
 *
 *
 */

#pragma once

namespace LightSpeed {
	class SeqFileInput;
	class SeqFileOutput;
}

namespace LightSpeed {

	namespace JSON {

		enum NodeType {

			///Node contains NULL value
			ndNull = 0,
			///Node is true or false
			ndBool = 1,
			///Node contains number without floating dot
			ndInt = 2,
			///Node contains general number
			ndFloat = 3,
			///standard JSON string ""
			ndString = 4,
			///standard JSON object {}
			ndObject = 5,
			///standard JSON array []
			ndArray = 6,
			///Node has been deleted, this node is used when differential JSON is created
			ndDelete = 7,
			///Custom node - custom serialization. Should not appear during parsing
			ndCustom = 0x100,

			ndClass = 5
		};


		class INode;
		class IFactory;

		///Smart pointer to node
		/**Automatically tracks reference counts for every node in tree allowing free unused nodes as required */
//		typedef ::LightSpeed::RefCntPtr<INode> Value;
		class Value: public ::LightSpeed::RefCntPtr<INode> {
		public:
			typedef ::LightSpeed::RefCntPtr<INode> Super;

			Value() {}
			Value(const Super &x):Super(x) {}
			Value(NullType x):Super(x) {}
	        Value(INode *p):Super(p) {}

			Value &operator=(const Value &other) {
				Super::operator=(other);return *this;
			}

			Value operator[](ConstStrA name) const;
			Value operator[](const char *name) const;
			Value operator[](int i) const;
			Value operator[](natural i) const;


		};

		typedef Value PNode;

		//Smart pointer to factory
		/**Automatically tracks reference counts for factories allowing free unused factories as required */
//		typedef ::LightSpeed::RefCntPtr<IFactory> PFactory;
		class PFactory: public ::LightSpeed::RefCntPtr<IFactory> {
		public:
			typedef ::LightSpeed::RefCntPtr<IFactory> Super;

			PFactory() {}
			PFactory(const Super &x):Super(x) {}
			PFactory(NullType x):Super(x) {}
			PFactory(IFactory *x):Super(x) {}

			PFactory &operator=(const PFactory &other) {
				Super::operator=(other);return *this;
			}



			///Syntax helper
			/** factory(x) -> factory->newValue(x) */
			template<typename T>
			Value operator()(const T &val);
			Value array();
			Value object();

		};

		



		template<typename Fn>
		class EntryEnum;


		///Describes key information for iterators and enumerator
		class IKey {
		public:
			///Type of key
			enum Type {
				/** String key, all string keys are stored as UTF-8 strings*/
				string,
				/** Key is index - for arrays. You should use getIndex(). Functions getString() and getStringUtf8
				 * don't need to work and may return empty content
				 */
				index
			};

			virtual Type getType() const = 0;
			virtual ConstStrA getString() const = 0;
			virtual natural getIndex() const = 0;
			virtual ~IKey() {}
		};
		///Enumerator for object nodes
		/**@obsolete
		 *
		 * Enumerators are obsolete, but can they are still in use in many projects. Enumerator
		 * is called for every field in the object
		 */
		class IEntryEnum {
		public:
			virtual bool operator()(const INode &nd, const IKey &name) const = 0;
			virtual ~IEntryEnum() {}			
			
			template<typename Fn>
			static EntryEnum<Fn> lambda(Fn fn) {return EntryEnum<Fn>(fn);}
		};		

		///Wrapper to implement enumerator using C++x11 lambda function
		template<typename Fn>
		class EntryEnum: public IEntryEnum {
		public:
			EntryEnum(const Fn &fn):fn(fn) {}
			virtual bool operator()(const INode &nd, const IKey &name) const {return fn(nd,name);}
		protected:
			Fn fn;
		};
		

		///Information about node retrieved through Iterator
		class KeyValue {
		public:
			///Name of node
			/** Field is used only when iterator processing through object. Otherwise it is not used */
			const IKey *key;
			/// Pointer to node
			INode *node;

			operator Value() const {return node;}
			INode *operator->() const {return node;}
			INode &operator*() const {return *node;}

			IKey::Type getKeyType() const {return key->getType();}
			natural getIndex() const {return key->getIndex();}
			ConstStrA getStringKey() const {return key->getString();}
		};

		typedef KeyValue NodeInfo;

		///Iterator through containers in JSON
		/**
		 * This allows to enumerate members of objects or values in arrays. To retrieve this object
		 * call INode::getFwIter()
		 */
		class Iterator: public IteratorBase<KeyValue, Iterator> {
		public:

			class IIntIter;
			///Initializes iterator which doesn't contains items
			/** This is returned by nodes not acting as containers */
			Iterator():iter(0),next(false) {}
			///Initializes iterator using pointer to IIntIter implementing iteration itself for specific node
			/**
			 * @param internalIter pointer to iterator.
			 * @note Iterator can take up to 68 naturals (272 bytes on 32bit and 544bytes on 64bit). This
			 * is enough space to store tree iterator for object-nodes.
			 */
			Iterator(const IIntIter *internalIter);
			///Initializes iterator as copy of another iterator
			Iterator(const Iterator &other);
			///Destroyes iterator
			~Iterator();

			///Retrieves next item from container
			/**
			 * @return next item
			 */
			const NodeInfo &getNext();
			///Retrieves next item, but doesn't advance iterator
			/**
			 * @return next item
			 */
			const NodeInfo &peek() const;
			///Retrieves whether there are items
			/**
			 * @retval true items follows
			 * @retval false no more items
			 * @note iterator caches state of this value, so it will not change when container is modified
			 */
			bool hasItems() const;

			///Searches for field from current position
			/** Because list of fields are ordered, function returns false, when current field
			 *  is after required field. Function returns true, when current field is equal to
			 *  required field. Function can move iterator to required field when current field is
			 *  before required field.
			 *
			 * @param fieldName Field to seek
			 * @param foundField optional - name of field, where iterator stops
			 * @retval true field found
			 * @retval false field not found
			 *
			 * @note Function sets iterator to place to retrieve value using getNext()
			 */



			const INode &getNextKC(ConstStrA fieldName);
			bool isNextKey(ConstStrA fieldName) const;

		protected:
			mutable NodeInfo tmp;
			mutable IIntIter *iter;
			mutable natural buffer[76]; //should be enough
			mutable bool next;

		};


		///JSON is represented as tree, so this is one node of tree
		/** Node can be Object, Array or any of leaf nodes, such a string, number, or null
		   
		    Every node can be asked to everything, but depend on
			type of node, some methods are unavailable. This is better
			than doing dynamic_cast depend on node type

		*/

		class INode: public ::LightSpeed::IRefCntInterface {
		public:

			///retrieves type of node
			/**
			 * @return type of node
			 */
			virtual NodeType getType() const = 0;
			///retrieves string value
			/**
			 * @return String value. If node is different type, it will be converted
			 */
			virtual ConstStrW getString() const = 0;
			///Retrieves int value
			/**
			 * @return integer value
			 */
			virtual integer getInt() const = 0;
			///Retrieves unsigned int value
			/**
			 * @return unsigned integer value
			 */
			virtual natural getUInt() const = 0;
			///Retrieves long int value
			/**
			 * @return long integer value
			 */
			virtual linteger getLongInt() const = 0;
			///Retrieves long unsigned int value
			/**
			 * @return long  unsigned integer value
			 */
			virtual lnatural getLongUInt() const = 0;
			///Retrieves float value
			/**
			 * @return float value
			 */
			virtual double getFloat() const = 0;
			///Retrieves boolean
			/**
			 * @return boolean value
			 */
			virtual bool getBool() const = 0;
			///Retrieves whether value is NULL
			/**
			 * @retval true value is null
			 * @retval false value is not null
			 */
			virtual bool isNull() const = 0;
			///Searches node for value with given name
			/**
			 * @param name of field
			 * @return pointer to node containing value. NULL returned, when
			 * node doesn't contain given field by name, or when node is
			 * not object
			 *
			 * @see getPtr
			 */
			virtual INode *getVariable(ConstStrA ) const = 0;
			///Retrieves count of values in array or object
			virtual natural getEntryCount() const = 0;
			///Retrieves entry referenced by index - must be array
			virtual INode *getEntry(natural idx) const = 0;
			///Retrieves count of values in array or object
			natural length() const {return getEntryCount();}
			///Searches for field of given name
			/**
			 * @param name of field.
			 * @return Returns pointer to value, returns NULL if field doesn't exist
			 * or target is not object. Function doesn't fail with exception so you
			 * can use it to test whether field exists
			 */
			INode *getPtr(ConstStrA v) const {
				return getVariable(v);
			}

			///Retrieves field on given index
			/**
			 * @param index of field.
			 * @return Returns pointer to value, returns NULL if field doesn't exist
			 * or target is not object. Function doesn't fail with exception so you
			 * can use it to test whether field exists
			 */
			INode *getPtr(natural idx) const {
				return getEntry(idx);
			}

			///Enumerates all entries 
			/**
			 * @param fn object receives results
			 * @retval true information found - stop enumerating
			 * @retval false information not found - processed all items	
			 */			 
			virtual bool enumEntries(const IEntryEnum &fn) const = 0;
			
			///enables access in MT environment
			/** All objects receives interlocked counters */
			virtual INode* enableMTAccess() = 0;
			

			virtual bool operator==(const INode &other) const = 0;
			bool operator !=(const INode &other) const {return !(*this == other);}

			///Creates copy of whole JSON tree using different factory
			virtual INode *clone(PFactory factory) const = 0;

			virtual ~INode() {}
			///Retrieves variable
			/**
			 * @param v name of variable
			 * @return reference to the value.
			 * @exception RequiredFieldException if not exists
			 */
			virtual INode &operator[](ConstStrA v) const = 0;

			///Retrieves field specified by index if it is array
			virtual INode &operator[](natural index) const = 0;

			///Retrieves field specified by index if it is array (C compatible)
			INode &operator[](int i) const {return operator[]((natural)i);}
			///Retrieves string content as UTF-8 string
			virtual ConstStrA getStringUtf8() const = 0;
			///Returns true, if string is stored in utf8 charset.
			/**
			 * @retval true string is stored as utf8. Call getStringUtf8() to retrieve content without conversion
			 * @return false string is stored as UNICODE. Call getString() to retrieve content without conversion
			 */
			virtual bool isUtf8() const = 0;
			///Adds new node into JSON container
			/**
			 * @param newNode pointer to new node. Don't add already
			 * added nodes. If nil used, adds NULL node
			 * @return reference to self enabling chain add
			 * 
			 * @note method is useful for arrays only
			 */
			virtual INode *add(Value newNode) = 0;
			///Adds new node into JSON container
			/**
			 * @param name name of node. For class only, ignored for arrays
			 * @param newNode pointer to new node. Don't add already
			 * added nodes. If nil used, adds NULL node
			 * @return reference to self enabling chain add
			 */
			virtual INode *add(ConstStrA name, Value newNode) = 0;

			///Adds new node directly from iterator result
			INode *add(const KeyValue &kv) {
				if (kv.getKeyType() == IKey::index) return add(kv.node);
				else return add(kv.getStringKey(),kv.node);
			}

			///Replaces value
			/**
			 * @param name key of which value replace
			 * @param newValue new value that replaces old one
			 * @param prevValue if not NULL, it will receive previous value
			 * @return pointer to this
			 *
			 * @note function does nothing if used on non-object value
			 */

			virtual INode *replace(ConstStrA name, Value newValue, Value *prevValue = 0) = 0;

			///Replaces value
			/**
			 * @param index index of the value to replace
			 * @param newValue new value that replaces old one
			 * @param prevValue if not NULL, it will receive previous value
			 * @return pointer to this
			 *
			 * @note function does nothing if used on non-array value
			 */
			virtual INode *replace(natural index, Value newValue, Value *prevValue = 0) = 0;

			///Copies array or object to the current array or object
			/** function cannot copy value for other node types. It just iterates though values and
			 * adds it to the current object. Items are not deeply copied, actually they are shared.
			 *
			 * @param from source object
			 */
			INode *copy(Value from) {
				for (JSON::Iterator iter = from->getFwIter(); iter.hasItems();) add(iter.getNext());
				return this;
			}

			///Returns true, when node is non-empty container
			virtual bool empty() const = 0;
			///erases item from object
			/**
			 * @param name name of field
			 * @return pointer to node allowing chains
			 */
			virtual INode* erase(ConstStrA name) = 0;
			///erases item from the array
			/**
			 * @param index index of item
			 * @return pointer to node allowing chains
			 */

			virtual INode* erase(natural index) = 0;

			///Erases all items in the container
			/** Function does nothing if value is not container
			 *
			 * */
			virtual INode *clear() = 0;

			///Retrieves iterator for container nodes
			virtual Iterator getFwIter() const = 0;

			ConstStrW operator()(ConstStrA name, ConstStrW defaultVal) const {
				const INode *k = getVariable(name); return k?k->getString():defaultVal;
			}
			ConstStrA operator()(ConstStrA name, ConstStrA defaultVal) const {
				const INode *k = getVariable(name); return k?k->getStringUtf8():defaultVal;
			}
			const wchar_t *operator()(ConstStrA  name, const wchar_t *defaultVal) const {
				const INode *k = getVariable(name); return k?k->getString().data():defaultVal;
			}
			const char *operator()(ConstStrA  name, const char *defaultVal) const {
				const INode *k = getVariable(name); return k?k->getStringUtf8().data():defaultVal;
			}
			natural operator()(ConstStrA name, natural defaultVal) const {
				const INode *k = getVariable(name); return k?(natural)k->getInt():defaultVal;
			}
			integer operator()(ConstStrA name, integer defaultVal) const {
				const INode *k = getVariable(name); return k?k->getInt():defaultVal;
			}
#ifdef _WIN64
			int operator()(ConstStrA name, int defaultVal) const {
				const INode *k = getVariable(name); return k?int(k->getInt()):defaultVal;
			}
#endif
			double operator()(ConstStrA  name, double defaultVal) const {
				const INode *k = getVariable(name); return k?k->getFloat():defaultVal;
			}
			float operator()(ConstStrA  name, float defaultVal) const {
				const INode *k = getVariable(name); return k?(float)k->getFloat():defaultVal;
			}
			bool operator()(ConstStrA name, bool defaultVal) const {
				const INode *k = getVariable(name); return k?k->getBool():defaultVal;
			}
			const INode * operator()(ConstStrA  name, const INode * defaultVal) const {
				const INode *k = getVariable(name); return k?k:defaultVal;
			}

			bool isNull(ConstStrA name, bool defaultVal) const {
				const INode *k = getVariable(name); return k?k->isNull():defaultVal;
			}

			bool isBool() const {return getType() == ndBool;}
			bool isIntNum() const {return getType() == ndInt;}
			bool isFloatNum() const {return getType() == ndFloat;}
			bool isNumber() const {return getType() == ndFloat || getType() == ndInt;}
			bool isString() const {return getType() == ndString;}
			bool isObject() const {return getType() == ndObject;}
			bool isArray() const {return getType() == ndArray;}
			bool isDeleteObj() const {return getType() == ndDelete;}
		};

		template<typename A,typename B>
		struct ConvHelper;

		template<typename X>
		class TypeToNodeDefinition;


		///Factory is responsible to create JSON node
		/** You should not create nodes directly, instead of use factory which can
		 * contain separate memory managment for nodes making creation of JSON very fast
		 *
		 * @note Note that you should not destroy factory if there are nodes created by it. First
		 * destroy JSON structure complette, then destroy factory. This doesn't apply to
		 * factory created by method create() without arguments
		 */
		class IFactory: public RefCntObj, public IInterface {
		public:


			///Create object (obsolete)
			virtual Value newClass() = 0;
			///Creates JSON object
			virtual Value newObject() = 0;
			///Creates JSON array
			virtual Value newArray() = 0;
			///Creates JSON number using unsigned value
			virtual Value newValue(natural v) = 0;
			///Creates JSON number using signed value
			virtual Value newValue(integer v) = 0;
#ifdef LIGHTSPEED_HAS_LONG_TYPES
			///Creates JSON number using unsigned value
			virtual Value newValue(lnatural v) = 0;
			///Creates JSON number using signed value
			virtual Value newValue(linteger v) = 0;
#endif
			///Creates JSON number using double-float value
			virtual Value newValue(double v) = 0;
			///Creates JSON bool and stores value
			virtual Value newValue(bool v) = 0;
			///Creates JSON string
			virtual Value newValue(ConstStrW v) = 0;
			///Creates JSON string
			virtual Value newValue(ConstStrA v) = 0;
			///Retrieves allocator used to allocate nodes
			virtual IRuntimeAlloc *getAllocator() const = 0;

			///to keep templates to work
			Value newValue(Value v) {return v;}
			
			Value newValue(NullType) {return newNullNode();}

#if __cplusplus >= 201103L
			Value newValue(std::nullptr_t x) {return newNullNode();}
#endif

			///Serialize custom value (undefined upper)
			template<typename T>
			Value newCustomValue(const T &x) {
				TypeToNodeDefinition<T> def(this);
				return def(x);
			}

			template<typename T>
			Value newValue(const T &val);


			///Creates new NULL node
			/**
			 * @note factory can use one NULL node for all required NULLs
			 */
			virtual Value newNullNode() = 0;
			///Creates new DELETE node
			/**
			 * @note factory can use one NULL node for all required DELETEs
			 */
			virtual Value newDeleteNode() = 0;
			///Merges two JSON trees
			/** Function merges only objects not arrays. Fields which contains DELETE-node
			 * will be removed. New fields in change-tree with names matching fields in base-tree will
			 * overwrite base-tree fields with new content
			 * @param base base tree
			 * @param change change tree (changes)
			 * @return result of merge
			 */
			virtual Value merge(const INode &base, const INode &change) = 0;
			///Create copy of the factory
			virtual IFactory *clone() = 0;

			///Composes string from JSON
			/**
			 * @param nd node to compose
			 * @return str string structure, reference returned is valid
			 * until method is used again or until object is destroyed
			 */
			 
			virtual ConstStrA toString(const INode &nd) = 0;
			
			///Sends JSON to stream
			/**
			 * @param nd node to send
			 * @return stream that will receive JSON
			 */
			virtual void toStream(const INode &nd, SeqFileOutput &stream) = 0;

			///Creates JSON tree from string
			/**
			 * @param text text contains serialized JSON
			 * @return parsed JSON structure as single node
			 *
			 * @note starting LightSpeed 13.04 there is new parser which can create different
			 * node objects for values and objects then object created by newValue() and newObject().
			 * There objects stores parsed strings and values more effectively taking minimum overhead
			 * but they cannot be edited. But you can still mix standard nodes with these special nodes
			 * without any noticeable difference
			 *
			 * @see parseFast
			 */
			virtual Value fromString(ConstStrA text) = 0;

			///Creates JSON tree from byte stream
			/**
			 * @param text text encoded into bytes containing serialized JSON
			 * @return parsed JSON structure as single node
			 *
			 * @note starting LightSpeed 13.04 there is new parser which can create different
			 * node objects for values and objects then object created by newValue() and newObject().
			 * There objects stores parsed strings and values more effectively taking minimum overhead
			 * but they cannot be edited. But you can still mix standard nodes with these special nodes
			 * without any noticeable difference
			 *
			 * @see parseFast
			 */
			virtual Value fromStream( SeqFileInput &stream ) = 0;

			virtual Value fromCharStream( IVtIterator<char> &iter) = 0;

			template<typename T>
			Value operator()(const T &v) {return newValue(v);}

			///Creates JSON string
			Value array() {return newArray();}
			Value object() {return newObject();}

			virtual ~IFactory()  {}


		};

		///Sets property of toString function
		/** This interface can be retrieved using getIfc<> on IFactory */
		class IFactoryToStringProperty {
		public:
			///Enable escaping for utf-8 characters
			/** by default UTF-8 strings are serialized as they are. Setting this property causes
			 * that all UTF-8 characters will be escaped using \uXXXX escape sequence. Only ASCII
			 * Characters will not be escaped. This can slow processing and make result bigger, but
			 * sometimes is need to overcome text encoding issues
			 *
			 * @param enable true
			 */
			virtual void enableUTFEscaping(bool enable) = 0;
		};
			
		///Creates default factory to build JSON
		/** This factory CAN BE destroyed even if there are nodes created by this factory, because
		 * factory uses standard allocator to allocate nodes
		 * @return pointer to factory
		 */
		PFactory create();
		///Creates factory which will use another allocator
		/**
		 * @return pointer to factory
		 */
		PFactory create(IRuntimeAlloc &alloc);
		///Creates fast fragmentation-free factory for usage in single thread
		PFactory createFast();
		///Retrieves global null node (without allocation)
		/** Factories uses this function, so you don't need it it call explicitly */
		Value getNullNode();
			

		///Custom node interface
		/** Interface is available with custom nodes, but it is not part
		 * of INode. You have to use getIfc<ICustomNode> to retrieve
		 * ICustomNode from INode if available;
		 */
		class ICustomNode {
		public:

			///custom serializer
			/**
			 * @param output reference to output iterator
			 *
			 * @note when serialization takes effect, function must
			 * render output directly to the stream and should not expect
			 * any future processing. For example in case, that string
			 * is rendered, it must contain quotes and invalid characters
			 * should be escaped
			 */
			virtual void serialize(IVtWriteIterator<char> &output, bool escapeUTF8) const = 0;
		};


		template<typename T>
		Value PFactory::operator()( const T &val )
		{
			return (*this)->operator ()(val);
		}





		template<typename A,typename B>
		struct ConvHelper {
			template<typename X>
			static Value conv(const X &x, IFactory &f) {
				return conv(x,f,typename MIsConvertible<X,A>::MValue());
			}
			template<typename X>
			static Value conv(const X &x, IFactory &f, MTrue) {
				return f.newValue(A(x));
			}
			template<typename X>
			static Value conv(const X &x, IFactory &f, MFalse) {
				return B::conv(x,f);
			}
		};

		template<typename A>
		struct ConvHelper<A,Empty> {
			template<typename X>
			static Value conv(const X &x, IFactory &f) {
				return conv(x,f,typename MIsConvertible<X,A>::MValue());
			}
			template<typename X>
			static Value conv(const X &x, IFactory &f, MTrue) {
				return f.newValue(A(x));
			}
			template<typename X>
			static Value conv(const X &x, IFactory &f, MFalse) {
				return f.newCustomValue(x);
			}
		};

		template<typename T>
		Value IFactory::newValue(const T &val) {
			typedef ConvHelper<integer,
					ConvHelper<natural,
#ifdef LIGHTSPEED_HAS_LONG_TYPES
					ConvHelper<linteger,
					ConvHelper<lnatural,
#endif
					ConvHelper<float,
					ConvHelper<double,
					ConvHelper<ConstStrA,
					ConvHelper<ConstStrW,
					ConvHelper<bool,Empty>
#ifdef LIGHTSPEED_HAS_LONG_TYPES					
					> > 
#endif				
					> > > > > >				Conv;

			return Conv::conv(val,*this);
		}



		extern LIGHTSPEED_EXPORT const char *strTrue;
		extern LIGHTSPEED_EXPORT const char *strFalse;
		extern LIGHTSPEED_EXPORT const char *strNull;
		extern LIGHTSPEED_EXPORT const char *strDelete;

	};




};

#include "jsonbuilder.h"
