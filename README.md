# gpio-charger-battery

1. Directory
```
{android_build}/kernel/drivers/power/yf3_battery_charger.c
```

2. DEVICE TREE
```
usb_charger: charger-gpio {
	compatible = "gpio-charger";
	charger-type = "mains";
	gpios = <&msm_gpio 0 0>;
};
```

3. Defconfig
```
# 8. add Charger YellowFin3
CONFIG_CHARGER_YELLOWFIN3=y
```

4. Kconfig
```
config CHARGER_YELLOWFIN3
	tristate "TI YellowFin3 battery charger support"
	depends on GPIOLIB
	help
	  Say Y to enable support for the TI YellowFin3 battery charger.
```

5. Makefile
```
obj-$(CONFIG_CHARGER_YELLOWFIN3)   += yf3_battery_charger.o
```
