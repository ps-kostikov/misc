#!/bin/bash

host=$1
username=pkostikov

echo $host

scp ~/.dupload.conf $username@$host:/home/$username
scp ~/$username.gpg $username@$host:/home/$username
scp -r ~/.gnupg $username@$host:/home/$username

ssh $username@$host "rm -rf /home/$username/misc"
tar cf - -C ~/Work/ misc/ | ssh $username@$host tar xf -

# todo: add sublime configs

for file in bash_aliases bashrc git-completion gitconfig screenrc vimrc
do ssh $username@$host "rm -rf /home/$username/.$file && ln -s /home/$username/misc/dotfiles/$file /home/$username/.$file"
done

ssh $username@$host "rm -f /home/$username/tools && ln -s /home/$username/misc/tools /home/$username/tools"
