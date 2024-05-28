# CI

## ROOT binaries without RooFit

### ROOT 6.36.00 - Ubuntu 24.04

The resulting binary was uploaded here:
<https://rembserj.web.cern.ch/rembserj/data/binaries/root_v6.32.04.ubuntu24.04.no-roofit.tar.gz>

Reproduce them for example in the build-ready docker container:
```bash
docker run --security-opt label=disable -it registry.cern.ch/root-ci/ubuntu2404:buildready
```

In the container
```bash
ROOT_VERSION=6.36.00

mkdir workspace
cd workspace

wget "https://root.cern/download/root_v$ROOT_VERSION.source.tar.gz"
tar -xf root_v$ROOT_VERSION.source.tar.gz
mv root-$ROOT_VERSION root_src
mkdir root_build
cd root_build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -Dmathmore=ON \
      -Dfail-on-missing=ON -Dfitsio=OFF \
      -Dmysql=OFF -Dpgsql=OFF -Dsqlite=OFF \
      -Dtmva=OFF -Dtmva-pymva=OFF -Droofit=OFF \
      -Dvdt=OFF -Dtestsupport=ON \
      -DCMAKE_INSTALL_PREFIX=../root_install ../root_src

cmake --build . --target install -j24
cd ..

BINARY_NAME=root_v$ROOT_VERSION.ubuntu24.04.no-roofit.RelWithDebInfo

mv root_install root

tar -czvf $BINARY_NAME.tar.gz root
```

On the host (figure out container name with `docker ps`):
```bash
ROOT_VERSION=6.36.00
BINARY_NAME=root_v$ROOT_VERSION.ubuntu24.04.no-roofit.RelWithDebInfo
docker cp <NAME>:workspace/$BINARY_NAME.tar.gz .
```
