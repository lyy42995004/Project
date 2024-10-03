bin = httpserver
cgi = test_cgi
cc = g++
LD_FLAGS = -std=c++11 -lpthread
src = main.cpp
cgi_src = cgi/test_cgi.cpp

ALL:$(bin) $(cgi)
.PHONY:ALL
$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)
$(cgi):$(cgi_src)
	$(cc) -o $@ $^ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -f $(bin)
	rm -f $(cgi)

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp -rf $(cgi) output/wwwroot