if [ $(uname -m) != "aarch64" ] && [ -v $GCC49_AARCH64_PREFIX ]
then
    echo "GCC49_AARCH64_PREFIX is not set"
    echo "set using export GCC49_AARCH64_PREFIX=<lib_path>/bin/aarch64-linux-gnu-"
    return 0
fi

NISTStatus=1;

function build_with_NIST()
{
    if [ ! -f "sts-2_1_2.zip" ]; then
        wget https://csrc.nist.gov/CSRC/media/Projects/Random-Bit-Generation/documents/sts-2_1_2.zip
        status=$?
        if [ $status -ne 0 ]; then
            echo "wget failed for NIST. Building sbsa without NIST"
            return $status
        fi
    fi

    if [ ! -d "ShellPkg/Application/sbsa-acs/test_pool/nist_sts/sts-2.1.2/" ]; then
        /usr/bin/unzip sts-2_1_2.zip -d ShellPkg/Application/sbsa-acs/test_pool/nist_sts/.
	status=$?
        if [ $status -ne 0 ]; then
            echo "unzip failed for NIST. Building sbsa without NIST"
            return $status
        fi
    fi

    cd ShellPkg/Application/sbsa-acs/test_pool/nist_sts/sts-2.1.2/
    if ! patch -R -p1 -s -f --dry-run < ../../../patches/nist_sbsa_sts.diff; then
        patch -p1 < ../../../patches/nist_sbsa_sts.diff
        status=$?
	if [ $status -ne 0 ]; then
            echo "patch failed for NIST. Building sbsa without NIST"
            return $status
        fi
    fi
    cd -

    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvsNist.inf -D ENABLE_NIST -D TARGET_EMULATION
    status=$?
    if [ $status -ne 0 ]; then
        echo "Build failed for NIST. Building sbsa without NIST"
        return $status
    fi

    return $status
}


if [ "$1" == "NIST" ]; then
    build_with_NIST
    NISTStatus=$?
fi

if [ "$1" == "ENABLE_OOB" ]; then
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sbsa-acs/baremetal_app/SbsaAvs.inf -D TARGET_EMULATION -D ENABLE_OOB
    return 0;
fi

if [ $NISTStatus -ne 0 ]; then
    build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m ShellPkg/Application/sbsa-acs/uefi_app/SbsaAvs.inf -D TARGET_EMULATION
fi

