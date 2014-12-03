#ifndef _LIGHTSPEED_MEMORY_HELPERALLOCATORS
#define _LIGHTSPEED_MEMORY_HELPERALLOCATORS
#pragma once

/**@file
 * contains allocators used in smart pointers
 */

namespace LightSpeed {

	///"Fake" allocator, that can be used in AllocPointer - delete arrays allocated by new []
	class DeleteArrayAllocator {
	public:
		template<typename T>
		static void destroyInstance(T *obj) {
			delete [] obj;
		}
		template<typename T>
		T *createInstance(const T *other) {
			return 0;
		}
	};

	///"Fake" allocator, that can be used in AllocPointer - call destructor without memory free
	class InPlaceDeleteAllocator {
	public:
		template<typename T>
		static void destroyInstance(T *obj) {
			if (obj) obj->~T();
		}
		template<typename T>
		T *createInstance(const T *other) {
			return 0;
		}
	};
}
#endif
