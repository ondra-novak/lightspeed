/*
 * textFormatManip.h
 *
 *  Created on: 2.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTFORMATMANIP_H_
#define LIGHTSPEED_TEXT_TEXTFORMATMANIP_H_

namespace LightSpeed {



	class TextFormatManip {
	public:

	enum Manip{
		setBase,
		setPrecision,
		setSci,
	};

	Manip m;
	natural param;

	TextFormatManip(Manip manip, natural param):m(manip),param(param) {}

	template<typename T>
	void doManip(T &stream) const;
	};

	inline TextFormatManip setBase(natural base) {return TextFormatManip(TextFormatManip::setBase,base);}
	inline TextFormatManip setPrecision(natural p) {return TextFormatManip(TextFormatManip::setPrecision,p);}
	inline TextFormatManip setSci(natural decimals) {return TextFormatManip(TextFormatManip::setSci,decimals);}
	inline TextFormatManip setSciMax(natural p) {return TextFormatManip(TextFormatManip::setSci,p);}


	template<typename T>
	void TextFormatManip::doManip(T &tt) const {
		switch (m) {
		case TextFormatManip::setBase: tt.setBase(param);break;
		case TextFormatManip::setPrecision: tt.setPrecision(param);break;
		case TextFormatManip::setSci: tt.setSci(param);break;
		}
	}



}

#endif /* LIGHTSPEED_TEXT_TEXTFORMATMANIP_H_ */
