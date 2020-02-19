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

6. <b>init.target.rc</b>

Direction: AOSP/device/qcom/msm8909/init.target.rc

```
on property:sys.boot_completed=1
   # Turn on gpio97
   write /sys/class/power_supply/charger-gpio/charge_now 1
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

Result: Mains

### Power - Batterry

1. Check status of plug-in charger

```
cat /sys/class/power-supply/charger-battery/online
```

Result: always plug-in <1>.

2. Check type of charger

```
cat /sys/class/power-supply/charger-battery/type
```

Result: Battery

3. Check voltage of charger

```
cat /sys/class/power-supply/charger-battery/voltage_now
```

Result: 4200 (4.2V = 4200 mV)

4. Check capacitive of charger

```
cat /sys/class/power-supply/charger-gpiobattery/capacitive
```

Result: 100 (100%)

5. Check status of charging now

```
cat /sys/class/power-supply/charger-battery/charge_now
```

Result: 0 (no charging)
