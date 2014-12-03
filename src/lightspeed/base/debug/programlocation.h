#ifndef bredy_base_PROGRAMLOCATION_H_
#define bredy_base_PROGRAMLOCATION_H_

namespace LightSpeed
{

    ///Describes location is source code
    /**
    To construct instance that contains current location, use macro THISLOCATION.
    */
    struct ProgramLocation {

        ///Name of the source file 
        const char *file;
        ///line in source file
        int line;
        ///function name
        const char *function;

        ///Constructor
        /**
        To construct current location, use THISLOCATION as constructor
        */
        ProgramLocation(const char *file,
                    int line,
                    const char *function):
                    file(file),line(line),function(function) {}    

		ProgramLocation():
		file(""),line(0),function("") {}    

					bool operator ==(const ProgramLocation &other) const {
            return file == other.file && line == other.line && function == other.function;
        }
        bool operator !=(const ProgramLocation &other) const {
            return !operator==(other);
        }

		bool isUndefined() const {return file == 0 || file[0] == 0;}

    };

    ///Always contains instance of ProgramLocation class with the current location
    /**
        ProgramLocation thisPlace = THISLOCATION;
    */
#ifdef _DEBUG
    #define THISLOCATION (::LightSpeed::ProgramLocation(__FILE__,__LINE__,__FUNCTION__))
#else
	#define THISLOCATION (::LightSpeed::ProgramLocation(__FUNCTION__,__LINE__,__FUNCTION__))
#endif
	
}

#endif /*PROGRAMLOCATION_H_*/
