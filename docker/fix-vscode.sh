#!/bin/bash

for i in $(docker ps --filter "volume=vscode" --format "{{.ID}}"); do
    docker stop "$i" || true 
done

for i in $(docker ps -a --filter "volume=vscode" --format "{{.ID}}"); do
    docker rm "$i" || true 
done
  
sudo systemctl restart docker

docker volume rm vscode || true