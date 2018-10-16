#!/bin/bash

if [ -z "$DATA_FILE_PATH" ]
then
  DATA_FILE_PATH=$DEPLOY_DATA_FILE
fi
if [ -z "$DEPLOY_IP" ]
then
  DEPLOY_IP=$DEPLOY_APPLICATION_IP
fi
if [ -z "$DEPLOY_PORT" ]
then
  DEPLOY_PORT=$DEPLOY_APPLICATION_PORT
fi
if [ -z "$TCP_CLIENT_DATA_TYPE" ]
then
  TCP_CLIENT_DATA_TYPE="bin"
fi
if [ -z "$TCP_CLIENT_NSOCKETS" ]
then
  TCP_CLIENT_NSOCKETS=2
fi
if [ -z "$TCP_CLIENT_BLOCK_SIZE" ]
then
  TCP_CLIENT_BLOCK_SIZE=4096
fi

CMD="./client_app -s -n $DEPLOY_IP -p $DEPLOY_PORT -f $DATA_FILE_PATH --type $TCP_CLIENT_DATA_TYPE --n_sockets $TCP_CLIENT_NSOCKETS -b $TCP_CLIENT_BLOCK_SIZE -t $ADDITIONAL_PARAMETERS"

echo "Run $CMD > client_app.out"
$CMD > client_app.out

if [ "$TCP_CLIENT_PRINT_OUTPUT" = "true" ]
then
  cat client_app.out
fi

echo "client application completed"
 