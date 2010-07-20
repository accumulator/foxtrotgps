## Process this file with automake to produce Makefile.in

SUBDIRS = po src pixmaps data

dist_doc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

git_index = $(wildcard .git/index)

ChangeLog: $(git_index)
	git log > $@.tmp && mv $@.tmp $@

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
