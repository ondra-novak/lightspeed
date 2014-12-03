/*
 * GeneratorTest.cpp
 *
 *  Created on: 23.5.2013
 *      Author: ondra
 */

#include "GeneratorTest.h"
#include "../lightspeed/base/iter/generator.tcc"
#include <utility>
#include <stdlib.h>
#include "../lightspeed/base/streams/standardIO.h"


namespace LightSpeedTest {

using namespace LightSpeed;


void perms(GeneratorWriter<ConstStrA> &writer, char *list, int i, int n) {
	int j;
	    if (i==n){
	     ConstStrA x(list,n+1);
	     writer.write(x);
	    }
	    else {
	        for (j=i; j<=n; j++){
	            std::swap(list[i],list[j]);
	            perms(writer,list,i+1,n);
	            std::swap(list[i],list[j]);
	        }
	    }
}

void permutations(GeneratorWriter<ConstStrA> writer, ConstStrA text) {

	char *t = (char *)alloca(text.length());
	for (natural i = 0; i < text.length(); i++) {
		t[i] = text[i];
	}

	perms(writer,t,0,text.length()-1);

}

LightSpeed::integer GeneratorTest::start(const Args& ) {

	ConstStrA example("ondrej");
	typedef GeneratorIterator<ConstStrA> GenIter;
	typedef GenIter::GeneratorFunction GenFn;
	GenIter iter(GenFn::create(&permutations,example),200);

	ConsoleA console;

	while (iter.hasItems()) {
		ConstStrA txt = iter.getNext();
		console.print("%1\n") << txt;
	}

	return 0;
}
} /* namespace LightSpeedTest */
