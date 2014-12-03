/*
 * generator.h
 *
 *  Created on: 8.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ITER_GENERATOR_H_
#define LIGHTSPEED_ITER_GENERATOR_H_

#include "iterator.h"
#include "../../mt/fiber.h"

namespace LightSpeed {

	template<typename T>
	class GeneratorWriter;

	///Generator's backend class
	/**
	 * Function that implements generator is communicating with this interface
	 * It complete hides implementation of the generator so the function
	 * can be written without no dependency to implementation of the generator's iterator.
	 *
	 * To work with this interface, use GeneratorWriter<T> class instead direct accessing
	 * this interface
	 *
	 * @tparam T subject of generating
	 */
	template<typename T>
	class GeneratorBackend: public Fiber{
	public:
		typedef Message<void, GeneratorWriter<T> > GeneratorFunction;
		virtual ~GeneratorBackend() {}
	protected:


		friend class GeneratorWriter<T>;

		typedef typename ConstStringT<T>::Iterator ValueIterator;


		///ctor
		GeneratorBackend():needItems(true) {}
		///dtor
		///pushes value to the generator iterator
		/**
		 * Function can implement swaping context and can cause undefined
		 * waiting during pushed value is processed.
		 *
		 * @param value value to push
		 * @exception WriteIteratorNoSpace there is no space to store value. It
		 * is thrown, when function is called with needItems equal to false
		 *
		 * @see GeneratorWriter::write()
		 */
		virtual void pushValue(ValueIterator &value) = 0;


		///contains state about whether other side of generator is able to receive items
		/** if value is true, function pushValue() will accept the item.
		 *  if value is false, function pushValue() throws the exception
		 *
		 *  @see GeneratorWriter::hasItems()
		 */
		bool needItems;

	public:
		void bootstrap(GeneratorFunction fn) {
			(fn)(GeneratorWriter<T>(*this));
		}
	};

	///Forks program to two parts, where one part is generating data and other part reading data from the iterator
	/**
	 * This iterator allows you to generate data using single function which
	 * perform generation in cycle without exiting cycle context, and
	 * also ready this generated data in another function, which is able to
	 * read this data also in cycle without exiting cycle context. Once
	 *
	 * Iterator request function as parameter (constructed as Message) It can
	 * be single function, member function with object specified and can
	 * have any count of parameters.
	 *
	 * Once iterator is constructed, function is called to prepare first value.
	 * Every call getNext() will cause that function is resumed to retrieve
	 * next value.
	 *
	 * The Iterator is standard iterator. Data don't need to be fully read.
	 * Destructor additional kills generator function. If function finished before
	 * iterator is destroyed, hasItems() is set to the false and no more
	 * data can be read from it
	 *
	 *
	 */


	template<typename T>
	class GeneratorIterator: public IteratorBase<T, GeneratorIterator<T> >,
							 private GeneratorBackend<T>
							 {
	public:

		typedef typename GeneratorBackend<T>::GeneratorFunction GeneratorFunction;


		GeneratorIterator(const IFiberFunction &fn, natural stackSize = 0);

		///Construct generator iterator
		/**
		 * @param fn function to call as generation function
		 * @param stackSize size of stack for the function. Size in bytes
		 * of space to store function context. Default value will create
		 * context with same size as current thread. You can specify any value
		 * that you want, bud prepare, that lower values can lead to stack overflow
		 *
		 * @note Arguments of the function are valid until first yield() or resume() is called. One of these
		 * functions is called by function GeneratorWriter::write(). Do not refer arguments
		 * directly, always make copy of them on the stack. This complication is due performance
		 * (iterator doesn't copy action, and caller can optimize copying)
		 */

		GeneratorIterator(const GeneratorFunction &fn, natural stackSize = 0);

		virtual ~GeneratorIterator();

		///Retrieves true, whether function can generate next value
		/**
		 * @retval true yes, next value will be ready
		 * @retval false no, no more values can be generated
		 * @note First call of hasItems() after getNext() causes generation
		 * of the new value. Returned value contains true, if function has
		 * been resumed, or false if exited
		 */
		bool hasItems() const;


		const T &getNext();
		///Returns next value without removing it from the buffer
		 /** @note First call of peek() after getNext() causes generation
		 * of the new value.
		 */
		const T &peek();



	protected:


		typedef typename GeneratorBackend<T>::ValueIterator ValueIterator;
		mutable ValueIterator *curStream;

		bool prepareNext() const;
		bool prepareNext();

		virtual void pushValue(ValueIterator &value);


	};



	///Routes writes from generator to the iterator
	/**
	 * Instance of this class must be constructed in the generator. It performs
	 * transfer items from the generator to the iterator.
	 *
	 * Class is implemented as write iterator. Function should be
	 * controlled using function hasItems() as usual and stop writing
	 * when this function returns false (future writings will throw exception).
	 *
	 * Sending items to the iterator is done by the function write()
	 *
	 * @note Because generator is always one item before iterator,
	 * generator will always generate one extra item before it can
	 * detect hasItems() false. The last written item is probably dropped. This
	 * breaks main rule of write iterator, where all items written before hasItems()
	 * returns false will be stored (or writing or destructor throws exception,
	 * when writing cannot be finished due an exception). This iterator
	 * will always drop the last item due implementation. It need to have always
	 * one item ready when first hasItems() or peek() is called on the iterator in
	 * order to response the question whether there is at least one item that can
	 * be read.
	 *
	 *
	 */
	template<typename T>
	class GeneratorWriter: public WriteIteratorBase<T, GeneratorWriter<T> > {
	public:

		///Constructs write
		/**
		 * Note that constructor has no parameters. It will connect the
		 * iterator using the fiber context (caller variable).
		 */
		GeneratorWriter();

		GeneratorWriter(GeneratorBackend<T> &otherSide):otherSide(otherSide) {}

		///returns true, if iterator has space for the item
		bool hasItems() const;
		///sends item to the other side
		/**
		 * @param value value to send. Function suspends itself until
		 * the value is not given up
		 * @exception WriteIteratorNoSpace Thrown, if other side drops reading.
		 * Note that, in this state, reading end now probably waiting in the destructor of the iterator
		 */
		void write(const T &value);

		///sends block of item to the other side
		/**
		 * Function switches to other side and doesn't return until whole
		 * buffer is processed
		 *
		 * @param buffer Buffer of items
		 * @param writeAll true to write all - this field required by interface,
		 * 		but ignored. Function always writes all items. However, other side can
		 * 		request destruction before all items are read. In this case.
		 * 		function returns count of items processed and any future calls
		 * 		ends with exception.
		 * @return function returns count of written items. It should be equal
		 * 		to size of buffer unless reader require to destroy iterator.
		 * 		If returned value is less than size of buffer, generator
		 * 		should exit, because any future writes are disalloved causing
		 * 		exception to be thrown
		 */
        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<T>::Remove,Traits> &buffer, bool writeAll);

		///sends block of item to the other side
		/**
		 * Function switches to other side and doesn't return until whole
		 * buffer is processed
		 *
		 * @param buffer Buffer of items
		 * @param writeAll true to write all - this field required by interface,
		 * 		but ignored. Function always writes all items. However, other side can
		 * 		request destruction before all items are read. In this case.
		 * 		function returns count of items processed and any future calls
		 * 		ends with exception.
		 * @return function returns count of written items. It should be equal
		 * 		to size of buffer unless reader require to destroy iterator.
		 * 		If returned value is less than size of buffer, generator
		 * 		should exit, because any future writes are disalloved causing
		 * 		exception to be thrown
		 */
        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<T>::Add,Traits> &buffer, bool writeAll);


	protected:
		GeneratorBackend<T> &otherSide;

		static GeneratorBackend<T> &findOtherSide();
	};

}



#endif /* GENERATOR_H_ */
