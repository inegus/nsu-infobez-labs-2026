#!/bin/bash

IMAGE_NAME="os-command-injection-demo"
CONTAINER_NAME="demo-server"
PORT=5000

docker build -t $IMAGE_NAME .

if [ $? -ne 0 ]; then
    echo "error building image"
    exit 1
fi

docker stop $CONTAINER_NAME 2>/dev/null
docker rm $CONTAINER_NAME 2>/dev/null

docker run -d -p $PORT:$PORT --name $CONTAINER_NAME $IMAGE_NAME

if [ $? -eq 0 ]; then
    echo "image started"
else
    echo "image not started"
    exit 1
fi