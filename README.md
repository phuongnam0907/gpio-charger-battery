# gpio-charger-battery

## Add to source code

1. <b>Directory</b>
```
{android_build}/kernel/drivers/power/yf3_battery_charger.c
```

2. <b>DEVICE TREE</b>
```
usb_charger: charger-gpio {
	compatible = "gpio-charger";
	charger-type = "mains";
	gpios = <&msm_gpio 0 1>;
	interrupt-controller;
	enable-battery-pin = <&msm_gpio 97 0>;
	};
```

3. <b>Defconfig</b>
```
# 7. add Charger YellowFin3
CONFIG_CHARGER_YELLOWFIN3=y
```

4. <b>Kconfig</b>
```
config CHARGER_YELLOWFIN3
	tristate "TI YellowFin3 battery charger support"
	depends on GPIOLIB
	help
	  Say Y to enable support for the TI YellowFin3 battery charger.
```

5. <b>Makefile</b>
```
obj-$(CONFIG_CHARGER_YELLOWFIN3)   += yf3_battery_charger.o
```

## How to get in Terminal

### Power - Mains

1. Check status of plug-in charger

```
cat /sys/class/power-supply/charger-gpio/online
```
Result: plug-in <1>, plug-outout <0>.

2. Check type of charger

```
cat /sys/class/power-supply/charger-gpio/type
```

### Power - Batterry

1. Check status of plug-in charger

```
cat /sys/class/power-supply/charger-battery/online
```
Result: plug-in <1>, plug-outout <0>.


2. Check type of charger

```
cat /sys/class/power-supply/charger-battery/type
```


3. Check type of charger

```
cat /sys/class/power-supply/charger-battery/type
```

4. Check status of plug-in charger

```
cat /sys/class/power-supply/charger-gpiobattery/online
```
Result: plug-in <1>, plug-outout <0>.

5. Check type of charger

```
cat /sys/class/power-supply/charger-battery/type
```

