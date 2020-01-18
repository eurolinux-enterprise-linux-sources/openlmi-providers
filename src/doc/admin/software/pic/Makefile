FIGS=$(wildcard *.eps *.svg *.dia)
TARGETS=$(foreach f,$(filter %.dia,$(FIGS)),$(basename $(f)).svg)

all: $(TARGETS)

%.pdf: %.eps
	epstopdf $^ --outfile=$@

%.eps: %.dia
	dia -t eps -e $@ $?

%.svg: %.dia
	dia -t svg -e $@ $?

%.pdf: %.svg
	inkscape --export-pdf=$@ -f $?
