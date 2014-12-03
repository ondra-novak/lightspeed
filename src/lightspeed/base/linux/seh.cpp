#include "seh.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <exception>

namespace LightSpeed {

static __thread __jmp_buf_tag *globjmpbuf = 0;

void LinuxSEH::signalHandler(int sig) {
	if (globjmpbuf) {

		siglongjmp(globjmpbuf ,sig);
	}
	char buff[256];
	sprintf(buff,"(SEH) Unhandled signal %d\n",sig);
	int e = write(2,buff,strlen(buff));
	(void)e;
	signal(sig,SIG_DFL);
}


LinuxSEH::LinuxSEH():swp(false) {
	prevjmpbuf = globjmpbuf;
	globjmpbuf  = jmpbuf;
}

LinuxSEH::~LinuxSEH() {
	if (globjmpbuf == jmpbuf) globjmpbuf = prevjmpbuf;
}

int LinuxSEH::except() {
	if (globjmpbuf == jmpbuf) globjmpbuf = prevjmpbuf;
	return signum;
}

static struct sigaction initSigAction() {
	struct sigaction sig;
	sig.sa_handler = &LinuxSEH::signalHandler;
	sig.sa_flags = 0;
	sig.sa_restorer = 0;
	sigemptyset(&sig.sa_mask);
	return sig;
}
const struct sigaction LinuxSEH::standardSigAction = initSigAction();

void LinuxSEH::init() {
	initSignal(SIGSEGV);
	initSignal(SIGBUS);
	initSignal(SIGILL);
}

void LinuxSEH::initSignal(int signum) {
	sigaction(signum,&standardSigAction,0);

}

void LinuxSEH::throwf(int sig) {
	if (globjmpbuf) {
		siglongjmp(globjmpbuf ,sig);
	}
	char buff[256];
	sprintf(buff,"(SEH) Unhandled user exception: %d\n",sig);
	int e = write(2,buff,strlen(buff));
	(void)e;
	std::terminate();
}


}
