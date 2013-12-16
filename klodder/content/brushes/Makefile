BRUSHCOMPILER = ../../brushcompiler/brushcompiler

all :
	$(BRUSHCOMPILER) -c brushes.txt -o brushes_hq.lib -s 1
	$(BRUSHCOMPILER) -c brushes.txt -o brushes_lq.lib -s 4
ifeq ($(OS), win)
	cp brushes_hq.lib ../../brushes_hq.lib
	cp brushes_hq.lib ../../brushes_lq.lib
else
	cp brushes_lq.lib ../../Resources/
endif