run: clean
	mkdir build
	cd build && cmake ..
	cd build && make
	cd build && ./server
coverage: test
	cd build && make coverage
	cd build && open coverage.html
clean:
	rm -rf build