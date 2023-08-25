#!/bin/bash

# Path to your aesdsocket binary
AESDSOCKET_BIN="./aesdsocket"

start() {
    echo "Starting aesdsocket daemon..."
    start-stop-daemon --start --background --make-pidfile --pidfile /var/run/aesdsocket.pid \
        --exec $AESDSOCKET_BIN -- -d
}

stop() {
    echo "Stopping aesdsocket daemon..."
    start-stop-daemon --stop --pidfile /var/run/aesdsocket.pid --retry TERM/5/KILL/5
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit 0

