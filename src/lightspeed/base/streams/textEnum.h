/*
 * textEnum.h
 *
 *  Created on: 31.7.2009
 *      Author: ondra
 */
#if 0
#ifndef LIGHTSPEED_STREAMS_TEXTENUM_H_
#define LIGHTSPEED_STREAMS_TEXTENUM_H_

#include "../memory/stdAllocator.h"
#include "../containers/arrayref.h"
#include "../containers/string.h"

namespace LightSpeed {


    template<typename EnumType, typename TextType = char>
    struct EnumToTextDef {

        EnumType eVal;
        const ConstStringT<TextType> textVal;

        typedef EnumType EnumT;
        typedef TextType TextT;

    };


    template<typename Table,
             typename Allocator = StdAlloc<natural> >
    class TextParseEnum: public IteratorFilterBase<
                        typename DeRefType<Table>::T::ItemT::TextT,
                        typename DeRefType<Table>::T::ItemT::EnumT,
                        TextParseEnum<Table,Allocator> >
    {
    public:
        typedef typename DeRefType<Table>::T::ItemT::TextT TextT;
        typedef typename DeRefType<Table>::T::ItemT::EnumT EnumT;

        TextParseEnum(Table table);

        bool needItems() const;
        bool canAccept(const TextT &x) const;
        void input(const TextT &x);
        bool hasItems() const;
        EnumT output();
        void flush();
    protected:
        Table table;
        natural pos;
        AutoArray<natural,Allocator> indexes;
        bool search(const TextT &x, AutoArray<natural,Allocator> *idxl) const;
        bool searchEnd(AutoArray<natural,Allocator> *idxl) const;
    };



    template<typename Table>
    class TextComposeEnum: public IteratorFilterBase<
                                typename DeRefType<Table>::T::ItemT::EnumT,
                                typename DeRefType<Table>::T::ItemT::TextT,
                                TextComposeEnum<Table> >
    {
    public:
        typedef typename DeRefType<Table>::T::ItemT::TextT TextT;
        typedef typename DeRefType<Table>::T::ItemT::EnumT EnumT;

        TextComposeEnum(Table table);

        bool needItems() const;
        bool canAccept(const EnumT &x) const;
        void input(const EnumT &x);
        bool hasItems() const;
        TextT output();
        void flush();
    protected:
        Table table;
        const TextT *reader;
    };


    //--------------------- IMPL ---------------------

    template<typename Table, typename Allocator>
    TextParseEnum<Table,Allocator>::TextParseEnum(Table table)
        :table(table),pos(0) {}

    template<typename Table, typename Allocator>
    bool TextParseEnum<Table,Allocator>::needItems() const {
        return pos != naturalNull;

    }
    template<typename Table, typename Allocator>
    bool TextParseEnum<Table,Allocator>::canAccept(const TextT &x) const {
        return search(x,0);
    }
    template<typename Table, typename Allocator>
    void TextParseEnum<Table,Allocator>::input(const TextT &x) {
        if (pos == naturalNull)
            throw WriteIteratorNotAcceptable(THISLOCATION);
        if (search(x,&indexes)) {
            pos++;
            if (indexes.size() == 1 && table[indexes[0]].textVal.size() <= pos)
                pos = naturalNull;
        } else {
            indexes.clear();
            pos = 0;
            throw WriteIteratorNotAcceptable(THISLOCATION);
        }
    }
    template<typename Table, typename Allocator>
    bool TextParseEnum<Table,Allocator>::hasItems() const {

        return pos == naturalNull || searchEnd(0);
    }
    template<typename Table, typename Allocator>
    typename TextParseEnum<Table,Allocator>::EnumT
                                    TextParseEnum<Table,Allocator>::output() {
        flush();
        return table[indexes[0]].eVal;
    }

    template<typename Table, typename Allocator>
    void TextParseEnum<Table,Allocator>::flush() {
        if (pos != naturalNull && !searchEnd(&indexes))
            throw IteratorNoMoreObjects(THISLOCATION,typeid(EnumT));
    }

    template class TextParseEnum<ArrayRef<EnumToTextDef<Direction::Type,char> > >;
}

#endif
#endif /* TEXTENUM_H_ */
