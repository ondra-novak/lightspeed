#ifndef LIGHTSPEED_ALIGN_H_
#define LIGHTSPEED_ALIGN_H_

namespace LightSpeed {

    static const natural preferedDataAlign = sizeof(void *); 
    
    template<natural size>
    struct AlignSz {
        
        static const natural unaligned = size % preferedDataAlign;
        static const natural padding = preferedDataAlign - unaligned;
        static const natural alignedSize = size + padding;
        
    };
    
    template<class T>
    struct AlignType: public AlignSz<sizeof(T)> {};

}
#endif /*ALIGN_H_*/
