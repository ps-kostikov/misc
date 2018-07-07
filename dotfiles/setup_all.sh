#!/bin/bash


if [ -z "$1" ]
then
    echo 'Configuring localhost ...'
    for file in bash_aliases bashrc gitconfig screenrc vimrc
    do
        echo "Processing $file ..."
        rm -f ~/.$file
        cp $file ~/.$file
    done
else
    host=$1
    echo "Configuring $host ..."
    for file in bash_aliases bashrc gitconfig screenrc vimrc
    do
        echo "Processing $file ..."
        ssh $host "rm -rf ~/.$file"
        scp $file $host:~/.$file
    done

fi

echo 'Done'
