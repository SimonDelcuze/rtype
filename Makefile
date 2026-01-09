SHELL := /bin/bash

.PHONY: all client server shared tests test_client test_server test_shared format lint pre_pr clean fclean re rebuild editor

NPROC := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

all:
	./scripts/build.sh
	cmake -S . -B build -DBUILD_CLIENT=ON -DBUILD_EDITOR=OFF
	cmake --build build --target rtype_server rtype_client -j $(NPROC)

rebuild:
	cmake --build build --target rtype_server rtype_client -j $(NPROC)

client:
	cmake -S . -B build -DBUILD_CLIENT=ON -DBUILD_EDITOR=OFF
	cmake --build build --target rtype_client -j $(NPROC)

server:
	cmake -S . -B build -DBUILD_CLIENT=OFF -DBUILD_EDITOR=OFF
	cmake --build build --target rtype_server -j $(NPROC)

editor:
	cmake -S . -B build -DBUILD_CLIENT=OFF -DBUILD_EDITOR=ON
	cmake --build build --target rtype_level_editor -j $(NPROC)

tests:
	cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_CLIENT=ON -DBUILD_EDITOR=OFF
	cmake --build build --target tests -j $(NPROC)


test_client:
	cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_CLIENT=ON -DBUILD_EDITOR=OFF
	cmake --build build --target rtype_client_tests -j $(NPROC)
	./build/tests/rtype_client_tests

test_server:
	cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_CLIENT=OFF -DBUILD_EDITOR=OFF
	cmake --build build --target rtype_server_tests -j $(nproc)
	./build/tests/rtype_server_tests

test_shared:
	cmake -S . -B build -DBUILD_TESTS=ON
	cmake --build build --target rtype_shared_tests -j $(nproc)
	./build/tests/rtype_shared_tests

format:
	./scripts/format.sh

format_client:
	./scripts/format.sh client shared

format_server:
	./scripts/format.sh server shared

format_tests:
	./scripts/format.sh tests

lint:
	./scripts/lint.sh

pre_push:
	./scripts/pre_push.sh

clean:
	rm -rf build

fclean: clean
	rm -f r-type_client r-type_server rtype_client_tests rtype_server_tests rtype_shared_tests r-type_level_editor

re: fclean all

coverage:
	./scripts/coverage.sh
