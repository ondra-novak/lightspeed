/**
 * @file
 * Simple implementation of countof operator
 * It returns count of items in array
 * 
 * $Id: countof.h 369 2013-04-04 13:15:58Z ondrej.novak $
 */

#ifndef COUNTOF_H_
#define COUNTOF_H_


namespace LightSpeed {

    ///Retrieves count of items in a static array
    /**
     * @param arr A static array of any type. It must be array, not pointer to array
     * @return count of items in array (not bytes)
     * @note Compiler should convert this function into constant for -O2 and better
     * optimization.
     */
    template<class T, int n>
    static inline natural countof(const T (&)[n]) {
        return (natural)n;
    }
    


}
#endif /*COUNTOF_H_*/
