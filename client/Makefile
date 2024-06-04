run: clean
	mkdir build
	cd build && cmake ..
	cd build && make
coverage: test
	cd build && make coverage
	cd build && open coverage.html
clean:
	rm -rf build