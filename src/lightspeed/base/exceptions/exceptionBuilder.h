/*
 * exceptionBuilder.h
 *
 *  Created on: 17.3.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTIONBUILDER_H_
#define LIGHTSPEED_EXCEPTIONBUILDER_H_



#define LIGHTSPD_DEF_EXCEPTION(name,base) class name: public base {\
		public: \
			typedef base Super; \
			LIGHTSPEED_EXCEPTIONFINAL; \



#define LIGHTSPD_EXCEPTION_PAR_0(name) \
			name (const ::LightSpeed::ProgramLocation &loc) \
				:Super(loc) {} \
		protected: \
			void message(ExceptionMsg &msg) const {

#define LIGHTSPD_END_EXCEPTION() }


#define LIGHTSPD_EXCEPTION_PAR_1(name,T1,P1) \
			name (const ::LightSpeed::ProgramLocation &loc, const T1 &par##P1) \
				:Super(loc),val##P1(par##P1) {} \
			const T1 &get##P1() const {return val##P1;} \
		protected: \
			T1 val##P1; \

#define LIGHTSPD_EXCEPTION_PAR_2(name,T1,P1,T2,P2) \
			name (const ::LightSpeed::ProgramLocation &loc, const T1 &par##P1, const T2 &par##P2) \
				:Super(loc),val##P1(par##P1),val##P2(par##P2) {} \
			const T1 &get##P1() const {return val##P1;} \
			const T2 &get##P2() const {return val##P2;} \
		protected: \
			T1 val##P1; \
			T2 val##P2;

#define LIGHTSPD_EXCEPTION_PAR_3(name,T1,P1,T2,P2,T3,P3) \
			name (const ::LightSpeed::ProgramLocation &loc, const T1 &par##P1, const T2 &par##P2, const T3 &par##P3) \
				:Super(loc),val##P1(par##P1),val##P2(par##P2),val##P3(par##P3) {} \
			const T1 &get##P1() const {return val##P1;} \
			const T2 &get##P2() const {return val##P2;} \
			const T3 &get##P3() const {return val##P3;} \
		protected: \
			T1 val##P1; \
			T2 val##P2; \
			T3 val##P3;

#define LIGHTSPD_EXCEPTION_PAR_4(name,T1,P1,T2,P2,T3,P3,T4,P4) \
			name (const ::LightSpeed::ProgramLocation &loc, const T1 &par##P1, const T2 &par##P2, const T3 &par##P3, const T4 &par##P4) \
				:Super(loc),val##P1(par##P1),val##P2(par##P2),val##P3(par##P3),val##P4(par##P4) {} \
			const T1 &get##P1() const {return val##P1;} \
			const T2 &get##P2() const {return val##P2;} \
			const T3 &get##P3() const {return val##P3;} \
			const T4 &get##P4() const {return val##P4;} \
		protected: \
			T1 val##P1; \
			T2 val##P2; \
			T3 val##P3; \
			T3 val##P4;

#define LIGHTSPD_EXCEPTION_PAR_5(name,T1,P1,T2,P2,T3,P3,T4,P4,T5,P5) \
			name (const ::LightSpeed::ProgramLocation &loc, const T1 &par##P1, const T2 &par##P2, const T3 &par##P3, const T4 &par##P4, const T5 &par##P5) \
				:Super(loc),val##P1(par##P1),val##P2(par##P2),val##P3(par##P3),val##P4(par##P4),val##P5(par##P5) {} \
			const T1 &get##P1() const {return val##P1;} \
			const T2 &get##P2() const {return val##P2;} \
			const T3 &get##P3() const {return val##P3;} \
			const T4 &get##P4() const {return val##P4;} \
			const T4 &get##P5() const {return val##P5;} \
		protected: \
			T1 val##P1; \
			T2 val##P2; \
			T3 val##P3; \
			T3 val##P4;

#define LIGHTSPD_EXCEPTION_MSG(object) virtual void message(::LightSpeed::ExceptionMsg &msg) const

#endif /* EXCEPTIONBUILDER_H_ */
