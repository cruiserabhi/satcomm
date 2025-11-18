export SIM_REPO := $(CURDIR)

ROOTFS ?= $(SIM_REPO)/rootfs
export ROOTFS
export ROOTFS_BN := $(shell basename ${ROOTFS})
export PATH := ${SIM_REPO}/build/build/bin:$(PATH)

THIRD_PARTY := ${SIM_REPO}/third_party
DOWNLOAD_PROXY ?=
DOWNLOAD_ENV := env
ifneq (${DOWNLOAD_PROXY},)
DOWNLOAD_ENV := env https_proxy=${DOWNLOAD_PROXY} http_proxy=${DOWNLOAD_PROXY}
endif

GRPC_VERSION ?= 1.52.1
GRPC_ARCHIVE := ${THIRD_PARTY}/grpc-v${GRPC_VERSION}.tar.gz
GRPC_ARCHIVE_URL ?= https://github.com/grpc/grpc/archive/refs/tags/v${GRPC_VERSION}.tar.gz
GRPC_SRC_DIR := ${THIRD_PARTY}/grpc-${GRPC_VERSION}
GRPC_BUILD_DIR := ${SIM_REPO}/build/build_grpc
CMAKE_PREFIX_LIST := ${ROOTFS}/lib/cmake;/usr/lib/x86_64-linux-gnu/cmake

# Ensure the shared build directory exists before any targets that rely on it.
builddir:
	mkdir -p build

${GRPC_ARCHIVE}:
	mkdir -p ${THIRD_PARTY}
	${DOWNLOAD_ENV} wget -O ${GRPC_ARCHIVE} ${GRPC_ARCHIVE_URL}

${GRPC_SRC_DIR}/CMakeLists.txt: ${GRPC_ARCHIVE}
	mkdir -p ${THIRD_PARTY}
	tar -xzf ${GRPC_ARCHIVE} -C ${THIRD_PARTY}

all: apps

rootfs: grpc jsoncpp all

setup: cmake grpc jsoncpp

clean:
	rm -rf build/build_telux_headers build/build_sim build/build_apps

cleanall: clean
	rm -rf build simulation/protos/proto-src

headers: | builddir
	cd build && mkdir -p build_telux_headers && cd build_telux_headers && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../include && make install

jsoncpp: | builddir
	if [ ! -d /usr/include/jsoncpp ] && [ ! -d /usr/include/json ]; then \
		echo "jsoncpp development headers are missing; please install libjsoncpp-dev"; \
		exit 1; \
	fi
	if ls /usr/lib/x86_64-linux-gnu/libjsoncpp.so* >/dev/null 2>&1; then \
		mkdir -p ${ROOTFS}/include/jsoncpp ${ROOTFS}/lib; \
		if [ -d /usr/include/jsoncpp ]; then cp -r /usr/include/jsoncpp/. ${ROOTFS}/include/jsoncpp/; fi; \
		if [ -d /usr/include/json ]; then cp -r /usr/include/json/. ${ROOTFS}/include/jsoncpp/; fi; \
		cp /usr/lib/x86_64-linux-gnu/libjsoncpp.so* ${ROOTFS}/lib/; \
	else \
		echo "jsoncpp libraries are missing; please install libjsoncpp-dev"; \
		exit 1; \
	fi

cmake: | builddir
	cd build && ${DOWNLOAD_ENV} wget https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.sh && chmod a+x ./cmake-3.15.3-Linux-x86_64.sh && ./cmake-3.15.3-Linux-x86_64.sh --prefix=${PWD}/build --skip-license && rm cmake-3.15.3-Linux-x86_64.sh

grpc: | builddir
	mkdir -p ${ROOTFS}/bin ${ROOTFS}/include ${ROOTFS}/lib
	if command -v grpc_cpp_plugin >/dev/null 2>&1 && pkg-config --exists protobuf grpc grpc++; then \
		echo "Using system gRPC/protobuf artifacts"; \
		cp $$(command -v grpc_cpp_plugin) ${ROOTFS}/bin/; \
		if command -v protoc >/dev/null 2>&1; then cp $$(command -v protoc) ${ROOTFS}/bin/; fi; \
		if pkg-config --variable=includedir protobuf >/dev/null 2>&1; then cp -r $$(pkg-config --variable=includedir protobuf)/google ${ROOTFS}/include/; fi; \
		if pkg-config --variable=includedir grpc >/dev/null 2>&1; then cp -r $$(pkg-config --variable=includedir grpc)/grpc ${ROOTFS}/include/; fi; \
		if pkg-config --variable=includedir grpc++ >/dev/null 2>&1; then cp -r $$(pkg-config --variable=includedir grpc++)/grpcpp ${ROOTFS}/include/; fi; \
		find $$(pkg-config --variable=libdir protobuf) -maxdepth 1 -name 'libprotobuf*.so*' -exec cp {} ${ROOTFS}/lib/ \; && \
		find $$(pkg-config --variable=libdir grpc) -maxdepth 1 -name 'libgrpc*.so*' -exec cp {} ${ROOTFS}/lib/ \; && \
		find $$(pkg-config --variable=libdir grpc++) -maxdepth 1 -name 'libgrpc++*.so*' -exec cp {} ${ROOTFS}/lib/ \; ; \
	else \
		$(MAKE) grpc-build; \
	fi


grpc-build: ${GRPC_SRC_DIR}/CMakeLists.txt cmake | builddir
	mkdir -p ${GRPC_BUILD_DIR}
	cd ${GRPC_BUILD_DIR} && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=module -DgRPC_CARES_PROVIDER=module -DgRPC_PROTOBUF_PROVIDER=module -DgRPC_RE2_PROVIDER=module -DgRPC_SSL_PROVIDER=module -DgRPC_ZLIB_PROVIDER=module ${GRPC_SRC_DIR}
	cd ${GRPC_BUILD_DIR} && cmake --build . --target grpc_cpp_plugin -- -j $(shell nproc)
	cd ${GRPC_BUILD_DIR} && cmake --build . --target install -- -j $(shell nproc)

sim: headers grpc jsoncpp | builddir
	cd build && export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ROOTFS}/lib/ && export PKG_CONFIG_PATH=${ROOTFS}/lib/pkgconfig:${PKG_CONFIG_PATH} && mkdir -p build_sim && cd build_sim && cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_LIST}' -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../simulation && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

apps: sim | builddir
	cd build && export PKG_CONFIG_PATH=${ROOTFS}/lib/pkgconfig:${PKG_CONFIG_PATH} && mkdir -p build_apps && cd build_apps && cmake -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_LIST}' -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_ALL_SAMPLES=ON -DTELSDK_FEATURE_SATCOM=ON ../../apps && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

docker-image: apps
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile.run . && echo "Docker image "telsdk-sim-image" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v telsdk_volume:/data --name telsdk_simulation telsdk-sim-image"

docker-development-image:
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image-develop -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile . && echo "Docker image "telsdk-sim-image-develop" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v <source-dir>:<destination-dir> --name telsdk_simulation telsdk-sim-image-develop"
