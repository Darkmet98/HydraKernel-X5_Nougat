/*
 * arizona-spi.c  --  Arizona SPI bus interface
 *
 * Copyright 2012 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#if defined(CONFIG_AUDIO_CODEC_FLORIDA)
#include <linux/of_gpio.h>
#endif

#include <linux/mfd/arizona/core.h>

#include "arizona.h"

static int arizona_spi_probe(struct spi_device *spi)
{
	const struct spi_device_id *id = spi_get_device_id(spi);
	struct arizona *arizona;
	const struct regmap_config *regmap_config;
#if defined(CONFIG_AUDIO_CODEC_FLORIDA)
	const struct regmap_config *regmap_32bit_config = NULL;
#endif
	int ret;
	int64_t type;

	if (spi->dev.of_node)
		type = arizona_of_get_type(&spi->dev);
	else
		type = id->driver_data;
	switch (type) {
#ifdef CONFIG_MFD_WM5102
	case WM5102:
		regmap_config = &wm5102_spi_regmap;
		break;
#endif
#ifdef CONFIG_MFD_FLORIDA
	case WM8280:
	case WM5110:
		regmap_config = &florida_spi_regmap;
		break;
#endif
	default:
		dev_err(&spi->dev, "Unknown device type %ld\n",
			id->driver_data);
		return -EINVAL;
	}

	arizona = devm_kzalloc(&spi->dev, sizeof(*arizona), GFP_KERNEL);
	if (arizona == NULL)
		return -ENOMEM;

	arizona->regmap = devm_regmap_init_spi(spi, regmap_config);
	if (IS_ERR(arizona->regmap)) {
		ret = PTR_ERR(arizona->regmap);
		dev_err(&spi->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
#if defined(CONFIG_AUDIO_CODEC_FLORIDA)

	}

	if (regmap_32bit_config) {
		arizona->regmap_32bit = devm_regmap_init_spi(spi,
							   regmap_32bit_config);
		if (IS_ERR(arizona->regmap_32bit)) {
			ret = PTR_ERR(arizona->regmap_32bit);
			dev_err(&spi->dev,
				"Failed to allocate dsp register map: %d\n",
				ret);
			return ret;
		}

	}
#endif

	arizona->type = id->driver_data;
	arizona->dev = &spi->dev;
	arizona->irq = spi->irq;

	return arizona_dev_init(arizona);
}

static int arizona_spi_remove(struct spi_device *spi)
{
	struct arizona *arizona = spi_get_drvdata(spi);
	arizona_dev_exit(arizona);
	return 0;
}

static const struct spi_device_id arizona_spi_ids[] = {
	{ "wm5102", WM5102 },
	{ "wm8280", WM8280 },
	{ "wm8281", WM8280 },
	{ "wm5110", WM5110 },
	{ },
};
MODULE_DEVICE_TABLE(spi, arizona_spi_ids);

static struct spi_driver arizona_spi_driver = {
	.driver = {
		.name	= "arizona",
		.owner	= THIS_MODULE,
		.pm	= &arizona_pm_ops,
		.of_match_table	= of_match_ptr(arizona_of_match),
	},
	.probe		= arizona_spi_probe,
	.remove		= arizona_spi_remove,
	.id_table	= arizona_spi_ids,
};

module_spi_driver(arizona_spi_driver);

MODULE_DESCRIPTION("Arizona SPI bus interface");
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
