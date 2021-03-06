#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/power/gpio-charger.h>

#define GPIO_ENABLE 1008

struct gpio_charger {
	const struct gpio_charger_platform_data *pdata;
	unsigned int irq;
	bool wakeup_enabled;
	struct power_supply charger;
	struct power_supply battery;
};

unsigned enable_gpio;

static irqreturn_t gpio_charger_irq(int irq, void *devid)
{
	struct power_supply *charger = devid;
	power_supply_changed(charger);
	return IRQ_HANDLED;
}

static inline struct gpio_charger *psy_to_gpio_charger(struct power_supply *psy)
{
	return container_of(psy, struct gpio_charger, charger);
}

static int gpio_charger_property_is_writeable(struct power_supply *psy,
                        enum power_supply_property psp)
{
	int retVal = 0;
    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        retVal = 1;
		break;
    case POWER_SUPPLY_PROP_CHARGE_NOW:
        retVal = 1;
		break;
    default:
        break;
    }
    return retVal;
}

static int gpio_charger_set_property(struct power_supply *psy,
                       enum power_supply_property psp,
                       const union power_supply_propval *val)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:
        break;
    case POWER_SUPPLY_PROP_CHARGE_NOW:
		if (val->intval > 1 || val->intval < 0) return -EINVAL;
        gpio_set_value(enable_gpio, val->intval);
        break;
    default:
        return -EPERM;
    }

    return 0;
}

static int gpio_charger_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct gpio_charger *gpio_charger = psy_to_gpio_charger(psy);
	const struct gpio_charger_platform_data *pdata = gpio_charger->pdata;
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (gpio_get_value(pdata->gpio)?0:1);
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = gpio_get_value(enable_gpio);
		break;
	default:	
		return -EINVAL;
	}
	return 0;
}

static int battery_charger_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = 100;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = 4200;
		break;
	default:	
		return -EINVAL;
	}
	return 0;
}

static enum power_supply_property gpio_charger_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CHARGE_NOW,
};

static enum power_supply_property battery_charger_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
    	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static struct gpio_charger_platform_data *gpio_charger_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct gpio_charger_platform_data *pdata;
	const char *chargetype;
	enum of_gpio_flags flags;
	int ret;

	if (!np)
		return ERR_PTR(-ENOENT);

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->name = np->name;

	pdata->gpio = of_get_gpio_flags(np, 0, &flags);
	if (pdata->gpio < 0) {
		if (pdata->gpio != -EPROBE_DEFER)
			dev_err(dev, "could not get charger gpio\n");
		return ERR_PTR(pdata->gpio);
	}

	pdata->gpio_active_low = !!(flags & OF_GPIO_ACTIVE_LOW);

	pdata->type = POWER_SUPPLY_TYPE_UNKNOWN;
	ret = of_property_read_string(np, "charger-type", &chargetype);
	if (ret >= 0) {
		if (!strncmp("unknown", chargetype, 7))
			pdata->type = POWER_SUPPLY_TYPE_UNKNOWN;
		else if (!strncmp("battery", chargetype, 7))
			pdata->type = POWER_SUPPLY_TYPE_BATTERY;
		else if (!strncmp("ups", chargetype, 3))
			pdata->type = POWER_SUPPLY_TYPE_UPS;
		else if (!strncmp("mains", chargetype, 5))
			pdata->type = POWER_SUPPLY_TYPE_MAINS;
		else if (!strncmp("usb-sdp", chargetype, 7))
			pdata->type = POWER_SUPPLY_TYPE_USB;
		else if (!strncmp("usb-dcp", chargetype, 7))
			pdata->type = POWER_SUPPLY_TYPE_USB_DCP;
		else if (!strncmp("usb-cdp", chargetype, 7))
			pdata->type = POWER_SUPPLY_TYPE_USB_CDP;
		else if (!strncmp("usb-aca", chargetype, 7))
			pdata->type = POWER_SUPPLY_TYPE_USB_ACA;
		else
			dev_warn(dev, "unknown charger type %s\n", chargetype);
	}

	return pdata;
}

static int gpio_charger_probe(struct platform_device *pdev)
{
	const struct gpio_charger_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_charger *gpio_charger;
	struct power_supply *charger;
	struct power_supply *battery;
	int ret;
	int irq;

	struct device_node *child = pdev->dev.of_node;
	enable_gpio = of_get_named_gpio(child, "enable-battery-pin", 0);

	ret = gpio_request(enable_gpio, "enable-battery-pin");
	if (ret)
		dev_err(&pdev->dev, "Failed to request gpio pin: %d\n", enable_gpio);
	gpio_direction_output(enable_gpio, 0);
	
	if (!pdata) {
		pdata = gpio_charger_parse_dt(&pdev->dev);
		if (IS_ERR(pdata)) {
			ret = PTR_ERR(pdata);
			if (ret != -EPROBE_DEFER)
				dev_err(&pdev->dev, "No platform data\n");
			return ret;
		}
	}

	if (!gpio_is_valid(pdata->gpio)) {
		dev_err(&pdev->dev, "Invalid gpio pin\n");
		return -EINVAL;
	}

	gpio_charger = devm_kzalloc(&pdev->dev, sizeof(*gpio_charger),GFP_KERNEL);
	if (!gpio_charger) {
		dev_err(&pdev->dev, "Failed to alloc driver structure\n");
		return -ENOMEM;
	}

	charger = &gpio_charger->charger;
	charger->name = pdata->name ? pdata->name : "gpio-charger";
	charger->type = pdata->type;
	charger->properties = gpio_charger_properties;
	charger->num_properties = ARRAY_SIZE(gpio_charger_properties);
	charger->get_property = gpio_charger_get_property;
	charger->supplied_to = pdata->supplied_to;
	charger->num_supplicants = pdata->num_supplicants;
	charger->property_is_writeable = gpio_charger_property_is_writeable;
	charger->set_property = gpio_charger_set_property;
	ret = gpio_request(pdata->gpio, dev_name(&pdev->dev));

	if (ret) {
		dev_err(&pdev->dev, "Failed to request gpio pin: %d\n", ret);
		goto err_free;
	}

	ret = gpio_direction_input(pdata->gpio);
	if (ret) {
		dev_err(&pdev->dev, "Failed to set gpio to input: %d\n", ret);
		goto err_gpio_free;
	}

	gpio_charger->pdata = pdata;
	ret = power_supply_register(&pdev->dev, charger);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register power supply: %d\n",
			ret);
		goto err_gpio_free;
	}

	irq = gpio_to_irq(pdata->gpio);
	if (irq > 0) {
		ret = request_any_context_irq(irq, gpio_charger_irq,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				dev_name(&pdev->dev), charger);
		if (ret < 0)
			dev_warn(&pdev->dev, "Failed to request irq: %d\n", ret);
		else
			gpio_charger->irq = irq;
	}

	// Fake battery
	battery = &gpio_charger->battery;
	battery->name = "charger-battery";
	battery->type = POWER_SUPPLY_TYPE_BATTERY;
	battery->properties = battery_charger_properties;
	battery->num_properties = ARRAY_SIZE(battery_charger_properties);
	battery->get_property = battery_charger_get_property;
	battery->supplied_to = pdata->supplied_to;
	battery->num_supplicants = pdata->num_supplicants;
	ret = power_supply_register(&pdev->dev, battery);

	platform_set_drvdata(pdev, gpio_charger);
	return 0;
err_gpio_free:
	gpio_free(pdata->gpio);
err_free:
	return ret;
}

static int gpio_charger_remove(struct platform_device *pdev)
{
	struct gpio_charger *gpio_charger = platform_get_drvdata(pdev);
	if (gpio_charger->irq)
			free_irq(gpio_charger->irq, &gpio_charger->charger);
	power_supply_unregister(&gpio_charger->charger);
	gpio_free(gpio_charger->pdata->gpio);
	platform_set_drvdata(pdev, NULL);
	gpio_set_value(enable_gpio, 0);
	gpio_free(enable_gpio);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int gpio_charger_suspend(struct device * dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct gpio_charger *gpio_charger = platform_get_drvdata(pdev);
	if (device_may_wakeup(dev) && gpio_charger->wakeup_enabled)
		disable_irq_wake(gpio_charger->irq);
	power_supply_changed(&gpio_charger->charger);
	return 0;
}

static int gpio_charger_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct gpio_charger *gpio_charger = platform_get_drvdata(pdev);
	power_supply_changed(&gpio_charger->charger);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(gpio_charger_pm_ops, gpio_charger_suspend, gpio_charger_resume);

static const struct of_device_id gpio_charger_match[] = {
	{ .compatible = "gpio-charger" },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_charger_match);

static struct platform_driver gpio_charger_driver = {
	.probe = gpio_charger_probe,
	.remove = gpio_charger_remove,
	.driver = {
		.name = "gpio-charger",
		.owner = THIS_MODULE,
		.pm = &gpio_charger_pm_ops,
		.of_match_table = gpio_charger_match,
	},
};

module_platform_driver(gpio_charger_driver);
MODULE_AUTHOR("Le Phuong Nam <le.phuong.nam@styl.solutions>");
MODULE_DESCRIPTION("Driver for chargers which report their online status through a GPIO");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:gpio-charger");
MODULE_VERSION("1.0.0");
