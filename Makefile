TAG := infinitime-build
# TODO: toolchain and sdk versions

BUILD_OUTPUT  ?= build/output
PUBLISH_FILES ?= $(BUILD_OUTPUT)/*app-dfu*.zip

TIMESTAMP := $(shell date +%H.%M.%S)

default: build

.PHONY: build-image
build-image:
	docker build -t $(TAG) docker/

.PHONY: build
build:
	@if [ -z "$$(docker image ls -q $(TAG))" ]; then \
		echo "Docker image for building not found, run 'make build-image' first"; \
	fi;
	docker run -it --rm -v $(PWD):/sources $(TAG)

.PHONY: clean
clean:
	docker rmi -f $(TAG)

ifneq ($(INFINITIME_PUBLISH_DIR),)
publish: build $(INFINITIME_PUBLISH_DIR)

$(INFINITIME_PUBLISH_DIR): $(PUBLISH_FILES)
	cp -f $? "$(INFINITIME_PUBLISH_DIR)/$(TIMESTAMP)-dfu.zip"
endif
