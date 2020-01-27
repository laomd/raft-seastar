# !/bin/bash

set -e

dnf install -y openssh-server passwd cracklib-dicts
adduser dev
passwd dev << EOF
dev12345678
dev12345678
EOF
ssh-keygen -A
su dev
ssh-keygen << EOF



EOF