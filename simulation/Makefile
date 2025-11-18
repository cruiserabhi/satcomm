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

define download_with_fallback
	@set -e; \
	urls="$(1)"; \
	out="$(2)"; \
	mkdir -p $$(dirname $$out); \
	for url in $$urls; do \
	        echo "Fetching $$url -> $$out"; \
	        if ${DOWNLOAD_ENV} wget -O $$out $$url; then exit 0; fi; \
	        if [ -n "${DOWNLOAD_PROXY}" ]; then echo "Proxy download failed for $$url; retrying without proxy"; fi; \
	        if env -u https_proxy -u http_proxy wget -O $$out $$url; then exit 0; fi; \
	done; \
	echo "ERROR: unable to download $$urls"; \
	exit 1
endef

GRPC_VERSION ?= 1.52.1
GRPC_ARCHIVE := ${THIRD_PARTY}/grpc-v${GRPC_VERSION}.tar.gz
GRPC_ARCHIVE_URL ?= https://github.com/grpc/grpc/archive/refs/tags/v${GRPC_VERSION}.tar.gz
GRPC_ARCHIVE_FALLBACK_URL ?= https://storage.googleapis.com/grpc-bazel-mirror/github.com/grpc/grpc/archive/refs/tags/v${GRPC_VERSION}.tar.gz
GRPC_SRC_DIR := ${THIRD_PARTY}/grpc-${GRPC_VERSION}
GRPC_BUILD_DIR := ${SIM_REPO}/build/build_grpc
CMAKE_PREFIX_LIST := ${ROOTFS}/lib/cmake;/usr/lib/x86_64-linux-gnu/cmake

JSONCPP_VERSION ?= 1.9.5
JSONCPP_ARCHIVE := ${THIRD_PARTY}/jsoncpp-${JSONCPP_VERSION}.tar.gz
JSONCPP_ARCHIVE_URL ?= https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/${JSONCPP_VERSION}.tar.gz
JSONCPP_ARCHIVE_FALLBACK_URL ?= https://downloads.sourceforge.net/project/jsoncpp/jsoncpp/${JSONCPP_VERSION}/jsoncpp-${JSONCPP_VERSION}.tar.gz
JSONCPP_SRC_DIR := ${THIRD_PARTY}/jsoncpp-${JSONCPP_VERSION}
JSONCPP_BUILD_DIR := ${SIM_REPO}/build/build_jsoncpp

CMAKE_INSTALLER := ${SIM_REPO}/build/cmake-3.15.3-Linux-x86_64.sh
CMAKE_INSTALLER_URL ?= https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.sh
CMAKE_INSTALLER_FALLBACK_URL ?= https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

# Ensure the shared build directory exists before any targets that rely on it.
builddir:
	mkdir -p build

${GRPC_ARCHIVE}:
	$(call download_with_fallback,${GRPC_ARCHIVE_URL} ${GRPC_ARCHIVE_FALLBACK_URL},${GRPC_ARCHIVE})

${GRPC_SRC_DIR}/CMakeLists.txt: ${GRPC_ARCHIVE}
	mkdir -p ${THIRD_PARTY}
	tar -xzf ${GRPC_ARCHIVE} -C ${THIRD_PARTY}

all: apps

rootfs: grpc jsoncpp all

setup: cmake grpc jsoncpp

clean:
	rm -rf build/build_telux_headers build/build_sim build/build_apps build/build_grpc build/build_jsoncpp

cleanall: clean
	rm -rf build simulation/protos/proto-src

headers: | builddir
	cd build && mkdir -p build_telux_headers && cd build_telux_headers && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../include && make install

jsoncpp: | builddir
	mkdir -p ${ROOTFS}/include/jsoncpp ${ROOTFS}/lib
	need_build=0; \
	if [ -d /usr/include/jsoncpp ] || [ -d /usr/include/json ]; then \
	        echo "Copying system jsoncpp headers into rootfs"; \
	        if [ -d /usr/include/jsoncpp ]; then cp -r /usr/include/jsoncpp/. ${ROOTFS}/include/jsoncpp/; fi; \
	        if [ -d /usr/include/json ]; then cp -r /usr/include/json/. ${ROOTFS}/include/jsoncpp/; fi; \
	else \
	        need_build=1; \
	fi; \
	if ls /usr/lib/x86_64-linux-gnu/libjsoncpp.so* >/dev/null 2>&1; then \
	        cp /usr/lib/x86_64-linux-gnu/libjsoncpp.so* ${ROOTFS}/lib/; \
	else \
	        need_build=1; \
	fi; \
	if [ $$need_build -eq 1 ] || [ ! -f ${ROOTFS}/lib/libjsoncpp.so ]; then \
	        $(MAKE) jsoncpp-build; \
	fi

${JSONCPP_ARCHIVE}:
	$(call download_with_fallback,${JSONCPP_ARCHIVE_URL} ${JSONCPP_ARCHIVE_FALLBACK_URL},${JSONCPP_ARCHIVE})

${JSONCPP_SRC_DIR}/CMakeLists.txt: ${JSONCPP_ARCHIVE}
	mkdir -p ${THIRD_PARTY}
	tar -xzf ${JSONCPP_ARCHIVE} -C ${THIRD_PARTY}

jsoncpp-build: ${JSONCPP_SRC_DIR}/CMakeLists.txt cmake | builddir
	mkdir -p ${JSONCPP_BUILD_DIR}
	cd ${JSONCPP_BUILD_DIR} && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON -DJSONCPP_WITH_TESTS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF -DCMAKE_INSTALL_PREFIX=${ROOTFS} ${JSONCPP_SRC_DIR}
	cd ${JSONCPP_BUILD_DIR} && cmake --build . --target install -- -j $(shell nproc)

cmake: | builddir
	$(call download_with_fallback,${CMAKE_INSTALLER_URL} ${CMAKE_INSTALLER_FALLBACK_URL},${CMAKE_INSTALLER})
	chmod a+x ${CMAKE_INSTALLER}
	cd build && ${CMAKE_INSTALLER} --prefix=${PWD}/build --skip-license
	rm -f ${CMAKE_INSTALLER}

grpc: | builddir
	mkdir -p ${ROOTFS}/bin ${ROOTFS}/include ${ROOTFS}/lib ${ROOTFS}/lib/pkgconfig
	if command -v grpc_cpp_plugin >/dev/null 2>&1 && pkg-config --exists protobuf grpc grpc++; then \
	        echo "Using system gRPC/protobuf artifacts"; \
	        cp $$(command -v grpc_cpp_plugin) ${ROOTFS}/bin/; \
	        if command -v protoc >/dev/null 2>&1; then cp $$(command -v protoc) ${ROOTFS}/bin/; fi; \
	        if pkg-config --variable=includedir protobuf >/dev/null 2>&1; then cp -r $$(pkg-config --variable=includedir protobuf)/google ${ROOTFS}/include/; fi; \
	        if pkg-config --variable=includedir grpc >/dev/null 2>&1; then cp -r $$(pkg-config --variable=includedir grpc)/grpc ${ROOTFS}/include/; fi; \
	        if pkg-config --variable=includedir grpc++ >/dev/null 2>&1; then cp -r $$(pkg-config --variable=includedir grpc++)/grpcpp ${ROOTFS}/include/; fi; \
	        if pkg-config --variable=pcfiledir protobuf >/dev/null 2>&1; then cp $$(pkg-config --variable=pcfiledir protobuf)/protobuf*.pc ${ROOTFS}/lib/pkgconfig/; fi; \
	        if pkg-config --variable=pcfiledir grpc >/dev/null 2>&1; then cp $$(pkg-config --variable=pcfiledir grpc)/grpc*.pc ${ROOTFS}/lib/pkgconfig/; fi; \
	        if pkg-config --variable=pcfiledir grpc++ >/dev/null 2>&1; then cp $$(pkg-config --variable=pcfiledir grpc++)/grpc++*.pc ${ROOTFS}/lib/pkgconfig/; fi; \
	        find $$(pkg-config --variable=libdir protobuf) -maxdepth 1 -name 'libprotobuf*.so*' -exec cp {} ${ROOTFS}/lib/ \; || true; \
	        find $$(pkg-config --variable=libdir grpc) -maxdepth 1 -name 'libgrpc*.so*' -exec cp {} ${ROOTFS}/lib/ \; || true; \
	        find $$(pkg-config --variable=libdir grpc++) -maxdepth 1 -name 'libgrpc++*.so*' -exec cp {} ${ROOTFS}/lib/ \; || true; \
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
