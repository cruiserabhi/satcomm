export SIM_REPO := $(CURDIR)
# Root of the satcomm repository (one level above simulation/).
REPO_TOP := $(abspath ${SIM_REPO}/..)

ROOTFS ?= $(SIM_REPO)/rootfs
export ROOTFS
export ROOTFS_BN := $(shell basename ${ROOTFS})
export PATH := ${SIM_REPO}/build/build/bin:$(PATH)

THIRD_PARTY := ${SIM_REPO}/third_party
DOWNLOAD_PROXY ?=
# Optional directory containing pre-downloaded dependency archives to avoid network fetches.
DOWNLOAD_CACHE ?= ${REPO_TOP}
DOWNLOAD_ENV := env
ifneq (${DOWNLOAD_PROXY},)
DOWNLOAD_ENV := env https_proxy=${DOWNLOAD_PROXY} http_proxy=${DOWNLOAD_PROXY}
endif

define download_with_fallback
	set -e; \
	urls="$(1)"; \
	out="$(2)"; \
	mkdir -p $$(dirname $$out); \
	if [ -n "${DOWNLOAD_CACHE}" ]; then \
		for url in $$urls; do \
			candidate=${DOWNLOAD_CACHE}/$$(basename $$url); \
			if [ -f $$candidate ]; then \
				echo "Using cached archive $$candidate -> $$out"; \
				cp $$candidate $$out; \
				exit 0; \
			fi; \
		done; \
	fi; \
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
LOCAL_GRPC_ARCHIVE ?= ${REPO_TOP}/grpc-${GRPC_VERSION}.tar.gz
LOCAL_GRPC_MASTER_ZIP ?= ${REPO_TOP}/grpc-master.zip
GRPC_SRC_DIR := ${THIRD_PARTY}/grpc-${GRPC_VERSION}
GRPC_BUILD_DIR := ${SIM_REPO}/build/build_grpc
CMAKE_PREFIX_LIST := ${ROOTFS}/lib/cmake;/usr/lib/x86_64-linux-gnu/cmake

JSONCPP_VERSION ?= 1.9.5
JSONCPP_ARCHIVE := ${THIRD_PARTY}/jsoncpp-${JSONCPP_VERSION}.tar.gz
JSONCPP_ARCHIVE_URL ?= https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/${JSONCPP_VERSION}.tar.gz
JSONCPP_ARCHIVE_FALLBACK_URL ?= https://downloads.sourceforge.net/project/jsoncpp/jsoncpp/${JSONCPP_VERSION}/jsoncpp-${JSONCPP_VERSION}.tar.gz
JSONCPP_SRC_DIR := ${THIRD_PARTY}/jsoncpp-${JSONCPP_VERSION}
JSONCPP_BUILD_DIR := ${SIM_REPO}/build/build_jsoncpp

CARES_ARCHIVE_NAME := c-ares-6654436a307a5a686b008c1d4c93b0085da6e6d8.tar.gz
LOCAL_CARES_MAIN_ZIP ?= ${REPO_TOP}/c-ares-main.zip
CARES_CACHE_ARCHIVE := ${DOWNLOAD_CACHE}/${CARES_ARCHIVE_NAME}
CARES_LOCAL_PREREQ :=

ifneq ($(wildcard ${LOCAL_CARES_MAIN_ZIP}),)
CARES_LOCAL_PREREQ := ${CARES_CACHE_ARCHIVE}
${CARES_CACHE_ARCHIVE}: ${LOCAL_CARES_MAIN_ZIP}
	@mkdir -p $(dir ${CARES_CACHE_ARCHIVE})
	@echo "Repacking ${LOCAL_CARES_MAIN_ZIP} into ${CARES_CACHE_ARCHIVE}"
	@tmpdir=$$(mktemp -d); \
		unzip -q ${LOCAL_CARES_MAIN_ZIP} -d $$tmpdir; \
		srcdir=$$(find $$tmpdir -maxdepth 1 -type d -name 'c-ares-*' | head -n1); \
		if [ -z "$$srcdir" ]; then echo "ERROR: Unable to locate c-ares sources in ${LOCAL_CARES_MAIN_ZIP}"; rm -rf $$tmpdir; exit 1; fi; \
		tar -czf ${CARES_CACHE_ARCHIVE} -C $$tmpdir $$(basename $$srcdir); \
		rm -rf $$tmpdir
endif

CMAKE_INSTALLER := ${SIM_REPO}/build/cmake-3.15.3-Linux-x86_64.sh
CMAKE_INSTALLER_URL ?= https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.sh
CMAKE_INSTALLER_FALLBACK_URL ?= https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.sh

# Ensure the shared build directory exists before any targets that rely on it.
builddir:
	mkdir -p build

${GRPC_ARCHIVE}:
	@mkdir -p $$(dirname ${GRPC_ARCHIVE})
	@if [ -f ${LOCAL_GRPC_ARCHIVE} ]; then \
		echo "Using local gRPC archive ${LOCAL_GRPC_ARCHIVE} -> ${GRPC_ARCHIVE}"; \
		cp ${LOCAL_GRPC_ARCHIVE} ${GRPC_ARCHIVE}; \
	elif [ -f ${LOCAL_GRPC_MASTER_ZIP} ]; then \
		echo "Repacking ${LOCAL_GRPC_MASTER_ZIP} into ${GRPC_ARCHIVE}"; \
		tmpdir=$$(mktemp -d); \
		unzip -q ${LOCAL_GRPC_MASTER_ZIP} -d $$tmpdir; \
		srcdir=$$(find $$tmpdir -maxdepth 1 -type d -name "grpc-*" | head -n1); \
		if [ -z "$$srcdir" ]; then echo "ERROR: Unable to locate grpc sources in ${LOCAL_GRPC_MASTER_ZIP}"; rm -rf $$tmpdir; exit 1; fi; \
		tar -czf ${GRPC_ARCHIVE} -C $$tmpdir $$(basename $$srcdir); \
		rm -rf $$tmpdir; \
	else \
		$(call download_with_fallback,${GRPC_ARCHIVE_URL} ${GRPC_ARCHIVE_FALLBACK_URL},${GRPC_ARCHIVE}); \
	fi

${GRPC_SRC_DIR}/CMakeLists.txt: ${GRPC_ARCHIVE}
	mkdir -p ${THIRD_PARTY}
	tar -xzf ${GRPC_ARCHIVE} -C ${THIRD_PARTY}

# Populate gRPC's bundled third_party sources so CMake does not attempt
# to download them.  We prefer locally provided archives placed in the
# repository root (DOWNLOAD_CACHE) before falling back to network URLs
# that match the hashes referenced in grpc/bazel/grpc_deps.bzl.
define ensure_grpc_dep
	set -e; \
	target_dir="$(1)"; \
	archive_name="$(2)"; \
	strip_components="$(3)"; \
	urls="$(4)"; \
	if [ -d $$target_dir ] && find $$target_dir -mindepth 1 -print -quit | grep -q .; then \
		echo "Using existing $$target_dir"; \
	else \
		archive_path=${THIRD_PARTY}/$$archive_name; \
		$(call download_with_fallback,$$urls,$$archive_path); \
		mkdir -p $$target_dir; \
		tar -xzf $$archive_path -C $$target_dir --strip-components=$$strip_components; \
	fi
endef

grpc-deps: ${GRPC_SRC_DIR}/CMakeLists.txt ${CARES_LOCAL_PREREQ}
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/abseil-cpp,abseil-cpp-20220623.0.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/abseil/abseil-cpp/archive/20220623.0.tar.gz https://github.com/abseil/abseil-cpp/archive/20220623.0.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/cares/cares,${CARES_ARCHIVE_NAME},1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/c-ares/c-ares/archive/6654436a307a5a686b008c1d4c93b0085da6e6d8.tar.gz https://github.com/c-ares/c-ares/archive/6654436a307a5a686b008c1d4c93b0085da6e6d8.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/re2,re2-2022-04-01.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/google/re2/archive/2022-04-01.tar.gz https://github.com/google/re2/archive/2022-04-01.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/zlib,zlib-04f42ceca40f73e2978b50e93806c2a18c1281fc.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/madler/zlib/archive/04f42ceca40f73e2978b50e93806c2a18c1281fc.tar.gz https://github.com/madler/zlib/archive/04f42ceca40f73e2978b50e93806c2a18c1281fc.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/boringssl-with-bazel,boringssl-6195bf8242156c9a2fa75702eee058f91b86a88b.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/google/boringssl/archive/6195bf8242156c9a2fa75702eee058f91b86a88b.tar.gz https://github.com/google/boringssl/archive/6195bf8242156c9a2fa75702eee058f91b86a88b.tar.gz)
	@# Prefer the locally supplied protobuf bundle when present.
	@if [ -d ${GRPC_SRC_DIR}/third_party/protobuf ] && find ${GRPC_SRC_DIR}/third_party/protobuf -mindepth 1 -print -quit | grep -q .; then \
		echo "Using existing ${GRPC_SRC_DIR}/third_party/protobuf"; \
	elif [ -f ${REPO_TOP}/protobuf-33.1.tar.gz ]; then \
		echo "Using local protobuf archive ${REPO_TOP}/protobuf-33.1.tar.gz"; \
		mkdir -p ${GRPC_SRC_DIR}/third_party/protobuf; \
		tar -xzf ${REPO_TOP}/protobuf-33.1.tar.gz -C ${GRPC_SRC_DIR}/third_party/protobuf --strip-components=1; \
	else \
		$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/protobuf,protobuf-24487dd1045c7f3d64a21f38a3f0c06cc4cf2edb.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/protocolbuffers/protobuf/archive/24487dd1045c7f3d64a21f38a3f0c06cc4cf2edb.tar.gz https://github.com/protocolbuffers/protobuf/archive/24487dd1045c7f3d64a21f38a3f0c06cc4cf2edb.tar.gz); \
	fi
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/googleapis,googleapis-2f9af297c84c55c8b871ba4495e01ade42476c92.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/googleapis/googleapis/archive/2f9af297c84c55c8b871ba4495e01ade42476c92.tar.gz https://github.com/googleapis/googleapis/archive/2f9af297c84c55c8b871ba4495e01ade42476c92.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/envoy-api,data-plane-api-bf6154e482bbd5e6f64032993206e66b6116f2bd.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/envoyproxy/data-plane-api/archive/bf6154e482bbd5e6f64032993206e66b6116f2bd.tar.gz https://github.com/envoyproxy/data-plane-api/archive/bf6154e482bbd5e6f64032993206e66b6116f2bd.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/opencensus-proto,opencensus-proto-0.3.0.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/census-instrumentation/opencensus-proto/archive/v0.3.0.tar.gz https://github.com/census-instrumentation/opencensus-proto/archive/v0.3.0.tar.gz)
	$(call ensure_grpc_dep,${GRPC_SRC_DIR}/third_party/xds,xds-06c439db220b89134a8a49bad41994560d6537c6.tar.gz,1,https://storage.googleapis.com/grpc-bazel-mirror/github.com/cncf/xds/archive/06c439db220b89134a8a49bad41994560d6537c6.tar.gz https://github.com/cncf/xds/archive/06c439db220b89134a8a49bad41994560d6537c6.tar.gz)

all: apps

rootfs: grpc jsoncpp all

setup: cmake grpc jsoncpp

clean:
	rm -rf build/build_telux_headers build/build_sim build/build_apps build/build_grpc build/build_jsoncpp

cleanall: clean
	rm -rf build simulation/protos/proto-src

headers: | builddir
	cd build && mkdir -p build_telux_headers && cd build_telux_headers && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} ${REPO_TOP}/include && make install

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
	@if command -v cmake >/dev/null 2>&1; then \
		echo "Using system CMake ($$(cmake --version | head -n1))"; \
	else \
		$(call download_with_fallback,${CMAKE_INSTALLER_URL} ${CMAKE_INSTALLER_FALLBACK_URL},${CMAKE_INSTALLER}); \
		chmod a+x ${CMAKE_INSTALLER}; \
		cd build && ${CMAKE_INSTALLER} --prefix=${PWD}/build --skip-license; \
		rm -f ${CMAKE_INSTALLER}; \
	fi

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


grpc-build: ${GRPC_SRC_DIR}/CMakeLists.txt grpc-deps cmake | builddir
	mkdir -p ${GRPC_BUILD_DIR}
	cd ${GRPC_BUILD_DIR} && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=module -DgRPC_CARES_PROVIDER=module -DgRPC_PROTOBUF_PROVIDER=module -DgRPC_RE2_PROVIDER=module -DgRPC_SSL_PROVIDER=module -DgRPC_ZLIB_PROVIDER=module ${GRPC_SRC_DIR}
	cd ${GRPC_BUILD_DIR} && cmake --build . --target grpc_cpp_plugin -- -j $(shell nproc)
	cd ${GRPC_BUILD_DIR} && cmake --build . --target install -- -j $(shell nproc)

sim: headers grpc jsoncpp | builddir
	cd build && export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ROOTFS}/lib/ && export PKG_CONFIG_PATH=${ROOTFS}/lib/pkgconfig:${PKG_CONFIG_PATH} && mkdir -p build_sim && cd build_sim && cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_LIST}' -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../simulation && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

apps: sim | builddir
        cd build && export PKG_CONFIG_PATH=${ROOTFS}/lib/pkgconfig:${PKG_CONFIG_PATH} && mkdir -p build_apps && cd build_apps && cmake -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_LIST}' -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_ALL_SAMPLES=ON -DTELSDK_FEATURE_SATCOM=ON ../../apps && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

harman-satcom-service: sim | builddir
        cd build && export PKG_CONFIG_PATH=${ROOTFS}/lib/pkgconfig:${PKG_CONFIG_PATH} && mkdir -p build_apps && cd build_apps && cmake -DCMAKE_PREFIX_PATH='${CMAKE_PREFIX_LIST}' -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_ALL_SAMPLES=ON -DTELSDK_FEATURE_SATCOM=ON ../../apps && cmake --build . --target harman_satcom_service AudioTest -- -j $(shell nproc) && cmake --build . --target install

docker-image: apps
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile.run . && echo "Docker image "telsdk-sim-image" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v telsdk_volume:/data --name telsdk_simulation telsdk-sim-image"

docker-development-image:
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image-develop -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile . && echo "Docker image "telsdk-sim-image-develop" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v <source-dir>:<destination-dir> --name telsdk_simulation telsdk-sim-image-develop"
