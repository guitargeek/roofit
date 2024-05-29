# CI

## ROOT binaries without RooFit

### ROOT 6.32.00 - Ubuntu 24.04

The resulting binary was uploaded here:
<https://rembserj.web.cern.ch/rembserj/data/binaries/root_v6.32.00.ubuntu24.04.no-roofit.tar.gz>

Reproduce them for example in the build-ready docker container:
```bash
docker run --security-opt label=disable -it registry.cern.ch/root-ci/ubuntu24:buildready
```

In the container
```bash
mkdir workspace
cd workspace

wget "https://root.cern/download/root_v6.32.00.source.tar.gz"
tar -xf root_v6.32.00.source.tar.gz
mv root-6.32.00 root_src
mkdir root_build
cd root_build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_STANDARD=20 -Dmathmore=ON \
      -Dfail-on-missing=ON -Dfitsio=OFF \
      -Dmysql=OFF -Dpgsql=OFF -Dsqlite=OFF \
      -Dtmva=OFF -Dtmva-pymva=OFF -Droofit=OFF \
      -Dvdt=OFF -Dtesting=ON \
      -DCMAKE_INSTALL_PREFIX=../root_install ../root_src

cmake --build . --target install -j24
cd ..

BINARY_NAME=root_v6.32.00.ubuntu24.04.no-roofit.RelWithDebInfo

mv root_install root

tar -czvf $BINARY_NAME.tar.gz root
```

On the host (figure out container name with `docker PS`):
```bash
BINARY_NAME=root_v6.32.00.ubuntu24.04.no-roofit.RelWithDebInfo
docker cp <NAME>:workspace/$BINARY_NAME.tar.gz .
```

### ROOT 6.32.00 - Ubuntu 22.04

The resulting binary was uploaded here:
<https://rembserj.web.cern.ch/rembserj/data/binaries/root_v6.32.00.ubuntu22.04.no-roofit.tar.gz>

Reproduce them for example in the build-ready docker container:
```bash
docker run --security-opt label=disable -it registry.cern.ch/root-ci/ubuntu22:buildready
```

In the container
```bash
mkdir workspace
cd workspace

wget "https://root.cern/download/root_v6.32.00.source.tar.gz"
tar -xf root_v6.32.00.source.tar.gz
mv root-6.32.00 root_src
mkdir root_build
cd root_build

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_STANDARD=20 -Dmathmore=ON \
      -Dfail-on-missing=ON -Dfitsio=OFF \
      -Dmysql=OFF -Dpgsql=OFF -Dsqlite=OFF \
      -Dtmva=OFF -Dtmva-pymva=OFF -Droofit=OFF \
      -Dvdt=OFF -Dtesting=ON \
      -DCMAKE_INSTALL_PREFIX=../root_install ../root_src

cmake --build . --target install -j24
cd ..

BINARY_NAME=root_v6.32.00.ubuntu22.04.no-roofit.RelWithDebInfo

mv root_install root

tar -czvf $BINARY_NAME.tar.gz root
```

On the host (figure out container name with `docker PS`):
```bash
BINARY_NAME=root_v6.32.00.ubuntu22.04.no-roofit.RelWithDebInfo
docker cp <NAME>:workspace/$BINARY_NAME.tar.gz .
```
