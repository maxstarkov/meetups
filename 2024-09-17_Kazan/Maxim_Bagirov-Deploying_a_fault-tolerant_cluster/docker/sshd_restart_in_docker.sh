#!/bin/bash

docker exec node1 systemctl restart sshd
docker exec node2 systemctl restart sshd
docker exec node3 systemctl restart sshd
