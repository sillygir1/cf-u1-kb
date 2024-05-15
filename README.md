# Panasonic Toughbook CF-U1 keyboard service.

## Info

This service is used to map side keys of Panasonic CF-U1 to various actions and to remap keyboard to make it easier to type. Only tested on Debian 12 with LXDE.

## Compiling

Run `compile.sh`

You can also use `run.sh` for testing (needs to be run with sudo)

## Installing a service

Create a symlink to `panasonic-kb.service`

```sh
sudo ln -s /path/to/panasonic-kb.service /etc/systemd/system/panasonic-kb.service
```

or put `panasonic-kb.service` to `/etc/systemd/system/`

```sh
sudo cp panasonic-kb.service /etc/systemd/system/
```

## Misc

Alternatively you can set keycodes running `setkeycodes.sh` as root and map keys using some other software.
