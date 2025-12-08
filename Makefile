SHELL := /bin/bash

.PHONY: all client server shared tests test_client test_server test_shared format lint pre_pr clean fclean re rebuild

all:
	./scripts/build.sh

rebuild:
	cmake --build build

client:
	./scripts/build.sh client

server:
	./scripts/build.sh server

tests:
	./scripts/run_tests.sh

test_client:
	./scripts/run_tests.sh client

test_server:
	./scripts/run_tests.sh server

test_shared:
	./scripts/run_tests.sh shared

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
	rm -f rtype_client rtype_server rtype_client_tests rtype_server_tests rtype_shared_tests

re: fclean all
