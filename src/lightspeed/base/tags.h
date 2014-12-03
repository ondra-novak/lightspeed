#include "compare.h"

#pragma  once
namespace LightSpeed {


	///Declaration of new tagdef
	/**
	 * Tag is identifier, which is unique identified in whole program and
	 * and can have associated text description. Tags should be declared
	 * as global variables, in special cases they can be declared inside
	 * of function while they are unique until function is finished.
	 *
	 * Class tagdef is used to declare new tagdef. It receives its unique
	 * identifier and associated text description. Description should be
	 * also declared globally, or programmer must ensure, that pointer to
	 * description is valid during validity of declaration of the tagdef.
	 *
	 * To work with tags, see class Tag
	 */
	 
	class tagdef {
	public:
		///contains name of the enum
		const char *const name;

		///constructor
		tagdef(const char *name):name(name) {}

		tagdef():name("<not set>") {}

	};


	///Variable that can keep tagdef as value
	/**
	 * It works as ordinary variable, which will accept tags 
	 * 
	 * Working with tags directly is not practical. It is better to use Tag object
	 * which allows to store tagdef identifier, compare two tagdef identifiers and retrieve 
	 * text description and unique identifier of the tagdef
	 */
	 
	class Tag {
	public:

		///Constructs variable
		Tag(const tagdef &e):e(&e) {}
		///Constructs empty variable
		Tag():e(0) {}

		///Constructs tagdef using its UID
		explicit Tag(natural uid):e((const tagdef *)(uid * sizeof(tagdef))) {}


		///Retrieves tagdef's internal UID
		/** Internal UID is set by compiler/linker 
		 * you cannot declare tagdef with specified UID.
		 */
		natural uid() const {return (natural)e / sizeof(tagdef);}

		///retrieves name (text description) of the tagdef
		/**
		 * @return name (text description). If variable is NIL, returns NULL
		 */		 
		const char *name() const {return e?e->name:0;}

		bool operator < (const Tag &other) const {return e < other.e;}
		bool operator > (const Tag &other) const {return e > other.e;}
		bool operator <= (const Tag &other) const {return e <= other.e;}
		bool operator >= (const Tag &other) const {return e >= other.e;}
		bool operator == (const Tag &other) const {return e == other.e;}
		bool operator != (const Tag &other) const {return e != other.e;}

		bool operator < (const tagdef &other) const {return e < &other;}
		bool operator > (const tagdef &other) const {return e > &other;}
		bool operator <= (const tagdef &other) const {return e <= &other;}
		bool operator >= (const tagdef &other) const {return e >= &other;}
		bool operator == (const tagdef &other) const {return e == &other;}
		bool operator != (const tagdef &other) const {return e != &other;}

		bool operator == (NullType ) const {return e == 0;}
		bool operator != (NullType ) const {return e != 0;}

	protected:
		const tagdef *e;

		friend class ComparableLess<Tag>;
	};
	


#define LIGHTSPEED_TAGDEF(X) ::LightSpeed::tagdef X(#X);


}
