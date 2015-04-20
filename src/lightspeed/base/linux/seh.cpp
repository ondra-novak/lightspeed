#include "seh.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <exception>
#include <execinfo.h>

namespace LightSpeed {

static __thread __jmp_buf_tag *globjmpbuf = 0;
static __thread SignalFunction globsigfn = 0;
void thread_sigsegv_handler(int sig, siginfo_t *, void *);

void LinuxSEH::signalHandler(int sig, siginfo_t *i, void *p) {
	if (globjmpbuf) {

		if (globsigfn) {
			if (globsigfn(sig,i,p)) {
				siglongjmp(globjmpbuf ,sig);
			}
		} else {
			siglongjmp(globjmpbuf ,sig);
		}
	}


/*	char buff[256];
	sprintf(buff,"(SEH) Unhandled signal %d\n",sig);
	int e = write(2,buff,strlen(buff));
	(void)e;
	*/
	signal(sig,SIG_DFL);
}


LinuxSEH::LinuxSEH():swp(false) {
	prevjmpbuf = globjmpbuf;
	globjmpbuf  = jmpbuf;
	prevsignfn = globsigfn;
	globsigfn = 0;
}

LinuxSEH::LinuxSEH(SignalFunction fn):swp(false) {
	prevjmpbuf = globjmpbuf;
	globjmpbuf  = jmpbuf;
	prevsignfn = globsigfn;
	globsigfn = fn;
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
	sig.sa_sigaction = &LinuxSEH::signalHandler;
	sig.sa_flags = SA_SIGINFO;
	sig.sa_restorer = 0;
	sigemptyset(&sig.sa_mask);
	return sig;
}
const struct sigaction LinuxSEH::standardSigAction = initSigAction();

void LinuxSEH::init() {
	initSignal(SIGSEGV);
	initSignal(SIGBUS);
	initSignal(SIGILL);
	initSignal(SIGABRT);
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


void LinuxSEH::output_stack_trace() {

	  void *array[30];
	  size_t size;
	  char **strings;
	  size_t i;

	  size = backtrace (array, 30);
	  strings = backtrace_symbols (array, size);

	  fprintf (stderr,"Obtained %zd stack frames.\n", size);

	  for (i = 0; i < size; i++)
	     fprintf (stderr,"%s\n", strings[i]);

	  free (strings);
}


}
