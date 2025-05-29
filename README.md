# USB Power On Screen Display
On Screen Display for my USB-C Power Meter (compatible to PLD's)

## Building from source
### Linux
```shell
sudo apt-get install -y libgtk-3-dev libwebkit2gtk-4.1-dev libappindicator3-dev
```

## Running
### Linux
You need to have access to `/dev/ttyUSB*`, this is usually managed by adding your use to a special group.
In Debian and Ubuntu this can be achieved by adding yourself to the `dialout` group:
```shell
sudo usermod -a -G dialout $USER
```
You have to re-login after that, then you can use usb-power-osd.