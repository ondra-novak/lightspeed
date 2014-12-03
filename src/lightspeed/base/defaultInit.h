
#ifndef LIGHTSPEED_DEFAULTINIT_H_
#define LIGHTSPEED_DEFAULTINIT_H_


namespace LightSpeed {

    
    template<class T>
    struct DefaultInit {
        operator T() const {return T();}
    };
    
    template<>
    struct DefaultInit<bool> {
        operator bool() const {return false;}
    };

    template<>
    struct DefaultInit<char> {
        operator char() const {return 0;}
    };

    template<>
    struct DefaultInit<unsigned char> {
        operator unsigned char() const {return 0;}
    };

    template<>
    struct DefaultInit<wchar_t> {
        operator wchar_t() const {return 0;}
    };

    template<>
    struct DefaultInit<short> {
        operator short() const {return 0;}
    };

    template<>
    struct DefaultInit<unsigned short> {
        operator unsigned short() const {return 0;}
    };

    template<>
    struct DefaultInit<int> {
        operator int() const {return 0;}
    };

    template<>
    struct DefaultInit<unsigned int> {
        operator unsigned int() const {return 0;}
    };

    template<>
    struct DefaultInit<long> {
        operator long() const {return 0;}
    };

    template<>
    struct DefaultInit<unsigned long> {
        operator unsigned long() const {return 0;}
    };

    template<>
    struct DefaultInit<long long> {
        operator long long() const {return 0;}
    };

    template<>
    struct DefaultInit<unsigned long long> {
        operator unsigned long long() const {return 0;}
    };

    template<>
    struct DefaultInit<float> {
        operator float() const {return 0;}
    };

    template<>
    struct DefaultInit<double> {
        operator double() const {return 0;}
    };

    template<class T>
    struct DefaultInit<T *> {
        operator T *() const {return 0;}
    };

    template<class T>
    struct DefaultInit<const T *> {
        operator const T *() const {return 0;}
    };

}


#endif 
