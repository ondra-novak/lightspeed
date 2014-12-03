/*
 * autoflush.h
 *
 *  Created on: 19.8.2009
 *      Author: ondra
 */

#ifndef AUTOFLUSH_H_
#define AUTOFLUSH_H_

#include "../containers/optional.h"
#include "../iter/iteratorChain.h"

namespace LightSpeed {

    ///Interface to control output stream
    /**For more information, see AutoFlushWriter
     *
     * @see AutoflushWriter, AutoflushReader
     */
    class IOutputControl {
    public:
        ///Enforces output to flush buffers
        /**
         * Calling this function, all buffered data are flushed to the output
         */
        virtual void flushBeforeRead() = 0;

        ///Sets address of dirty flag
        /**
         * By default, dirty flag is declared inside of this interface, but
         * time to time can be useful to define another address of dirty flag.
         * Because dirty flag can be checked very often, it is a lot of faster to
         * check single byte at the address, instead of calling virtual function.
         *
         * So if you implement central point that is able to flush multiple
         * outputs depends on multiple inputs, you probably need one
         * central dirty flag for all outputs and inputs.
         *
         * @param dirtyFlagAddr New address of dirty flag. Address musts
         *  be valid until there is at-least one object that uses this dirty
         *  flag. Set NULL to enable internal dirty flag
         *
         */
        void setDirtyFlagAddr(bool *dirtyFlagAddr) {
            if (dirtyFlagAddr == 0) dirty = &dirtyFlag;
            dirty = dirtyFlagAddr;
        }

        ///Tests, whether output is dirty.
        /**
         * Dirty output means, that there is posibilty of unsaved data stored
         * in the output buffer. Calling of flushBeforeRead() causes,
         * reseting this flag. Any later write to the output sets this flag
         * to the true
         * @retval true output stream should be flushed.
         * @retval false output stream don't need to be flushed
         *
         * @note flag doesn't check, whether buffers are really flushed. It
         * is only set or reset by derived iterator during data operations. It
         * is possible, that flag will contain false and there will still be
         * data in the buffer, because data has been added to the buffer
         * by different way
         */
        bool isDirty() const {return *dirty;}

        void setDirty(bool state) {
            *dirty = state;
        } 

        ///Constructor
        IOutputControl():dirty(&dirtyFlag),dirtyFlag(false) {}
        IOutputControl(const IOutputControl &):dirty(&dirtyFlag),dirtyFlag(false) {}
		
		virtual ~IOutputControl() {}
    protected:
        bool *dirty;
        bool dirtyFlag;

    };

    ///This template class is write iterator filter performing automatic flushing
    /** Automatic flushing can be performed by two ways.
     *
     * The first way uses special
     * character which trigers flush after it is written. This is used with
     * for example line terminals, where output is flushed after each line written.
     *
     * The second way is coopeartion with the input. When program requests
     * data from the input stream, output stream is flushed first. Flushing
     * of output stream can prevent input deadlock, especially in the
     * situation, when other side can generate data after it reads all
     * data sent by this side. Without flushing output, data can be hold in
     * the buffer future waiting on input stream causes deadlock.
     *
     * Class AutoflushWriter can provide both ways. Second way is implemented
     * in cooperation with AutoflushReader and IOutputControl interface.
     *
     * @param Iterator output iterator, where data are writen
     *
     * @see AutoflushReader
     */
    template<typename Iterator>
    class AutoflushWriter: public IteratorChain<Iterator,
                            AutoflushWriter<Iterator>, WriteIteratorBase >,
                           public IOutputControl {
    public:

		typedef IteratorChain<Iterator,AutoflushWriter<Iterator>, ::LightSpeed::WriteIteratorBase > Super;
        typedef typename DeRefType<Iterator>::T::ItemT ItemT;
        AutoflushWriter(Iterator iter):Super(iter) {}
        AutoflushWriter(Iterator iter, const ItemT flushChar)
                :Super(iter),flushChar(flushChar) {}

        bool hasItems() const {return this->iter.hasItems();}
        bool canAccept(const ItemT &x) const {return this->iter.canAccept(x);}
        natural getRemain() const {return this->iter.getRemain();}
        void write(const ItemT &x)  {
            this->iter.write(x);
            if (flushChar.isSet() && flushChar.get() == x)
                this->iter.flush();
            else
                this->setDirty(true);
        }
        void skip() {this->iter.skip();}
        void flush() {this->iter.flush();}
        ///Sets flush character
        /**
         * @param chr new flush character. Default character is not defined,
         *  causing that feature is turned off
         */
        void setFlushChar(const ItemT &chr) {flushChar.set(chr);}
        ///Unsets flush character
        /**
         * autoflush by character is turned off
         */
        void unsetFlushChar() {flushChar.unset();}
        /// Tests whether flush character is set
        /**
         * @retval true flush character is set
         * @retval false flush characte is not set
         */
        bool isSetFlushChar() const {return flushChar.isSet();}
        ///Retrieves flush character
        /**
         * @return current flush character. Function can be called only
         * when flush character is set (see isSetFlushChar()). Otherwise
         * content of return value is undefined
         */
        const ItemT &getFlushChar() const {return flushChar.get();}

        ///Forces flush before read
        /** Function is called from AutoflushReader
         */
        virtual void flushBeforeRead() {
            flush();
        }


    public:
        Optional<ItemT> flushChar;
    };


    ///This template class is read iterator filter performing automatic flushing of the connected output iterator
    /**
     * To connect input with the output, you have to add this filter into the
     * input chain and connect it with the requested output iterator.
     * Output iterator must be declared using AutoflushWriter template class
     *
     * You can later connect an instance of the output iterator with an instance
     * of this class. Any reading (including of test operations, such a
     * hasItems() or getRemain()) causes flush of the output, when
     * output is reported as dirty (so flush is not called more then once, between
     * writes)
     *
     * @param Iterator next chained iterator
     *
     * @see AutoflushWriter
     */

    template<typename Iterator>
    class AutoflushReader: public IteratorChain<Iterator,
                            AutoflushReader<Iterator>,IteratorBase > {
    public:
		typedef IteratorChain<Iterator,AutoflushReader<Iterator>,::LightSpeed::IteratorBase > Super;
        typedef typename DeRefType<Iterator>::T::ItemT ItemT;


        AutoflushReader(Iterator iter):Super(iter),flushIfc(0) {}
        AutoflushReader(Iterator iter, IOutputControl *flushIfc)
                :Super(iter),flushIfc(flushIfc) {}

        bool hasItems() const {
            flushBeforeRead();
            return this->iter.hasItems();
        }

        natural getRemain() const {
            flushBeforeRead();
            return this->iter.getRemain();
        }

        const ItemT &peek() const {
            flushBeforeRead();
            return this->iter.peek();
        }

        const ItemT &getNext()  {
            flushBeforeRead();
            return this->iter.getNext();

        }

        void skip() const {
            flushBeforeRead();
             this->iter.skip();
        }

        template<typename Traits>
        natural blockRead(FlatArrayRef<ItemT,Traits> buffer, bool readAll = false) {
            flushBeforeRead();
            return this->iter.blockRead(buffer, readAll);
        }

        void flush() {
            flushBeforeRead();
        }

        ///Connects output stream with the input stream
        /**
         * @param flushIfc Pointer to IOutputControl interface. This
         *  interface is implemented by the AutoflushWrthis->iter. Set to 0 to
         *  turn off the feature
         */
        void connectOutput(IOutputControl *flushIfc) {
            this->flushIfc = flushIfc;
        }

        ///Returns current connected output
        /**
         * @return pointer to IOutputControl implemented by the
         * AutoflushWrthis->iter. Returns 0, if not set
         */
        IOutputControl *getConnectedOutput() const {
            return flushIfc;
        }


    protected:
        mutable IOutputControl *flushIfc;

        void flushBeforeRead() const {
            if (flushIfc && flushIfc->isDirty()) {
                flushIfc->flushBeforeRead();
                flushIfc->setDirty(false);
            }
        }

    };

}


#endif /* AUTOFLUSH_H_ */
