if [ $# -ne 2 ]
then
    echo "Only two arguments should be passed"
    exit 1
elif [ ! -d $1 ]
then
    echo "First arg is not a directory"
    exit 1
fi

findstr=$2
fcnt=0
scnt=0

dfs () {
    local dir=$1
    for i in `ls $dir`
    do
        if [ -d $i ]
        then
            dfs $i
        else
            fcnt=$(($fcnt+1))
            scnt=$(($scnt+`grep -c $findstr "$dir/$i"`))
        fi
    done
}

dfs $1
echo "The number of files are $fcnt and the number of matching lines are $scnt"