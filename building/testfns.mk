.INTERMEDIATE: testfn testbuildin testheader

testfn:	
	@echo '#!/bin/bash'>$@
	@echo 'echo "void $$1(); int main() {$$1();return 0;}" > testfn.c' >>$@
	@echo 'if gcc -o testfn.out testfn.c 2> /dev/null; then echo "#define $$2 1"; else echo "#define $$2 0"; fi' >>$@
	@echo 'rm -f testfn.out testfn.c' >>$@
	@chmod +x $@

testbuildin:
	@echo '#!/bin/bash'>$@
	@echo 'echo "int main() {$$1;return 0;}"  > testfn.c' >>$@
	@echo 'if gcc -o testfn.out testfn.c 2> /dev/null; then echo "#define $$2 1"; else echo "#define $$2 0"; fi' >>$@
	@echo 'rm -f testfn.out testfn.c' >>$@
	@chmod +x $@

testheader:
	@echo '#!/bin/bash' >$@
	@echo 'echo -e "#include <$$1>\\n\\nint main() {return 0;}"  > testheader.c' >>$@
	@echo 'if gcc -o testheader.out testheader.c 2> /dev/null; then echo "#define $$2 1"; else echo "#define $$2 0"; fi' >>$@
	@echo 'rm -f testheader.out testheader.c' >>$@
	@chmod +x $@
	