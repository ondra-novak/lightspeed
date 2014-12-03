/** @file
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
 * 
 * $Id: assert.h 2190 2011-12-05 13:31:13Z ondrej.novak $
 *
 * DESCRIPTION
 * Short description
 * 
 * AUTHOR
 * Ondrej Novak <ondrej.novak@firma.seznam.cz>
 *
 */


#ifndef LIGHTSPEED_BASE_COMPILETIME_ASSERT_H_
#define LIGHTSPEED_BASE_COMPILETIME_ASSERT_H_

namespace LightSpeed {


	template<bool value> class Assert;
	template<> class Assert<true> {};


}



#endif /* LIGHTSPEED_BASE_COMPILETIME_ASSERT_H_ */
