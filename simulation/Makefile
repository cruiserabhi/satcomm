export SIM_REPO := $(CURDIR)

ROOTFS ?= $(SIM_REPO)/rootfs
export ROOTFS
export ROOTFS_BN := $(shell basename ${ROOTFS})

all: apps

rootfs: grpc jsoncpp all

setup: cmake grpc jsoncpp

clean:
	rm -rf build/build_telux_headers build/build_sim build/build_apps

cleanall: clean
	rm -rf build simulation/protos/proto-src

headers:
	cd build && mkdir -p build_telux_headers && cd build_telux_headers && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../include && make install

jsoncpp:
	cd build && if [ ! -d jsoncpp ] ; then git clone https://git.codelinaro.org/clo/le/jsoncpp.git jsoncpp ; fi && mkdir -p build_jsoncpp && cd build_jsoncpp && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_INCLUDEDIR=${ROOTFS}/include/jsoncpp ../jsoncpp && cmake --build . -- -j $(shell nproc) && cmake --build . --target install
	mv ${ROOTFS}/lib/x86_64-linux-gnu/libjson* ${ROOTFS}/lib/ && find ${ROOTFS}/lib/x86_64-linux-gnu -type f -name 'libjson*' -delete

cmake:
	mkdir build && cd build && wget https://cmake.org/files/v3.15/cmake-3.15.3-Linux-x86_64.sh && chmod a+x ./cmake-3.15.3-Linux-x86_64.sh && ./cmake-3.15.3-Linux-x86_64.sh --prefix=${PWD}/build --skip-license && rm cmake-3.15.3-Linux-x86_64.sh

grpc:
	cd build && if [ ! -d grpc_repo ] ; then git clone --recurse-submodules -b v1.48.0 --depth 1 --shallow-submodules https://git.codelinaro.org/clo/le/grpc_repo.git ; fi && mkdir -p build_grpc && cd build_grpc && ${PWD}/build/bin/cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../grpc_repo && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

sim: headers
	cd build && export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${ROOTFS}/lib/ && mkdir -p build_sim && cd build_sim && cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH=${PWD}/build/lib/cmake/ -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../simulation && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

apps: sim
	cd build && mkdir -p build_apps && cd build_apps && cmake -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_ALL_SAMPLES=ON ../../apps && cmake --build . -- -j $(shell nproc) && cmake --build . --target install

docker-image: apps
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile.run . && echo "Docker image "telsdk-sim-image" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v telsdk_volume:/data --name telsdk_simulation telsdk-sim-image"

docker-development-image:
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image-develop -f ${SIM_REPO}/simulation/docker/ubuntu_$${UBUNTU_VERSION}/Dockerfile . && echo "Docker image "telsdk-sim-image-develop" is created. Use below command to drop to the shell" && echo "docker run -ti --device=/dev/snd/ --rm -h telsdk_simulation -v <source-dir>:<destination-dir> --name telsdk_simulation telsdk-sim-image-develop"
