#!/bin/sh

export PATH=/bin

/bin/busybox --install -s /bin

mount -t proc none /proc
mount -t devtmpfs none /dev

echo "Coucou les amis !"

sh
