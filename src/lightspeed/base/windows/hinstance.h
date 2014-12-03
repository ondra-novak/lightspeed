#pragma once

#include "../framework/services.h"
#include "../framework/app.h"

namespace LightSpeed {

	class HInstanceService: public IInterface {
	public:
	

		HInstanceService(HINSTANCE hInstance, IServices &ref):hInstance(hInstance),ref(ref) {
			ref.addService(typeid(HInstanceService),this);
		}

		~HInstanceService() {
			ref.removeService(typeid(HInstanceService));
		}

		///Retrieves current hInstance value
		/** 
		*	@return current hInstance value
		*/
		const HINSTANCE &getHInstance() const { return hInstance; }

		///Sets new hInstance value
		/** 
		*	@param new hInstance value
		*/
		void setHInstance(const HINSTANCE &val) { hInstance = val; }

	protected:
		HINSTANCE hInstance;
		IServices &ref;
	};

	static inline HINSTANCE getCurAppHInstance() {
		AppBase &app = App::current();
		HInstanceService *inst = app.getIfcPtr<HInstanceService>();
		if (inst == 0) return 0;
		return inst->getHInstance();
	}

}