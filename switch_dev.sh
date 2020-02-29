#!/bin/bash

echo current remote:
git remote -v

git remote remove origin
git remote add origin git@github.com:0xcafed00d/tac08_dev.git

echo new remote:

git remote -v

read -p "About to push to new remote: Are you sure? " -n 1 -r
echo   
if [[ $REPLY =~ ^[Yy]$ ]]
then
	git push --set-upstream origin master
fi

