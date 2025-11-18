export SIM_REPO := $(CURDIR)

ROOTFS ?= $(SIM_REPO)/rootfs
export ROOTFS
export ROOTFS_BN := $(shell basename ${ROOTFS})

# Ensure the shared build directory exists before any targets that rely on it.
builddir:
	mkdir -p build

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
	cd build && wget https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.sh && chmod a+x ./cmake-3.15.3-Linux-x86_64.sh && ./cmake-3.15.3-Linux-x86_64.sh --prefix=${PWD}/build --skip-license && rm cmake-3.15.3-Linux-x86_64.sh

grpc: | builddir
	if ! command -v grpc_cpp_plugin >/dev/null 2>&1; then \
		echo "grpc_cpp_plugin is missing; please install gRPC C++ development tools"; \
		exit 1; \
	fi
	if ! pkg-config --exists protobuf; then \
		echo "protobuf development files are missing; please install libprotobuf-dev"; \
		exit 1; \
	fi
	mkdir -p ${ROOTFS}/lib
	find /usr/lib/x86_64-linux-gnu -maxdepth 1 -name 'libprotobuf*.so*' -o -name 'libgrpc*.so*' -exec cp {} ${ROOTFS}/lib/ \;

sim: headers grpc jsoncpp | builddir
	cd build && export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ROOTFS}/lib/ && mkdir -p build_sim && cd build_sim && cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../simulation && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

apps: sim | builddir
	cd build && mkdir -p build_apps && cd build_apps && cmake -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_ALL_SAMPLES=ON -DTELSDK_FEATURE_SATCOM=ON ../../apps && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

docker-image: apps
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile.run . && echo "Docker image "telsdk-sim-image" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v telsdk_volume:/data --name telsdk_simulation telsdk-sim-image"

docker-development-image:
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image-develop -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile . && echo "Docker image "telsdk-sim-image-develop" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v <source-dir>:<destination-dir> --name telsdk_simulation telsdk-sim-image-develop"
