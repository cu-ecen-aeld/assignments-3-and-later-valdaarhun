if [ $# -ne 2 ]
then
    echo "Only two arguments should be passed"
    exit 1
fi

writefile=$1
writestr=$2

{
    dirpath=`dirname $writefile`
    if [ ! -d $dirpath ]
    then
        mkdir -p $dirpath
    fi
    echo $writestr > $writefile
} 2>/dev/null || echo "Failed to create $writefile"
