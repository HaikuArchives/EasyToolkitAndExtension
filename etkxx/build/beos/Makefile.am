AUTOMAKE_OPTIONS = 1.7

SUBDIRS = example

EXTRA_DIST = etkxx-x86.proj etkxx-x86-ppc.proj graphics-x11-x86.proj
CLEANFILES = libetkxx.so libetkxx.so.xMAP libgraphics-x11.so
DISTCLEANFILES = Makefile.in

clean-local:
	-rm -rf \(Objects.etkxx-x86\)
	-rm -rf \(Objects.graphics-x11-x86\)
	-rm -rf \(Objects.etkxx-x86-ppc\)
	-rm -f *.d
