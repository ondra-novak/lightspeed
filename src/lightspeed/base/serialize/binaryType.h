#pragma once

namespace LightSpeed {

	///Designed for serializers
	/**
	 * Can be serialized as other types. Parser/formatter should serialize
	 this class as unspecified binary data, i.e. sequence of bytes with undefined
	 meaning. Note that binary type can cause that result will not be portable to
	 another systems. Useful to transfer data with standarized format, for example
	 images, sounds, files, etc.

	 Type doesn't store size of data. You have to handle this by own. Binary serializers
	 don't need to remember size at all. This can cause loose of synchronization.
	 
	 */
	 
	class BinaryType {
	public:

		///Constructs serializable region
		/**
		 * @param ptr pointer to block
		 * @param size size of block
		 *
		 */
		 
		BinaryType(void *ptr, natural size)
			:ptr(ptr),size(size) {}
		void *getAddress() {return ptr;}
		const void *getAddress() const {return ptr;}
		natural getSize() const {return size;}


	protected:
		void *ptr;
		natural size;
	};


}