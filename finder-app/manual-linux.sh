#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aesd-autograder
KERNEL_REPO=https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
KERNEL_VERSION=v6.12.11
BUSYBOX_VERSION=1_37_stable
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
NPROC=$(( $(nproc)-1 ))
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux/arch/${ARCH}/boot/Image ]; then
    cd ${OUTDIR}/linux
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC} defconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC} dtbs
fi

echo "Adding the Image in outdir"
cp -p ${OUTDIR}/linux/arch/${ARCH}/boot/Image \
    ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone --depth 1 --single-branch --branch ${BUSYBOX_VERSION} https://git.busybox.net/busybox/
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # git am ${FINDER_APP_DIR}/0001-Arch-dep-check-for-sha1-hw-acceleration.patch
else
    cd busybox
fi
# Make and install busybox
# make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC} defconfig
cp ${FINDER_APP_DIR}/busybox_config ${OUTDIR}/busybox/.config
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j ${NPROC} install

cd ${OUTDIR}/rootfs

# Add library dependencies to rootfs
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter" | grep -oE "/.*[^]]" | sed 's/\/lib\///g' \
    | while read line; do cp ${FINDER_APP_DIR}/lib/$line ${OUTDIR}/rootfs/lib; done

${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library" | grep -oE "\[.*\]" | sed 's/[][]//g' \
    | while read line; do cp ${FINDER_APP_DIR}/lib/$line ${OUTDIR}/rootfs/lib64; done

# Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp writer ${OUTDIR}/rootfs/home

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp finder.sh ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
cp autorun-qemu.sh ${OUTDIR}/rootfs/home

mkdir -p ${OUTDIR}/rootfs/home/conf
cp ../conf/username.txt ${OUTDIR}/rootfs/home/conf
cp ../conf/assignment.txt ${OUTDIR}/rootfs/home/conf

cd ${OUTDIR}/rootfs
cat home/finder-test.sh | sed 's/..\/conf/conf/g' > home/tmp.sh
mv home/tmp.sh home/finder-test.sh
chmod +x ${OUTDIR}/rootfs/home/finder-test.sh

# Chown the root directory
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner=root:root > ${OUTDIR}/initramfs.cpio

# Create initramfs.cpio.gz
cd ${OUTDIR}
gzip -f initramfs.cpio
