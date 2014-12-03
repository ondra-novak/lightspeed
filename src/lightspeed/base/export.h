


#ifdef LIGHTSPEED_IMPORTDLL
#define LIGHTSPEED_EXPORT __declspec(dllimport)
//#pragma warning (disable : 4275) //non-DLL bases are okay in LightSpeed
//#pragma warning (disable : 4251) //non-DLL identifiers are okay in LightSpeed
#endif

#ifndef LIGHTSPEED_EXPORT

///This attribute is prepared to MSVC compiler while LightSpeed is compiled as DLL
/**
 * To create DLL, create project generating a DLL file, link with LightSpeed library
 * and specify functions and variables to export in DEF file. Linker helps with
 * names because it will report unresolved externals. Collect these errors, extracts
 * names and create DEF file.
 *
 * (It is not recommended to export all functions, because generated DLL is very large.
 * It is also recommended to assign every function a unique number and disable exporting
 * names. DLL will be significantly smaller).
 *
 * To use such a DLL under MSVC project, define LIGHTSPEED_EXPORT as __declspec(dllimport).
 * It will optimize calling functions and allows to access exported static variables.
 *
 * This macro is empty to work with other compilers.
 *
 */
#define LIGHTSPEED_EXPORT
#endif


#ifndef EXTERN_TEMPLATE 
#define EXTERN_TEMPLATE extern template
#endif

#define extern_template EXTERN_TEMPLATE

#ifdef _MSC_EXTENSIONS
#pragma warning (disable : 4231) //disable extern template warning
#endif

