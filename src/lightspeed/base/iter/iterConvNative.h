/*
 * iterConvNative.h
 *
 *  Created on: 16. 7. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ITER_ITERCONVNATIVE_H_
#define LIGHTSPEED_BASE_ITER_ITERCONVNATIVE_H_


namespace LightSpeed {


///Converts items in the stream/iterator using standard constructor or cast operator
template<typename In, typename Out>
class NativeConvert:  public ConverterBase<In, Out, NativeConvert<In, Out> > {
public:

	const Out &getNext() {
		needItems = true;
		hasItems = false;
		return out;
	}
	const Out &peek() const {
		return out;
	}
	void write(const In &item) {
		out = Constructor1<Out, In>(item);
		needItems = false;
		hasItems = true;
	}

protected:
	Optional<Out> out;

};

}


#endif /* LIGHTSPEED_BASE_ITER_ITERCONVNATIVE_H_ */
