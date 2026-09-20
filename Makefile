
all:
	git pull 
	cmake -S . -B build
	cmake --build build -j$(nproc)


clean:
	rm -rfv build/

