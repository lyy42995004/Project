bin = httpserver
cgi = test_cgi
cc = g++
LD_FLAGS = -std=c++11 -lpthread
src = main.cpp
cgi_src = cgi/test_cgi.cpp
cgi_dest = wwwroot/test_cgi

ALL:$(bin) $(cgi_dest)
.PHONY:ALL
$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)
$(cgi_dest):$(cgi_src)
	$(cc) -o $@ $^ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -f $(bin)
	rm -f $(cgi_dest)

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output