#!/bin/bash


if [ -z "$1" ]
then
    echo 'Configuring localhost ...'
    for file in bash_aliases bash_logout bashrc gitconfig screenrc vimrc
    do
        echo "Processing $file ..."
        rm -f ~/.$file
        cp $file ~/.$file
    done

    echo "Processing ssh/config ..."
    mkdir -p ~/.ssh
    cp ssh_config ~/.ssh/config
else
    host=$1
    echo "Configuring $host ..."
    for file in bash_aliases bash_logout bashrc gitconfig screenrc vimrc
    do
        echo "Processing $file ..."
        ssh $host "rm -rf ~/.$file"
        scp $file $host:~/.$file
    done

    echo "Processing ssh/config ..."
    ssh $host "mkdir -p ~/.ssh"
    ssh $host "rm -rf ~/.ssh/config"
    scp ssh_config $host:~/.ssh/config
fi

echo 'Done'
