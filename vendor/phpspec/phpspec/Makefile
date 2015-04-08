all:
	@echo "Only build-phar target is currently supported."

build-phar:
	@echo "--> Checking for composer command line tool"
	command -v composer >/dev/null && continue || { echo "composer command not found."; exit 1; }
	@echo "--> Cleaning vendor directory"
	rm -Rfv vendor
	@echo "--> Installing dependencies without dev"
	composer install --no-dev
	@echo "--> Building Phar"
	box build
	@echo "--> Success"
