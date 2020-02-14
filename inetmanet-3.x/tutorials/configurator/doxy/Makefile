#
# Definitions
#
DOXYGEN=doxygen
OUTDIR=../html

#
# Targets
#
all:
	rm -rf $(OUTDIR)
	mkdir $(OUTDIR)
	rm -f ../omnetpp.uncommented.ini
	cat ../omnetpp.ini | grep -v "#!" >> ../omnetpp.uncommented.ini
	$(DOXYGEN)
	rm -f ../omnetpp.uncommented.ini
	cp *.gif *.png *.svg $(OUTDIR)
clean:
	-rm -rf $(OUTDIR)
