
if [ -v $GCC49_AARCH64_PREFIX ]
then
    echo "GCC49_AARCH64_PREFIX is not set"
    echo "set using export GCC49_AARCH64_PREFIX=<lib_path>/bin/aarch64-linux-gnu-"
    return 0
fi

build -a AARCH64 -t GCC49 -p ShellPkg/ShellPkg.dsc -m AppPkg/Applications/sbsa-acs/uefi_app/SbsaAvs.inf

