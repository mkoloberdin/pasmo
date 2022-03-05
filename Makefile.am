# Makefile.am for Pasmo

bin_PROGRAMS = pasmo

sources = \
	asm.h asm.cxx \
	asmerror.h asmerror.cxx \
	asmfile.h asmfile.cxx \
	cpc.h cpc.cxx \
	macro.h macro.cxx \
	nullstream.h nullstream.cxx \
	pasmotypes.h pasmotypes.cxx \
	spectrum.h spectrum.cxx \
	tap.h tap.cxx \
	token.h token.cxx \
	tzx.h tzx.cxx

pasmo_SOURCES = pasmo.cxx $(sources)

#---------------------------------------------------------------

check_PROGRAMS = test_token test_asm

test_token_SOURCES = test_protocol.cxx test_protocol.h \
	test_token.cxx \
	$(sources)

test_asm_SOURCES = test_protocol.cxx test_protocol.h \
	test_asm.cxx \
	$(sources)

LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
		$(top_srcdir)/tap-driver.sh

TESTS = test_token test_asm test_cli.sh

#---------------------------------------------------------------

tgz: dist
	cp $(distdir).tar.gz $(distdir).tgz

rpm: dist
	rpm -ta $(distdir).tar.gz

#---------------------------------------------------------------

EXAMPLE_ASM = \
	align.asm \
	all.asm \
	allb.asm \
	alocal.asm \
	bad.asm \
	black.asm \
	callvers.asm \
	defb.asm \
	echovers.asm \
	fill8k.asm \
	hellocpc.asm \
	hellocpm.asm \
	hellospec.asm \
	hilo.asm \
	hola.asm \
	if.asm \
	include.asm \
	indent.asm \
	jumptable.asm \
	lee.asm \
	limit64.asm \
	local.asm \
	macro.asm \
	mas.asm \
	protector.asm \
	reserv.asm \
	showfcb.asm \
	showkeys.asm \
	showline.asm \
	t86.asm \
	test.asm \
	tmacro.asm \
	undoc.asm

TEST_ASM = \
	autolocal_test.asm \
	incbin_test.asm \
	macro_unclosed_test.asm \
	defl_test.asm \
	include_bad_test.asm \
	include_notfound_test.asm \
	else_unclosed_test.asm \
	testaux/included_in_test.asm \
	proc_test.asm proc_unclosed_test.asm \
	rept_test.asm rept_unclosed_test.asm rept_exitm_unclosed_test.asm \
	irp_unclosed_test.asm irp_exitm_unclosed_test.asm \
	if_test.asm \
	include_test.asm \
	public_test.asm \
	if_unclosed_test.asm \
	macro_endp_test.asm \
	test.asm

#---------------------------------------------------------------

EXTRA_DIST = \
	bootstrap \
	do_conf \
	do_conf_cover \
	gen_cover \
	test_cli.sh \
	all.check \
	pasmodoc.html \
	$(TEST_ASM) \
	$(EXAMPLE_ASM)

#---------------------------------------------------------------

code-coverage-clean:
	find . -name "*.gcda" -delete
	find . -name "*.gcno" -delete
	rm -rf cov_html
	rm -f app.info

test-aux-files-clean:
	rm -f asmtested.bin asmtested.sym \
	black.tap black.tzx black.cdt black.p3d black.ams

clean-local: code-coverage-clean test-aux-files-clean

#---------------------------------------------------------------

ACLOCAL_AMFLAGS = -I .

# End of Makefile.am for Pasmo
