#!/bin/bash

service dbus start
bluetoothd &

./bonsai
