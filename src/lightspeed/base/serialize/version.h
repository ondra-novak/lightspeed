#ifndef LIGHTSPEED_SERIALIZER_VERSION_H_
#define LIGHTSPEED_SERIALIZER_VERSION_H_

#include "../exceptions/serializerException.h"
#include "../compare.h"
#include <set>

 
namespace LightSpeed
{

    template<typename VerObj, typename Less = std::less<VerObj> >
    class Version: public ComparableLess<Version<VerObj,Less> > {
        
    public:
        ///super class
        typedef ComparableLess<Version<VerObj,Less> > Super;
        
        
        ///Creates version object using the version value
        Version(const VerObj &ver):version(ver) {}
        ///Creates version object with the default value
        Version() {}
        
        ///Retrieves current version
        const VerObj &getVersion() const {return version;}
        
        ///implements comparsion lessThan
        /** Comparsion must be implemented to allow comparsion using the
         * operators < > =
         * @param other other object which is the compared
         */
        bool lessThan(const Super &other) {
            const VerObj &otherVal = other.Invoke().getVersion();
            Less less;
            return less(version,otherVal);
        }
        
        ///Perform version synchronization with the archive
        /** Version synchronization is operation, when version is written down
         * to or read up from the archive. Storing archive will write down the
         * current version, loading archive will retrieve version from the
         * archive. Retrieved version is compared with the current
         * version and if the archive version is newer then current,
         * exception is thrown.
         * 
         * @param arch archive which is used to synchronize
         */
        template<typename Archive>
        void versionSync(Archive &arch) {
            Less less;
            VerObj save = version;
            arch(*this);
            if (less(save,this))
                throw ArchiveUnsupportedVersionExceptionT<VerObj>(
                        THISLOCATION, save, version);            
        }
        
        ///Perform version synchronization with the archive
        /** Version synchronization is operation, when version is written down
         * to or read up from the archive. Storing archive will write down the
         * current version, loading archive will retrieve version from the
         * archive. Retrieved version is compared with the current
         * version and if the archive version is newer then current,
         * exception is thrown.
         * 
         * @param arch archive which is used to synchronize
         * @param name name of field in the archive which contains version
         * @retval true synchronization has been processed
         * @retval false synchronization has not been processed, because field
         *      name cannot be found
         */
        template<typename Archive>
        bool versionSync(Archive &arch, const char *name) {
            Less less;
            VerObj save = version;
            if (!arch(*this, name))
                return false;
            if (less(save,this))
                throw ArchiveUnsupportedVersionExceptionT<VerObj>(
                        THISLOCATION, save, version);
            return true;
        }
        
        
        ///Tests minimal version of version object
        /**Function is designed to test current version. Version
         * object should be synced with the archive and contain number
         * of archive version. Function minVer() returns true, if required 
         * version is below or equal to current version and return false
         * if requested version is above current version.
         * 
         * Function can be used in condition branch to skip variables not
         * serialized in older versions. Only when function returns true (archive
         * version is above or equal to required version), variables should
         * be serialized.
         * 
         * @param reqver required version
         * @retval true required version is equal or below the current version
         * @retval false required version is above current version
         * 
         * @note To simulate function @b maxVer, you can use inverted result
         * of function minVer. In format developer there is not often situation
         * where you test wether required version is above or equal to current
         * version, because if something were in older version and must
         * be skipped, you will in most cases specify version, when the
         * variables are no longer valid
         */
        bool minVer(const VerObj &reqver) {
            Less less;
            return !less(version,reqver);
        }
        

        ///Tests whether specified version is newer 
        /**
         * @param ver required version
         * @retval true required version is newer
         * @retval false required version is not newer
         */
        bool newer(const VerObj &ver) {
            Less less;
            return less(version, ver);
        }

        ///Tests whether specified version is older 
        /**
         * @param ver required version
         * @retval true required version is older
         * @retval false required version is not older
         */
        bool older(const VerObj &ver) {
            Less less;
            return less(ver, version);
        }                        
    private:
        VerObj version;
    };
    

	typedef Version<natural> VersionNr;
} // namespace LightSpeed

#endif /*VERSION_H_*/
