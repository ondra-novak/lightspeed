#pragma once

#include "../containers/linkedList.h"
#include "requiredField.h"
#include "../memory/staticAlloc.h"
namespace LightSpeed {


	///Structure serializer
	/** Struct allows to serialize and deserialize section
	 * even if fields are in random order. This is typical for
	 * formats, which can be created or edited by user in the ordinary
	 * text editor.
	 *
	 * Serializer typically identify sections by the name and by the order.
	 * Some formats doesn't store names, so still by the order remains. Only
	 * formats, where names are used can skip sections containing the default content.
	 *
	 * If you want to allow random ordering, you have to use SerializeStruct
	 * class or Serializer::Struct specialization. All sections created using
	 * this class are stored as tagged, so formatter must store the names of
	 * sections to allows later identification.
	 *
	 * Storing using this class has no difference in compare to storing using
	 * common serializer. But loading has some difficults. During loading
	 * fields from the archive, program cannot use already loaded fields, because
	 * - simply they are not probably loaded yet. Serializer used operator()
	 * to postponing request while the order of archive is different. Once
	 * all fields are processed and instance of structure is destroyed,
	 * all fields should contain data or default content.
	 *
	 * Class is implemented using linked list that works as queue. If
	 * requested section is not expected in the archive, it is enqueued
	 * and can be loaded later, when archive reaches it.
	 *
	 *
	 *  @tparam Archive archive used for this struct
	 *  @tparam SectId type used as section name (section id). Fields
	 *  in the struct must use the same identifier
	 *
	 *
	 *
	 */
	template<typename Archive, typename SectId >
	class SerializeStruct {

		typedef typename Archive::Factory AllocFact;


		typedef void (*ExchangeFunction)(Archive &arch, void *ptr);

		struct Record {
			void *variablePtr;
			SectId name;
			ExchangeFunction fn;
			bool required;

			Record(void *variablePtr,const SectId &name, ExchangeFunction fn, bool required)
				:variablePtr(variablePtr),name(name),fn(fn),
				required(required) {}
			Record(const SectId &name)
				:variablePtr(0),name(name),fn(0) {}
		};

		public:

		///Defines, how serialize will handle unknown fields
		enum StrictMode {
			///Default mode - unknown fields are skipped (if archive allows this)
			normal,
			///Strict mode - unknown fields throws the exception
			strict
		};

		///Inicializes structure serializer
		/**
		 *
		 * @param arch archive used with serializer
		 * @param mode required mode
		 */
		SerializeStruct(Archive &arch, StrictMode mode = normal)
			:arch(arch),itemStack(arch.getFactory()),strictMode(mode == strict) {}
		~SerializeStruct() {
			if (!std::uncaught_exception()) commit();
		}

		///Commits all items in the struct
		/** By default, this is called in the destructor. Sometime can
		 * be useful to call this manually. Commiting the struct
		 * causes, that all postponed fields will be loaded. If normal
		 * mode is in effected, function will skip all not postponed
		 * sections. If strict mode is in effect, function will throw
		 * exception on not regustered section
		 *
		 *
		 * @note After each serializer action, class tries to
		 * match all postponed sections to data following in the archive
		 * to keep set of postponed section small as possible.
		 */
		void commit();

		///Serialization of simple variable with the name
		/**
		* @param x variable to serialize
		* @param name name of variable
		* @retval true successfully serialized
		* @retval false cannot open section with this name (probably, not found)
		*/
		template<class T> 
		void operator()(T &x, const SectId &name) {
			setExchange(x,name,false);
		}
		///Serialization of simple variable with the name and default value
		/**
		* @param x variable to serialize
		* @param name name of variable
		* @param defValue default value
		* @retval true successfully serialized
		* @retval false not serialized, used default value
		*/
		template<class T>
		void operator()(T &x, const SectId &name, const T &defValue) {
			if (arch.loading()) x = defValue;
			setExchange(x,name,false);
		}
		
		template<class T>
		void operator()(T &x, const SectId &name, _TRequiredField m) {
			setExchange(x,name,true);
		}

	protected:

		typedef LinkedList<Record, AllocFact> Stack;
		Archive &arch;
		Stack itemStack;

		template<typename T>
		bool setExchange(T &x, const SectId &name, bool required);

		template<typename T>
		static void exchangeProxy(Archive &arch, void *ptr);
		bool strictMode;

		bool commitNext();
	};

	template<typename Archive, typename SectId >
	template<typename T>
	void SerializeStruct<Archive,SectId>::exchangeProxy( Archive &arch, void *ptr )
	{
		T *p = reinterpret_cast<T *>(ptr);		
		arch(*p);
	}
	template<typename Archive, typename SectId >
	template<typename T>
	bool SerializeStruct<Archive,SectId>::setExchange(
			T &x, const SectId &name, bool required)
	{
		if (arch.loading()) {
			if (arch.getFormatter().openTaggedSection(name)) {
				arch(x);
				arch.getFormatter().closeTaggedSection(name);
				commitNext();
				return true;
			} else {
				itemStack.add(Record(&x,name,&exchangeProxy<T>,required));
				return false;
			}

		} else {
			arch.getFormatter().openTaggedSection(name);
			arch(x);
			arch.getFormatter().closeTaggedSection(name);
			return true;
		}
	}

	template<typename Archive, typename SectId  >
	bool SerializeStruct<Archive, SectId>::commitNext() {
		bool rep = false;
		bool retval = false;
		do {
			rep = false;
			SectId name;
			if (arch.getFormatter().openTaggedSection(name)) {
				arch.getFormatter().closeTaggedSection(name);
			} else {

				typename Stack::Iterator iter = itemStack.getFwIter();
				while (iter.hasItems()) {
					const Record &rc = iter.peek();
					if (rc.name == name) {
						arch.getFormatter().openTaggedSection(name);
						rc.fn(arch,rc.variablePtr);
						arch.getFormatter().closeTaggedSection(name);
						rep = true;
						itemStack.erase(iter);
						retval = true;
						break;
					} else {
						iter.skip();
					}
				}
			}
		} while (rep && arch.getFormatter().hasSectionItems());
		return retval;
	}



	template<typename Archive, typename SectId  >
	void SerializeStruct<Archive, SectId>::commit()
	{

		if (arch.storing()) 
			return;

		while (!itemStack.empty() && arch.getFormatter().hasSectionItems()) {

			if (!commitNext()) {
				SectId sect;
				arch.getFormatter().openTaggedSection(sect);
				if (strictMode == strict) {
					throw UnexpectedSectionException(THISLOCATION,sect.toString());
				} else {
					arch.getFormatter().openTaggedSection(sect);
					arch.getFormatter().closeTaggedSection(sect);
				}
			}
		}

		while (!itemStack.empty()) {
			const Record &rc = itemStack.getFirst();
			if (rc.required)
				throw RequiredSectionException(THISLOCATION,rc.name.toString());
			else
				itemStack.eraseFirst();
		}
	}
}
