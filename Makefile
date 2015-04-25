.PHONY: build
build:
	docker build -t nyan .

.PHONY: nyan
nyan: build
	docker run -it nyan
