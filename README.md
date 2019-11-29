# gpio-charger-battery

//Directory: {android_build}/kernel/drivers/power/yf3_battery_charger.c

//DEVICE TREE

	usb_charger: charger-gpio {
		compatible = "gpio-charger";
		charger-type = "mains";
		gpios = <&msm_gpio 0 0>;
	};

// Defconfig
	# 8. add Charger YellowFin3
	CONFIG_CHARGER_YELLOWFIN3=y

//Kconfig

config CHARGER_YELLOWFIN3
	tristate "TI YellowFin3 battery charger support"
	depends on GPIOLIB
	help
	  Say Y to enable support for the TI YellowFin3 battery charger.

// Makefile

	obj-$(CONFIG_CHARGER_YELLOWFIN3)   += yf3_battery_charger.o
