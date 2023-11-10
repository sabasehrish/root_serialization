source /cvmfs/cms.cern.ch/el8_amd64_gcc12/external/cmake/3.25.2-286587a5b04682914f36aa1a7526e333/etc/profile.d/init.sh
pushd $CMSSW_BASE
eval $(scram tool info lz4|grep LZ4_BASE)
eval $(scram tool info xz|grep XZ_BASE)
eval $(scram tool info zstd|grep ZSTD_BASE)
eval $(scram tool info tbb|grep TBB_BASE)
popd
rm -rf build
mkdir build
pushd build
cmake ../ \
  -DCMAKE_PREFIX_PATH="${LZ4_BASE};${XZ_BASE}" \
  -Dzstd_DIR=${ZSTD_BASE}/lib/cmake/zstd \
  -DTBB_DIR=${TBB_BASE}/lib/cmake/TBB \
  -DROOT_DIR=$ROOTSYS/cmake \
  -DENABLE_HDF5=OFF \
  -DCMAKE_BUILD_TYPE=Release
make -j4
popd
